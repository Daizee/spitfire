//
// Map.cpp
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

#include "Map.h"
#include "Client.h"
#include "City.h"
#include "server.hpp"


namespace spitfire {
namespace server {

extern int DEF_MAPSIZE;

extern char itoh(int num);

Map::Map(server * sptr)
{
	m_main = sptr;
	m_tile = new Tile[DEF_MAPSIZE*DEF_MAPSIZE];
	for (int y = 0; y < DEF_MAPSIZE; ++y)
	{
		for (int x = 0; x < DEF_MAPSIZE; ++x)
		{
			// 			m_tile[y*DEF_MAPSIZE+x].x = x;
			// 			m_tile[y*DEF_MAPSIZE+x].y = y;
			GETIDFROMXY(x,y);
			m_tile[y*DEF_MAPSIZE+x].m_id = GETID;
			m_tile[y*DEF_MAPSIZE+x].m_zoneid = GetStateFromXY(x,y);
		}
	}
	memset(&m_openflats, 0, sizeof(m_openflats));
	memset(&m_totalflats, 0, sizeof(m_totalflats));
	memset(&m_cities, 0, sizeof(m_cities));
	memset(&m_occupiedtiles, 0, sizeof(m_occupiedtiles));
	memset(&m_occupiabletiles, 0, sizeof(m_occupiabletiles));
	memset(&m_npcs, 0, sizeof(m_npcs));
	memset(&m_stats, 0, sizeof(m_stats));

	states = new string[16];

	states[0] = DEF_STATE1;
	states[1] = DEF_STATE2;
	states[2] = DEF_STATE3;
	states[3] = DEF_STATE4;
	states[4] = DEF_STATE5;
	states[5] = DEF_STATE6;
	states[6] = DEF_STATE7;
	states[7] = DEF_STATE8;
	states[8] = DEF_STATE9;
	states[9] = DEF_STATE10;
	states[10] = DEF_STATE11;
	states[11] = DEF_STATE12;
	states[12] = DEF_STATE13;
	states[13] = DEF_STATE14;
	states[14] = DEF_STATE15;
	states[15] = DEF_STATE16;

	//CalculateOpenTiles();
}


Map::~Map(void)
{
	delete[] m_tile;
	delete[] states;
}

bool Map::AddCity(int id, City * city)
{
	return true;
}


void Map::CalculateOpenTiles()
{
	int tempstate = 0;
	Tile * tile;
	memset(&m_openflats, 0, sizeof(m_openflats));
	memset(&m_totalflats, 0, sizeof(m_totalflats));
	memset(&m_cities, 0, sizeof(m_cities));
	memset(&m_occupiedtiles, 0, sizeof(m_occupiedtiles));
	memset(&m_occupiabletiles, 0, sizeof(m_occupiabletiles));
	memset(&m_npcs, 0, sizeof(m_npcs));
	for (int i = 0; i < DEF_STATES; ++i)
		m_openflatlist[i].clear();
	memset(&m_stats, 0, sizeof(m_stats));
	for (int y = 0; y < DEF_MAPSIZE; ++y)
	{
		for (int x = 0; x < DEF_MAPSIZE; ++x)
		{
			tempstate = GetStateFromXY(x,y);


			tile = &(m_tile[y * DEF_MAPSIZE + x]);

			if (tile->m_type >= FLAT)
			{
				m_occupiabletiles[tempstate]++;
				if (tile->m_type == FLAT)
				{
					m_totalflats[tempstate]++;
					if (tile->m_ownerid == -1)
					{
						m_openflats[tempstate]++;

						m_openflatlist[tempstate].push_back(y * DEF_MAPSIZE + x);
					}
					else
					{
						m_occupiedtiles[tempstate]++;
					}
				}
				else
				{
					m_occupiedtiles[tempstate]++;

					if (tile->m_type == CASTLE)
					{
						m_cities[tempstate]++;
					}
					else
					{
						m_npcs[tempstate]++;
					}
				}
			}
		}
	}
	for (int i = 0; i < DEF_STATES; ++i)
	{
		m_stats[i].numbercities = m_cities[i] + m_npcs[i];
		m_stats[i].playerrate = int((float(m_occupiedtiles[i]) / float(m_occupiabletiles[i])) * 100);
		m_stats[i].players = m_cities[i];
	}
}

int Map::GetRandomOpenTile(int zone)
{
	if (m_openflats[zone] == 0 || m_openflatlist[zone].size() == 0)
		return -1;
	int index = rand()%m_openflatlist[zone].size();
	int randomid = m_openflatlist[zone].at(index);
	vector<int32_t>::iterator iter;
	iter = m_openflatlist[zone].begin();
	iter += index;
	m_openflatlist[zone].erase(iter);
	return randomid;
}

int Map::GetStateFromXY(int x, int y)
{
	if (y >= 0 && y < DEF_MAPSIZE*0.25)
	{
		if (x >= 0 && x < DEF_MAPSIZE*0.25)
		{
			return 0;
		}
		else if (x >= DEF_MAPSIZE*0.25 && x < DEF_MAPSIZE*0.5)
		{
			return 1;
		}
		else if (x >= DEF_MAPSIZE*0.5 && x < DEF_MAPSIZE*0.75)
		{
			return 2;
		}
		else if (x >= DEF_MAPSIZE*0.75 && x < DEF_MAPSIZE)
		{
			return 3;
		}
	}
	else if (y >= DEF_MAPSIZE*0.25 && y < DEF_MAPSIZE*0.5)
	{
		if (x >= 0 && x < DEF_MAPSIZE*0.25)
		{
			return 4;
		}
		else if (x >= DEF_MAPSIZE*0.25 && x < DEF_MAPSIZE*0.5)
		{
			return 5;
		}
		else if (x >= DEF_MAPSIZE*0.5 && x < DEF_MAPSIZE*0.75)
		{
			return 6;
		}
		else if (x >= DEF_MAPSIZE*0.75 && x < DEF_MAPSIZE)
		{
			return 7;
		}
	}
	else if (y >= DEF_MAPSIZE*0.5 && y < DEF_MAPSIZE*0.75)
	{
		if (x >= 0 && x < DEF_MAPSIZE*0.25)
		{
			return 8;
		}
		else if (x >= DEF_MAPSIZE*0.25 && x < DEF_MAPSIZE*0.5)
		{
			return 9;
		}
		else if (x >= DEF_MAPSIZE*0.5 && x < DEF_MAPSIZE*0.75)
		{
			return 10;
		}
		else if (x >= DEF_MAPSIZE*0.75 && x < DEF_MAPSIZE)
		{
			return 11;
		}
	}
	else if (y >= DEF_MAPSIZE*0.75 && y < DEF_MAPSIZE)
	{
		if (x >= 0 && x < DEF_MAPSIZE*0.25)
		{
			return 12;
		}
		else if (x >= DEF_MAPSIZE*0.25 && x < DEF_MAPSIZE*0.5)
		{
			return 13;
		}
		else if (x >= DEF_MAPSIZE*0.5 && x < DEF_MAPSIZE*0.75)
		{
			return 14;
		}
		else if (x >= DEF_MAPSIZE*0.75 && x < DEF_MAPSIZE)
		{
			return 15;
		}
	}
	return -1;
}

int Map::GetStateFromID(int id)
{
	GETXYFROMID(id);
	return GetStateFromXY(GETX, GETY);
}

amf3object Map::GetTileRangeObject(int32_t clientid, int x1, int x2, int y1, int y2)
{
	amf3object data = amf3object();

	amf3array castles;

	string mapStr = "";

	data["x1"] = x1;
	data["x2"] = x2;
	data["y1"] = y1;
	data["y2"] = y2;

	if ((abs(x1) >= DEF_MAPSIZE) && (abs(x2) >= DEF_MAPSIZE))
	{
		x1 = x1%DEF_MAPSIZE;
		x2 = x2%DEF_MAPSIZE;
	}
	else if (abs(x2) >= DEF_MAPSIZE)
	{
		x1 -= DEF_MAPSIZE;
		x2 = x2%DEF_MAPSIZE;
	}
	if ((abs(y1) >= DEF_MAPSIZE) && (abs(y2) >= DEF_MAPSIZE))
	{
		y1 = y1%DEF_MAPSIZE;
		y2 = y2%DEF_MAPSIZE;
	}
	else if (abs(y2) >= DEF_MAPSIZE)
	{
		y1 -= DEF_MAPSIZE;
		y2 = y2%DEF_MAPSIZE;
	}

	Log("x1 %d x2 %d y1 %d y2 %d", x1, x2, y1, y2);

	if (x1 < 0 || y1 < 0)
	{
		int xdiff = 0;
		int ydiff = 0;
		if (x1 < 0)
		{
			if (x2 < 0)
			{
				xdiff = x2-x1;
			}
			else
			{
				xdiff = abs(x1)+x2;
			}
		}
		else
		{
			xdiff = x2 - x1;
		}
		if (y1 < 0)
		{
			if (y2 < 0)
			{
				ydiff = y2-y1;
			}
			else
			{
				ydiff = abs(y1)+y2;
			}
		}
		else
		{
			ydiff = y2 - y1;
		}
		if (xdiff > 20 || ydiff > 20 || xdiff <= 0 || ydiff <= 0)
		{
			data["ok"] = -1;
			data["errorMsg"] = "Out of range.";
			data["packageId"] = 0.0f;

			return data;
		}
		Log("xdiff %d ydiff %d", xdiff, ydiff);

		//valid request 20x20 max

		for (int y = y1; y <= y1+ydiff; ++y)
		{
			for (int x = x1; x <= x1+xdiff; ++x)
			{
				x1 = x1%DEF_MAPSIZE;
				x2 = x2%DEF_MAPSIZE;

				int idfromxy = y*DEF_MAPSIZE+x;
				if ((x < 0) && (y < 0))
				{
					idfromxy = (y+DEF_MAPSIZE)*DEF_MAPSIZE+(x+DEF_MAPSIZE);
				}
				else if (x < 0)
				{
					idfromxy = y*DEF_MAPSIZE+(x+DEF_MAPSIZE);
				}
				else if (y < 0)
				{
					idfromxy = (y+DEF_MAPSIZE)*DEF_MAPSIZE+x;
				}
				else if ((x > DEF_MAPSIZE) && (y > DEF_MAPSIZE))
				{
					idfromxy = (y%DEF_MAPSIZE)*DEF_MAPSIZE+((x%DEF_MAPSIZE)+DEF_MAPSIZE);
				}
				else if (x > DEF_MAPSIZE)
				{
					idfromxy = y*DEF_MAPSIZE+((x%DEF_MAPSIZE)+DEF_MAPSIZE);
				}
				else if (y > DEF_MAPSIZE)
				{
					idfromxy = ((y%DEF_MAPSIZE)+DEF_MAPSIZE)*DEF_MAPSIZE+x;
				}


				if (m_tile[idfromxy].m_type > 10)
				{
					amf3object castleobject = amf3object();
					castleobject["id"] = m_tile[idfromxy].m_id;
					castleobject["name"] = m_tile[idfromxy].m_city->m_cityname.c_str();
					castleobject["state"] = m_tile[idfromxy].m_city->m_status;
					if (m_tile[idfromxy].m_npc)
					{
						castleobject["npc"] = true;
					}
					else
					{
						Client * client = ((PlayerCity*)m_tile[idfromxy].m_city)->m_client;
						castleobject["prestige"] = client->m_prestige;
						castleobject["honor"] = client->m_honor;
						castleobject["userName"] = client->m_playername.c_str();
						castleobject["flag"] = client->m_flag.c_str();

						if (client->m_allianceid > 0)
							castleobject["allianceName"] = client->m_alliancename.c_str();

						int relation = m_main->m_alliances->GetRelation(clientid, m_tile[idfromxy].m_ownerid);
						switch (relation)
						{
						case DEF_SELFRELATION:
						case DEF_ALLY:
						case DEF_ALLIANCE:
							castleobject["canLoot"] = false;
							castleobject["canOccupy"] = false;
							castleobject["canScout"] = false;
							castleobject["canSend"] = true;
							castleobject["canTrans"] = true;
							break;
						case DEF_ENEMY:
							castleobject["canLoot"] = true;
							castleobject["canOccupy"] = true;
							castleobject["canScout"] = true;
							castleobject["canSend"] = true;
							castleobject["canTrans"] = true;
							break;
						case DEF_NEUTRAL:
						case DEF_NORELATION:
						default:
							castleobject["canLoot"] = true;
							castleobject["canOccupy"] = true;
							castleobject["canScout"] = true;
							castleobject["canSend"] = false;
							castleobject["canTrans"] = false;
							break;
						}

						castleobject["playerLogoUrl"] = client->m_faceurl;

						castleobject["relation"] = relation;
						castleobject["state"] = client->m_status;
						castleobject["userName"] = client->m_playername;
						castleobject["zoneName"] = states[GetStateFromID(idfromxy)];
						castleobject["furlough"] = client->m_beginner;
						castleobject["npc"] = false;
					}
					castles.Add(castleobject);

					mapStr += (char)itoh(m_tile[idfromxy].m_type);
					mapStr += (char)itoh(m_tile[idfromxy].m_city->m_level);
				}
				else
				{
					mapStr += (char)itoh(m_tile[idfromxy].m_type);
					mapStr += (char)itoh(m_tile[idfromxy].m_level);
				}
			}
		}
	}
	else
	{

		if ((x2 - x1) > 20 || (y2 - y1) > 20)
		{
			data["ok"] = -1;
			data["errorMsg"] = "Out of range.";
			data["packageId"] = 0.0f;

			return data;
		}


		for (int y = y1; y <= y2; ++y)
		{
			for (int x = x1; x <= x2; ++x)
			{
				GETIDFROMXY(x,y);
				if (m_tile[idfromxy].m_type > 10)
				{
					amf3object castleobject = amf3object();
					castleobject["id"] = m_tile[idfromxy].m_id;
					castleobject["name"] = m_tile[idfromxy].m_city->m_cityname.c_str();
					castleobject["state"] = m_tile[idfromxy].m_city->m_status;
					if (m_tile[idfromxy].m_npc)
					{
						castleobject["npc"] = true;
					}
					else
					{
						Client * client = ((PlayerCity*)m_tile[idfromxy].m_city)->m_client;
						castleobject["prestige"] = client->m_prestige;
						castleobject["honor"] = client->m_honor;
						castleobject["userName"] = client->m_playername.c_str();
						castleobject["flag"] = client->m_flag.c_str();

						if (client->m_allianceid > 0)
							castleobject["allianceName"] = client->m_alliancename.c_str();

						int relation = m_main->m_alliances->GetRelation(clientid, m_tile[idfromxy].m_ownerid);
						switch (relation)
						{
						case DEF_SELFRELATION:
						case DEF_ALLY:
						case DEF_ALLIANCE:
							castleobject["canLoot"] = false;
							castleobject["canOccupy"] = false;
							castleobject["canScout"] = false;
							castleobject["canSend"] = true;
							castleobject["canTrans"] = true;
							break;
						case DEF_ENEMY:
							castleobject["canLoot"] = true;
							castleobject["canOccupy"] = true;
							castleobject["canScout"] = true;
							castleobject["canSend"] = true;
							castleobject["canTrans"] = true;
							break;
						case DEF_NEUTRAL:
						case DEF_NORELATION:
						default:
							castleobject["canLoot"] = true;
							castleobject["canOccupy"] = true;
							castleobject["canScout"] = true;
							castleobject["canSend"] = false;
							castleobject["canTrans"] = false;
							break;
						}

						castleobject["playerLogoUrl"] = client->m_faceurl;

						castleobject["relation"] = relation;
						castleobject["state"] = client->m_status;
						castleobject["userName"] = client->m_playername;
						castleobject["zoneName"] = states[GetStateFromID(idfromxy)];
						castleobject["furlough"] = client->m_beginner;
						castleobject["npc"] = false;
					}
					castles.Add(castleobject);

					mapStr += (char)itoh(m_tile[idfromxy].m_type);
					mapStr += (char)itoh(m_tile[idfromxy].m_city->m_level);
				}
				else
				{
					mapStr += (char)itoh(m_tile[idfromxy].m_type);
					mapStr += (char)itoh(m_tile[idfromxy].m_level);
				}
			}
		}
	}


	data["castles"] = castles;


	data["mapStr"] = mapStr.c_str();

	data["ok"] = 1;
	data["packageId"] = 0.0f;

	return data;
}

amf3object Map::GetMapCastle(int32_t fieldid, int32_t clientid)
{
	amf3object field;

	Tile * tile = &this->m_tile[fieldid];

	if (tile->m_ownerid > 0)
	{
		Client * client = m_main->GetClient(tile->m_ownerid);
		if (tile->m_type < 11)
		{
			field["allianceName"] = client->m_alliancename;
			field["flag"] = client->m_flag;
			field["honor"] = client->m_honor;
			field["id"] = fieldid;
			//field["name"] = tile->m_city->m_cityname;
			field["prestige"] = client->m_prestige;
			int relation = m_main->m_alliances->GetRelation(clientid, client->m_clientnumber);
			switch (relation)
			{
			case DEF_SELFRELATION:
			case DEF_ALLY:
				field["canLoot"] = false;
				field["canOccupy"] = false;
				field["canScout"] = false;
				field["canSend"] = true;
				field["canTrans"] = true;
				break;
			case DEF_ENEMY:
				field["canLoot"] = true;
				field["canOccupy"] = true;
				field["canScout"] = true;
				field["canSend"] = true;
				field["canTrans"] = true;
				break;
			case DEF_NEUTRAL:
			case DEF_NORELATION:
			default:
				field["canLoot"] = true;
				field["canOccupy"] = true;
				field["canScout"] = true;
				field["canSend"] = false;
				field["canTrans"] = false;
				break;
			}
			field["relation"] = relation;
			field["state"] = client->m_status;
			field["userName"] = client->m_playername;
			field["zoneName"] = states[GetStateFromID(fieldid)];
			field["furlough"] = client->m_beginner;
			field["npc"] = false;
		}
		else
		{
			field["allianceName"] = client->m_alliancename;
			field["flag"] = client->m_flag;
			field["honor"] = client->m_honor;
			field["id"] = fieldid;
			field["name"] = tile->m_city->m_cityname;
			field["playerLogoUrl"] = client->m_faceurl;
			field["prestige"] = client->m_prestige;
			int relation = m_main->m_alliances->GetRelation(clientid, client->m_clientnumber);
			switch (relation)
			{
			case DEF_SELFRELATION:
			case DEF_ALLY:
				field["canLoot"] = false;
				field["canOccupy"] = false;
				field["canScout"] = false;
				field["canSend"] = true;
				field["canTrans"] = true;
				break;
			case DEF_ENEMY:
				field["canLoot"] = true;
				field["canOccupy"] = true;
				field["canScout"] = true;
				field["canSend"] = true;
				field["canTrans"] = true;
				break;
			case DEF_NEUTRAL:
			case DEF_NORELATION:
			default:
				field["canLoot"] = true;
				field["canOccupy"] = true;
				field["canScout"] = true;
				field["canSend"] = false;
				field["canTrans"] = false;
				break;
			}
			field["relation"] = relation;
			field["state"] = client->m_status;
			field["userName"] = client->m_playername;
			field["zoneName"] = states[GetStateFromID(fieldid)];
			field["furlough"] = client->m_beginner;
			field["npc"] = false;
		}
	}
	else if (tile->m_npc)
	{
		field["allianceName"] = "";
		field["npc"] = true;
		field["canLoot"] = true;
		field["canOccupy"] = true;
		field["canScout"] = true;
		field["canSend"] = false;
		field["canTrans"] = false;
		field["zoneName"] = states[GetStateFromID(fieldid)];
		field["id"] = fieldid;
		field["name"] = tile->m_city->m_cityname;
	}
	else
	{
		field["allianceName"] = "";
		field["npc"] = false;
		field["canLoot"] = true;
		field["canOccupy"] = true;
		field["canScout"] = true;
		field["canSend"] = false;
		field["canTrans"] = false;
		field["zoneName"] = states[GetStateFromID(fieldid)];
		field["id"] = fieldid;
	}

	return field;
}

}
}