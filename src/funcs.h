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

#include <math.h>

using namespace std;


namespace spitfire {
namespace server {


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
	if (IsObject(data) && KeyExists(data, "castleId") && (int)data["castleId"] != client->m_currentcityid) \
{ \
	Log("castleId does not match castle focus! gave: %d is:%d - cmd: %s - accountid:%d - playername: %s", (int)data["castleId"], client->m_currentcityid, cmd.c_str(), client->m_accountid, (char*)client->m_playername.c_str()); \
}

#define VERIFYCASTLEID() \
	if (!IsObject(data) || !KeyExists(data, "castleId") ) \
{ \
	Log("castleId not received!"); \
	return; \
}
// 
// #define _HAS_ITERATOR_DEBUGGING 0
// #define _ITERATOR_DEBUG_LEVEL 0





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
// 
// #define SFLOGGING 5
// 
// #define SFERROR(a) Log("Fatal error! File: %s Line: %d Error: %s", __FILE__, __LINE__, (char*)a)
// #if SFLOGGING>=5
// #define SFLOGFINEST(a) Log("\033[36m%s:%d - %s\033[0m", __FILE__, __LINE__, (char*)a)
// #else
// #define SFLOGFINEST(a)
// #endif
// 
// #if SFLOGGING>=4
// #define SFLOGINFO(a) Log("INFO: %s:%d - %s", __FILE__, __LINE__, (char*)a)
// #else
// #define SFLOGFINEST(a)
// #endif
// 
// #if SFLOGGING>=3
// #define SFLOGWARNING(a) Log("WARNING: %s:%d - %s", __FILE__, __LINE__, (char*)a)
// #else
// #define SFLOGFINEST(a)
// #endif
// 
// #if SFLOGGING>=2
// #define SFLOGERROR(a) Log("ERROR: %s:%d - %s", __FILE__, __LINE__, (char*)a)
// #else
// #define SFLOGFINEST(a)
// #endif
// 
// #if SFLOGGING>=1
// #define SFLOGFATAL(a) Log("FATAL: %s:%d - %s", __FILE__, __LINE__, (char*)a)
// #else
// #define SFLOGFINEST(a)
// #endif
// 
// 
// 
// #define SFLOG(a, ...) DLog("DLog: %s:%d - %s", __FILE__, __LINE__, (char*)a, __VA_ARGS__ )


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

}
}