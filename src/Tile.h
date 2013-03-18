//
// Tile.h
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

namespace spitfire {
namespace server {

using namespace std;

class City;

class Tile
{
public:
	Tile(void);
	~Tile(void);


	amf3object ToObject();
	string GetName();

	City * m_city;
	char m_castleicon;
	int m_castleid;
	int m_id;
	bool m_npc;
	int m_ownerid;
	char m_powerlevel;
	char m_state;
	char m_type;
	char m_status;
	char m_zoneid;
	char m_level;
/*	short x, y;*/

	/*
	["canColonize"] Type: Boolean - Value: True
	["canDeclaredWar"] Type: Boolean - Value: False
	["canLoot"] Type: Boolean - Value: False
	["canOccupy"] Type: Boolean - Value: True
	["canScout"] Type: Boolean - Value: True
	["canSend"] Type: Boolean - Value: False
	["canTrans"] Type: Boolean - Value: False
	["castleIcon"] Type: Integer - Value: 0
	["castleId"] Type: Integer - Value: 0
	["colonialRelation"] Type: Integer - Value: 0
	["colonialStatus"] Type: Integer - Value: 0
	["declaredWarStartTime"] Type: Number - Value: 0.000000
	["declaredWarStatus"] Type: Integer - Value: 0
	["freshMan"] Type: Boolean - Value: False
	["furlough"] Type: Boolean - Value: False
	["honor"] Type: Integer - Value: 0
	["id"] Type: Integer - Value: 323903
	["name"] Type: String - Value: Barbarian's city
	["npc"] Type: Boolean - Value: True
	["ownerPlayerId"] Type: Integer - Value: 0
	["powerLevel"] Type: Integer - Value: 4
	["prestige"] Type: Integer - Value: 0
	["relation"] Type: Integer - Value: 0
	["state"] Type: Integer - Value: 1
	["type"] Type: Integer - Value: 12
	["zoneName"] Type: String - Value: CARINTHIA*/
};

}
}