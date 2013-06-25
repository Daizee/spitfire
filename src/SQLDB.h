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


typedef struct
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
} SQLARRAY;

class CSQLDB
{
public:
	CSQLDB(void);
	~CSQLDB(void);
	int Init(const char *host, const char *user, const char *passwd, const char *db);
	MYSQL * mySQL;
	my_bool Query(char * query, ...);
	my_bool Select(char * query, ...);
	my_bool Fetch(void);
	my_bool Reset(void);
	my_bool GetField(int row, char * fieldname, void * var, int varlimit = 0);
	char * GetString(int row, char * fieldname);
	int32_t GetInt(int row, char * fieldname);
	uint32_t GetUInt(int row, char * fieldname);
	int64_t GetInt64(int row, char * fieldname);
	uint64_t GetUInt64(int row, char * fieldname);
	double GetDouble(int row, char * fieldname);
	my_bool processing;
	st_mysql_res* m_pQueryResult;
	string m_szQuery;
	int m_iRows, m_iFields;
	MYSQL_FIELD  **	field;
	MYSQL_ROW	   myRow;
	SQLARRAY	** resultarray;
	my_bool failed;
};

}
}