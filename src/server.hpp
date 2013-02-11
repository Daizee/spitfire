//
// server.hpp
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

#ifndef SPITFIRE_SERVER_HPP
#define SPITFIRE_SERVER_HPP

#include <asio.hpp>
#include <iostream>
#include <list>
#include <string>
#include <cctype>
#include <boost/noncopyable.hpp>
#include "connection.hpp"
#include "connection_manager.hpp"
#include "request_handler.hpp"

#include "Map.h"
#include "amf3.h"
#include "Alliance.h"
#include "City.h"
#include "Client.h"
#include "SQLDB.h"

#include <lua/lua.hpp>
#include <lua/lauxlib.h>

namespace spitfire {
namespace server {

/// The top-level class of the Spitfire server.
class server
	: private boost::noncopyable
{
public:
	/// Construct the server to listen on the specified TCP address and port, and
	/// serve up files from the given directory.
	explicit server();
	~server() { delete m_clients; delete m_map; delete accounts; delete msql; delete msql2; }

	string sqlhost, sqluser, sqlpass, bindaddress, bindport;

	/// Run the server's io_service loop.
	void run();

	/// Stop the server.
	void stop();

private:
	/// Handle completion of an asynchronous accept operation.
	void handle_accept(const asio::error_code& e);

	/// Handle a request to stop the server.
	void handle_stop();

	/// The io_service used to perform asynchronous operations.
	asio::io_service io_service_;

	/// The signal_set is used to register for process termination notifications.
	asio::signal_set signals_;

	/// Acceptor used to listen for incoming connections.
	asio::ip::tcp::acceptor acceptor_;

	/// The connection manager which owns all live connections.
	connection_manager connection_manager_;

	/// The next connection to be accepted.
	connection_ptr new_connection_;

	/// The handler for all incoming requests.
	request_handler request_handler_;


	// Server itself
public:

	CSQLDB * accounts;
	CSQLDB * msql;
	CSQLDB * msql2;

	//Lua
	lua_State *L;

	bool LoadConfig();

	char serverstatus;

	uint32_t maxplayersloaded, maxplayersonline, currentplayersonline;

	string * m_servername;
	Map * m_map;
	Client * NewClient();
	Client * GetClient(int accountid);
	Client * GetClientByParent(int accountid);
	Client * GetClientByName(string name);
	int32_t  GetClientID(int32_t accountid);
	void CloseClient(int id);
	City * AddPlayerCity(int clientindex, int tileid, uint32_t castleid);
	City * AddNpcCity(int tileid);
	void MassMessage(string str, bool nosender = false, bool tv = false, bool all = false);

	Client ** m_clients; // max connectable clients

	void SendObject(uint s, amf3object & object)
	{
		if (serverstatus == 0)
			return;
		if ((s == 0) || (m_clients[s]))
			return;
		char buffer[15000];
		int length = 0;
		amf3writer * writer;

		writer = new amf3writer(buffer+4);

		writer->Write(object);

		(*(int*)buffer) = length = writer->position;
		ByteSwap(*(int*)buffer);

		m_clients[s]->socket->write(buffer, length+4);
		delete writer;
	}

	void SendObject(connection * s, amf3object & object)
	{
		if (s == 0)
			return;
		SendObject(*s, object);
	}
	void SendObject(connection & s, amf3object & object)
	{
		try
		{
			if (serverstatus == 0)
				return;
			char buffer[15000];
			int length = 0;
			amf3writer * writer;

			writer = new amf3writer(buffer+4);

			writer->Write(object);

			(*(int*)buffer) = length = writer->position;
			ByteSwap(*(int*)buffer);

			s.write(buffer, length+4);
			delete writer;
		}
		catch (std::exception& e)
		{
			std::cerr << "exception: " << __FILE__ << " @ " << __LINE__ << "\n";
			std::cerr << e.what() << "\n";
		}

	}

	vector<City*> m_city;// npc cities
	AllianceCore * m_alliances;

	bool ParseChat(Client * client, char * str);

	int16_t GetRelation(int32_t client1, int32_t client2);

	uint64_t ltime;

#pragma region structs and stuff

	struct stPrereq
	{
		int32_t id;
		int32_t level;
	};

	struct stItem
	{
		string name;
		int32_t cost;
		int32_t saleprice;
		int32_t daylimit;
		int32_t type;
		bool cangamble;
		bool buyable;
		int32_t rarity;//no not the pony
	};

	string m_itemxml;

	struct stRarityGamble
	{
		vector<stItem*> common;
		vector<stItem*> special;
		vector<stItem*> rare;
		vector<stItem*> superrare;
		vector<stItem*> ultrarare;
	} m_gambleitems;
	stItem m_items[DEF_MAXITEMS];
	int m_itemcount;

	struct stBuildingConfig
	{
		int32_t food;
		int32_t wood;
		int32_t iron;
		int32_t stone;
		int32_t gold;
		int32_t population;
		double time;
		double destructtime;
		stPrereq buildings[3];
		stPrereq techs[3];
		stPrereq items[3];
		int32_t limit;
		int32_t inside;
		int32_t prestige;
	};

	struct stMarketEntry
	{
		double amount;
		double price;
		uint32_t clientid;
		uint32_t cityid;
	};

	struct stArmyMovement
	{
		PlayerCity::stTroops troops;
		Hero * hero;
		Client * client;
	};
	struct stTrainingAction
	{
		int32_t troopcount;
		int8_t trooptype;
		int8_t buildingid;
		Client * client;
	};
	struct stBuildingAction
	{
		PlayerCity * city;
		Client * client;
		int16_t positionid;
	};
	struct stResearchAction
	{
		PlayerCity * city;
		Client * client;
		int16_t researchid;
	};

	struct stTimedEvent
	{
		int8_t type;
		void * data;
	};

	struct stIntRank
	{
		uint32_t value;
		uint32_t id;
	};

	struct stClientRank
	{
		Client * client;
		int32_t rank;
	};

	struct stHeroRank
	{
		int16_t stratagem;
		int16_t power;
		int16_t management;
		int16_t grade;
		string name;
		string kind;
		int32_t rank;
	};

	struct stCastleRank
	{
		int16_t level;
		int32_t population;
		string name;
		string grade;
		string alliance;
		string kind;
		int32_t rank;
	};

	struct stSearchClientRank
	{
		list<stClientRank> ranklist;
		list<stClientRank> * rlist;
		string key;
		uint64_t lastaccess;
	};

	struct stSearchHeroRank
	{
		list<stHeroRank> ranklist;
		list<stHeroRank> * rlist;
		string key;
		uint64_t lastaccess;
	};

	struct stSearchCastleRank
	{
		list<stCastleRank> ranklist;
		list<stCastleRank> * rlist;
		string key;
		uint64_t lastaccess;
	};

	struct stSearchAllianceRank
	{
		list<stAlliance> ranklist;
		list<stAlliance> * rlist;
		string key;
		uint64_t lastaccess;
	};

	struct stPacketOut
	{
		int32_t client;
		amf3object obj;
	};



#pragma endregion

	list<stMarketEntry> m_marketbuy;
	list<stMarketEntry> m_marketsell;

	static bool comparebuy(stMarketEntry first, stMarketEntry second);
	static bool comparesell(stMarketEntry first, stMarketEntry second);
	void AddMarketEntry(stMarketEntry me, int8_t type);

#define DEF_TIMEDARMY 1
#define DEF_TIMEDBUILDING 2
#define DEF_TIMEDRESEARCH 3

	stItem * GetItem(string name);

	double GetPrestigeOfAction(int8_t action, int8_t id, int8_t level, int8_t thlevel);


	void MassDisconnect();
	void AddTimedEvent(stTimedEvent & te);


	stBuildingConfig m_buildingconfig[35][10];
	stBuildingConfig m_researchconfig[25][10];
	stBuildingConfig m_troopconfig[20];

	list<stTimedEvent> armylist;
	list<stTimedEvent> buildinglist;
	list<stTimedEvent> researchlist;

	queue<stPacketOut> m_packetout;

	int64_t m_heroid;
	int64_t m_cityid;
	int32_t m_allianceid;

	vector<int32_t> m_deletedhero;
	vector<int32_t> m_deletedcity;

	int RandomStat();


	Hero * CreateRandomHero(int innlevel);



	static bool compareprestige(stClientRank first, stClientRank second);
	static bool comparehonor(stClientRank first, stClientRank second);
	static bool comparetitle(stClientRank first, stClientRank second);
	static bool comparepop(stClientRank first, stClientRank second);
	static bool comparecities(stClientRank first, stClientRank second);

	void SortPlayers();


	list<stClientRank> m_prestigerank;
	list<stClientRank> m_honorrank;
	list<stClientRank> m_titlerank;
	list<stClientRank> m_populationrank;
	list<stClientRank> m_citiesrank;



	list<stHeroRank> m_herorankstratagem;
	list<stHeroRank> m_herorankpower;
	list<stHeroRank> m_herorankmanagement;
	list<stHeroRank> m_herorankgrade;

	void SortHeroes();
	static bool comparestratagem(stHeroRank first, stHeroRank second);
	static bool comparepower(stHeroRank first, stHeroRank second);
	static bool comparemanagement(stHeroRank first, stHeroRank second);
	static bool comparegrade(stHeroRank first, stHeroRank second);


	list<stCastleRank> m_castleranklevel;
	list<stCastleRank> m_castlerankpopulation;

	void SortCastles();
	static bool comparepopulation(stCastleRank first, stCastleRank second);
	static bool comparelevel(stCastleRank first, stCastleRank second);


	list<stSearchClientRank> m_searchclientranklist;
	list<stSearchHeroRank> m_searchheroranklist;
	list<stSearchCastleRank> m_searchcastleranklist;
	list<stSearchAllianceRank> m_searchallianceranklist;

	void * DoRankSearch(string key, int8_t type, void * subtype, int16_t page, int16_t pagesize);
	void CheckRankSearchTimeouts(uint64_t time);
};

} // namespace server
} // namespace spitfire

#endif // SPITFIRE_SERVER_HPP
