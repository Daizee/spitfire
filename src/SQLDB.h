//
// SQLDB.h
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

//#include "GameServer.h"

#ifdef WIN32
#include <my_global.h>
#include <my_sys.h>
#include <mysql.h>
#else
#include <mysql/my_global.h>
#include <mysql/my_sys.h>
#include <mysql/mysql.h>
#endif

namespace spitfire {
namespace server {


typedef struct tagXSCO
{
	string   data;
	int      len;
	int      type;
	union{
		int64_t val;
		uint64_t uval;
		double   fval;
	};
	string   fname;
	int      field;
	uint64_t row;
} SQLXSCO, *PSQLXSCO, **PPSQLXSCO;

#define QUERY_BUFFER 20480
class CSQLDB
{
public:
	CSQLDB(void);
	~CSQLDB(void);
	int Init(const char *host, const char *user, const char *passwd, const char *db);
	MYSQL * mySQL;
	bool Query(char * query, ...);
	bool Insert(char * query, ...);
	bool Select(char * query, ...);
	bool Delete(char * query, ...);
	bool Update(char * query, ...);
	bool Fetch(void);
	bool Reset(void);
	bool GetField(int row, char * fieldname, void * var, int varlimit = 0);
	char * GetString(int row, char * fieldname);
	int32_t GetInt(int row, char * fieldname);
	uint32_t GetUInt(int row, char * fieldname);
	int64_t GetInt64(int row, char * fieldname);
	uint64_t GetUInt64(int row, char * fieldname);
	double GetDouble(int row, char * fieldname);
	bool m_bInProcess;
	st_mysql_res* m_pQueryResult;
	char m_szQuery [QUERY_BUFFER];
	int m_iRows, m_iFields;
	MYSQL_FIELD  **	field;
	MYSQL_ROW	   myRow;
	PPSQLXSCO	xsco;
	bool failed;
};

}
}