//
// Tile.cpp
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

#include "Tile.h"
#include "City.h"
#include "Client.h"

namespace spitfire {
namespace server {

Tile::Tile(void)
{
	m_city = 0;
	m_castleicon = 0;
	m_castleid = -1;
	m_id = -1;
	m_npc = false;
	m_ownerid = -1;
	m_powerlevel = -1;
	m_state = -1;
	m_type = FLAT;
	m_status = -1;
	m_zoneid = -1;
	m_level = -1;
	/*	x = y = -1;*/
}


Tile::~Tile(void)
{
}


amf3object * Tile::ToObject()
{
	PlayerCity * city = (PlayerCity*)m_city;
	amf3object * obj2 = new amf3object();
	amf3object & obj = *obj2;
	obj["id"] = m_id;
	obj["name"] = city->m_cityname;
	obj["npc"] = m_npc;
	obj["prestige"] = city->m_client->m_prestige;
	obj["honor"] = city->m_client->m_honor;
	obj["state"] = city->m_client->m_status;
	obj["userName"] = city->m_client->m_playername;
	obj["flag"] = city->m_client->m_flag;
	obj["allianceName"] = city->m_client->m_alliancename;
	return obj2;
}

}
}