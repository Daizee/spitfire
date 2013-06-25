//
// funcs.cpp
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

namespace spitfire {
namespace server {

#include <sys/timeb.h>

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

void Log( char * str, ...)
{
	va_list argptr;
	va_start(argptr, str);

	char str2[3000];
	memset(str2, 0, 3000);
#ifdef WIN32
	_vsprintf_s_l(str2, 3000, str, NULL, argptr);
	printf(str2);
	printf("\n");
#else
	vsnprintf(str2, 3000, str, argptr);
	printf("%s\n", str2);
#endif
	va_end(argptr);
}


void DLog( char * fmt, char * file, int line, char * str, ...)
{
	va_list argptr;
	va_start(argptr, str);

	char str2[3000];
	memset(str2, 0, 3000);

	char str3[3000];
	memset(str3, 0, 3000);

#ifdef WIN32
	_vsprintf_s_l(str2, 3000, str, NULL, argptr);
	_sprintf_s_l(str3, 3000, fmt, NULL, file, line, str2);
	printf(str3);
#else
	vsnprintf(str2, 3000, str, argptr);
	snprintf(str3, 3000, fmt, file, line, str2);
	printf("%s\n", str3);
#endif
	va_end(argptr);

}

inline void a_swap(unsigned char * a, unsigned char * b)
{
	register char c, d;
	c = *a;
	d = *b;
	*b = c;
	*a = d;
}
void ByteSwap5(unsigned char * b, int n)
{
	register int i = 0;
	register int j = n-1;
	while (i<j)
	{
		a_swap(&b[i], &b[j]);
		i++, j--;
	}
}

int htoi(char hex)
{
	switch (hex)
	{
	case 49:
		return 1;
	case 50:
		return 2;
	case 51:
		return 3;
	case 52:
		return 4;
	case 53:
		return 5;
	case 54:
		return 6;
	case 55:
		return 7;
	case 56:
		return 8;
	case 57:
		return 9;
	case 48:
		return 0;
	case 97://a
		return 10;
	case 98://b
		return 11;
	case 99://c
		return 12;
	case 100://d
		return 13;
	case 101://e
		return 14;
	case 102://f
		return 15;
	}
	return 0;
}
char itoh(int num)
{
	switch (num)
	{
	case 1:
		return 49;
	case 2:
		return 50;
	case 3:
		return 51;
	case 4:
		return 52;
	case 5:
		return 53;
	case 6:
		return 54;
	case 7:
		return 55;
	case 8:
		return 56;
	case 9:
		return 57;
	case 0:
		return 48;
	case 10://a
		return 97;
	case 11://b
		return 98;
	case 12://c
		return 99;
	case 13://d
		return 100;
	case 14://e
		return 101;
	case 15://f
		return 102;
	}
	return 0;
}
//m_tile[y*mapsize+x].m_id = y*mapsize+x;

// y = tid / G_pCore->m_servers[bot->m_serverid]->m_mapsize;
// x = (tid - (y * G_pCore->m_servers[bot->m_serverid]->m_mapsize));
// 
// inline int GetXYFromID(int id)
// {
// 	int x,y;
// 	y = id / DEF_MAPSIZE;
// 	x = id - (DEF_MAPSIZE * y);
// }
// 
// inline int GetIDFromXY(int x, int y)
// {
// 	return y*mapsize+x;
// }

// #define DEF_STATE1 "FRIESLAND"
// #define DEF_STATE2 "SAXONY"
// #define DEF_STATE3 "NORTH MARCH"
// #define DEF_STATE4 "BOHEMIA"
// #define DEF_STATE5 "LOWER LORRAINE"
// #define DEF_STATE6 "FRANCONIA"
// #define DEF_STATE7 "THURINGIA"
// #define DEF_STATE8 "MORAVIA"
// #define DEF_STATE9 "UPPER LORRAINE"
// #define DEF_STATE10 "SWABIA"
// #define DEF_STATE11 "BAVARIA"
// #define DEF_STATE12 "CARINTHIA"
// #define DEF_STATE13 "BURGUNDY"
// #define DEF_STATE14 "LOMBARDY"
// #define DEF_STATE15 "TUSCANY"
// #define DEF_STATE16 "ROMAGNA"



typedef struct tagBOX
{
	int x1;
	int x2;
	int y1;
	int y2;
} BOX;




char * GetBuildingName(int id)
{
	switch (id)
	{
	case 1:
		return "Cottage";
	case 2:
		return "Barracks";
	case 3:
		return "Warehouse";
	case 4:
		return "Sawmill";
	case 5:
		return "Stonemine";
	case 6:
		return "Ironmine";
	case 7:
		return "Farm";
	case 20:
		return "Stable";
	case 21:
		return "Inn";
	case 22:
		return "Forge";
	case 23:
		return "Marketplace";
	case 24:
		return "Relief Station";
	case 25:
		return "Academy";
	case 26:
		return "Workshop";
	case 27:
		return "Feasting Hall";
	case 28:
		return "Embassy";
	case 29:
		return "Rally Spot";
	case 30:
		return "Beacon Tower";
	case 31:
		return "Town Hall";
	case 32:
		return "Walls";
	}
	return "Failed";
}
#ifdef WIN32
int vasprintf(char **str, const char *fmt, va_list ap)
{
	int ret = -1;
	va_list ap2;
	char *string, *newstr;
	size_t len;

	VA_COPY(ap2, ap);
	if ((string = (char*)malloc(INIT_SZ)) == NULL)
		goto fail;

	ret = vsnprintf(string, INIT_SZ, fmt, ap2);
	if (ret >= 0 && ret < INIT_SZ) { /* succeeded with initial alloc */
		*str = string;
	} else if (ret == INT_MAX || ret < 0) { /* Bad length */
		goto fail;
	} else {        /* bigger than initial, realloc allowing for nul */
		len = (size_t)ret + 1;
		if ((newstr = (char*)realloc(string, len)) == NULL) {
			free(string);
			goto fail;
		} else {
			va_end(ap2);
			VA_COPY(ap2, ap);
			ret = vsnprintf(newstr, len, fmt, ap2);
			if (ret >= 0 && (size_t)ret < len) {
				*str = newstr;
			} else { /* failed with realloc'ed string, give up */
				free(newstr);
				goto fail;
			}
		}
	}
	va_end(ap2);
	return (ret);

fail:
	*str = NULL;
	errno = ENOMEM;
	va_end(ap2);
	return (-1);
}

int asprintf(char **str, const char *fmt, ...)
{
	va_list ap;
	int ret;

	*str = NULL;
	va_start(ap, fmt);
	ret = vasprintf(str, fmt, ap);
	va_end(ap);

	return ret;
}
#endif

}
}