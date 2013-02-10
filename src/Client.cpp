//
// Client.cpp
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


#include "Client.h"
#include "Tile.h"
#include "server.hpp"
#include "City.h"

namespace spitfire {
namespace server {


extern int DEF_MAPSIZE;
extern uint64_t unixtime();

#ifndef __WIN32__
#define _ASSERT(x)
#endif


Client::Client(server * core)
{
	m_accountexists = false;
	m_socknum = 0;
	m_accountid = 0;
	m_main = core;
	m_playerid = 0;
	PACKETSIZE = 1024 * 150;
	socket = 0;
	m_loggedin = false;
	m_clientnumber = -1;
	m_playername = "Testname";
	m_flag = "Evony";
	m_alliancename = "";
	m_email = "";
	m_password = "";
	m_prestige = 0;
	m_honor = 0;
	m_status = 0;
	m_rank = 0;
	m_prestigerank = 0;
	m_title = 0;
	m_connected = 0;
	//char * readytoprocess = new char[PACKETSIZE];
	processoffset = 0;
	m_citycount = 0;
	m_lastlogin = 0;
	m_creation = 0;
	m_sex = 0;
	m_allianceid = -1;
	m_currentcityindex = -1;
	m_currentcityid = -1;
	m_population = 0;
	m_cents = 0;
	m_office = "Civilian";
	m_bdenyotherplayer = false;
	m_parentid = 0;
	m_icon = 0;
	m_changedface = false;

	memset(m_ipaddress, 0, 16);

	m_currentcityid = m_currentcityindex = -1;
	//m_city;// = new ArrayList();

	//memset(&m_buffs, 0, sizeof(m_buffs));
	memset(&m_research, 0, sizeof(m_research));
	//memset(&m_items, 0, sizeof(m_items));

	for (int i = 0; i < DEF_MAXITEMS; ++i)
	{
		m_items[i].count = 0;
		m_items[i].id = "";
		m_items[i].maxcount = 0;
		m_items[i].mincount = 0;
	}
	// 	for (int i = 0; i < 30; ++i)
	// 	{
	// 		m_buffs[i].endtime = 0.0;
	// 		m_buffs[i].id = "";
	// 	}
}

Client::~Client(void)
{
	//	for (int i = 0; i < m_city.size(); ++i)
	//		delete (City*)m_city.at(i);
}

Alliance * Client::GetAlliance()
{
	return m_main->m_alliances->AllianceById(m_allianceid);
};


amf3object  Client::ToObject()
{
	amf3object obj = amf3object();
	obj["newReportCount_trade"] = 0;
	obj["newMaileCount_system"] = 0;
	obj["newReportCount"] = 0;
	obj["isSetSecurityCode"] = false;
	obj["mapSizeX"] = DEF_MAPSIZE;
	obj["mapSizeY"] = DEF_MAPSIZE;
	obj["newReportCount_other"] = 0;
	obj["buffs"] = BuffsArray();
	obj["gamblingItemIndex"] = 12;
	obj["changedFace"] = false;
	obj["castles"] = CastleArray();
	obj["playerInfo"] = PlayerInfo();
	obj["redCount"] = 0;
	obj["newMaileCount_inbox"] = 0;

	string s;
	{
		time_t ttime;
		time(&ttime);
		struct tm * timeinfo;
		timeinfo = localtime(&ttime);

		stringstream ss;
		ss << (timeinfo->tm_year + 1900) << ".";
		if (timeinfo->tm_mon < 9)
			ss << "0" << (timeinfo->tm_mon+1);
		else
			ss << (timeinfo->tm_mon+1);
		ss << ".";
		if (timeinfo->tm_mday < 10)
			ss << "0" << timeinfo->tm_mday;
		else
			ss << timeinfo->tm_mday;
		ss << " ";
		if (timeinfo->tm_hour < 10)
			ss << "0" << timeinfo->tm_hour;
		else
			ss << timeinfo->tm_hour;
		ss << ".";
		if (timeinfo->tm_min < 10)
			ss << "0" << timeinfo->tm_min;
		else
			ss << timeinfo->tm_min;
		ss << ".";
		if (timeinfo->tm_sec < 10)
			ss << "0" << timeinfo->tm_sec;
		else
			ss << timeinfo->tm_sec;

		s = ss.str();
	}

	//	obj["currentDateTime"] = "2011.07.27 03.20.32";
	obj["currentDateTime"] = s.c_str();
	obj["newReportCount_army"] = 0;
	obj["friendArmys"] = amf3array();
	obj["saleTypeBeans"] = SaleTypeItems();
	obj["autoFurlough"] = false;
	obj["castleSignBean"] = amf3array();
	obj["furloughDay"] = 0;
	obj["tutorialStepId"] = 0;//10101; -- can set any tutorial
	obj["newReportCount_army"] = 0;
	obj["newMailCount"] = 0;
	obj["furlough"] = false;
	obj["gameSpeed"] = 5;
	obj["enemyArmys"] = amf3array();
	obj["currentTime"] = (double)unixtime();
	obj["items"] = Items();
	obj["freshMan"] = false;
	obj["finishedQuestCount"] = 0;
	obj["selfArmys"] = amf3array();
	obj["saleItemBeans"] = SaleItems();

	return obj;
}

amf3array Client::Items()
{
	//	amf3object retobj;
	//amf3array * itemarray = new amf3array();
	amf3array itemarray = amf3array();
	//	retobj.type = Array;
	//	retobj._value._array = itemarray;

	amf3object obj = amf3object();
	//age2?
	//obj["appearance"] = 96;
	//obj["help"] = 1;

	for (int i = 0; i < DEF_MAXITEMS; ++i)
	{
		if (m_items[i].count > 0)
		{
			obj = amf3object();
			obj["id"] = m_items[i].id;
			obj["count"] = m_items[i].count;
			obj["minCount"] = m_items[i].mincount;
			obj["maxCount"] = m_items[i].maxcount;
			itemarray.Add(obj);
		}
	}
	return itemarray;
}
amf3array Client::SaleTypeItems()
{
	amf3array array = amf3array();
	amf3object obj1 = amf3object();
	amf3object obj2 = amf3object();
	amf3object obj3 = amf3object();
	amf3object obj4 = amf3object();
	obj1["typeName"] = "\xE5\x8A\xA0\xE9\x80\x9F";//加速
	obj2["typeName"] = "\xE5\xAE\x9D\xE7\xAE\xB1";//宝箱
	obj3["typeName"] = "\xE7\x94\x9F\xE4\xBA\xA7";//生产
	obj4["typeName"] = "\xE5\xAE\x9D\xE7\x89\xA9";//宝物
	stringstream ss1, ss2, ss3, ss4;

	bool count1, count2, count3, count4;
	count1 = count2 = count3 = count4 = false;

	for (int i = 0; i < DEF_MAXITEMS; ++i)
	{
		if (m_main->m_items[i].buyable)
		{
			if (m_main->m_items[i].type == 1)//speed up tab
			{
				if (count1)
					ss1 << "#";
				ss1 << m_main->m_items[i].name;
				count1 = true;
			}
			else if (m_main->m_items[i].type == 2)//chest tab
			{
				if (count2)
					ss2 << "#";
				ss2 << m_main->m_items[i].name;
				count2 = true;
			}
			else if (m_main->m_items[i].type == 3)//produce tab
			{
				if (count3)
					ss3 << "#";
				ss3 << m_main->m_items[i].name;
				count3 = true;
			}
			else if (m_main->m_items[i].type == 4)//items tab
			{
				if (count4)
					ss4 << "#";
				ss4 << m_main->m_items[i].name;
				count4 = true;
			}
		}
	}
	obj1["items"] = ss1.str();
	obj2["items"] = ss2.str();
	obj3["items"] = ss3.str();
	obj4["items"] = ss4.str();
	array.Add(obj1);
	array.Add(obj2);
	array.Add(obj3);
	array.Add(obj4);

	//	obj["items"] = "consume.2.a#consume.2.b#consume.2.b.1#consume.2.c#consume.2.c.1#consume.2.d#player.fort.1.c#player.troop.1.b#consume.transaction.1";

	// 	obj = amf3object();
	// 	obj["typeName"] = "\xE5\xAE\x9D\xE7\xAE\xB1";//宝箱
	// 	obj["items"] = "player.box.special.1#player.box.special.2#player.box.special.3#player.box.currently.1#player.box.gambling.1#player.box.gambling.2#player.box.gambling.3#player.box.gambling.4#player.box.gambling.5#player.box.gambling.6#player.box.gambling.7#player.box.gambling.8#player.box.gambling.9#player.box.gambling.10#player.box.gambling.11#player.box.gambling.12";
	// 	array.Add(obj);
	// 
	// 	obj = amf3object();
	// 	obj["typeName"] = "\xE7\x94\x9F\xE4\xBA\xA7";//生产
	// 	obj["items"] = "player.resinc.1#player.resinc.1.b#player.resinc.2#player.resinc.2.b#player.resinc.3#player.resinc.3.b#player.resinc.4#player.resinc.4.b#player.gold.1.a#player.gold.1.b";
	// 	array.Add(obj);
	// 
	// 	obj = amf3object();
	// 	obj["typeName"] = "\xE5\xAE\x9D\xE7\x89\xA9";//宝物
	// 	obj["items"] = "player.speak.bronze_publicity_ambassador.permanent#player.speak.bronze_publicity_ambassador.permanent.15#player.pop.1.a#hero.management.1#player.experience.1.b#player.experience.1.a#player.queue.building#player.experience.1.c#player.peace.1#player.heart.1.a#hero.intelligence.1#consume.blueprint.1#consume.refreshtavern.1#consume.move.1#player.more.castle.1.a#player.name.1.a#consume.changeflag.1#player.troop.1.a#player.attackinc.1#player.attackinc.1.b#consume.hegemony.1#player.defendinc.1#player.defendinc.1.b#player.relive.1#hero.reset.1#hero.reset.1.a#player.destroy.1.a#hero.power.1#consume.1.a#consume.1.b#consume.1.c";
	// 	array.Add(obj);
	return array;
}
amf3array Client::SaleItems()
{
	amf3array array = amf3array();
	amf3object obj = amf3object();
	obj["saleType"] = 1;
	obj["items"] = "player.resinc.1";
	array.Add(obj);

	obj = amf3object();
	obj["saleType"] = 1;
	obj["items"] = "player.resinc.2";
	array.Add(obj);

	obj = amf3object();
	obj["saleType"] = 1;
	obj["items"] = "player.resinc.3";
	array.Add(obj);

	obj = amf3object();
	obj["saleType"] = 1;
	obj["items"] = "player.resinc.4";
	array.Add(obj);

	obj = amf3object();
	obj["saleType"] = 0;
	obj["items"] = "consume.1.c";
	array.Add(obj);

	obj = amf3object();
	obj["saleType"] = 1;
	obj["items"] = "consume.2.b.1";
	array.Add(obj);

	obj = amf3object();
	obj["saleType"] = 1;
	obj["items"] = "player.gold.1.a";
	array.Add(obj);

	obj = amf3object();
	obj["saleType"] = 1;
	obj["items"] = "consume.refreshtavern.1";
	array.Add(obj);

	obj = amf3object();
	obj["saleType"] = 1;
	obj["items"] = "consume.2.b";
	array.Add(obj);

	obj = amf3object();
	obj["saleType"] = 1;
	obj["items"] = "consume.transaction.1";
	array.Add(obj);
	return array;
}
amf3array Client::CastleArray()
{
	// 	Amf3Array array = new Amf3Array();
	// 	for (int i = 0; i < m_citycount; ++i)
	// 		array.DenseArray.Add(((City)m_city[i]).ToObject());
	// 	return array;
	amf3array array = amf3array();
	for (int32_t i = 0; i < m_citycount; ++i)
	{
		amf3object temp = ((PlayerCity*)m_city.at(i))->ToObject();
		array.Add(temp);
	}
	return array;
}
amf3array Client::BuffsArray()
{
	amf3array array = amf3array();
	for (int32_t i = 0; i < m_buffs.size(); ++i)
	{
		amf3object temp = amf3object();
		temp["desc"] = m_buffs[i].desc;
		temp["endTime"] = m_buffs[i].endtime;
		temp["desc"] = m_buffs[i].id;
		array.Add(temp);
	}
	return array;
}
amf3object Client::PlayerInfo()
{
	amf3object obj = amf3object();
	if (m_allianceid > 0)
	{
		obj["alliance"] = m_main->m_alliances->AllianceById(m_allianceid)->m_name;
		obj["allianceLevel"] = m_main->m_alliances->GetAllianceRank(m_alliancerank);
		obj["levelId"] = m_alliancerank;
	}
	obj["createrTime"] = m_creation;//-3*7*24*60*60*1000;
	obj["office"] = m_office;
	obj["sex"] = m_sex;
	obj["honor"] = m_honor;
	obj["bdenyotherplayer"] = m_bdenyotherplayer;
	obj["id"] = m_accountid;
	obj["accountName"] = m_email;
	obj["prestige"] = m_prestige;
	obj["faceUrl"] = m_faceurl;
	obj["flag"] = m_flag;
	obj["userId"] = m_parentid;
	obj["userName"] = m_playername;
	obj["castleCount"] = m_citycount;
	obj["medal"] = m_cents;
	obj["ranking"] = m_prestigerank;
	obj["titleId"] = m_title;
	obj["lastLoginTime"] = m_lastlogin;
	m_population = 0;

	for (int i = 0; i < m_citycount; ++i)
		m_population += ((PlayerCity*)m_city.at(i))->m_population;

	obj["population"] = m_population;
	return obj;
}

void Client::ParseBuffs(char * str)
{
	if (str && strlen(str) > 0)
	{
		string id;
		int64_t endtime;
		string descname;


		char * ch = 0, * cr = 0;
		char * tok;
		tok = strtok_s(str, "|", &ch);
		do
		{
			tok = strtok_s(tok, ",", &cr);
			if (tok != 0)
				id = tok;

			tok = strtok_s(0, ",", &cr);
			if (tok != 0)
				descname = _atoi64(tok);

			tok = strtok_s(0, ",", &cr);
			if (tok != 0)
				endtime = _atoi64(tok);

			// 			tok = strtok_s(0, ",", &cr);
			// 			_ASSERT(tok != 0);
			// 			descname = tok;

			SetBuff(id, descname, endtime);
			tok = strtok_s(0, "|", &ch);
		} while (tok != 0);
	}
}

void Client::SetBuff(string type, string desc, int64_t endtime, int8_t param)
{
	for (int i = 0; i < m_buffs.size(); ++i)
	{
		if (m_buffs[i].id == type)
		{
			m_buffs[i].endtime = endtime;
			BuffUpdate(type, desc, endtime);
			return;
		}
	}
	stBuff buff;
	buff.desc = desc;
	buff.endtime = endtime;
	buff.id = type;

	m_buffs.push_back(buff);

	BuffUpdate(type, desc, endtime, param);
	return;
}

void Client::RemoveBuff(string type)
{
	vector<stBuff>::iterator iter;
	if (m_buffs.size() > 0)
		for (iter = m_buffs.begin(); iter != m_buffs.end(); )
		{
			if (iter->id == type)
			{
				BuffUpdate(type, iter->desc, iter->endtime, 1);//1 = remove
				m_buffs.erase(iter++);
				return;
			}
			++iter;
		}
		return;
}

void Client::ParseItems(char * str)
{
	if (str && strlen(str) > 0)
	{
		string id;
		int64_t amount;
		string descname;


		char * ch = 0, * cr = 0;
		char * tok;
		tok = strtok_s(str, "|", &ch);
		do
		{
			tok = strtok_s(tok, ",", &cr);
			if (tok != 0)
				id = tok;

			tok = strtok_s(0, ",", &cr);
			if (tok != 0)
				amount = _atoi64(tok);

			// 			tok = strtok_s(0, ",", &cr);
			// 			_ASSERT(tok != 0);
			// 			descname = tok;

			SetItem(id, amount);
			tok = strtok_s(0, "|", &ch);
		} while (tok != 0);
	}
}

void Client::ParseResearch(char * str)
{
	if (str && strlen(str) > 0)
	{
		uint16_t id = 0;
		uint16_t level;
		uint32_t castleid;
		double endtime;
		double starttime;


		char * ch = 0, * cr = 0;
		char * tok;
		tok = strtok_s(str, "|", &ch);
		do
		{
			id = level = castleid = endtime = starttime = 0;

			tok = strtok_s(tok, ",", &cr);
			if (tok != 0)
				id = atoi(tok);

			tok = strtok_s(0, ",", &cr);
			if (tok != 0)
				level = atoi(tok);

			tok = strtok_s(0, ",", &cr);
			if (tok != 0)
				castleid = atoi(tok);

			tok = strtok_s(0, ",", &cr);
			if (tok != 0)
				starttime = atof(tok);

			tok = strtok_s(0, ",", &cr);
			if (tok != 0)
				endtime = atof(tok);

			SetResearch(id, level, castleid, starttime, endtime);
			tok = strtok_s(0, "|", &ch);
		} while (tok != 0);
	}
}

void Client::ParseMisc(char * str)
{
	if (str && strlen(str) > 0)
	{
		char * ch = 0, * cr = 0;
		char * tok;
		tok = strtok_s(str, "|", &ch);
		do
		{
			tok = strtok_s(tok, ",", &ch);
			if (tok != 0)
				m_icon = atoi(tok);

			tok = strtok_s(tok, ",", &ch);
			if (tok != 0)
			{
				m_allianceapply = tok;
				Alliance * alliance = m_main->m_alliances->AllianceByName(m_allianceapply);
				if (alliance != (Alliance*)-1)
				{
					alliance->RequestJoin(this, unixtime());
				}
			}

			tok = strtok_s(tok, ",", &ch);
			if (tok != 0)
				m_changedface = (bool)tok;

			tok = strtok_s(0, "|", &ch);
		} while (tok != 0);
	}
}

PlayerCity * Client::GetCity(int32_t castleid)
{
	for (int32_t i = 0; i < m_citycount; ++i)
	{
		if (m_city[i]->m_castleid == castleid)
			return m_city[i];
	}
	return 0;
};

PlayerCity * Client::GetFocusCity()
{
	return (PlayerCity*)m_city[m_currentcityindex];
};

void Client::SetResearch(uint16_t id, int16_t level, int16_t castleid, double starttime, double endtime)
{
	if (id < 0 || id > 24)
		_ASSERT(0);
	m_research[id].level = level;
	m_research[id].endtime = endtime;
	m_research[id].starttime = starttime;
	m_research[id].castleid = castleid;
}

void Client::SetItem(string type, int16_t dir)
{
	stItem * sitem = 0;
	for (int i = 1; i < 100; ++i)
	{
		//Log("item %s - %s - %d", m_items[i].id.c_str(), type.c_str(), (bool)(m_items[i].id == type));
		if (m_items[i].id == type)
		{
			m_items[i].count += dir;
			sitem = &m_items[i];
			break;
		}
	}
	if (sitem == 0)
		for (int i = 1; i < 100; ++i)
		{
			if (m_items[i].id.length() == 0)
			{
				m_items[i].id = type;
				m_items[i].count += dir;
				sitem = &m_items[i];
				break;
			}
		}

		if (sitem == 0)
		{
			Log("Error in SetItem item:%s num:%d", type.c_str(), dir);
			return;
		}

		amf3object obj = amf3object();
		obj["cmd"] = "server.ItemUpdate";
		obj["data"] = amf3object();
		amf3object & data = obj["data"];
		amf3array array = amf3array();

		amf3object item = amf3object();
		item["id"] = (char*)type.c_str();
		item["count"] = sitem->count;
		item["minCount"] = 0;
		item["maxCount"] = 0;
		array.Add(item);

		data["items"] = array;

		m_main->SendObject(socket, obj);
}

int16_t Client::GetItemCount(string type)
{
	for (int i = 0; i < DEF_MAXITEMS; ++i)
	{
		if (m_items[i].id == type)
		{
			return m_items[i].count;
		}
	}
	return 0;
}

int16_t Client::GetItemCount(int16_t type)
{
	if (type < 0 || type > DEF_MAXITEMS)
		return 0;
	return m_items[type].count;
}

int16_t Client::GetResearchLevel(int16_t id)
{
	return m_research[id].level;
}

void Client::CalculateResources()
{
	m_population = 0;
	for (int i = 0; i < m_citycount; ++i)
		m_population += ((PlayerCity*)m_city.at(i))->m_population;

	for (int i = 0; i < m_city.size(); ++i)
	{
		if (m_city[i])
		{
			m_city[i]->CalculateResourceStats();
			m_city[i]->CalculateStats();
			m_city[i]->CalculateResources();
		}
	}
}

void Client::PlayerUpdate()
{
	amf3object obj = amf3object();
	obj["cmd"] = "server.PlayerInfoUpdate";
	obj["data"] = amf3object();
	amf3object & data = obj["data"];
	data["playerInfo"] = PlayerInfo();

	m_main->SendObject(socket, obj);
}

void Client::ItemUpdate(char * itemname)
{
	amf3object obj = amf3object();
	obj["cmd"] = "server.ItemUpdate";
	obj["data"] = amf3object();
	amf3object & data = obj["data"];

	amf3array items = amf3array();

	for (int i = 0; i < DEF_MAXITEMS; ++i)
	{
		if (!m_items[i].id.compare(itemname))
		{
			amf3object ta = amf3object();
			ta["id"] = itemname;
			ta["count"] = m_items[i].count;
			ta["minCount"] = m_items[i].mincount;
			ta["maxCount"] = m_items[i].maxcount;
			items.Add(ta);
			break;
		}
	}

	data["items"] = items;

	m_main->SendObject(socket, obj);
}

void Client::HeroUpdate(int heroid, int castleid)
{
	amf3object obj = amf3object();
	obj["cmd"] = "server.HeroUpdate";
	obj["data"] = amf3object();
	amf3object & data = obj["data"];

	data["castleId"] = castleid;
	for (int i = 0; i < m_citycount; ++i)
	{
		for (int k = 0; k < 10; ++k)
		{
			if (m_city[i]->m_heroes[k]->m_id == heroid)
			{
				data["hero"] = m_city[i]->m_heroes[k]->ToObject();
				m_main->SendObject(socket, obj);
				return;
			}
		}
	}
	// TODO error case maybe? - void Client::HeroUpdate(int heroid, int castleid)
	return;
}

void Client::MailUpdate()
{
	amf3object obj = amf3object();
	obj["cmd"] = "server.NewMail";
	obj["data"] = amf3object();
	amf3object & data = obj["data"];

	data["count_system"] = 1;
	data["count"] = 2;
	data["count_inbox"] = 1;

	// TODO count mail properly - void Client::MailUpdate()

	m_main->SendObject(socket, obj);

	return;
}


void Client::BuffUpdate(string name, string desc, int64_t endtime, int8_t type)
{
	amf3object obj = amf3object();
	obj["cmd"] = "server.PlayerBuffUpdate";
	obj["data"] = amf3object();
	amf3object & data = obj["data"];

	amf3object buffbean = amf3object();

	buffbean["descName"] = desc;
	buffbean["endTime"] = endtime;
	buffbean["typeId"] = name;

	data["buffBean"] = buffbean;

	data["updateType"] = (int32_t)type;

	m_main->SendObject(socket, obj);
	return;
}

}
}