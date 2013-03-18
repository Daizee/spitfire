//
// SQLDB.cpp
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

#include "funcs.h"
#include "SQLDB.h"

namespace spitfire {
namespace server {

//MYSQL * CSQLDB::mySQL;

CSQLDB::CSQLDB(void)
{
	processing = FALSE;
	m_iRows = 0;
	m_iFields = 0;
	myRow = 0;
	field = 0;
	m_pQueryResult = 0;
	resultarray = 0;
	mySQL = 0;
	failed = true;
}

CSQLDB::~CSQLDB(void)
{
	Reset();
	if (!failed)
		mysql_close(mySQL);
}
int CSQLDB::Init(const char *host, const char *user, const char *passwd, const char *db)
{
	//Init mySQL
	mySQL = mysql_init(mySQL);
	my_bool rec = 1;
	mysql_options(mySQL, MYSQL_OPT_RECONNECT, &rec);
	if(!mysql_real_connect(mySQL, host, user, passwd, db, 3306, NULL, NULL))
	{
		printf("(!!!) mySql connection rejected!\n");
		mysql_close(mySQL);
		return 0;
	}
	failed = false;
	//printf("mySQL connected successfully.\n");
	return 1;
}
my_bool CSQLDB::Reset(void)
{
#ifndef WIN32
		struct timespec req={0};
		req.tv_sec = 0;
		req.tv_nsec = 1000000L;//1ms
#endif
	while (processing)
	{
#ifdef WIN32
		Sleep(1);
#else
		nanosleep(&req,NULL);
#endif
	}
	m_szQuery.clear();
	if (m_pQueryResult)
		mysql_free_result(m_pQueryResult);
	m_pQueryResult = 0;
	if (resultarray)
	{
		for (int i = 0; i < m_iRows; i++)
		{
			if (resultarray[i])
			{
// 				for (int j = 0; j < m_iFields; j++)
// 				{
// 					if (resultarray[i][j].data)
// 						delete[] resultarray[i][j].data;
// 				}
				delete[] resultarray[i];
			}
		}
		delete[] resultarray;
		resultarray = 0;
	}
	if (field)
		delete[] field;
	field = 0;
	return true;
}
my_bool CSQLDB::Query(char * query, ...)
{
	//mysql_ping(mySQL);
	Reset();
	processing = true;

	va_list argptr;
	va_start(argptr, query);
	char * newquery = 0;
	int ret;
	ret = vasprintf(&newquery, query, argptr);
	va_end(argptr);

	if (ret == -1)
	{
		//failed to create query string
		processing = false;
		return false;
	}
	else
	{
		m_szQuery = newquery;
		free(newquery);
		mysql_query(mySQL, m_szQuery.c_str());
		m_pQueryResult = mysql_store_result(mySQL);
		m_szQuery.clear();
		processing = false;
		return mysql_affected_rows(mySQL);
	}
}
my_bool CSQLDB::Select(char * query, ...)
{
	//mysql_ping(mySQL);
	Reset();
	processing = true;

	va_list argptr;
	va_start(argptr, query);
	char * newquery = 0;
	int ret;
	ret = vasprintf(&newquery, query, argptr);
	va_end(argptr);

	if (ret == -1)
	{
		//failed to create query string
		processing = false;
		return false;
	}
	else
	{
		m_szQuery = newquery;
		free(newquery);
		mysql_query(mySQL, m_szQuery.c_str());
		m_pQueryResult = mysql_store_result(mySQL);
		m_szQuery.clear();
		if (!m_pQueryResult)
		{
			processing = false;
			Reset();
			return false;
		}
		m_iRows = (int)mysql_num_rows(m_pQueryResult);
		m_iFields = mysql_num_fields(m_pQueryResult);
		processing = false;
		if (m_iRows == 0)
		{
			Reset();
			return false;
		}
		return m_iRows;
	}
}
my_bool CSQLDB::Fetch(void)
{
	mysql_ping(mySQL);
	if (!m_iFields || !m_iRows)
	{
		Reset();
		return false;
	}
	if (!m_pQueryResult)
	{
		Log("MySQL error m_pQueryResult = null %s:%d", __FILE__, __LINE__);
		return false;
	}
	resultarray = new SQLARRAY*[m_iRows];
	for(ushort b = 0; b < m_iRows; b++)
	{
		resultarray[b] = new SQLARRAY[m_iFields];
		field = new MYSQL_FIELD*[m_iFields+1];

		myRow = mysql_fetch_row(m_pQueryResult);
		mysql_field_seek(m_pQueryResult, 0);
		for(ushort f = 0; f < m_iFields; f++)
		{
			field[f] = mysql_fetch_field(m_pQueryResult);
			resultarray[b][f].type = field[f]->type;
			resultarray[b][f].fname = field[f]->name;
			resultarray[b][f].row = b;
			//resultarray[b][f].data = 0;
			switch (field[f]->type)
			{
			case MYSQL_TYPE_DECIMAL:
			case MYSQL_TYPE_TINY:
			case MYSQL_TYPE_SHORT:
			case MYSQL_TYPE_LONG:
			case MYSQL_TYPE_LONGLONG:
			case MYSQL_TYPE_INT24:
				//number
#ifdef WIN32
				resultarray[b][f].val = _atoi64(myRow[f]);
#else
				resultarray[b][f].val = atoll(myRow[f]);
#endif
				break;
			case MYSQL_TYPE_FLOAT:
			case MYSQL_TYPE_DOUBLE:
				resultarray[b][f].fval = atof(myRow[f]);
				break;
			case MYSQL_TYPE_TINY_BLOB:
			case MYSQL_TYPE_MEDIUM_BLOB:
			case MYSQL_TYPE_LONG_BLOB:
			case MYSQL_TYPE_BLOB:
			case MYSQL_TYPE_VAR_STRING:
			case MYSQL_TYPE_STRING:
				//string
				if (myRow[f] == 0)
				{
					//null string
					resultarray[b][f].len = 0;
					//resultarray[b][f].data = 0;
				}
				else
				{
					resultarray[b][f].len = strlen(myRow[f]);
					//resultarray[b][f].data = new char[resultarray[b][f].len+1];
					//memset(resultarray[b][f].data, 0, resultarray[b][f].len+1);
					resultarray[b][f].data = (string)myRow[f];
				}
				break;
			}
		}
	}
	return TRUE;
}
my_bool CSQLDB::GetField(int row, char * fieldname, void * var, int varlimit)
{
	try
	{
		if (resultarray)
			for (int i = 0; i < m_iFields; i++)
			{
				if (resultarray[row][i].fname == fieldname)
				{
					switch(resultarray[row][i].type)
					{
					case MYSQL_TYPE_TINY:
					case MYSQL_TYPE_SHORT:
						*(short*)var = (short)resultarray[row][i].val;
						break;
					case MYSQL_TYPE_LONG:
					case MYSQL_TYPE_INT24:
						*(int*)var = (int)resultarray[row][i].val;
						break;
					case MYSQL_TYPE_FLOAT:
					case MYSQL_TYPE_DECIMAL:
						*(float*)var = (float)resultarray[row][i].val;
						break;
					case MYSQL_TYPE_DOUBLE:
						*(double*)var = (double)resultarray[row][i].val;
						break;
					case MYSQL_TYPE_LONGLONG:
						*(uint64_t*)var = (uint64_t)resultarray[row][i].val;
						break;
					case MYSQL_TYPE_TINY_BLOB:
					case MYSQL_TYPE_MEDIUM_BLOB:
					case MYSQL_TYPE_LONG_BLOB:
					case MYSQL_TYPE_BLOB:
					case MYSQL_TYPE_VAR_STRING:
					case MYSQL_TYPE_STRING:
						if (resultarray[row][i].len)
#ifdef WIN32
							memcpy_s(var, varlimit, (char*)resultarray[row][i].data.c_str(), resultarray[row][i].len);
#else
							if (resultarray[row][i].len > varlimit)
								memcpy(var, (char*)resultarray[row][i].data.c_str(), varlimit);
							else
								memcpy(var, (char*)resultarray[row][i].data.c_str(), resultarray[row][i].len);

#endif
						break;
					}
					return true;
				}
			}
			return false;
	}
	catch(...)
	{
		Log("Error in GetField(), possible memory corruption. Please debug.");
		return false;
	}
}
char * CSQLDB::GetString(int row, char * fieldname)
{
	if (resultarray)
		for (int i = 0; i < m_iFields; i++)
		{
			if (resultarray[row][i].fname == fieldname)
			{
				return (char*)resultarray[row][i].data.c_str();
			}
		}
		return FALSE;
}
int32_t CSQLDB::GetInt(int row, char * fieldname)
{
	if (resultarray)
		for (int i = 0; i < m_iFields; i++)
		{
			if (resultarray[row][i].fname == fieldname)
			{
				return (int)resultarray[row][i].val;
			}
		}
		return FALSE;
}
uint32_t CSQLDB::GetUInt(int row, char * fieldname)
{
	if (resultarray)
		for (int i = 0; i < m_iFields; i++)
		{
			if (resultarray[row][i].fname == fieldname)
			{
				return (int)resultarray[row][i].uval;
			}
		}
		return FALSE;
}
int64_t CSQLDB::GetInt64(int row, char * fieldname)
{
	if (resultarray)
		for (int i = 0; i < m_iFields; i++)
		{
			if (resultarray[row][i].fname == fieldname)
			{
				return resultarray[row][i].val;
			}
		}
		return FALSE;
}
uint64_t CSQLDB::GetUInt64(int row, char * fieldname)
{
	if (resultarray)
		for (int i = 0; i < m_iFields; i++)
		{
			if (resultarray[row][i].fname == fieldname)
			{
				return resultarray[row][i].uval;
			}
		}
		return FALSE;
}
double CSQLDB::GetDouble(int row, char * fieldname)
{
	if (resultarray)
		for (int i = 0; i < m_iFields; i++)
		{
			if (resultarray[row][i].fname == fieldname)
			{
				return resultarray[row][i].fval;
			}
		}
		return FALSE;
}

}
}