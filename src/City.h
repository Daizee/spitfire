//
// City.h
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

#pragma once

#include <string>
#include "amf3.h"
#include "Hero.h"
//#include "defines.h"

namespace spitfire {
namespace server {

using namespace std;

class Client;

char * GetBuildingName(int id);

// #pragma pack(push)  /* push current alignment to stack */
// #pragma pack(1)     /* set alignment to 1 byte boundary */
// 

// BUILDING IDS

#define B_COTTAGE 1
#define B_BARRACKS 2
#define B_WAREHOUSE 3
#define B_SAWMILL 4
#define B_STONEMINE 5
#define B_IRONMINE 6
#define B_FARM 7
#define B_STABLE 20
#define B_INN 21
#define B_FORGE 22
#define B_MARKETPLACE 23
#define B_RELIEFSTATION 24
#define B_ACADEMY 25
#define B_WORKSHOP 26
#define B_FEASTINGHALL 27
#define B_EMBASSY 28
#define B_RALLYSPOT 29
#define B_BEACONTOWER 30
#define B_TOWNHALL 31
#define B_WALLS 32

#define TR_WORKER 2
#define TR_WARRIOR 3
#define TR_SCOUT 4
#define TR_PIKE 5
#define TR_SWORDS 6
#define TR_ARCHER 7
#define TR_TRANSPORTER 8
#define TR_CAVALRY 9
#define TR_CATAPHRACT 10
#define TR_BALLISTA 11
#define TR_RAM 12
#define TR_CATAPULT 13

#define TR_TRAP 14
#define TR_ABATIS 15
#define TR_ARCHERTOWER 16
#define TR_ROLLINGLOG 17
#define TR_TREBUCHET 18

struct stResources
{
	double food;
	double stone;
	double iron;
	double wood;
	double gold;
	stResources& operator -=(const stResources& b)
	{
		this->food -= b.food;
		this->stone -= b.stone;
		this->iron -= b.iron;
		this->wood -= b.wood;
		this->gold -= b.gold;
		return *this;
	}
	stResources& operator +=(const stResources& b)
	{
		this->food += b.food;
		this->stone += b.stone;
		this->iron += b.iron;
		this->wood += b.wood;
		this->gold += b.gold;
		return *this;
	}
}; // 40 bytes
struct stForts
{
	int32_t traps;
	int32_t abatis;
	int32_t towers;
	int32_t logs;
	int32_t trebs;
}; // 20 bytes
struct stBuilding
{
	int16_t id;
	int16_t level;
	//short appearance; //age2
	int16_t type;
	int16_t status;
	double endtime;
	double starttime;
	//short help; //age2
	//string name;
	amf3object ToObject()
	{
		amf3object obj;
		obj["startTime"] = starttime;
		obj["endTime"] = endtime;//(endtime > 0)?(endtime-1000):(0);// HACK: Attempt to correct "lag" issues.
		obj["level"] = level;
		obj["status"] = status;
		obj["typeId"] = type;
		obj["positionId"] = id;
		obj["name"] = GetBuildingName(type);
		return obj;
	};
}; // 21 bytes

class City
{
public:
	City();
	~City(void);

	bool SetBuilding(int16_t type, int16_t level, int16_t position, int16_t status = 0, double starttime = 0, double endtime = 0);
	void SetResources(int64_t food, int64_t wood, int64_t stone, int64_t iron, int64_t gold);
	void SetForts(int32_t traps, int32_t abatis, int32_t towers, int32_t logs, int32_t trebs);
	int32_t GetForts(int32_t type);

	string m_cityname;
	int8_t m_status;
	int32_t m_tileid;
	int8_t m_type;
	int8_t m_level;

	int8_t m_loyalty;
	int8_t m_grievance;

	stResources m_resources;
	stResources m_maxresources;

	stForts m_forts;

