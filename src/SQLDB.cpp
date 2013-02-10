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
	m_bInProcess = FALSE;
	m_iRows = 0;
	m_iFields = 0;
	myRow = 0;
	field = 0;
	memset(m_szQuery, 0, QUERY_BUFFER);
	m_pQueryResult = 0;
	xsco = 0;
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
bool CSQLDB::Reset(void)
{
	m_bInProcess = FALSE;
	memset(m_szQuery, 0, QUERY_BUFFER);
	if (m_pQueryResult)
		mysql_free_result(m_pQueryResult);
	m_pQueryResult = NULL;
	if (xsco)
	{
		for (int i = 0; i < m_iRows; i++)
		{
			if (xsco[i])
			{
				// 				for (int j = 0; j < m_iFields; j++)
				// 				{
				// 					if (xsco[i][j].data)
				// 						delete[] xsco[i][j].data;
				// 				}
				delete[] xsco[i];
			}
		}
		delete[] xsco;
		xsco = 0;
	}
	if (field)
		delete[] field;
	field = 0;
	return 1;
}
bool CSQLDB::Query(char * query, ...)
{
	if (m_bInProcess)
		Reset();

	va_list argptr;
	va_start(argptr, query);

	memset(m_szQuery, 0, QUERY_BUFFER);
#ifdef __WIN32__
	_vsprintf_s_l(m_szQuery, QUERY_BUFFER, query, NULL, argptr);
#else
	vsprintf(m_szQuery, query, argptr);
#endif
	va_end(argptr);


	m_bInProcess = TRUE;
	//strcpy_s(m_szQuery, 2048, str2);
	mysql_query(mySQL, m_szQuery);
	m_pQueryResult = mysql_store_result(mySQL);
	return mysql_affected_rows(mySQL);
	if (m_pQueryResult)
		return TRUE;
	else
		return FALSE;
}
bool CSQLDB::Insert(char * query, ...)
{
	mysql_ping(mySQL);
	if (m_bInProcess)
		Reset();

	va_list argptr;
	va_start(argptr, query);

	memset(m_szQuery, 0, QUERY_BUFFER);
#ifdef __WIN32__
	_vsprintf_s_l(m_szQuery, QUERY_BUFFER, query, NULL, argptr);
#else
	vsprintf(m_szQuery, query, argptr);
#endif
	va_end(argptr);

	m_bInProcess = TRUE;
	//strcpy_s(m_szQuery, 2048, str2);
	mysql_query(mySQL, m_szQuery);
	m_pQueryResult = mysql_store_result(mySQL);
	return (bool)mysql_affected_rows(mySQL);
}
bool CSQLDB::Select(char * query, ...)
{
	mysql_ping(mySQL);
	if (m_bInProcess)
		Reset();

	va_list argptr;
	va_start(argptr, query);

	memset(m_szQuery, 0, QUERY_BUFFER);
#ifdef __WIN32__
	_vsprintf_s_l(m_szQuery, QUERY_BUFFER, query, NULL, argptr);
#else
	vsprintf(m_szQuery, query, argptr);
#endif
	va_end(argptr);


	m_bInProcess = TRUE;
	//strcpy_s(m_szQuery, 2048, str2);
	mysql_query(mySQL, m_szQuery);
	m_pQueryResult = mysql_store_result(mySQL);
	if (!m_pQueryResult)
	{
		Reset();
		return FALSE;
	}
	m_iRows = (int)mysql_num_rows(m_pQueryResult);
	m_iFields = mysql_num_fields(m_pQueryResult);
	if (m_iRows == 0)
	{
		Reset();
		return FALSE;
	}
	return m_iRows;
}
bool CSQLDB::Delete(char * query, ...)
{
	mysql_ping(mySQL);
	if (m_bInProcess)
		Reset();

	va_list argptr;
	va_start(argptr, query);

	memset(m_szQuery, 0, QUERY_BUFFER);
#ifdef __WIN32__
	_vsprintf_s_l(m_szQuery, QUERY_BUFFER, query, NULL, argptr);
#else
	vsprintf(m_szQuery, query, argptr);
#endif
	va_end(argptr);


	m_bInProcess = TRUE;
	//strcpy_s(m_szQuery, 2048, str2);
	mysql_query(mySQL, m_szQuery);
	m_pQueryResult = mysql_store_result(mySQL);
	return (bool)mysql_affected_rows(mySQL);
}
bool CSQLDB::Update(char * query, ...)
{
	mysql_ping(mySQL);
	if (m_bInProcess)
		Reset();

	va_list argptr;
	va_start(argptr, query);

	memset(m_szQuery, 0, QUERY_BUFFER);
#ifdef __WIN32__
	_vsprintf_s_l(m_szQuery, QUERY_BUFFER, query, NULL, argptr);
#else
	vsprintf(m_szQuery, query, argptr);
#endif
	va_end(argptr);


	m_bInProcess = TRUE;
	//strcpy_s(m_szQuery, 2048, str2);
	mysql_query(mySQL, m_szQuery);
	m_pQueryResult = mysql_store_result(mySQL);
	return (bool)mysql_affected_rows(mySQL);
}
bool CSQLDB::Fetch(void)
{
	mysql_ping(mySQL);
	if (!m_iFields || !m_iRows)
	{
		Reset();
		return FALSE;
	}
	xsco = new PSQLXSCO[m_iRows];
	for(ushort b = 0; b < m_iRows; b++)
	{
		xsco[b] = new SQLXSCO[m_iFields];
		field = new MYSQL_FIELD*[m_iFields+1];
		if (!m_pQueryResult)
		{
			Log("Mysql error m_pQueryResult = null %s:%d", __FILE__, __LINE__);
			return false;
		}
		myRow = mysql_fetch_row(m_pQueryResult);
		mysql_field_seek(m_pQueryResult, 0);
		for(ushort f = 0; f < m_iFields; f++)
		{
			field[f] = mysql_fetch_field(m_pQueryResult);
			xsco[b][f].type = field[f]->type;
			xsco[b][f].fname = field[f]->name;
			xsco[b][f].row = b;
			//xsco[b][f].data = 0;
			switch (field[f]->type)
			{
			case MYSQL_TYPE_DECIMAL:
			case MYSQL_TYPE_TINY:
			case MYSQL_TYPE_SHORT:
			case MYSQL_TYPE_LONG:
			case MYSQL_TYPE_LONGLONG:
			case MYSQL_TYPE_INT24:
				//number
#ifdef __WIN32__
				xsco[b][f].val = _atoi64(myRow[f]);
#else
				xsco[b][f].val = atoll(myRow[f]);
#endif
				break;
			case MYSQL_TYPE_FLOAT:
			case MYSQL_TYPE_DOUBLE:
				xsco[b][f].val = atof(myRow[f]);
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
					xsco[b][f].len = 0;
					//xsco[b][f].data = 0;
				}
				else
				{
					xsco[b][f].len = strlen(myRow[f]);
					//xsco[b][f].data = new char[xsco[b][f].len+1];
					//memset(xsco[b][f].data, 0, xsco[b][f].len+1);
					xsco[b][f].data = (string)myRow[f];
				}
				break;
			}
		}
	}
	return TRUE;
}
bool CSQLDB::GetField(int row, char * fieldname, void * var, int varlimit)
{
	try
	{
		if (xsco)
			for (int i = 0; i < m_iFields; i++)
			{
				if (xsco[row][i].fname == fieldname)
				{
					switch(xsco[row][i].type)
					{
					case MYSQL_TYPE_TINY:
					case MYSQL_TYPE_SHORT:
						*(short*)var = (short)xsco[row][i].val;
						break;
					case MYSQL_TYPE_LONG:
					case MYSQL_TYPE_INT24:
						*(int*)var = (int)xsco[row][i].val;
						break;
					case MYSQL_TYPE_FLOAT:
					case MYSQL_TYPE_DECIMAL:
						*(float*)var = (float)xsco[row][i].val;
						break;
					case MYSQL_TYPE_DOUBLE:
						*(double*)var = (double)xsco[row][i].val;
						break;
					case MYSQL_TYPE_LONGLONG:
						*(uint64_t*)var = (uint64_t)xsco[row][i].val;
						break;
					case MYSQL_TYPE_TINY_BLOB:
					case MYSQL_TYPE_MEDIUM_BLOB:
					case MYSQL_TYPE_LONG_BLOB:
					case MYSQL_TYPE_BLOB:
					case MYSQL_TYPE_VAR_STRING:
					case MYSQL_TYPE_STRING:
						if (xsco[row][i].len)
#ifdef __WIN32__
							memcpy_s(var, varlimit, (char*)xsco[row][i].data.c_str(), xsco[row][i].len);
#else
							memcpy(var, (char*)xsco[row][i].data.c_str(), xsco[row][i].len);
#endif
						break;
					}
					return TRUE;
				}
			}
			return FALSE;
	}
	catch(...)
	{
#ifdef __WIN32__
		::MessageBoxA(NULL, "Error in GetField(), possible memory corruption. Please debug.", "ERROR", MB_OK);
#endif
		return FALSE;
	}
}
char * CSQLDB::GetString(int row, char * fieldname)
{
	if (xsco)
		for (int i = 0; i < m_iFields; i++)
		{
			if (xsco[row][i].fname == fieldname)
			{
				return (char*)xsco[row][i].data.c_str();
			}
		}
		return FALSE;
}
int32_t CSQLDB::GetInt(int row, char * fieldname)
{
	if (xsco)
		for (int i = 0; i < m_iFields; i++)
		{
			if (xsco[row][i].fname == fieldname)
			{
				return (int)xsco[row][i].val;
			}
		}
		return FALSE;
}
uint32_t CSQLDB::GetUInt(int row, char * fieldname)
{
	if (xsco)
		for (int i = 0; i < m_iFields; i++)
		{
			if (xsco[row][i].fname == fieldname)
			{
				return (int)xsco[row][i].uval;
			}
		}
		return FALSE;
}
int64_t CSQLDB::GetInt64(int row, char * fieldname)
{
	if (xsco)
		for (int i = 0; i < m_iFields; i++)
		{
			if (xsco[row][i].fname == fieldname)
			{
				return xsco[row][i].val;
			}
		}
		return FALSE;
}
uint64_t CSQLDB::GetUInt64(int row, char * fieldname)
{
	if (xsco)
		for (int i = 0; i < m_iFields; i++)
		{
			if (xsco[row][i].fname == fieldname)
			{
				return xsco[row][i].uval;
			}
		}
		return FALSE;
}
double CSQLDB::GetDouble(int row, char * fieldname)
{
	if (xsco)
		for (int i = 0; i < m_iFields; i++)
		{
			if (xsco[row][i].fname == fieldname)
			{
				return xsco[row][i].val;
			}
		}
		return FALSE;
}

}
}