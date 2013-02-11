//
// server.cpp
// Project Spitfire
//
// Copyright (c) 2013 Daizee (rensiadz at gmail dot com)
//
// This file is part of Spitfire.
// 
// Spitfire is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Spitfire is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Spitfire.  If not, see <http://www.gnu.org/licenses/>.

#include "server.hpp"
#include <boost/bind.hpp>
#include <signal.h>
#include "connection.hpp"
#include <sys/timeb.h>

#define DEF_NOMAPDATA

namespace spitfire {
namespace server {


#ifdef WIN32
	unsigned __stdcall  SaveData(void *ch);
	unsigned __stdcall  TimerThread(void *ch);
#else
	void * SaveData(void *ch);
	void * TimerThread(void *ch);
#endif

int DEF_MAPSIZE = 0;

using namespace std;

extern char * strtolower(char * x);
extern string makesafe(string in);
extern size_t ci_find(const string& str1, const string& str2);
extern bool ci_equal(char ch1, char ch2);
extern server * gserver;

uint64_t unixtime()
{
#ifdef WIN32
	struct __timeb64 tstruct;
	_ftime64_s( &tstruct );
#else
	struct timeb tstruct;
	ftime( &tstruct );
#endif
	return tstruct.millitm + tstruct.time*1000;
}

server::server()
  : io_service_(),
    signals_(io_service_),
    acceptor_(io_service_),
    connection_manager_(),
    new_connection_(new connection(io_service_,
          connection_manager_, request_handler_)),
	accounts(0),
	msql(0),
	msql2(0),
	m_map(0)
{
	serverstatus = 0;//offline
	m_servername = 0;
	memset(&m_buildingconfig, 0, sizeof(m_buildingconfig));
	memset(&m_researchconfig, 0, sizeof(m_researchconfig));
	memset(&m_troopconfig, 0, sizeof(m_troopconfig));

	m_heroid = 1000;
	m_cityid = 100000;
	m_allianceid = 100;
	m_itemcount = 0;


	L = lua_open();
	luaL_openlibs(L);

	if (!LoadConfig())
	{
		throw exception("Error in config. Exiting.");
	}

	m_clients = new Client*[maxplayersloaded];
	for (int i = 0; i < maxplayersloaded; ++i)
	{
		m_clients[i] = 0;
	}


	// Register to handle the signals that indicate when the server should exit.
	// It is safe to register for the same signal multiple times in a program,
	// provided all registration for the specified signal is made through Asio.
	signals_.add(SIGINT);
	signals_.add(SIGTERM);
	#if defined(SIGQUIT)
	signals_.add(SIGQUIT);
	#endif // defined(SIGQUIT)
	signals_.async_wait(boost::bind(&server::handle_stop, this));

	// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
	asio::ip::tcp::resolver resolver(io_service_);
	asio::ip::tcp::resolver::query query(bindaddress, bindport);
	asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
	bool test = true;
	try { 
		acceptor_.bind(endpoint);
	}
	catch (std::exception& e)
	{
		test = false;
	}
	if (test == false)
	{
		throw exception("Invalid bind address or port 443 already in use! Exiting.");
	}
}

void server::run()
{
	// Prestart server
	accounts = new CSQLDB;
	msql = new CSQLDB;
	msql2 = new CSQLDB;

	if ((accounts->Init(sqlhost.c_str(), sqluser.c_str(), sqlpass.c_str(), "evony") == 0) ||
		(msql->Init(sqlhost.c_str(), sqluser.c_str(), sqlpass.c_str(), "s2") == 0) ||
		(msql2->Init(sqlhost.c_str(), sqluser.c_str(), sqlpass.c_str(), "s2") == 0))
	{
		delete accounts;
		delete msql;
		delete msql2;
		throw exception("Unable to connect to mysql server!");
	}

	printf("Start up procedure\n");

#pragma region data load

	accounts->Select("SELECT * FROM `settings` where `server`='%s';", "2");
	accounts->Fetch();


	string temp;
	for (int i = 0; i < accounts->m_iRows; ++i)
	{
		if (!strcmp(accounts->GetString(i, "setting"), "mapsize"))
			DEF_MAPSIZE = atoi(accounts->GetString(i, "options"));
		if (!strcmp(accounts->GetString(i, "setting"), "itemxml"))
			m_itemxml = accounts->GetString(i, "options");
	}
	accounts->Reset();

	m_map = new Map(this);
	m_alliances = new AllianceCore(this);

	Log("Loading configurations.");


	accounts->Select("SELECT * FROM `config_building`;");
	accounts->Fetch();

	for (int i = 0; i < accounts->m_iRows; ++i)
	{
		char * tok;
		char * ch = 0, * cr = 0;

		int cfgid = accounts->GetInt(i, "buildingid");
		int level = accounts->GetInt(i, "level")-1;
		m_buildingconfig[cfgid][level].time = accounts->GetInt(i, "buildtime");
		m_buildingconfig[cfgid][level].destructtime = accounts->GetInt(i, "buildtime")/2;
		m_buildingconfig[cfgid][level].food = accounts->GetInt(i, "food");
		m_buildingconfig[cfgid][level].wood = accounts->GetInt(i, "wood");
		m_buildingconfig[cfgid][level].stone = accounts->GetInt(i, "stone");
		m_buildingconfig[cfgid][level].iron = accounts->GetInt(i, "iron");
		m_buildingconfig[cfgid][level].gold = accounts->GetInt(i, "gold");
		m_buildingconfig[cfgid][level].prestige = accounts->GetInt(i, "gold");

		char * str = accounts->GetString(i, "prereqbuilding");

		int x = 0;

		for (tok = strtok_s(str, "|", &ch); tok != 0; tok = strtok_s(0, "|", &ch))
		{
			tok = strtok_s(tok, ",", &cr); assert(tok != 0);
			m_buildingconfig[cfgid][level].buildings[x].id = atoi(tok);
			tok = strtok_s(0, ",", &cr); assert(tok != 0);
			m_buildingconfig[cfgid][level].buildings[x].level = atoi(tok);
			x++;
		}

		str = accounts->GetString(i, "prereqtech");
		x = 0;

		for (tok = strtok_s(str, "|", &ch); tok != 0; tok = strtok_s(0, "|", &ch))
		{
			tok = strtok_s(tok, ",", &cr); assert(tok != 0);
			m_buildingconfig[cfgid][level].techs[x].id = atoi(tok);
			tok = strtok_s(0, ",", &cr); assert(tok != 0);
			m_buildingconfig[cfgid][level].techs[x].level = atoi(tok);
			x++;
		}

		str = accounts->GetString(i, "prereqitem");
		x = 0;

		for (tok = strtok_s(str, "|", &ch); tok != 0; tok = strtok_s(0, "|", &ch))
		{
			tok = strtok_s(tok, ",", &cr); assert(tok != 0);
			m_buildingconfig[cfgid][level].items[x].id = atoi(tok);
			tok = strtok_s(0, ",", &cr); assert(tok != 0);
			m_buildingconfig[cfgid][level].items[x].level = atoi(tok);
			x++;
		}

		m_buildingconfig[cfgid][level].limit = accounts->GetInt(i, "limit");
		m_buildingconfig[cfgid][level].inside = accounts->GetInt(i, "inside");
	}

	accounts->Reset();

	accounts->Select("SELECT * FROM `config_troops`;");
	accounts->Fetch();

	for (int i = 0; i < accounts->m_iRows; ++i)
	{
		char * tok;
		char * ch = 0, * cr = 0;

		int cfgid = accounts->GetInt(i, "troopid");
		m_troopconfig[cfgid].time = accounts->GetInt(i, "buildtime");
		m_troopconfig[cfgid].destructtime = 0;
		m_troopconfig[cfgid].food = accounts->GetInt(i, "food");
		m_troopconfig[cfgid].wood = accounts->GetInt(i, "wood");
		m_troopconfig[cfgid].stone = accounts->GetInt(i, "stone");
		m_troopconfig[cfgid].iron = accounts->GetInt(i, "iron");
		m_troopconfig[cfgid].gold = accounts->GetInt(i, "gold");
		m_troopconfig[cfgid].inside = accounts->GetInt(i, "inside");
		m_troopconfig[cfgid].population = accounts->GetInt(i, "population");

		char * str = accounts->GetString(i, "prereqbuilding");

		int x = 0;

		for (tok = strtok_s(str, "|", &ch); tok != 0; tok = strtok_s(0, "|", &ch))
		{
			tok = strtok_s(tok, ",", &cr); assert(tok != 0);
			m_troopconfig[cfgid].buildings[x].id = atoi(tok);
			tok = strtok_s(0, ",", &cr); assert(tok != 0);
			m_troopconfig[cfgid].buildings[x].level = atoi(tok);
			x++;
		}

		str = accounts->GetString(i, "prereqtech");
		x = 0;

		for (tok = strtok_s(str, "|", &ch); tok != 0; tok = strtok_s(0, "|", &ch))
		{
			tok = strtok_s(tok, ",", &cr); assert(tok != 0);
			m_troopconfig[cfgid].techs[x].id = atoi(tok);
			tok = strtok_s(0, ",", &cr); assert(tok != 0);
			m_troopconfig[cfgid].techs[x].level = atoi(tok);
			x++;
		}

		str = accounts->GetString(i, "prereqitem");
		x = 0;

		for (tok = strtok_s(str, "|", &ch); tok != 0; tok = strtok_s(0, "|", &ch))
		{
			tok = strtok_s(tok, ",", &cr); assert(tok != 0);
			m_troopconfig[cfgid].items[x].id = atoi(tok);
			tok = strtok_s(0, ",", &cr); assert(tok != 0);
			m_troopconfig[cfgid].items[x].level = atoi(tok);
			x++;
		}
	}

	accounts->Reset();

	accounts->Select("SELECT * FROM `config_research`;");
	accounts->Fetch();

	for (int i = 0; i < accounts->m_iRows; ++i)
	{
		char * tok;
		char * ch = 0, * cr = 0;

		int cfgid = accounts->GetInt(i, "researchid");
		int level = accounts->GetInt(i, "level")-1;
		m_researchconfig[cfgid][level].time = accounts->GetInt(i, "buildtime");
		m_researchconfig[cfgid][level].destructtime = accounts->GetInt(i, "buildtime")/2;
		m_researchconfig[cfgid][level].food = accounts->GetInt(i, "food");
		m_researchconfig[cfgid][level].wood = accounts->GetInt(i, "wood");
		m_researchconfig[cfgid][level].stone = accounts->GetInt(i, "stone");
		m_researchconfig[cfgid][level].iron = accounts->GetInt(i, "iron");
		m_researchconfig[cfgid][level].gold = accounts->GetInt(i, "gold");

		char * str = accounts->GetString(i, "prereqbuilding");

		int x = 0;

		for (tok = strtok_s(str, "|", &ch); tok != 0; tok = strtok_s(0, "|", &ch))
		{
			tok = strtok_s(tok, ",", &cr); assert(tok != 0);
			m_researchconfig[cfgid][level].buildings[x].id = atoi(tok);
			tok = strtok_s(0, ",", &cr); assert(tok != 0);
			m_researchconfig[cfgid][level].buildings[x].level = atoi(tok);
			x++;
		}

		str = accounts->GetString(i, "prereqtech");
		x = 0;

		for (tok = strtok_s(str, "|", &ch); tok != 0; tok = strtok_s(0, "|", &ch))
		{
			tok = strtok_s(tok, ",", &cr); assert(tok != 0);
			m_researchconfig[cfgid][level].techs[x].id = atoi(tok);
			tok = strtok_s(0, ",", &cr); assert(tok != 0);
			m_researchconfig[cfgid][level].techs[x].level = atoi(tok);
			x++;
		}

		str = accounts->GetString(i, "prereqitem");
		x = 0;

		for (tok = strtok_s(str, "|", &ch); tok != 0; tok = strtok_s(0, "|", &ch))
		{
			tok = strtok_s(tok, ",", &cr); assert(tok != 0);
			m_researchconfig[cfgid][level].items[x].id = atoi(tok);
			tok = strtok_s(0, ",", &cr); assert(tok != 0);
			m_researchconfig[cfgid][level].items[x].level = atoi(tok);
			x++;
		}

		m_researchconfig[cfgid][level].limit = accounts->GetInt(i, "limit");
		m_researchconfig[cfgid][level].inside = accounts->GetInt(i, "inside");
	}

	accounts->Reset();

	accounts->Select("SELECT * FROM `config_items`;");
	accounts->Fetch();

	for (int i = 0; i < accounts->m_iRows; ++i)
	{
		int id = accounts->GetInt(i, "id");
		m_items[id].name = accounts->GetString(i, "name");
		m_items[id].cost = accounts->GetInt(i, "cost");
		m_items[id].saleprice = accounts->GetInt(i, "cost");
		m_items[id].buyable = accounts->GetInt(i, "buyable");
		m_items[id].cangamble = accounts->GetInt(i, "cangamble");
		m_items[id].daylimit = accounts->GetInt(i, "daylimit");
		m_items[id].type = accounts->GetInt(i, "itemtype");
		m_items[id].rarity = accounts->GetInt(i, "rarity");
		++m_itemcount;

		if (m_items[id].cangamble)
		{
			switch (m_items[id].rarity)
			{
			default:
			case 5:
				m_gambleitems.common.push_back(&m_items[id]);
				break;
			case 4:
				m_gambleitems.special.push_back(&m_items[id]);
				break;
			case 3:
				m_gambleitems.rare.push_back(&m_items[id]);
				break;
			case 2:
				m_gambleitems.superrare.push_back(&m_items[id]);
				break;
			case 1:
				m_gambleitems.ultrarare.push_back(&m_items[id]);
				break;
			}
		}
	}

	accounts->Reset();

	Log("Loading map data.");


#ifndef _DEBUG
	for (int x = 0; x < (DEF_MAPSIZE*DEF_MAPSIZE); x += (DEF_MAPSIZE*DEF_MAPSIZE)/10)
	{
		//msql->Reset();
		//msql->Select("SELECT * FROM `tiles` ORDER BY `id` ASC LIMIT %d,%d;", x, (DEF_MAPSIZE*DEF_MAPSIZE)/200);
		//msql->Fetch();

		char query[1024];
		sprintf(query, "SELECT `id`,`ownerid`,`type`,`level` FROM `tiles` ORDER BY `id` ASC LIMIT %d,%d", x, ((DEF_MAPSIZE*DEF_MAPSIZE)/10));
		mysql_query(gserver->msql->mySQL, query);
		st_mysql_res* m_pQueryResult;
		m_pQueryResult = mysql_store_result(msql->mySQL);
		int m_iRows = (int)mysql_num_rows(m_pQueryResult);
		int m_iFields = mysql_num_fields(m_pQueryResult);
		//MYSQL_FIELD ** field = new MYSQL_FIELD*[m_iFields+1];;
		MYSQL_ROW myRow;
		mysql_field_seek(m_pQueryResult, 0);



		for (int i = 0; i < m_iRows; ++i)
		{
			myRow = mysql_fetch_row(m_pQueryResult);
			int64_t id = _atoi64(myRow[0]);
			int64_t ownerid = _atoi64(myRow[1]);
			int64_t type = _atoi64(myRow[2]);
			int64_t level = _atoi64(myRow[3]);

			//			int id = msql->GetInt(i, "id");
			//			int ownerid = msql->GetInt(i, "ownerid");
			//			int type = msql->GetInt(i, "type");
			//			int level = msql->GetInt(i, "level");
			gserver->m_map->m_tile[id].m_id = id;
			gserver->m_map->m_tile[id].m_ownerid = ownerid;
			gserver->m_map->m_tile[id].m_type = type;
			gserver->m_map->m_tile[id].m_level = level;


			if (type == NPC)
			{
				NpcCity * city = (NpcCity *)gserver->AddNpcCity(id);
				city->Initialize(true, true);
				city->m_level = level;
				city->m_ownerid = ownerid;
				gserver->m_map->m_tile[id].m_zoneid = gserver->m_map->GetStateFromID(id);
			}

			if ((id+1)%((DEF_MAPSIZE*DEF_MAPSIZE)/100) == 0)
			{
				Log("%d%%", int((double(double(id+1)/(DEF_MAPSIZE*DEF_MAPSIZE)))*double(100)));
			}
		}
		mysql_free_result(m_pQueryResult);
	}

	msql->Reset();
#else
	//this fakes map data
	for (int x = 0; x < (DEF_MAPSIZE*DEF_MAPSIZE); x += 1/*(DEF_MAPSIZE*DEF_MAPSIZE)/10*/)
	{
		m_map->m_tile[x].m_id = x;
		m_map->m_tile[x].m_ownerid = -1;
		m_map->m_tile[x].m_type = rand()%11;
		m_map->m_tile[x].m_level = rand()%11;


		if ((x+1)%((DEF_MAPSIZE*DEF_MAPSIZE)/100) == 0)
		{
			Log("%d%%", int((double(double(x+1)/(DEF_MAPSIZE*DEF_MAPSIZE)))*double(100)));
		}
	}

#endif

	m_map->CalculateOpenTiles();

	Log("Loading account data.");


	msql->Select("SELECT COUNT(*) AS a FROM `accounts`");
	msql->Fetch();
	int accountcount = msql->GetInt(0, "a");

	msql->Reset();
	msql->Select("SELECT `accounts`.*,`evony`.`account`.`email`,`evony`.`account`.`password` FROM `accounts` LEFT JOIN `evony`.`account` ON (`evony`.`account`.`id`=`accounts`.`parentid`) ORDER BY `accountid` ASC");
	msql->Fetch();

	Client * client;

	int count = 0;

	count = 0;

	for (int i = 0; i < msql->m_iRows; ++i)
	{
		count++;

		client = NewClient();
		client->m_accountexists = true;
		client->m_accountid = msql->GetInt(i, "accountid");
		client->m_parentid = msql->GetInt(i, "parentid");
		client->m_playername = msql->GetString(i, "username");
		client->m_password = msql->GetString(i, "password");
		client->m_email = msql->GetString(i, "email");
		client->m_allianceid = msql->GetInt(i, "allianceid");
		client->m_alliancerank = msql->GetInt(i, "alliancerank");
		client->m_lastlogin = msql->GetDouble(i, "lastlogin");
		client->m_creation = msql->GetDouble(i, "creation");
		client->m_status = msql->GetInt(i, "status");
		client->m_sex = msql->GetInt(i, "sex");
		client->m_flag = msql->GetString(i, "flag");
		client->m_faceurl = msql->GetString(i, "faceurl");
		client->m_cents = msql->GetInt(i, "cents");
		client->m_prestige = msql->GetDouble(i, "prestige");
		client->m_honor = msql->GetDouble(i, "honor");

		client->ParseBuffs(msql->GetString(i, "buffs"));
		client->ParseResearch(msql->GetString(i, "research"));
		client->ParseItems(msql->GetString(i, "items"));
		client->ParseMisc(msql->GetString(i, "misc"));

		if (accountcount > 101)
			if ((count)%((accountcount)/100) == 0)
			{
				Log("%d%%", int((double(double(count)/accountcount+1))*double(100)));
			}
	}
	msql->Reset();


	Log("Loading city data.");


	msql->Select("SELECT COUNT(*) AS a FROM `cities`");
	msql->Fetch();
	int citycount = msql->GetInt(0, "a");

	msql->Reset();
	msql->Select("SELECT * FROM `cities`");
	msql->Fetch();

	count = 0;

	for (int i = 0; i < msql->m_iRows; ++i)
	{
		count++;

		int accountid = msql->GetInt(i, "accountid");
		int cityid = msql->GetInt(i, "id");
		int fieldid = msql->GetInt(i, "fieldid");
		client = GetClient(accountid);
		GETXYFROMID(fieldid);
		if (client == 0)
		{
			Log("City exists with no account attached. - accountid:%d cityid:%d coord:(%d,%d)", accountid, cityid, xfromid, yfromid);
			continue;
		}
		PlayerCity * city = (PlayerCity *)AddPlayerCity(client->m_clientnumber, fieldid, cityid);
		city->m_resources.food = msql->GetDouble(i, "food");
		city->m_resources.wood = msql->GetDouble(i, "wood");
		city->m_resources.iron = msql->GetDouble(i, "iron");
		city->m_resources.stone = msql->GetDouble(i, "stone");
		city->m_resources.gold = msql->GetDouble(i, "gold");
		city->m_cityname = msql->GetString(i, "name");
		city->m_logurl = msql->GetString(i, "logurl");
		city->m_tileid = fieldid;
		//		city->m_accountid = accountid;
		//		city->m_client = client;
		city->m_creation = msql->GetDouble(i, "creation");

		//		client->m_citycount++;

		// 		server->m_map->m_tile[fieldid].m_city = city;
		// 		server->m_map->m_tile[fieldid].m_npc = false;
		// 		server->m_map->m_tile[fieldid].m_ownerid = accountid;
		// 		server->m_map->m_tile[fieldid].m_type = CASTLE;
		// 
		// 		//server->m_map->m_tile[fieldid].m_zoneid = server->m_map->GetStateFromID(fieldid);
		// 
		// 		server->m_map->m_tile[fieldid].m_castleid = cityid;


		city->ParseTroops(msql->GetString(i, "troop"));
		city->ParseBuildings(msql->GetString(i, "buildings"));
		city->ParseFortifications(msql->GetString(i, "fortification"));
		city->ParseMisc(msql->GetString(i, "misc"));
		//city->ParseHeroes(msql->GetString(i, "heroes"));
		//city->ParseTrades(msql->GetString(i, "trades"));
		//city->ParseArmyMovement(msql->GetString(i, "buffs"));

		msql2->Select("SELECT * FROM `heroes` WHERE `castleid`="XI64, city->m_castleid);
		msql2->Fetch();

		for (int a = 0; a < msql2->m_iRows; ++a)
		{
			Hero * temphero;
			temphero = new Hero();
			temphero->m_id = msql2->GetInt(a, "id");
			temphero->m_status = msql2->GetInt(a, "status");
			temphero->m_itemid = msql2->GetInt(a, "itemid");
			temphero->m_itemamount = msql2->GetInt(a, "itemamount");
			temphero->m_basestratagem = msql2->GetInt(a, "basestratagem");
			temphero->m_stratagem = msql2->GetInt(a, "stratagem");
			temphero->m_stratagemadded = msql2->GetInt(a, "stratagemadded");
			temphero->m_stratagembuffadded = msql2->GetInt(a, "stratagembuffadded");
			temphero->m_basepower = msql2->GetInt(a, "basepower");
			temphero->m_power = msql2->GetInt(a, "power");
			temphero->m_poweradded = msql2->GetInt(a, "poweradded");
			temphero->m_powerbuffadded = msql2->GetInt(a, "powerbuffadded");
			temphero->m_basemanagement = msql2->GetInt(a, "basemanagement");
			temphero->m_management = msql2->GetInt(a, "management");
			temphero->m_managementadded = msql2->GetInt(a, "managementadded");
			temphero->m_managementbuffadded = msql2->GetInt(a, "managementbuffadded");
			temphero->m_logourl = msql2->GetString(a, "logurl");
			temphero->m_name = msql2->GetString(a, "name");
			temphero->m_remainpoint = msql2->GetInt(a, "remainpoint");
			temphero->m_level = msql2->GetInt(a, "level");
			temphero->m_upgradeexp = msql2->GetDouble(a, "upgradeexp");
			temphero->m_experience = msql2->GetDouble(a, "experience");
			temphero->m_loyalty = msql2->GetInt(a, "loyalty");
			city->m_heroes[a] = temphero;
			if (temphero->m_status == 1)
			{
				city->m_mayor = temphero;
			}
			if (temphero->m_id > m_heroid)
				m_heroid = temphero->m_id + 1;
		}
		if (cityid >= m_cityid)
			m_cityid = cityid + 1;


		client->CalculateResources();

		if (citycount > 101)
			if ((count)%((citycount)/100) == 0)
			{
				Log("%d%%", int((double(double(count)/citycount+1))*double(100)));
			}
	}
	msql->Reset();

	for (int i = 0; i < maxplayersloaded; ++i)
	{
		Client * client = m_clients[i];
		if (client)
			for (int a = 0; a < 25; ++a)
			{
				if (client->m_research[a].castleid > 0)
				{
					PlayerCity * pcity = client->GetCity(client->m_research[a].castleid);
					if (pcity != 0)
					{
						pcity->m_researching = true;

						if (client->m_research[a].castleid != 0)
						{
							server::stResearchAction * ra = new server::stResearchAction;

							server::stTimedEvent te;
							ra->city = pcity;
							ra->client = pcity->m_client;
							ra->researchid = a;
							te.data = ra;
							te.type = DEF_TIMEDRESEARCH;

							AddTimedEvent(te);
						}
					}
					else
					{
						Log("Castleid does not exist for research! rsrch:%d castleid:%d accountid:%d", a, client->m_research[a].castleid, client->m_accountid);
					}
				}
			}
	}



	Log("Loading alliance data.");

	count = 0;

	msql->Select("SELECT COUNT(*) AS a FROM `alliances`");
	msql->Fetch();
	int alliancecount = msql->GetInt(0, "a");

	msql->Reset();
	msql->Select("SELECT * FROM `alliances`");
	msql->Fetch();

	Alliance * alliance;

	for (int i = 0; i < msql->m_iRows; ++i)
	{
		count++;
		alliance = m_alliances->CreateAlliance(msql->GetString(i, "name"), msql->GetInt(i, "leader"), msql->GetInt(i, "id"));
		//alliance->m_allianceid = msql->GetInt(i, "id");
		alliance->ParseMembers(msql->GetString(i, "members"));
		alliance->ParseRelation(&alliance->m_enemies, msql->GetString(i, "enemies"));
		alliance->ParseRelation(&alliance->m_allies, msql->GetString(i, "allies"));
		alliance->ParseRelation(&alliance->m_neutral, msql->GetString(i, "neutrals"));
		alliance->m_name = msql->GetString(i, "name");
		alliance->m_founder = msql->GetString(i, "founder");
		alliance->m_note = msql->GetString(i, "note");

		if (alliance->m_allianceid >= m_allianceid)
			m_allianceid = alliance->m_allianceid + 1;


		if (alliancecount > 101)
			if ((count)%((alliancecount)/100) == 0)
			{
				Log("%d%%", int((double(double(count)/alliancecount+1))*double(100)));
			}
	}
	msql->Reset();


	Log("Incrementing valleys.");


	for (int i = 0; i < DEF_MAPSIZE*DEF_MAPSIZE; ++i)
	{
		if (m_map->m_tile[i].m_type < 11 && m_map->m_tile[i].m_ownerid < 0)
		{
			m_map->m_tile[i].m_level++;
			if (m_map->m_tile[i].m_level > 10)
				m_map->m_tile[i].m_level = 1;
		}
	}
#pragma endregion


  // The io_service::run() call will block until all asynchronous operations
  // have finished. While the server is running, there is always at least one
  // asynchronous operation outstanding: the asynchronous accept call waiting
  // for new incoming connections.
	
	serverstatus = 1;//online

	unsigned uAddr;

#ifndef WIN32
	pthread_t hTimerThread;
	if (pthread_create(&hTimerThread, NULL, TimerThread, 0))
	{
		SFERROR("pthread_create");
	}
#else
	HANDLE hTimerThread;
	hTimerThread = (HANDLE)_beginthreadex(0, 0, TimerThread, 0, 0, &uAddr);
#endif

	// Finally listen on the socket and start accepting connections
	acceptor_.listen();
	acceptor_.async_accept(new_connection_->socket(),
		boost::bind(&server::handle_accept, this,
		asio::placeholders::error));

	io_service_.run();

#ifndef WIN32
	pthread_t hSaveThread;
	if (pthread_create(&hSaveThread, NULL, SaveData, 0))
	{
		SFERROR("pthread_create");
	}
#else
	HANDLE hSaveThread;
	hSaveThread = (HANDLE)_beginthreadex(0, 0, SaveData, 0, 0, &uAddr);
#endif
	lua_close(L);
}

bool server::LoadConfig()
{
	if (luaL_dofile(L, "config.lua") != 0)
	{
		Log("%s", lua_tostring(L,-1));
		return false;
	}
	lua_getglobal(L, "server");

	lua_getfield(L, -1, "ipaddress");
	bindaddress = lua_tostring(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "port");
	bindport = lua_tostring(L, -1);
	Log("Binding on %s:%s", bindaddress.c_str(), bindport.c_str());
	lua_pop(L, 1);

	lua_getfield(L, -1, "mysqlhost");
	sqlhost = lua_tostring(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "mysqluser");
	sqluser = lua_tostring(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "mysqlpass");
	sqlpass = lua_tostring(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "maxplayersloaded");
	maxplayersloaded = atoi(lua_tostring(L, -1));
	lua_pop(L, 1);

	lua_getfield(L, -1, "maxplayersonline");
	maxplayersonline = atoi(lua_tostring(L, -1));
	lua_pop(L, 1);

	return true;
}

void server::stop()
{
  // Post a call to the stop function so that server::stop() is safe to call
  // from any thread.
  io_service_.post(boost::bind(&server::handle_stop, this));
}

void server::handle_accept(const asio::error_code& e)
{
  if (!e)
  {
    connection_manager_.start(new_connection_);
    new_connection_.reset(new connection(io_service_,
          connection_manager_, request_handler_));
    acceptor_.async_accept(new_connection_->socket(),
        boost::bind(&server::handle_accept, this,
          asio::placeholders::error));
  }
}

void server::handle_stop()
{
  // The server is stopped by cancelling all outstanding asynchronous
  // operations. Once all operations have finished the io_service::run() call
  // will exit.
  acceptor_.close();
  connection_manager_.stop_all();
}


// Timers

extern bool TimerThreadRunning;
#ifdef WIN32
unsigned __stdcall  TimerThread(void *ch)
{
	_tzset();
#else
void * TimerThread(void *ch)
{
	struct timespec req={0};
	req.tv_sec = 0;
	req.tv_nsec = 1000000L;//1ms
#endif

	TimerThreadRunning = true;

	uint64_t t1htimer;
	uint64_t t30mintimer;
	uint64_t t6mintimer;
	uint64_t t5mintimer;
	uint64_t t3mintimer;
	uint64_t t1mintimer;
	uint64_t t5sectimer;
	uint64_t t1sectimer;
	uint64_t t100msectimer;
	uint64_t ltime;

	list<server::stTimedEvent>::iterator iter;

	t1htimer = t30mintimer = t6mintimer = t5mintimer = t3mintimer = t1mintimer = t5sectimer = t1sectimer = t100msectimer = unixtime();

#ifndef WIN32
	pthread_t hSaveThread;
#else
	HANDLE hSaveThread;
#endif
	unsigned uAddr;

	try {
		while (gserver->serverstatus)
		{
			gserver->ltime = ltime = unixtime();

			if (t1sectimer < ltime)
			{
				t1sectimer += 1000;
			}
			if (t100msectimer < ltime)
			{
				MULTILOCK(M_TIMEDLIST, M_CLIENTLIST);
				for (int i = 0; i < gserver->maxplayersloaded; ++i)
				{
					if (gserver->m_clients[i])
					{
						Client * client = gserver->m_clients[i];
						for (int j = 0; j < client->m_city.size(); ++j)
						{
							PlayerCity * city = client->m_city[j];
							vector<PlayerCity::stTroopQueue>::iterator tqiter;
							for (tqiter = client->m_city[j]->m_troopqueue.begin(); tqiter != client->m_city[j]->m_troopqueue.end(); )
							{
								list<PlayerCity::stTroopTrain>::iterator iter;
								iter = tqiter->queue.begin();
								if (iter != tqiter->queue.end() && iter->endtime <= ltime)
								{
									//troops done training
									double gain = iter->count * gserver->GetPrestigeOfAction(DEF_TRAIN, iter->troopid, 1, city->m_level);
									client->m_prestige += gain;
									if (city->m_mayor)
									{
										city->m_mayor->m_experience += gain;
										city->HeroUpdate(city->m_mayor, 2);
									}

									if (tqiter->positionid == -2)
									{
										city->SetForts(iter->troopid, iter->count);
									}
									else
										city->SetTroops(iter->troopid, iter->count);
									tqiter->queue.erase(iter++);
									iter->endtime = ltime + iter->costtime;
								}
								++tqiter;
							}
						}
					}
				}
				UNLOCK(M_TIMEDLIST);
				UNLOCK(M_CLIENTLIST);

				LOCK(M_TIMEDLIST);
				if (gserver->armylist.size() > 0)
					for (iter = gserver->armylist.begin(); iter != gserver->armylist.end(); )
					{

					}
					UNLOCK(M_TIMEDLIST);

					LOCK(M_TIMEDLIST);
					if (gserver->buildinglist.size() > 0)
						for (iter = gserver->buildinglist.begin(); iter != gserver->buildinglist.end(); )
						{
							server::stBuildingAction * ba = (server::stBuildingAction *)iter->data;
							Client * client = ba->client;
							PlayerCity * city = ba->city;
							stBuilding * bldg = ba->city->GetBuilding(ba->positionid);
							if (bldg->endtime < ltime)
							{
								if (bldg->status == 1)
								{
									//build/upgrade
									bldg->status = 0;
									bldg->level++;
									ba->city->SetBuilding(bldg->type, bldg->level, ba->positionid, 0, 0.0, 0.0);

									if (bldg->type == 21)
									{
										for (int i = 0; i < 10; ++i)
										{
											if (city->m_innheroes[i])
											{
												delete city->m_innheroes[i];
												city->m_innheroes[i] = 0;
											}
										}
									}

									amf3object obj = amf3object();
									obj["cmd"] = "server.BuildComplate";
									obj["data"] = amf3object();

									amf3object & data = obj["data"];

									data["buildingBean"] = bldg->ToObject();
									data["castleId"] = city->m_castleid;

									gserver->buildinglist.erase(iter++);

									double gain = gserver->GetPrestigeOfAction(DEF_BUILDING, bldg->type, bldg->level, city->m_level);
									client->m_prestige += gain;

									delete ba;

									client->CalculateResources();
									city->CalculateStats();
									if (city->m_mayor)
									{
										city->m_mayor->m_experience += gain;
										city->HeroUpdate(city->m_mayor, 2);
									}
									//city->CastleUpdate();
									client->PlayerUpdate();
									city->ResourceUpdate();

									if (client->m_connected)
										gserver->SendObject(client->socket, obj);

									continue;
								}
								else if (bldg->status == 2)
								{
									//destruct
									bldg->status = 0;
									bldg->level--;

									stResources res;
									res.food = gserver->m_buildingconfig[bldg->type][bldg->level].food/3;
									res.wood = gserver->m_buildingconfig[bldg->type][bldg->level].wood/3;
									res.stone = gserver->m_buildingconfig[bldg->type][bldg->level].stone/3;
									res.iron = gserver->m_buildingconfig[bldg->type][bldg->level].iron/3;
									res.gold = gserver->m_buildingconfig[bldg->type][bldg->level].gold/3;
									ba->city->m_resources += res;

									if (bldg->level == 0)
										ba->city->SetBuilding(0, 0, ba->positionid, 0, 0.0, 0.0);
									else
										ba->city->SetBuilding(bldg->type, bldg->level, ba->positionid, 0, 0.0, 0.0);

									client->CalculateResources();
									city->CalculateStats();


									amf3object obj = amf3object();
									obj["cmd"] = "server.BuildComplate";
									obj["data"] = amf3object();

									amf3object & data = obj["data"];

									data["buildingBean"] = bldg->ToObject();
									data["castleId"] = city->m_castleid;

									if (client->m_connected)
										gserver->SendObject(client->socket, obj);

									gserver->buildinglist.erase(iter++);

									client->CalculateResources();
									city->CalculateStats();
									city->ResourceUpdate();


									continue;
								}
							}
							++iter;
						}
						UNLOCK(M_TIMEDLIST);

						LOCK(M_TIMEDLIST);
						if (gserver->researchlist.size() > 0)
							for (iter = gserver->researchlist.begin(); iter != gserver->researchlist.end(); )
							{
								server::stResearchAction * ra = (server::stResearchAction *)iter->data;
								Client * client = ra->client;
								PlayerCity * city = ra->city;
								if (ra->researchid != 0)
								{
									if (client->m_research[ra->researchid].endtime < ltime)
									{
										city->m_researching = false;
										client->m_research[ra->researchid].level++;
										client->m_research[ra->researchid].endtime = 0;
										client->m_research[ra->researchid].starttime = 0;
										client->m_research[ra->researchid].castleid = 0;

										amf3object obj = amf3object();
										obj["cmd"] = "server.ResearchCompleteUpdate";
										obj["data"] = amf3object();

										amf3object & data = obj["data"];

										data["castleId"] = city->m_castleid;


										gserver->researchlist.erase(iter++);

										double gain = gserver->GetPrestigeOfAction(DEF_RESEARCH, ra->researchid, client->m_research[ra->researchid].level, city->m_level);
										client->m_prestige += gain;


										client->CalculateResources();
										city->CalculateStats();
										if (city->m_mayor)
										{
											city->m_mayor->m_experience += gain;
											city->HeroUpdate(city->m_mayor, 2);
										}
										//city->CastleUpdate();
										client->PlayerUpdate();
										city->ResourceUpdate();

										if (client->m_connected)
											gserver->SendObject(client->socket, obj);

										delete ra;

										continue;
									}
								}
								else
								{
									gserver->researchlist.erase(iter++);
									continue;
								}
								++iter;
							}
							UNLOCK(M_TIMEDLIST);
							t100msectimer += 100;
			}
			if (t5sectimer < ltime)
			{
				LOCK(M_RANKEDLIST);
				gserver->SortPlayers();
				gserver->SortHeroes();
				gserver->SortCastles();
				gserver->m_alliances->SortAlliances();
				UNLOCK(M_RANKEDLIST);
				for (int i = 0; i < gserver->maxplayersloaded; ++i)
				{
					if (gserver->m_clients[i])
					{
						LOCK(M_CLIENTLIST);
						Client * client = gserver->m_clients[i];
						for (int j = 0; j < client->m_buffs.size(); ++j)
						{
							if (client->m_buffs[j].id.length() != 0)
							{
								if (ltime > client->m_buffs[j].endtime)
								{
									if (client->m_buffs[j].id == "PlayerPeaceBuff")
									{
										client->SetBuff("PlayerPeaceCoolDownBuff", "Truce Agreement in cooldown.", ltime + (12*60*60*1000));
									}
									client->RemoveBuff(client->m_buffs[j].id);
								}
							}
						}
						UNLOCK(M_CLIENTLIST);
					}
				}

				t5sectimer += 5000;
			}
			if (t1mintimer < ltime)
			{
				for (int i = 0; i < gserver->maxplayersloaded; ++i)
				{
					if (gserver->m_clients[i])
					{
						gserver->m_clients[i]->CalculateResources();

						if (gserver->m_clients[i]->m_socknum > 0 && (gserver->m_clients[i]->m_currentcityindex != -1) && gserver->m_clients[i]->m_city[gserver->m_clients[i]->m_currentcityindex])
						{
							gserver->m_clients[i]->m_city[gserver->m_clients[i]->m_currentcityindex]->ResourceUpdate();
						}
					}
				}

				LOCK(M_TIMEDLIST);
				gserver->CheckRankSearchTimeouts(ltime);
				UNLOCK(M_TIMEDLIST);

				t1mintimer += 60000;
			}
			if (t3mintimer < ltime)
			{
				hSaveThread = (HANDLE)_beginthreadex(0, 0, SaveData, 0, 0, &uAddr);
				t3mintimer += 180000;
			}
			if (t5mintimer < ltime)
			{
#ifndef WIN32
			if (pthread_create(&hSaveThread, NULL, SaveData, 0))
			{
				SFERROR("pthread_create");
			}
#else
				//hSaveThread = (HANDLE)_beginthreadex(0, 0, SaveData, 0, 0, &uAddr);
#endif
				t5mintimer += 300000;
			}
			if (t6mintimer < ltime)
			{
				LOCK(M_CASTLELIST);
				for (int i = 0; i < gserver->m_city.size(); ++i)
				{
					if (gserver->m_city.at(i)->m_type == NPC)
						((NpcCity*)gserver->m_city.at(i))->CalculateStats(true, true);
					if (gserver->m_city.at(i)->m_type == CASTLE)
						((PlayerCity*)gserver->m_city.at(i))->RecacluateCityStats();
				}
				UNLOCK(M_CASTLELIST);
				t6mintimer += 360000;
			}
			if (t30mintimer < ltime)
			{
				LOCK(M_RANKEDLIST);
				gserver->SortPlayers();
				gserver->SortHeroes();
				gserver->SortCastles();
				gserver->m_alliances->SortAlliances();
				UNLOCK(M_RANKEDLIST);
				t30mintimer += 1800000;
			}
			if (t1htimer < ltime)
			{

				t1htimer += 3600000;
			}
#ifdef WIN32
			Sleep(1);
#else
			nanosleep(&req,NULL);
#endif
		}
		TimerThreadRunning = false;
	}
	catch (...)
	{
		TimerThreadRunning = false;
	}
#ifdef WIN32
	_endthread();
#endif
	return 0;
}

#ifdef WIN32
unsigned __stdcall  SaveData(void *ch)
{
#else
void * SaveData(void *ch)
{
#endif
	Log("Saving data.");

// #ifdef __WIN32__
// 	_endthread();
// 	return 0;
// #endif

	Map * newmap;
	Client * client;

// 	newmap = new Map(server);
// 
// 	Log("Loading old map data.");
// 
// 	for (int x = 0; x < (DEF_MAPSIZE*DEF_MAPSIZE); x += (DEF_MAPSIZE*DEF_MAPSIZE)/10)
// 	{
// 		char query[1024];
// 		sprintf(query, "SELECT `id`,`ownerid`,`type`,`level` FROM `tiles` ORDER BY `id` ASC LIMIT %d,%d", x, ((DEF_MAPSIZE*DEF_MAPSIZE)/10));
// 		mysql_query(msql->mySQL, query);
// 		st_mysql_res* m_pQueryResult;
// 		m_pQueryResult = mysql_store_result(msql->mySQL);
// 		int m_iRows = (int)mysql_num_rows(m_pQueryResult);
// 		int m_iFields = mysql_num_fields(m_pQueryResult);
// 		//MYSQL_FIELD ** field = new MYSQL_FIELD*[m_iFields+1];;
// 		MYSQL_ROW myRow;
// 		mysql_field_seek(m_pQueryResult, 0);
// 
// 
// 
// 		for (int i = 0; i < m_iRows; ++i)
// 		{
// 			myRow = mysql_fetch_row(m_pQueryResult);
// 			int64_t id = _atoi64(myRow[0]);
// 			int64_t ownerid = _atoi64(myRow[1]);
// 			int64_t type = _atoi64(myRow[2]);
// 			int64_t level = _atoi64(myRow[3]);
// 
// 			newmap->m_tile[id].m_id = id;
// 			newmap->m_tile[id].m_ownerid = ownerid;
// 			newmap->m_tile[id].m_type = type;
// 			newmap->m_tile[id].m_level = level;
// 
// 			if ((id+1)%((DEF_MAPSIZE*DEF_MAPSIZE)/10) == 0)
// 			{
// 				Log("%d%%", int((double(double(id+1)/(DEF_MAPSIZE*DEF_MAPSIZE)))*double(100)));
// 			}
// 		}
// 		mysql_free_result(m_pQueryResult);
// 	}
// 
// 	msql->Reset();

	Log("Saving tile data.");


#pragma region old save
	//old save
	/*string updatestr = "";
	char temp[40];
	bool btype, blevel, bowner;
	for (int i = 0; i < DEF_MAPSIZE*DEF_MAPSIZE; ++i)
	{
		btype = blevel = bowner = false;
		updatestr += "UPDATE `tiles` SET ";
		if (newmap->m_tile[i].m_type != server->m_map->m_tile[i].m_type)
		{
			sprintf_s(temp, 40, "`type`=%d", server->m_map->m_tile[i].m_type);
			updatestr += temp;
			btype = true;
		}
		if (newmap->m_tile[i].m_level != server->m_map->m_tile[i].m_level)
		{
			if (btype)
			{
				sprintf_s(temp, 40, ",`level`=%d", server->m_map->m_tile[i].m_level);
				updatestr += temp;
			}
			else
			{
				sprintf_s(temp, 40, "`level`=%d", server->m_map->m_tile[i].m_level);
				updatestr += temp;
			}
			blevel = true;
		}
		if (newmap->m_tile[i].m_ownerid != server->m_map->m_tile[i].m_ownerid)
		{
			if (blevel || btype)
			{
				sprintf_s(temp, 40, ",`ownerid`=%d", server->m_map->m_tile[i].m_ownerid);
				updatestr += temp;
			}
			else
			{
				sprintf_s(temp, 40, "`ownerid`=%d", server->m_map->m_tile[i].m_ownerid);
				updatestr += temp;
			}
			bowner = true;
		}
		if (btype || blevel || bowner)
		{
			sprintf_s(temp, 40, " WHERE `id`=%d; ", i);
			updatestr += temp;
		}
		if ((i+1)%((DEF_MAPSIZE*DEF_MAPSIZE)/10) == 0)
		{
			Log("%d%%", int((double(double(i+1)/(DEF_MAPSIZE*DEF_MAPSIZE)))*double(100)));
		}
		//if (i%10 == 9)
		if (i%5 == 4)
		{
			msql->Update((char*)updatestr.c_str());
			msql->Reset();
			updatestr = "";
		}

	}*/
#pragma endregion

	/*
	 *
	 *
	 UPDATE `tiles`
		SET `level` = CASE `id`
			WHEN 0 THEN 8
			WHEN 1 THEN 2
			WHEN 2 THEN 4
		END
	 WHERE `id` IN (0,1,2);
	 **/

#ifndef DEF_NOMAPDATA

	try
	{
		LOCK(M_MAP);
		string fullstr = "";
		string updatestr = "UPDATE `tiles` SET ";
		string typehstr  = " `type` = CASE `id` ";
		string levelhstr = " `level` = CASE `id` ";
		string ownerhstr = " `ownerid` = CASE `id` ";
		string typestr  = "";
		string levelstr = "";
		string ownerstr = "";
		string endstr = " END, ";
		string endstr2 = " END ";
		string wherestr = "WHERE `id` IN (";
		string instr  = "";
		char temp[40];
		bool btype, blevel, bowner;
		for (int i = 0; i < DEF_MAPSIZE*DEF_MAPSIZE; ++i)
		{
			stringstream ss;
			ss << " WHEN " << i << " THEN " << gserver->m_map->m_tile[i].m_type;
			typestr = ss.str();
			ss.str("");
			ss << " WHEN " << i << " THEN " << gserver->m_map->m_tile[i].m_level;
			levelstr = ss.str();
			ss.str("");
			ss << " WHEN " << i << " THEN " << gserver->m_map->m_tile[i].m_ownerid;
			ownerstr = ss.str();
			ss.str("");
			ss << i;
			instr = ss.str();
			ss.str("");
	
			if (i%200 == 199)
			{
				fullstr = updatestr + typehstr + typestr + endstr + levelhstr + levelstr + endstr + ownerhstr + ownerstr + endstr2 + wherestr + instr;
				fullstr += ");";
				gserver->msql->Update((char*)fullstr.c_str());
				gserver->msql->Reset();
				instr = typestr = levelstr = ownerstr = fullstr = "";
			}
			else
			{
				instr += ",";
			}
			if ((i+1)%((DEF_MAPSIZE*DEF_MAPSIZE)/20) == 0)
			{
				//Log("%d%%", int((double(double(i+1)/(DEF_MAPSIZE*DEF_MAPSIZE)))*double(100)));
			}
		}
		UNLOCK(M_MAP);
	}
	catch (std::exception& e)
	{
		Log("SaveData() Exception: %s", e.what());
		UNLOCK(M_MAP);
	}
	catch(...)
	{
		Log("SaveData() Exception.");
		UNLOCK(M_MAP);
	}
	//delete newmap;

#endif

	Log("Saving alliance data.");

	try
	{
		LOCK(M_ALLIANCELIST);
		for (int i = 0; i < DEF_MAXALLIANCES; ++i)
		{
			Alliance * alliance = gserver->m_alliances->m_alliances[i];
			if (!alliance)
				continue;

			gserver->msql->Select("SELECT COUNT(*) AS a FROM `alliances` WHERE `id`=%d", alliance->m_allianceid);
			gserver->msql->Fetch();
			int alliancecount = gserver->msql->GetInt(0, "a");

			string members;
			string enemies;
			string allies;
			string neutrals;


			stringstream ss;

			for (int j = 0; j < alliance->m_members.size(); ++j)
			{
				if (j != 0)
					ss << "|";
				Client * temp = gserver->GetClient(alliance->m_members[j].clientid);
				ss << temp->m_accountid << "," << temp->m_alliancerank;
			}
			members = ss.str();
			ss.str("");
			for (int j = 0; j < alliance->m_enemies.size(); ++j)
			{
				if (j != 0)
					ss << "|";
				ss << alliance->m_enemies[j];
			}
			enemies = ss.str();
			ss.str("");
			for (int j = 0; j < alliance->m_allies.size(); ++j)
			{
				if (j != 0)
					ss << "|";
				ss << alliance->m_allies[j];
			}
			allies = ss.str();
			ss.str("");
			for (int j = 0; j < alliance->m_neutral.size(); ++j)
			{
				if (j != 0)
					ss << "|";
				ss << alliance->m_neutral[j];
			}
			neutrals = ss.str();
			ss.str("");

			members = makesafe(members);
			enemies = makesafe(enemies);
			allies = makesafe(allies);
			neutrals = makesafe(neutrals);

			string note, intro, motd;

			note = makesafe(alliance->m_note);
			intro = makesafe(alliance->m_intro);
			motd = makesafe(alliance->m_motd);


			if (alliancecount > 0)
			{
				//update
				gserver->msql->Update("UPDATE `alliances` SET `leader`=%d,`name`='%s',`founder`='%s',`note`='%s',`intro`='%s',`motd`='%s',`members`='%s',`enemies`='%s',`allies`='%s',`neutrals`='%s' WHERE `id`=%d LIMIT 1;",
							alliance->m_ownerid, alliance->m_name.c_str(), alliance->m_founder.c_str(), note.c_str(), intro.c_str(), motd.c_str(), members.c_str(), enemies.c_str(), allies.c_str(), neutrals.c_str(), alliance->m_allianceid);
			}
			else
			{
				//insert
				gserver->msql->Insert("INSERT INTO `alliances` (`id`,`leader`,`name`,`founder`,`note`,`intro`,`motd`,`members`,`enemies`,`allies`,`neutrals`) VALUES (%d,%d,'%s','%s','%s','%s','%s','%s','%s','%s','%s');",
							 alliance->m_allianceid, alliance->m_ownerid,alliance->m_name.c_str(), alliance->m_founder.c_str(), alliance->m_note.c_str(), alliance->m_intro.c_str(), alliance->m_motd.c_str(), members.c_str(), enemies.c_str(), allies.c_str(), neutrals.c_str());
			}
		}
		UNLOCK(M_ALLIANCELIST);
	}
	catch (std::exception& e)
	{
		Log("SaveData() Exception: %s", e.what());
		UNLOCK(M_ALLIANCELIST);
	}
	catch(...)
	{
		Log("SaveData() Exception.");
		UNLOCK(M_ALLIANCELIST);
	}
	gserver->msql->Reset();

	Log("Saving player data.");

	char temp1[1000];

	try
	{
		LOCK(M_CLIENTLIST);
		for (int i = 0; i < gserver->maxplayersloaded; ++i)
		{
			if (!gserver->m_clients[i])
				continue;
			Client * tempclient = gserver->m_clients[i];
			string buffs;
			string research;
			string items;
			string misc;

			memset(temp1, 0, 1000);

			Client::stResearch * rsrch;
			for (int j = 0; j < 25; ++j)
			{
				if (j != 0)
					research += "|";
				rsrch = &gserver->m_clients[i]->m_research[j];
				sprintf_s(temp1, 1000, "%d,%d,%d,"DBL","DBL"", j, rsrch->level, rsrch->castleid, rsrch->starttime, rsrch->endtime);
				research += temp1;
			}

			for (int j = 0; j < gserver->m_clients[i]->m_buffs.size(); ++j)
			{
				if (gserver->m_clients[i]->m_buffs[j].id.length() > 0)
				{
					if (j != 0)
						buffs += "|";
					sprintf_s(temp1, 1000, "%s,%s,"DBL"", (char*)gserver->m_clients[i]->m_buffs[j].id.c_str(), (char*)gserver->m_clients[i]->m_buffs[j].desc.c_str(), gserver->m_clients[i]->m_buffs[j].endtime);
					buffs += temp1;
				}
			}

			//sprintf_s(temp1, 1000, "%d", tempclient->m_cents);
			//misc = temp1;

			{
				stringstream ss;
				for (int j = 1; j < DEF_MAXITEMS; ++j)
				{
					if (gserver->m_clients[i]->m_items[j].id.length() > 0)
					{
						if (j != 1)
							ss << "|";
						ss << gserver->m_clients[i]->m_items[j].id << "," << gserver->m_clients[i]->m_items[j].count;
					}
				}
				items = ss.str();
			}
			{
				stringstream ss;
				ss << tempclient->m_icon << "," << tempclient->m_allianceapply << "," << tempclient->m_changedface;
				misc = ss.str();
			}
			gserver->msql->Update("UPDATE `accounts` SET `username`='%s',`lastlogin`="DBL",`ipaddress`='%s',`status`=%d,`buffs`='%s',`flag`='%s',`faceurl`='%s',`research`='%s',`items`='%s',`allianceid`=%d,`alliancerank`=%d,`misc`='%s',`cents`=%d,`prestige`="DBL",`honor`="DBL" WHERE `accountid`=%d;",
				(char*)tempclient->m_playername.c_str(), tempclient->m_lastlogin, tempclient->m_ipaddress, tempclient->m_status, (char*)buffs.c_str(),
				(char*)tempclient->m_flag.c_str(), (char*)tempclient->m_faceurl.c_str(), (char*)research.c_str(), (char*)items.c_str(), tempclient->m_allianceid, tempclient->m_alliancerank, (char*)misc.c_str(), tempclient->m_cents, tempclient->m_prestige, tempclient->m_honor, tempclient->m_accountid);
			gserver->msql->Reset();

			try
			{
				LOCK(M_CASTLELIST);
				if (tempclient)
				for (int k = 0; k < gserver->m_clients[i]->m_city.size(); ++k)
				{
					PlayerCity * tempcity = gserver->m_clients[i]->m_city[k];
					string misc2 = "";
					string transingtrades = "";
					string troop = "";
					string buildings = "";
					string fortification = "";
					string trades = "";

					gserver->msql->Select("SELECT COUNT(*) AS a FROM `cities` WHERE `id`=%d", tempcity->m_castleid);
					gserver->msql->Fetch();
					int citycount = gserver->msql->GetInt(0, "a");

					sprintf_s(temp1, 1000, "%d,%d,%d,%d,%d", tempcity->m_forts.traps, tempcity->m_forts.abatis, tempcity->m_forts.towers, tempcity->m_forts.logs, tempcity->m_forts.trebs);
					fortification = temp1;

					for (int j = 1; j < 12; ++j)
					{
						if (j != 1)
							troop += "|";
						sprintf_s(temp1, 1000, XI64, tempcity->GetTroops(j));
						troop += temp1;
					}

					sprintf_s(temp1, 1000, "%d,"DBL","DBL","DBL","DBL","DBL",%d,%d,"DBL"", tempcity->m_population, tempcity->m_workrate.gold, tempcity->m_workrate.food, tempcity->m_workrate.wood, tempcity->m_workrate.iron, tempcity->m_workrate.stone, (int)tempcity->m_loyalty, (int)tempcity->m_grievance, tempcity->m_timers.updateresources);
					misc2 = temp1;

					for (int j = 0; j < 35; ++j)
					{
						if (tempcity->m_innerbuildings[j].type > 0)
						{
							if (j != 0)
								buildings += "|";
							sprintf_s(temp1, 1000, "%d,%d,%d,%d,"DBL","DBL"", tempcity->m_innerbuildings[j].type, tempcity->m_innerbuildings[j].level, tempcity->m_innerbuildings[j].id, tempcity->m_innerbuildings[j].status, tempcity->m_innerbuildings[j].starttime, tempcity->m_innerbuildings[j].endtime);
							buildings += temp1;
						}
					}
					for (int j = 0; j <= 40; ++j)
					{
						if (tempcity->m_outerbuildings[j].type > 0)
						{
							buildings += "|";
							sprintf_s(temp1, 1000, "%d,%d,%d,%d,"DBL","DBL"", tempcity->m_outerbuildings[j].type, tempcity->m_outerbuildings[j].level, tempcity->m_outerbuildings[j].id, tempcity->m_outerbuildings[j].status, tempcity->m_outerbuildings[j].starttime, tempcity->m_outerbuildings[j].endtime);
							buildings += temp1;
						}
					}

					if (citycount > 0)
					{
						//update
						gserver->msql->Update("UPDATE `cities` SET `misc`='%s',`status`=%d,`allowalliance`=%d,`logurl`='%s',`fieldid`=%d,`transingtrades`='%s',`troop`='%s',`name`='%s',`buildings`='%s',`fortification`='%s',`trades`='%s', \
									 `gooutforbattle`=%d,`hasenemy`=%d,`gold`="DBL",`food`="DBL",`wood`="DBL",`iron`="DBL",`stone`="DBL",`creation`="DBL" WHERE `id`="XI64" LIMIT 1;",
									 (char*)misc2.c_str(), (int)tempcity->m_status, (int)tempcity->m_allowalliance, (char*)tempcity->m_logurl.c_str(), tempcity->m_tileid, (char*)transingtrades.c_str(),
									 (char*)troop.c_str(), (char*)tempcity->m_cityname.c_str(), (char*)buildings.c_str(), (char*)fortification.c_str(), (char*)trades.c_str(), (int)tempcity->m_gooutforbattle, (int)tempcity->m_hasenemy,
									 tempcity->m_resources.gold, tempcity->m_resources.food, tempcity->m_resources.wood, tempcity->m_resources.iron, tempcity->m_resources.stone, tempcity->m_creation, tempcity->m_castleid);
					}
					else
					{
						//insert
						gserver->msql->Insert("INSERT INTO `cities` (`accountid`,`misc`,`status`,`allowalliance`,`logurl`,`fieldid`,`transingtrades`,`troop`,`name`,`buildings`,`fortification`,`trades`,`gooutforbattle`,`hasenemy`,`gold`,`food`,`wood`,`iron`,`stone`,`creation`) \
									 VALUES (%d, '%s',%d,%d,'%s',%d,'%s','%s','%s','%s','%s','%s',%d,%d,"DBL","DBL","DBL","DBL","DBL","DBL");",
									 tempcity->m_accountid, (char*)misc2.c_str(), (int)tempcity->m_status, (int)tempcity->m_allowalliance, (char*)tempcity->m_logurl.c_str(), tempcity->m_tileid, (char*)transingtrades.c_str(), (char*)tempcity->m_cityname.c_str(), (char*)buildings.c_str(), (char*)fortification.c_str(), (char*)trades.c_str(), (int)tempcity->m_gooutforbattle,
									 (int)tempcity->m_hasenemy, tempcity->m_resources.gold, tempcity->m_resources.food, tempcity->m_resources.wood, tempcity->m_resources.iron, tempcity->m_resources.stone, tempcity->m_creation);
					}


					try
					{
						LOCK(M_HEROLIST);
						for (int a = 0; a < 10; ++a)
						{
							Hero * temphero = tempcity->m_heroes[a];

							if (temphero)
							{
								gserver->msql->Select("SELECT COUNT(*) AS a FROM `heroes` WHERE `id`=%d", temphero->m_id);
								gserver->msql->Fetch();
								int herocount = gserver->msql->GetInt(0, "a");

								if (herocount > 0)
								{
									//update
									gserver->msql->Update("UPDATE `heroes` SET `ownerid`="XI64",`castleid`="XI64",`name`='%s',`status`=%d,`itemid`=%d,`itemamount`=%d,`basestratagem`=%d,`stratagem`=%d,`stratagemadded`=%d,`stratagembuffadded`=%d,\
												 `basepower`=%d,`power`=%d,`poweradded`=%d,`powerbuffadded`=%d,`basemanagement`=%d,`management`=%d,`managementadded`=%d,`managementbuffadded`=%d,\
												 `logurl`='%s',`remainpoint`=%d,`level`=%d,`upgradeexp`="DBL",`experience`="DBL",`loyalty`=%d WHERE `id`="XI64" LIMIT 1;",
												 tempcity->m_accountid, tempcity->m_castleid, temphero->m_name.c_str(), (int)temphero->m_status, temphero->m_itemid, temphero->m_itemamount, (int)temphero->m_basestratagem, (int)temphero->m_stratagem, (int)temphero->m_stratagemadded, (int)temphero->m_stratagembuffadded,
												 (int)temphero->m_basepower, (int)temphero->m_power, (int)temphero->m_poweradded, (int)temphero->m_powerbuffadded, (int)temphero->m_basemanagement, (int)temphero->m_management, (int)temphero->m_managementadded, (int)temphero->m_managementbuffadded,
												 (char*)temphero->m_logourl.c_str(), (int)temphero->m_remainpoint, (int)temphero->m_level, temphero->m_upgradeexp, temphero->m_experience, (int)temphero->m_loyalty, temphero->m_id);
								}
								else
								{
									//insert
									gserver->msql->Insert("INSERT INTO `heroes` (`id`,`ownerid`,`castleid`,`name`,`status`,`itemid`,`itemamount`,`basestratagem`,`stratagem`,`stratagemadded`,`stratagembuffadded`,\
												 `basepower`,`power`,`poweradded`,`powerbuffadded`,`basemanagement`,`management`,`managementadded`,`managementbuffadded`,\
												 `logurl`,`remainpoint`,`level`,`upgradeexp`,`experience`,`loyalty`) VALUES ("XI64","XI64","XI64",'%s',%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,'%s',%d,%d,"DBL","DBL",%d);",
												 temphero->m_id, tempcity->m_accountid, tempcity->m_castleid, temphero->m_name.c_str(), (int)temphero->m_status, temphero->m_itemid, temphero->m_itemamount, (int)temphero->m_basestratagem, (int)temphero->m_stratagem, (int)temphero->m_stratagemadded, (int)temphero->m_stratagembuffadded,
												 (int)temphero->m_basepower, (int)temphero->m_power, (int)temphero->m_poweradded, (int)temphero->m_powerbuffadded, (int)temphero->m_basemanagement, (int)temphero->m_management, (int)temphero->m_managementadded, (int)temphero->m_managementbuffadded,
												 (char*)temphero->m_logourl.c_str(), (int)temphero->m_remainpoint, (int)temphero->m_level, temphero->m_upgradeexp, temphero->m_experience, (int)temphero->m_loyalty);
								}
							}
						}
						UNLOCK(M_HEROLIST);
					}
					catch (std::exception& e)
					{
						Log("SaveData() Exception: %s", e.what());
						UNLOCK(M_HEROLIST);
					}
					catch(...)
					{
						Log("SaveData() Exception.");
						UNLOCK(M_HEROLIST);
					}

				}
				UNLOCK(M_CASTLELIST);
			}
			catch (std::exception& e)
			{
				Log("SaveData() Exception: %s", e.what());
				UNLOCK(M_CASTLELIST);
			}
			catch(...)
			{
				Log("SaveData() Exception.");
				UNLOCK(M_CASTLELIST);
			}
			if ((i+1)%(gserver->maxplayersloaded/10) == 0)
			{
				Log("%d%%", int((double(double(i+1)/gserver->maxplayersloaded))*double(100)));
			}
		}
		UNLOCK(M_CLIENTLIST);
	}
	catch (std::exception& e)
	{
		Log("SaveData() Exception: %s", e.what());
		UNLOCK(M_CLIENTLIST);
	}
	catch(...)
	{
		Log("SaveData() Exception.");
		UNLOCK(M_CLIENTLIST);
	}

	vector<int32_t>::iterator iter;

	LOCK(M_DELETELIST);
	if (gserver->m_deletedhero.size() > 0)
	{
		for (iter = gserver->m_deletedhero.begin(); iter != gserver->m_deletedhero.end(); ++iter)
		{
			gserver->msql->Delete("DELETE FROM `heroes` WHERE `id`=%d LIMIT 1", *iter);
		}
	}
	if (gserver->m_deletedcity.size() > 0)
	{
		for (iter = gserver->m_deletedcity.begin(); iter != gserver->m_deletedcity.end(); ++iter)
		{
			gserver->msql->Delete("DELETE FROM `cities` WHERE `id`=%d LIMIT 1", *iter);
		}
	}
	UNLOCK(M_DELETELIST);


#ifdef WIN32
	_endthread();
#endif
	return 0;
}

// Server code

server::stItem * server::GetItem(string name)
{
	for (int i = 0; i < DEF_MAXITEMS; ++i)
	{
		if (m_items[i].name == name)
			return &m_items[i];
	}
	return 0;
}
bool server::comparebuy(stMarketEntry first, stMarketEntry second)
{
	if (first.price > second.price)
		return true;
	else
		return false;
}
bool server::comparesell(stMarketEntry first, stMarketEntry second)
{
	if (first.price < second.price)
		return true;
	else
		return false;
}
double server::GetPrestigeOfAction(int8_t action, int8_t id, int8_t level, int8_t thlevel)
{
	double prestige = 0;
	switch (action)
	{
	case DEF_RESEARCH:
		switch(id)
		{
		case T_AGRICULTURE:
			prestige = 26*2;
			break;
		case T_LUMBERING:
			prestige = 31*2;
			break;
		case T_MASONRY:
			prestige = 41*2;
			break;
		case T_MINING:
			prestige = 55*2;
			break;
		case T_METALCASTING:
			prestige = 57*2;//not final
			break;
		case T_INFORMATICS:
			prestige = 59*2;//not final
			break;
		case T_MILITARYSCIENCE:
			prestige = 61*2;
			break;
		case T_MILITARYTRADITION:
			prestige = 78*2;
			break;
		case T_IRONWORKING:
			prestige = 26*2;//not final
			break;
		case T_LOGISTICS:
			prestige = 26*2;//not final
			break;
		case T_COMPASS:
			prestige = 26*2;//not final
			break;
		case T_HORSEBACKRIDING:
			prestige = 26*2;//not final
			break;
		case T_ARCHERY:
			prestige = 26*2;//not final
			break;
		case T_STOCKPILE:
			prestige = 26*2;//not final
			break;
		case T_MEDICINE:
			prestige = 26*2;//not final
			break;
		case T_CONSTRUCTION:
			prestige = 26*2;//not final
			break;
		case T_ENGINEERING:
			prestige = 26*2;//not final
			break;
		case T_MACHINERY:
			prestige = 26*2;//not final
			break;
		case T_PRIVATEERING:
			prestige = 26*2;//not final
			break;
		}
		break;
	case DEF_BUILDING:
		switch(id)
		{
		case B_COTTAGE:
			prestige = 4;
			break;
		case B_BARRACKS:
			prestige = 28;
			break;
		case B_WAREHOUSE:
			prestige = 21;
			break;
		case B_SAWMILL:
			prestige = 7;
			break;
		case B_STONEMINE:
			prestige = 9;
			break;
		case B_IRONMINE:
			prestige = 11;
			break;
		case B_FARM:
			prestige = 5;
			break;
		case B_STABLE:
			prestige = 36;
			break;
		case B_INN:
			prestige = 26;
			break;
		case B_FORGE:
			prestige = 26;
			break;
		case B_MARKETPLACE:
			prestige = 32;
			break;
		case B_RELIEFSTATION:
			prestige = 26;//not final
			break;
		case B_ACADEMY:
			prestige = 30;
			break;
		case B_WORKSHOP:
			prestige = 38;
			break;
		case B_FEASTINGHALL:
			prestige = 35;
			break;
		case B_EMBASSY:
			prestige = 18;
			break;
		case B_RALLYSPOT:
			prestige = 25;
			break;
		case B_BEACONTOWER:
			prestige = 39;
			break;
		case B_TOWNHALL:
			prestige = 112;
			break;
		case B_WALLS:
			prestige = 128;
			break;
		}
		break;
	case DEF_TRAIN:
		switch(id)
		{
		case TR_WORKER:
			prestige = 0.5;
			break;
		case TR_WARRIOR:
			prestige = 0.5;
			break;
		case TR_SCOUT:
			prestige = 1.0;//not final
			break;
		case TR_PIKE:
			prestige = 2.0;
			break;
		case TR_SWORDS:
			prestige = 3;//not final
			break;
		case TR_ARCHER:
			prestige = 4;//not final
			break;
		case TR_TRANSPORTER:
			prestige = 5;//not final
			break;
		case TR_CAVALRY:
			prestige = 6;//not final
			break;
		case TR_CATAPHRACT:
			prestige = 7;//not final
			break;
		case TR_BALLISTA:
			prestige = 8;//not final
			break;
		case TR_RAM:
			prestige = 9;//not final
			break;
		case TR_CATAPULT:
			prestige = 10;//not final
			break;
		case TR_TRAP:
			prestige = 1;//not final
			break;
		case TR_ABATIS:
			prestige = 2;//not final
			break;
		case TR_ARCHERTOWER:
			prestige = 3;//not final
			break;
		case TR_ROLLINGLOG:
			prestige = 4;//not final
			break;
		case TR_TREBUCHET:
			prestige = 5;//not final
			break;
		}
		break;
	}
	for (int i = 0; i < level; ++i)
		prestige *= 2;
	for (int i = 0; i < thlevel-1; ++i)
		prestige /= 2;
	return prestige;
}
void server::AddTimedEvent(stTimedEvent & te)
{
	switch (te.type)
	{
	case DEF_TIMEDARMY:
		armylist.push_back(te);
		break;
	case DEF_TIMEDBUILDING:
		buildinglist.push_back(te);
		break;
	case DEF_TIMEDRESEARCH:
		researchlist.push_back(te);
		break;
	}
}
void server::MassDisconnect()
{
	for (int i = 0; i < maxplayersloaded; ++i)
		if (m_clients[i]->socket)
		{
			amf3object obj;
			obj["cmd"] = "server.SystemInfoMsg";
			obj["data"] = amf3object();
			amf3object & data = obj["data"];
			data["alliance"] = false;
			data["tV"] = false;
			data["noSenderSystemInfo"] = true;
			data["msg"] = "Server shutting down.";

			SendObject(m_clients[i]->socket, obj);

			m_clients[i]->socket->stop();
			//shutdown(sockets->fdsockets[i].fdsock, 0);
			//closesocket(sockets->fdsockets[i].fdsock);
		}
}
void server::MassMessage(string str, bool nosender /* = false*/, bool tv /* = false*/, bool all /* = false*/)
{
	for (int i = 0; i < maxplayersloaded; ++i)
		if (m_clients[i] && m_clients[i]->socket)
		{
			amf3object obj;
			obj["cmd"] = "server.SystemInfoMsg";
			obj["data"] = amf3object();
			amf3object & data = obj["data"];
			data["alliance"] = all;
			data["tV"] = tv;
			data["noSenderSystemInfo"] = nosender;
			data["msg"] = str;

			SendObject(m_clients[i]->socket, obj);
		}
}
int server::RandomStat()
{
	int rnd = rand()%10000;
	if ((rnd >= 0) && (rnd < 6500))
	{
		return rand()%21 + 15; // 15-35
	}
	else if ((rnd >= 6500) && (rnd < 8500))
	{
		return rand()%21 + 30; // 30-50
	}
	else if ((rnd >= 8500) && (rnd < 9500))
	{
		return rand()%16 + 45; // 45-60
	}
	else if ((rnd >= 9500) && (rnd < 9900))
	{
		return rand()%11 + 60; // 60-70
	}
	else if ((rnd >= 9900) && (rnd < 9950))
	{
		return rand()%6 + 70; // 70-75
	}
	else if ((rnd >= 9950) && (rnd < 9975))
	{
		return rand()%6 + 75; // 75-80
	}
	else if ((rnd >= 9975) && (rnd < 10000))
	{
		return rand()%6 + 80; // 80-85
	}
	return 10;
}
Hero * server::CreateRandomHero(int innlevel)
{
	Hero * hero = new Hero();

	int maxherolevel = innlevel * 5;

	hero->m_level = (rand()%maxherolevel)+1;
	hero->m_basemanagement = RandomStat();
	hero->m_basestratagem = RandomStat();
	hero->m_basepower = RandomStat();

	int remainpoints = hero->m_level;

	hero->m_power = rand()%remainpoints;
	remainpoints -= hero->m_power;
	hero->m_power += hero->m_basepower;
	if (remainpoints > 0)
	{
		hero->m_management = rand()%remainpoints;
		remainpoints -= hero->m_management;
		hero->m_management += hero->m_basemanagement;
	}
	if (remainpoints > 0)
	{
		hero->m_stratagem = remainpoints;
		remainpoints -= hero->m_stratagem;
		hero->m_stratagem += hero->m_basestratagem;
	}


	hero->m_loyalty = 70;
	hero->m_experience = 0;
	hero->m_upgradeexp = hero->m_level * hero->m_level * 100;
	hero->m_id = 0;
	char tempstr[30];
	sprintf(tempstr, "Test Name%d%d%d", hero->m_power, hero->m_management, hero->m_stratagem);
	hero->m_name = tempstr;
	hero->m_logourl = "images/icon/player/faceA20.jpg";

	return hero;
}
bool server::compareprestige(stClientRank first, stClientRank second)
{
	if (first.client->m_prestige > second.client->m_prestige)
		return true;
	else
		return false;
}
bool server::comparehonor(stClientRank first, stClientRank second)
{
	if (first.client->m_honor > second.client->m_honor)
		return true;
	else
		return false;
}
bool server::comparetitle(stClientRank first, stClientRank second)
{
	if (first.client->m_title > second.client->m_title)
		return true;
	else
		return false;
}
bool server::comparepop(stClientRank first, stClientRank second)
{
	if (first.client->m_population > second.client->m_population)
		return true;
	else
		return false;
}
bool server::comparecities(stClientRank first, stClientRank second)
{
	if (first.client->m_citycount > second.client->m_citycount)
		return true;
	else
		return false;
}
void server::SortPlayers()
{

	m_prestigerank.clear();
	m_honorrank.clear();
	m_titlerank.clear();
	m_populationrank.clear();
	m_citiesrank.clear();

	for (int i = 1; i < maxplayersloaded; ++i)
	{
		if (m_clients[i])
		{
			stClientRank rank;
			rank.client = m_clients[i];
			rank.rank = 0;

			m_prestigerank.push_back(rank);
			m_honorrank.push_back(rank);
			m_titlerank.push_back(rank);
			m_populationrank.push_back(rank);
			m_citiesrank.push_back(rank);
		}
	}

	m_prestigerank.sort(compareprestige);
	m_honorrank.sort(comparehonor);
	m_titlerank.sort(comparetitle);
	m_populationrank.sort(comparepop);
	m_citiesrank.sort(comparecities);


	list<stClientRank>::iterator iter;
	int num = 1;
	for ( iter = m_prestigerank.begin() ; iter != m_prestigerank.end(); ++iter )
	{
		iter->rank = num;
		iter->client->m_prestigerank = num++;
		if (iter->client->m_connected)
			iter->client->PlayerUpdate();
	}
	num = 1;
	for ( iter = m_honorrank.begin() ; iter != m_honorrank.end(); ++iter )
	{
		iter->rank = num++;
	}
	num = 1;
	for ( iter = m_titlerank.begin() ; iter != m_titlerank.end(); ++iter )
	{
		iter->rank = num++;
	}
	num = 1;
	for ( iter = m_populationrank.begin() ; iter != m_populationrank.end(); ++iter )
	{
		iter->rank = num++;
	}
	num = 1;
	for ( iter = m_citiesrank.begin() ; iter != m_citiesrank.end(); ++iter )
	{
		iter->rank = num++;
	}
}
bool server::comparestratagem(stHeroRank first, stHeroRank second)
{
	if (first.stratagem > second.stratagem)
		return true;
	else
		return false;
}
bool server::comparepower(stHeroRank first, stHeroRank second)
{
	if (first.power > second.power)
		return true;
	else
		return false;
}
bool server::comparemanagement(stHeroRank first, stHeroRank second)
{
	if (first.management > second.management)
		return true;
	else
		return false;
}
bool server::comparegrade(stHeroRank first, stHeroRank second)
{
	if (first.grade> second.grade)
		return true;
	else
		return false;
}
void server::SortHeroes()
{

	m_herorankstratagem.clear();
	m_herorankpower.clear();
	m_herorankmanagement.clear();
	m_herorankgrade.clear();

	for (uint32_t i = 1; i < maxplayersloaded; ++i)
	{
		if (m_clients[i])
		{
			for (uint32_t j = 0; j < m_clients[i]->m_city.size(); ++j)
			{
				if (m_clients[i]->m_city.at(j))
				{
					for (uint32_t k = 0; k < 10; ++k)
					{
						if (m_clients[i]->m_city[j]->m_heroes[k])
						{
							Hero * hero = m_clients[i]->m_city[j]->m_heroes[k];
							stHeroRank rank;
							rank.grade = hero->m_level;
							rank.stratagem = hero->m_stratagem;
							rank.management = hero->m_management;
							rank.power = hero->m_power;
							rank.name = hero->m_name;
							rank.kind = m_clients[i]->m_playername;
							rank.rank = 0;
							m_herorankstratagem.push_back(rank);
							m_herorankpower.push_back(rank);
							m_herorankmanagement.push_back(rank);
							m_herorankgrade.push_back(rank);
						}
					}
				}
			}
		}
	}

	m_herorankstratagem.sort(comparestratagem);
	m_herorankpower.sort(comparepower);
	m_herorankmanagement.sort(comparemanagement);
	m_herorankgrade.sort(comparegrade);

	list<stHeroRank>::iterator iter;
	int num = 1;
	for ( iter = m_herorankstratagem.begin() ; iter != m_herorankstratagem.end(); ++iter )
	{
		iter->rank = num++;
	}
	num = 1;
	for ( iter = m_herorankpower.begin() ; iter != m_herorankpower.end(); ++iter )
	{
		iter->rank = num++;
	}
	num = 1;
	for ( iter = m_herorankmanagement.begin() ; iter != m_herorankmanagement.end(); ++iter )
	{
		iter->rank = num++;
	}
	num = 1;
	for ( iter = m_herorankgrade.begin() ; iter != m_herorankgrade.end(); ++iter )
	{
		iter->rank = num++;
	}
}
bool server::comparepopulation(stCastleRank first, stCastleRank second)
{
	if (first.population > second.population)
		return true;
	else
		return false;
}
bool server::comparelevel(stCastleRank first, stCastleRank second)
{
	if (first.level > second.level)
		return true;
	else
		return false;
}
void server::SortCastles()
{

	m_castleranklevel.clear();
	m_castlerankpopulation.clear();

	for (int i = 1; i < maxplayersloaded; ++i)
	{
		if (m_clients[i])
		{
			for (int j = 0; j < m_clients[i]->m_city.size(); ++j)
			{
				if (m_clients[i]->m_city.at(j))
				{
					PlayerCity * city = m_clients[i]->m_city[j];
					stCastleRank rank;
					stringstream ss;
					string grade;
					int16_t level = city->GetBuildingLevel(B_TOWNHALL);
					ss << "Level " << level;
					grade = ss.str();
					if (m_clients[i]->HasAlliance())
						rank.alliance = m_clients[i]->GetAlliance()->m_name;
					else
						rank.alliance = "";
					rank.level = level;
					rank.population = city->m_population;
					rank.name = city->m_cityname;
					rank.grade = grade;
					rank.kind = m_clients[i]->m_playername;
					rank.rank = 0;
					m_castleranklevel.push_back(rank);
					m_castlerankpopulation.push_back(rank);
				}
			}
		}
	}

	m_castleranklevel.sort(comparelevel);
	m_castlerankpopulation.sort(comparepopulation);

	list<stCastleRank>::iterator iter;
	int num = 1;
	for ( iter = m_castleranklevel.begin() ; iter != m_castleranklevel.end(); ++iter )
	{
		iter->rank = num++;
	}
	num = 1;
	for ( iter = m_castlerankpopulation.begin() ; iter != m_castlerankpopulation.end(); ++iter )
	{
		iter->rank = num++;
	}
}
int32_t  server::GetClientID(int32_t accountid)
{
	for (int i = 0; i < maxplayersloaded; ++i)
	{
		if (m_clients[i])
			if (m_clients[i]->m_accountid == accountid)
				return i;
	}
	return -1;
}
Client * server::GetClient(int accountid)
{
	for (int i = 0; i < maxplayersloaded; ++i)
	{
		if (m_clients[i])
			if (m_clients[i]->m_accountid == accountid)
				return m_clients[i];
	}
	return 0;
}
Client * server::GetClientByParent(int accountid)
{
	for (int i = 0; i < maxplayersloaded; ++i)
	{
		if (m_clients[i])
			if (m_clients[i]->m_parentid == accountid)
				return m_clients[i];
	}
	return 0;
}
Client * server::GetClientByName(string name)
{
	for (int i = 0; i < maxplayersloaded; ++i)
	{
		if (m_clients[i])
			if (m_clients[i]->m_playername == name)
				return m_clients[i];
	}
	return 0;
}
void server::CloseClient(int id)
{
	if (m_clients[id] && m_clients[id]->m_loggedin)
	{
		amf3object obj;
		obj["cmd"] = "gameClient.kickout";
		obj["data"] = amf3object();
		amf3object & data = obj["data"];
		data["msg"] = "Disconnecting";

		SendObject(m_clients[id]->socket, obj);
		m_clients[id]->socket->stop();
	}
}
City * server::AddPlayerCity(int clientindex, int tileid, uint32_t castleid)
{
	PlayerCity * city = new PlayerCity();
	city->m_type = CASTLE;
	city->m_logurl = "";
	city->m_status = 0;
	city->m_tileid = tileid;
	city->m_castleid = castleid;
	city->m_accountid = m_clients[clientindex]->m_accountid;

	city->SetBuilding(31, 1, -1, 0, 0, 0);
	city->SetResources(0, 0, 0, 0, 0);
	city->m_client = m_clients[clientindex];
	string temp = "";
	stringstream ss;
	ss << "50,10.000000,100.000000,100.000000,100.000000,100.000000,90,0," << (double)unixtime();
	temp = ss.str();

	city->ParseMisc(temp);

	m_clients[clientindex]->m_city.push_back(city);
	m_clients[clientindex]->m_citycount++;

	m_map->m_tile[tileid].m_city = city;
	m_city.push_back(city);


	m_map->m_tile[tileid].m_npc = false;
	m_map->m_tile[tileid].m_ownerid = m_clients[clientindex]->m_accountid;
	m_map->m_tile[tileid].m_city = city;
	m_map->m_tile[tileid].m_type = CASTLE;
	m_map->m_tile[tileid].m_castleid = castleid;

	return city;
}
City * server::AddNpcCity(int tileid)
{
	NpcCity * city = new NpcCity();
	city->m_tileid = tileid;
	city->m_type = NPC;
	city->m_cityname = "Barbarian City";
	city->m_status = 0;
	m_city.push_back(city);
	m_map->m_tile[tileid].m_city = city;
	m_map->m_tile[tileid].m_npc = true;
	m_map->m_tile[tileid].m_type = NPC;
	return city;
}
Client * server::NewClient()
{
	for (int i = 1; i < maxplayersloaded; ++i)
	{
		if (m_clients[i] == 0)
		{
			m_clients[i] = new Client(this);
			m_clients[i]->m_clientnumber = i;
			Log("New client # %d", i);
			return m_clients[i];
		}
	}
	Log("Client list full! %d players on server.", maxplayersloaded);
	return 0;
}
bool server::ParseChat(Client * client, char * str)
{
	if (str && strlen(str) > 0)
	{
		if (!memcmp(str, "\\", 1))
		{
			char * command, * ctx;
			command = strtok_s(str, "\\ ", &ctx);

			amf3object obj;
			obj["cmd"] = "server.SystemInfoMsg";
			obj["data"] = amf3object();
			amf3object & data = obj["data"];
			data["alliance"] = false;
			data["tV"] = false;
			data["noSenderSystemInfo"] = true;

			if (!strcmp(command, "motd"))
			{
				if ((client->m_allianceid <= 0) || (client->m_alliancerank < 3))
				{
					data["msg"] = "You do not have an alliance or are not the proper rank.";
				}
				else
				{
					m_alliances->AllianceById(client->m_allianceid)->m_motd = (str+strlen(command)+2);
					string s;
					s = "Alliance MOTD set to '";
					s += m_alliances->AllianceById(client->m_allianceid)->m_motd;
					s += "'";
					data["msg"] = s;
				}
			}
			else if (!strcmp(command, "save"))
			{
#ifndef WIN32
				pthread_t hSaveThread;
				if (pthread_create(&hSaveThread, NULL, SaveData, 0))
				{
					SFERROR("pthread_create");
				}
#else
				uint32_t uAddr;
				HANDLE hSaveThread;
				hSaveThread = (HANDLE)_beginthreadex(0, 0, SaveData, 0, 0, &uAddr);
#endif
				data["msg"] = "Data being saved.";
			}
			else if (!strcmp(command, "cents"))
			{
				data["msg"] = "500 Cents granted.";
				client->m_cents += 500;
				client->PlayerUpdate();
			}
			else if (!strcmp(command, "resources"))
			{
				stResources res = {100000,100000,100000,100000,100000};
				client->GetFocusCity()->m_resources += res;
				data["msg"] = "Resources granted.";
				client->GetFocusCity()->ResourceUpdate();
			}
			else if (!strcmp(command, "tempvar"))
			{
				command = strtok_s(0, " ", &ctx);
				client->m_tempvar = atoi(command);
				string s;
				s = "Tempvar set to '<u>";
				s += command;
				s += "</u>'.";
				data["msg"] = s;
			}
			else if (!strcmp(command, "buff"))
			{
				command = strtok_s(0, " |", &ctx);
				if (!command)
				{
					data["msg"] = "Invalid syntax";
					if (client->m_connected)
						SendObject(client->socket, obj);
					return false;
				}

				string buffid = command;
				command = strtok_s(0, "|", &ctx);
				if (!command)
				{
					data["msg"] = "Invalid syntax";
					if (client->m_connected)
						SendObject(client->socket, obj);
					return false;
				}
				string desc = command;
				command = strtok_s(0, "| ", &ctx);
				if (!command)
				{
					data["msg"] = "Invalid syntax";
					if (client->m_connected)
						SendObject(client->socket, obj);
					return false;
				}
				int64_t var = atoi(command) + unixtime();
				string s;
				s = "Buff set to '<u>";
				s += buffid;
				s += "</u>'.";
				data["msg"] = s;

				client->SetBuff(buffid, desc, var);
			}
			else if (!strcmp(command, "commands"))
			{
				data["msg"] = "Commands: motd cents resources";
			}
			else
			{
				string s;
				s = "Command '<u>";
				s += command;
				s += "</u>' does not exist.";
				data["msg"] = s;
			}
			if (client->m_connected)
				SendObject(client->socket, obj);
			return false;
		}
	}
	return true;
}
int16_t server::GetRelation(int32_t client1, int32_t client2)
{
	if (client1 >= 0 && client2 >= 0)
	{
		return m_alliances->GetRelation(client1, client2);
	}
	return 0;
}
void * server::DoRankSearch(string key, int8_t type, void * subtype, int16_t page, int16_t pagesize)
{
	if (type == 1)//client lists
	{
		list<stSearchClientRank>::iterator iter;
		for ( iter = m_searchclientranklist.begin(); iter != m_searchclientranklist.end(); )
		{
			if (iter->rlist == subtype && iter->key == key)
				return &iter->ranklist;
			++iter;
		}

		stSearchClientRank searchrank;
		searchrank.key = key;
		searchrank.lastaccess = unixtime();
		searchrank.rlist = (list<stClientRank>*)subtype;

		list<stClientRank>::iterator iterclient;

		for ( iterclient = ((list<stClientRank>*)subtype)->begin(); iterclient != ((list<stClientRank>*)subtype)->end(); )
		{
			if (ci_find(iterclient->client->m_playername, key) != string::npos)
			{
				stClientRank clientrank;
				clientrank.client = iterclient->client;
				clientrank.rank = iterclient->rank;
				searchrank.ranklist.push_back(clientrank);
			}
			++iterclient;
		}
		m_searchclientranklist.push_back(searchrank);
		return &m_searchclientranklist.back();
	}
	else if (type == 2)//hero lists
	{
		list<stSearchHeroRank>::iterator iter;
		for ( iter = m_searchheroranklist.begin(); iter != m_searchheroranklist.end(); )
		{
			if (iter->rlist == subtype && iter->key == key)
				return &iter->ranklist;
			++iter;
		}

		stSearchHeroRank searchrank;
		searchrank.key = key;
		searchrank.lastaccess = unixtime();
		searchrank.rlist = (list<stHeroRank>*)subtype;

		list<stHeroRank>::iterator iterhero;

		for ( iterhero = ((list<stHeroRank>*)subtype)->begin(); iterhero != ((list<stHeroRank>*)subtype)->end(); )
		{
			if (ci_find(iterhero->name, key) != string::npos)
			{
				stHeroRank herorank;
				herorank.grade = iterhero->grade;
				herorank.kind = iterhero->kind;
				herorank.management = iterhero->management;
				herorank.power = iterhero->power;
				herorank.name = iterhero->name;
				herorank.stratagem = iterhero->stratagem;
				herorank.rank = iterhero->rank;
				searchrank.ranklist.push_back(herorank);
			}
			++iterhero;
		}
		m_searchheroranklist.push_back(searchrank);
		return &m_searchheroranklist.back();
	}
	else if (type == 3)//castle lists
	{
		list<stSearchCastleRank>::iterator iter;
		for ( iter = m_searchcastleranklist.begin(); iter != m_searchcastleranklist.end(); )
		{
			if (iter->rlist == subtype && iter->key == key)
				return &iter->ranklist;
			++iter;
		}

		stSearchCastleRank searchrank;
		searchrank.key = key;
		searchrank.lastaccess = unixtime();
		searchrank.rlist = (list<stCastleRank>*)subtype;

		list<stCastleRank>::iterator itercastle;

		for ( itercastle = ((list<stCastleRank>*)subtype)->begin(); itercastle != ((list<stCastleRank>*)subtype)->end(); )
		{
			if (ci_find(itercastle->name, key) != string::npos)
			{
				stCastleRank castlerank;
				castlerank.grade = itercastle->grade;
				castlerank.alliance = itercastle->alliance;
				castlerank.kind = itercastle->kind;
				castlerank.level = itercastle->level;
				castlerank.name = itercastle->name;
				castlerank.population = itercastle->population;
				castlerank.rank = itercastle->rank;
				searchrank.ranklist.push_back(castlerank);
			}
			++itercastle;
		}
		m_searchcastleranklist.push_back(searchrank);
		return &m_searchcastleranklist.back();
	}
	else if (type == 4)//alliance lists
	{
		list<stSearchAllianceRank>::iterator iter;
		for ( iter = m_searchallianceranklist.begin(); iter != m_searchallianceranklist.end(); )
		{
			if (iter->rlist == subtype && iter->key == key)
				return &iter->ranklist;
			++iter;
		}

		stSearchAllianceRank searchrank;
		searchrank.key = key;
		searchrank.lastaccess = unixtime();
		searchrank.rlist = (list<stAlliance>*)subtype;

		list<stAlliance>::iterator iteralliance;

		for ( iteralliance = ((list<stAlliance>*)subtype)->begin(); iteralliance != ((list<stAlliance>*)subtype)->end(); )
		{
			if (ci_find(iteralliance->ref->m_name, key) != string::npos)
			{
				stAlliance alliancerank;
				alliancerank.ref = iteralliance->ref;
				alliancerank.rank = iteralliance->rank;
				searchrank.ranklist.push_back(alliancerank);
			}
			++iteralliance;
		}
		m_searchallianceranklist.push_back(searchrank);
		return &m_searchallianceranklist.back();
	}
}
void server::CheckRankSearchTimeouts(uint64_t time)
{
	list<stSearchClientRank>::iterator iterclient;
	list<stSearchHeroRank>::iterator iterhero;
	list<stSearchCastleRank>::iterator itercastle;

	for ( iterclient = m_searchclientranklist.begin(); iterclient != m_searchclientranklist.end(); )
	{
		if (iterclient->lastaccess + 30000 < time)
		{
			m_searchclientranklist.erase(iterclient++);
			continue;
		}
		++iterclient;
	}

	for ( iterhero = m_searchheroranklist.begin(); iterhero != m_searchheroranklist.end(); )
	{
		if (iterhero->lastaccess + 30000 < time)
		{
			m_searchheroranklist.erase(iterhero++);
			continue;
		}
		++iterhero;
	}

	for ( itercastle = m_searchcastleranklist.begin(); itercastle != m_searchcastleranklist.end(); )
	{
		if (itercastle->lastaccess + 30000 < time)
		{
			m_searchcastleranklist.erase(itercastle++);
			continue;
		}
		++itercastle;
	}
}
void server::AddMarketEntry(stMarketEntry me, int8_t type)
{
	if (type == 1)
	{
		m_marketbuy.push_back(me);
		m_marketbuy.sort(comparebuy);
	}
	else if (type == 2)
	{
		m_marketsell.push_back(me);
		m_marketsell.sort(comparesell);
	}
}
} // namespace server
} // namespace http
