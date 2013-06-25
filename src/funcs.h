//
// funcs.h
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

#ifdef WIN32
#include <SDKDDKVer.h>
#include <tchar.h>
#include <direct.h>
#include <process.h>
//#else
//#include <thread>
#endif

#include <stdarg.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <queue>

#include <stdio.h>

#include <memory>
#include <memory.h>

#include "amfdefs.h"
#include "amf3.h"

#include <math.h>

using namespace std;


namespace spitfire {
namespace server {


class Hero;
class Client;
class City;
class PlayerCity;
class NpcCity;
class Alliance;

char * GetBuildingName(int id);
uint64_t unixtime();


#define DEF_STATES 16

#define DEF_STATE1 "FRIESLAND"
#define DEF_STATE2 "SAXONY"
#define DEF_STATE3 "NORTH MARCH"
#define DEF_STATE4 "BOHEMIA"
#define DEF_STATE5 "LOWER LORRAINE"
#define DEF_STATE6 "FRANCONIA"
#define DEF_STATE7 "THURINGIA"
#define DEF_STATE8 "MORAVIA"
#define DEF_STATE9 "UPPER LORRAINE"
#define DEF_STATE10 "SWABIA"
#define DEF_STATE11 "BAVARIA"
#define DEF_STATE12 "CARINTHIA"
#define DEF_STATE13 "BURGUNDY"
#define DEF_STATE14 "LOMBARDY"
#define DEF_STATE15 "TUSCANY"
#define DEF_STATE16 "ROMAGNA"




#define DEF_MAXQUEUE 1000

//#define DEF_MAPSIZE 800

#ifdef WIN32
#define DBL "%Lf"
#define DBL2 "Lf"
#define XI64 "%I64d"
#else
#define DBL "%f"
#define DBL2 "f"
#define XI64 "%lld"
#endif



#ifdef WIN32
#define SLEEP(a) Sleep(a)
#else
#define SLEEP(a) { struct timespec req={0}; req.tv_sec = 0; req.tv_nsec = 1000000 * a; nanosleep(&req,NULL); }
#endif

//begin mutexes
#define M_CLIENTLIST 1
#define M_HEROLIST 2
#define M_CASTLELIST 3
#define M_ALLIANCELIST 4
#define M_TIMEDLIST 5
#define M_MAP 6
#define M_DELETELIST 7
#define M_RANKEDLIST 8


// #define MULTILOCK(a,b) while (!server->m_mutex.multilock(a,b,0, __LINE__)) { SLEEP(10); }
// #define MULTILOCK2(a,b,c) while (!server->m_mutex.multilock(a,b,c, __LINE__)) { SLEEP(10); }
// #define LOCK(a)	while (!server->m_mutex.lock(a, __LINE__)) { SLEEP(10); }
// #define UNLOCK(a) server->m_mutex.unlock(a, __LINE__)
#define MULTILOCK(a,b)
#define MULTILOCK2(a,b,c)
#define LOCK(a)
#define UNLOCK(a)

//end mutexes

#define DEF_RESEARCH 1
#define DEF_BUILDING 2
#define DEF_TRAIN 3



#ifndef WIN32
#define strtok_s strtok_r
#define _atoi64 atoll
#define sprintf_s snprintf
#define strcpy_s(a,b,c) strcpy(a,c)
#endif


#define KeyExists(x,y) ((x._value._object->Exists(y))>=0)
#define IsString(x) (x.type==String)
#define IsObject(x) (x.type==Object)

#define CHECKCASTLEID() \
	if (IsObject(data) && KeyExists(data, "castleId") && (uint32_t)data["castleId"] != client->m_currentcityid) \
{ \
	Log("castleId does not match castle focus! gave: %d is:%d - cmd: %s - accountid:%d - playername: %s", (uint32_t)data["castleId"], client->m_currentcityid, cmd.c_str(), client->m_accountid, (char*)client->m_playername.c_str()); \
}

#define VERIFYCASTLEID() \
	if (!IsObject(data) || !KeyExists(data, "castleId") ) \
{ \
	Log("castleId not received!"); \
	return; \
}
// 
#define _HAS_ITERATOR_DEBUGGING 1
#define _ITERATOR_DEBUG_LEVEL 2





#define SOCKET_SERVER 2

//#define DEF_MAXCLIENTS 1024
#define DEF_MAXPHP 100
#define DEF_MAXALLIANCES 1000
#define DEF_MAXITEMS 400

#define DEF_LISTENSOCK 1
#define DEF_NORMALSOCK 2

// typedef unsigned long DWORD;
// typedef unsigned short WORD;
// 
// typedef unsigned int uint;
// typedef unsigned char uchar;
// typedef unsigned long ulong;
// typedef unsigned short ushort;
// typedef unsigned short word;
// typedef unsigned int dword;


// typedef unsigned char uint8_t;
// typedef unsigned short int uint16_t;
// typedef unsigned long int uint32_t;
//typedef unsigned long long int uint64_t;
// typedef char int8_t;
// typedef short int int16_t;
// typedef long int int32_t;
//typedef long long int int64_t;

#define GETX xfromid
#define GETY yfromid
#define GETID idfromxy

#define GETXYFROMID(a) short yfromid = short(a / DEF_MAPSIZE); short xfromid = short(a % DEF_MAPSIZE);

#define GETIDFROMXY(x,y) int idfromxy = y*DEF_MAPSIZE+x;


extern char * CITYLIST;


//typedef uint64_t I64;

#define DEF_MAXBOTS 200
#define DEF_MAXQUEUE 1000
#define DEF_MAXSOCKETS 500
#define LUACPPCOMMANDS 100

#define DEF_SOCKETTHREADS 2

#define ByteSwap(x) ByteSwap5((unsigned char *) &x, sizeof(x))

extern void Log( char * str, ...);
extern void DLog( char * fmt, char * file, int line, char * str, ...);
extern void a_swap(unsigned char * a, unsigned char * b);
extern void ByteSwap5(unsigned char * b, int n);

#ifdef WIN32

#ifndef VA_COPY
# ifdef HAVE_VA_COPY
#  define VA_COPY(dest, src) va_copy(dest, src)
# else
#  ifdef HAVE___VA_COPY
#   define VA_COPY(dest, src) __va_copy(dest, src)
#  else
#   define VA_COPY(dest, src) (dest) = (src)
#  endif
# endif
#endif

#define INIT_SZ 256

extern int vasprintf(char **str, const char *fmt, va_list ap);
extern int asprintf(char **str, const char *fmt, ...);
#endif

#define DEF_NORMAL 0
#define DEF_PEACETIME 1
#define DEF_TRUCE 2
#define DEF_BEGINNER 3
#define DEF_HOLIDAY 5

#define FOREST 1
#define DESERT 2
#define HILL 3
#define SWAMP 4
#define GRASS 5
#define LAKE 6
#define FLAT 10
#define CASTLE 11
#define NPC 12


#define WORKER 1
#define WARRIOR 2
#define SCOUT 3
#define PIKE 4
#define SWORDS 5
#define ARCHER 6
#define CAVALRY 7
#define CATAPHRACT 8
#define TRANSPORTER 9
#define RAM 10
#define CATAPULT 11


#define DEF_ALLIANCEHOST 4
#define DEF_ALLIANCEVICEHOST 5
#define DEF_ALLIANCEPRES 6
#define DEF_ALLIANCEOFFICER 7
#define DEF_ALLIANCEMEMBER 8


#define DEF_HEROIDLE 0
#define DEF_HEROMAYOR 1
#define DEF_HEROREINFORCE //exists?
#define DEF_HEROATTACK 3
#define DEF_HEROSCOUT //exists?


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

// TROOP IDS
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


// RESEARCH IDS
#define T_AGRICULTURE 1
#define T_LUMBERING 2
#define T_MASONRY 3
#define T_MINING 4
#define T_METALCASTING 5
#define T_INFORMATICS 7
#define T_MILITARYSCIENCE 8
#define T_MILITARYTRADITION 9
#define T_IRONWORKING 10
#define T_LOGISTICS 11
#define T_COMPASS 12
#define T_HORSEBACKRIDING 13
#define T_ARCHERY 14
#define T_STOCKPILE 15
#define T_MEDICINE 16
#define T_CONSTRUCTION 17
#define T_ENGINEERING 18
#define T_MACHINERY 19
#define T_PRIVATEERING 20

#pragma region structs and stuff

struct stResources
{
	double gold;
	double food;
	double wood;
	double stone;
	double iron;
	stResources& operator -=(const stResources& b)
	{
		this->gold -= b.gold;
		this->food -= b.food;
		this->wood -= b.wood;
		this->stone -= b.stone;
		this->iron -= b.iron;
		return *this;
	}
	stResources& operator +=(const stResources& b)
	{
		this->gold += b.gold;
		this->food += b.food;
		this->wood += b.wood;
		this->stone += b.stone;
		this->iron += b.iron;
		return *this;
	}
	amf3object ToObject()
	{
		amf3object obj = amf3object();
		obj["wood"] = wood;
		obj["food"] = food;
		obj["stone"] = stone;
		obj["gold"] = gold;
		obj["iron"] = iron;
		return obj;
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
struct stPrereq
{
	int32_t id;
	int32_t level;
};
struct stItemConfig
{
	string name;
	int32_t cost;
	int32_t saleprice;
	int32_t daylimit;
	int32_t type;
	bool cangamble;
	bool buyable;
	int32_t rarity;
};
struct stRarityGamble
{
	vector<stItemConfig*> common;
	vector<stItemConfig*> special;
	vector<stItemConfig*> rare;
	vector<stItemConfig*> superrare;
	vector<stItemConfig*> ultrarare;
};
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
	stTroops& operator -=(const stTroops& b)
	{
		this->worker -= b.worker;
		this->warrior -= b.warrior;
		this->scout -= b.scout;
		this->pike -= b.pike;
		this->sword -= b.sword;
		this->archer -= b.archer;
		this->cavalry -= b.cavalry;
		this->cataphract -= b.cataphract;
		this->transporter -= b.transporter;
		this->ballista -= b.ballista;
		this->ram -= b.ram;
		this->catapult -= b.catapult;
		return *this;
	}
	stTroops& operator +=(const stTroops& b)
	{
		this->worker += b.worker;
		this->warrior += b.warrior;
		this->scout += b.scout;
		this->pike += b.pike;
		this->sword += b.sword;
		this->archer += b.archer;
		this->cavalry += b.cavalry;
		this->cataphract += b.cataphract;
		this->transporter += b.transporter;
		this->ballista += b.ballista;
		this->ram += b.ram;
		this->catapult += b.catapult;
		return *this;
	}
	amf3object ToObject()
	{
		amf3object obj = amf3object();
		obj["peasants"] = worker;
		obj["catapult"] = catapult;
		obj["archer"] = archer;
		obj["ballista"] = ballista;
		obj["scouter"] = scout;
		obj["carriage"] = transporter;
		obj["heavyCavalry"] = cataphract;
		obj["militia"] = warrior;
		obj["lightCavalry"] = cavalry;
		obj["swordsmen"] = sword;
		obj["pikemen"] = pike;
		obj["batteringRam"] = ram;
		return obj;
	}
};
struct stAlliance
{
	int32_t id;
	union
	{
		int32_t members;
		double honor;
		double prestige;
	};
	uint32_t rank;
	Alliance * ref;
};
struct stArmyMovement
{
	stArmyMovement() {	memset(&resources, 0, sizeof(stResources)); memset(&troops, 0, sizeof(stTroops)); }
	Hero * hero;
	string heroname;
	int16_t direction;
	stResources resources;
	string startposname;
	string king;
	stTroops troops;
	uint64_t starttime;
	uint64_t armyid;
	uint64_t reachtime;
	uint32_t herolevel;
	uint64_t resttime;
	uint32_t missiontype;
	uint32_t startfieldid;
	uint32_t targetfieldid;
	string targetposname;
	City * city;
	Client * client;
	amf3object ToObject()
	{
		amf3object obj = amf3object();
		obj["hero"] = heroname;
		obj["direction"] = direction;
		obj["resource"] = resources.ToObject();
		obj["startPosName"] = startposname;
		obj["king"] = king;
		obj["troop"] = troops.ToObject();
		obj["startTime"] = starttime;
		obj["armyId"] = armyid;
		obj["reachTime"] = reachtime;
		obj["heroLevel"] = herolevel;
		obj["restTime"] = resttime;
		obj["missionType"] = missiontype;
		obj["startFieldId"] = startfieldid;
		obj["targetFieldId"] = targetfieldid;
		obj["targetPosName"] = targetposname;
		return obj;
	}
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
struct stBuff
{
	string id;
	string desc;
	double endtime;
};
struct stItem
{
	string id;
	int16_t count;
	int16_t mincount;
	int16_t maxcount;
};
struct stResearch
{
	int16_t level;
	uint32_t castleid;
	double endtime;
	double starttime;
};

#pragma endregion


}
}