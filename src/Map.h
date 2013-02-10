//
// Map.h
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

#include "Tile.h"
#include "funcs.h"
#include "amf3.h"

namespace spitfire {
namespace server {

class City;
class server;

class Map
{
public:
	Map(server * sptr);
	~Map(void);

	server * m_main;
	string * states;

	void CalculateOpenTiles();
	int GetStateFromXY(int x, int y);
	int GetStateFromID(int id);
	int GetRandomOpenTile(int zone);
	amf3object GetTileRangeObject(int32_t clientid, int x1, int x2, int y1, int y2);
	amf3object GetMapCastle(int32_t fieldid, int32_t clientid);

	bool AddCity(int id, City * city);

	Tile * m_tile;
	int32_t m_totalflats[DEF_STATES];
	int32_t m_openflats[DEF_STATES];
	int32_t m_npcs[DEF_STATES];
	int32_t m_cities[DEF_STATES];
	int32_t m_occupiedtiles[DEF_STATES];
	int32_t m_occupiabletiles[DEF_STATES];
	struct mapstats
	{
		int players;
		int numbercities;
		int playerrate;
	} m_stats[DEF_STATES];

	vector<int32_t> m_openflatlist[DEF_STATES];
};


}
}