	stBuilding m_innerbuildings[35]; // 735 bytes
	stBuilding m_outerbuildings[41]; // 840 bytes
	// 
	// 	public bool m_npc = true;
	// 	public Client m_client = null;
	// 	public int m_herocount = 0;
	// 	public ArrayList m_heroes = new ArrayList();
	// 	public bool m_allowAlliance = false;
	// 	//public ArrayList m_buildings = new ArrayList();
	// 	public Building[] m_buildings;
};

//#pragma pack(pop)   /* restore original alignment from stack */

class NpcCity: public City
{
public:
	NpcCity();
	~NpcCity(void);
	void Initialize(bool resources, bool troops);
	void SetTroops(int32_t warrior, int32_t pike, int32_t sword, int32_t archer, int32_t cavalry);
	void SetupBuildings();
	void CalculateStats(bool resources, bool troops);

	struct stTroops
	{
		int32_t warrior;
		int32_t pike;
		int32_t sword;
		int32_t archer;
		int32_t cavalry;
	} m_troops, m_maxtroops;

	stForts m_maxforts;

	Hero m_temphero;

	uint64_t m_calculatestuff;


	int32_t m_ownerid;
};

class PlayerCity: public City
{
public:
	PlayerCity();
	~PlayerCity(void);

	Client * m_client;
	int64_t m_castleid;
	int64_t m_accountid;
	string m_logurl;
	int32_t m_population;
	uint32_t m_availablepopulation;
	uint32_t m_maxpopulation;
	bool m_allowalliance;
	bool m_gooutforbattle;
	bool m_hasenemy;
	double m_creation;
	bool m_researching;

	double m_lastcomfort;
	double m_lastlevy;

	struct stTroops
	{
		int64_t worker;
		int64_t warrior;
		int64_t scout;
		int64_t pike;
		int64_t sword;
		int64_t archer;
		int64_t cavalry;
		int64_t cataphract;
		int64_t transporter;
		int64_t ballista;
		int64_t ram;
		int64_t catapult;
	} m_troops, m_injuredtroops; // size 44 bytes

	stResources m_production;
	stResources m_workpopulation;
	stResources m_workrate;
	stResources m_storepercent;

	double m_productionefficiency;
	int32_t m_resourcebaseproduction;
	double m_resourcemanagement;
	int32_t m_resourcetech;
	int32_t m_resourcevalley;

	double m_troopconsume;

	struct stTimers
	{
		double updateresources;
	} m_timers;

	Hero * m_heroes[10]; // 75 bytes * 10
	Hero * m_innheroes[10]; // 75 bytes * 10

	Hero * m_mayor;

	amf3object ToObject();
	amf3array Buildings();
	amf3object Troops();
	amf3object InjuredTroops();
	amf3object Resources();
	amf3array HeroArray();
	amf3object Fortifications();

	void SetTroops(int8_t type, int64_t amount);
	void SetForts(int32_t type, int32_t count);
	int64_t GetTroops(int8_t type);
	void SetMaxResources(int64_t food, int64_t wood, int64_t stone, int64_t iron);

	void ParseBuildings(char * str);
	void ParseTroops(char * str);
	void ParseFortifications(char * str);
	void ParseMisc(string str);

	bool CheckBuildingPrereqs(int16_t type, int16_t level);


	void CalculateStats();
	void CalculateResources();
	void RecacluateCityStats();
	void CalculateResourceStats();


	void CastleUpdate();
	void ResourceUpdate();
	void HeroUpdate(Hero * hero, int16_t updatetype);
	void TroopUpdate();
	void FortUpdate();

	amf3array ResourceProduceData();

	struct stTroopTrain
	{
		int16_t troopid;
		int32_t count;
		int32_t queueid;
		double starttime;
		double endtime;
		double costtime;
	};
	struct stTroopQueue
	{
		int8_t status;
		int16_t positionid;
		int32_t nextqueueid;
		list<stTroopTrain> queue;
	};

	vector<stTroopQueue> m_troopqueue;

	int16_t HeroCount();
	stTroopQueue * GetBarracksQueue(int16_t position);
	int8_t AddToBarracksQueue(int8_t position, int16_t troopid, int32_t count, bool isshare, bool isidle);


	int16_t GetBuildingLevel(int16_t id);
	stBuilding * GetBuilding(int16_t position);
	int16_t GetTechLevel(int16_t id);
	int16_t GetBuildingCount(int16_t id);
}; // 3,448 + base


}
}