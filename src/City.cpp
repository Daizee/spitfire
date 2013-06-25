//
// City.cpp
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

#include "City.h"
#include "Client.h"
#include "server.hpp"

namespace spitfire {
namespace server {

extern uint64_t unixtime();

City::City()
{
	m_cityname = "";
	m_status = 0;
	m_tileid = 0;
	m_type = 0;
	m_level = 0;
	m_loyalty = 0;
	m_grievance = 0;
	memset(&m_resources, 0, sizeof(m_resources));
	memset(&m_forts, 0, sizeof(m_forts));
	memset(&m_outerbuildings, 0, sizeof(m_outerbuildings));
	memset(&m_innerbuildings, 0, sizeof(m_innerbuildings));
}


City::~City(void)
{
}

bool City::SetBuilding(int16_t type, int16_t level, int16_t position, int16_t status, double starttime, double endtime)
{
	if (position > 1000)
	{
		if (position-1000 > 40)
		{
			if (this->m_type == NPC)
				Log("Problem setting out of range NPC building id : %d - tileid: %d", position, m_tileid);
			else if (this->m_type == CASTLE)
				Log("Problem setting out of range PLAYER building id : %d - castleid: %d - accountid: %d", position, ((PlayerCity*)this)->m_castleid, ((PlayerCity*)this)->m_accountid);
			return false;
		}
		m_outerbuildings[position-1000].id = position;
		m_outerbuildings[position-1000].type = type;
		m_outerbuildings[position-1000].level = level;
		m_outerbuildings[position-1000].status = status;
		m_outerbuildings[position-1000].starttime = starttime;
		m_outerbuildings[position-1000].endtime = endtime;
		return true;
	}
	else
	{
		if ((position < -2) || (position > 33))
		{
			if (this->m_type == NPC)
				Log("Problem setting out of range NPC building id : %d - tileid: %d", position, m_tileid);
			else if (this->m_type == CASTLE)
				Log("Problem setting out of range PLAYER building id : %d - castleid: %d - accountid: %d", position, ((PlayerCity*)this)->m_castleid, ((PlayerCity*)this)->m_accountid);
			return false;
		}


		if ((this->m_type == CASTLE) && ((type == B_BARRACKS) || (type == B_WALLS)))
		{
			if ((level > 0) && (((PlayerCity*)this)->GetBarracksQueue(position) == 0))
			{
				stTroopQueue queue;
				queue.nextqueueid = 0;
				queue.positionid = position;
				queue.status = 0;
				((PlayerCity*)this)->m_troopqueue.push_back(queue);
			}
			else if ((level == 0) && (((PlayerCity*)this)->GetBarracksQueue(position) != 0))
			{
				vector<stTroopQueue>::iterator iter;
				if (((PlayerCity*)this)->m_troopqueue.size() > 0)
				{
					for (iter = ((PlayerCity*)this)->m_troopqueue.begin(); iter != ((PlayerCity*)this)->m_troopqueue.end(); ++iter)
					{
						if (iter->positionid == position)
							((PlayerCity*)this)->m_troopqueue.erase(iter++);
					}
				}
			}
		}
		if ((type == B_TOWNHALL) && ((this->m_type == CASTLE) || (this->m_type == NPC)))
		{
			m_level = level;
		}

		position += 2;

		m_innerbuildings[position].id = position-2;
		m_innerbuildings[position].type = type;
		m_innerbuildings[position].level = level;
		m_innerbuildings[position].status = status;
		m_innerbuildings[position].starttime = starttime;
		m_innerbuildings[position].endtime = endtime;
		return true;
	}
	return false;
}

PlayerCity::PlayerCity()
{
	m_client = 0;
	m_castleid = 0;
	m_accountid = 0;
	memset(&m_troops, 0, sizeof(m_troops));
	memset(&m_injuredtroops, 0, sizeof(m_injuredtroops));
	memset(&m_maxresources, 0, sizeof(m_maxresources));
	memset(&m_production, 0, sizeof(m_production));
	memset(&m_workpopulation, 0, sizeof(m_workpopulation));
	memset(&m_workrate, 0, sizeof(m_workrate));
	memset(&m_storepercent, 0, sizeof(m_storepercent));

	m_population = 0;
	m_maxpopulation = 50;
	m_allowalliance = false;
	m_gooutforbattle = false;
	m_hasenemy = false;
	m_creation = 0;
	m_troopconsume = 0;
	m_productionefficiency = 0;

	m_logurl = "images/icon/cityLogo/citylogo_01.png";

	for (int i = 0; i < 10; ++i)
	{
		m_heroes[i] = 0;
		m_innheroes[i] = 0;
	}

	m_mayor = 0;
	m_lastcomfort = 0;
	m_lastlevy = 0;
	m_researching = false;
}

PlayerCity::~PlayerCity(void)
{
}

NpcCity::NpcCity()
{
	m_calculatestuff = unixtime();
	m_ownerid = 0;
	memset(&m_troops, 0, sizeof(m_troops));
	m_temphero.m_name = "Test NPC Hero";
}

void City::SetResources(int64_t food, int64_t wood, int64_t stone, int64_t iron, int64_t gold)
{
	m_resources.food  = food;
	m_resources.gold  = gold;
	m_resources.wood  = wood;
	m_resources.iron  = iron;
	m_resources.stone = stone;
}

void PlayerCity::SetMaxResources(int64_t food, int64_t wood, int64_t stone, int64_t iron)
{
	m_maxresources.food  = food;
	m_maxresources.wood  = wood;
	m_maxresources.iron  = iron;
	m_maxresources.stone = stone;
}

void NpcCity::SetTroops(int32_t warrior, int32_t pike, int32_t sword, int32_t archer, int32_t cavalry)
{
	m_troops.warrior = warrior;
	m_troops.pike = pike;
	m_troops.sword = sword;
	m_troops.archer = archer;
	m_troops.cavalry = cavalry;
}

void City::SetForts(int32_t traps, int32_t abatis, int32_t towers, int32_t logs, int32_t trebs)
{
	m_forts.traps = traps;
	m_forts.abatis = abatis;
	m_forts.towers = towers;
	m_forts.logs = logs;
	m_forts.trebs = trebs;
}

void PlayerCity::SetForts(int32_t type, int32_t count)
{
	switch (type)
	{
	case TR_TRAP:
		m_forts.traps += count;
		break;
	case TR_ABATIS:
		m_forts.abatis += count;
		break;
	case TR_ARCHERTOWER:
		m_forts.towers += count;
		break;
	case TR_ROLLINGLOG:
		m_forts.logs += count;
		break;
	case TR_TREBUCHET:
		m_forts.trebs += count;
		break;
	}
	FortUpdate();
}

int32_t City::GetForts(int32_t type)
{
	switch (type)
	{
	case TR_TRAP:
		return m_forts.traps;
		break;
	case TR_ABATIS:
		return m_forts.abatis;
		break;
	case TR_ARCHERTOWER:
		return m_forts.towers;
		break;
	case TR_ROLLINGLOG:
		return m_forts.logs;
		break;
	case TR_TREBUCHET:
		return m_forts.trebs;
		break;
	}
	return 0;
}

void NpcCity::SetupBuildings()
{
	SetBuilding(4, m_level, 1011+((m_level-1)*3), 0, 0, 0);
	SetBuilding(5, m_level, 1012+((m_level-1)*3), 0, 0, 0);
	SetBuilding(6, m_level, 1013+((m_level-1)*3), 0, 0, 0);
	for (int i = 0; i < 10 + (m_level-1)*3; ++i)
	{
		SetBuilding(7, m_level, 1001+i, 0, 0, 0);
	}
	SetBuilding(25, m_level, 0, 0, 0, 0);
	SetBuilding(22, m_level, 1, 0, 0, 0);
	SetBuilding(28, m_level, 2, 0, 0, 0);
	SetBuilding(27, m_level, 3, 0, 0, 0);
	SetBuilding(23, m_level, 4, 0, 0, 0);
	SetBuilding(30, m_level, 5, 0, 0, 0);
	SetBuilding(20, m_level, 6, 0, 0, 0);

	SetBuilding(21, m_level, 7, 0, 0, 0);
	SetBuilding(29, m_level, 8, 0, 0, 0);
	SetBuilding(24, m_level, 9, 0, 0, 0);
	SetBuilding(26, m_level, 10, 0, 0, 0);
	SetBuilding(2, m_level, 11, 0, 0, 0);
	for (int i = 12; i < 32; ++i)
	{
		SetBuilding(1, m_level, i, 0, 0, 0);
	}

	SetBuilding(31, m_level, -1, 0, 0, 0);
	SetBuilding(32, m_level, -2, 0, 0, 0);
}

void NpcCity::Initialize(bool resources, bool troops)
{
	switch (m_level)
	{
	case 1:
		SetResources(55000, 100000, 20000, 20000, 20000);
		SetTroops(50, 40, 30, 10, 8);
		SetForts(1000, 0, 0, 0, 0);
		SetupBuildings();
		break;
	case 2:
		SetResources(65000, 200000, 30000, 30000, 30000);
		SetTroops(50, 45, 40, 30, 25);
		SetForts(1850, 550, 0, 0, 0);
		SetupBuildings();
		break;
	case 3:
		SetResources(75000, 900000, 75000, 75000, 75000);
		SetTroops(200, 160, 65, 40, 60);
		SetForts(2000, 1000, 650, 0, 0);
		SetupBuildings();
		break;
	case 4:
		SetResources(300000, 1600000, 120000, 120000, 120000);
		SetTroops(400, 400, 100, 100, 150);
		SetForts(4500, 1875, 550, 0, 0);
		SetupBuildings();
		break;
	case 5:
		SetResources(450000, 3000000, 180000, 180000, 180000);
		SetTroops(750, 1000, 350, 250, 200);
		SetForts(3750, 1875, 1250, 750, 0);
		SetupBuildings();
		break;
	case 6:
		SetResources(600000, 4000000, 200000, 200000, 200000);
		SetTroops(4000, 1750, 550, 500, 450);
		SetForts(4250, 1500, 1500, 950, 400);
		SetupBuildings();
		break;
	case 7:
		SetResources(800000, 4500000, 500000, 500000, 500000);
		SetTroops(12000, 3000, 750, 800, 750);
		SetForts(5600, 2800, 1850, 1100, 700);
		SetupBuildings();
		break;
	case 8:
		SetResources(1000000, 8000000, 800000, 800000, 800000);
		SetTroops(15000, 6750, 4000, 3000, 2000);
		SetForts(7200, 3600, 2400, 1440, 900);
		SetupBuildings();
		break;
	case 9:
		SetResources(1200000, 14000000, 550000, 550000, 550000);
		SetTroops(60000, 18000, 2000, 6750, 2500);
		SetForts(9000, 4500, 3000, 1800, 1125);
		SetupBuildings();
		break;
	case 10:
		SetResources(1500000, 19000000, 600000, 600000, 600000);
		SetTroops(400000, 0, 0, 0, 0);
		SetForts(11000, 5500, 3666, 2200, 1375);
		SetupBuildings();
		break;
	}
	memcpy(&m_maxresources, &m_resources, sizeof(stResources));
	memcpy(&m_maxtroops, &m_troops, sizeof(stTroops));
	memcpy(&m_maxforts, &m_forts, sizeof(stForts));
}

void NpcCity::CalculateStats(bool resources, bool troops)
{
	int64_t add;
	if (resources)
	{
		add = m_maxresources.food/80;
		if (m_maxresources.food > (m_resources.food + add))
			m_resources.food += add;
		else
			m_resources.food = m_maxresources.food;

		add = m_maxresources.wood/80;
		if (m_maxresources.wood > (m_resources.wood + add))
			m_resources.wood += add;
		else
			m_resources.wood = m_maxresources.wood;

		add = m_maxresources.stone/80;
		if (m_maxresources.stone > (m_resources.stone + add))
			m_resources.stone += add;
		else
			m_resources.stone = m_maxresources.stone;

		add = m_maxresources.iron/80;
		if (m_maxresources.iron > (m_resources.iron + add))
			m_resources.iron += add;
		else
			m_resources.iron = m_maxresources.iron;

		add = m_maxresources.gold/80;
		if (m_maxresources.gold > (m_resources.gold + add))
			m_resources.gold += add;
		else
			m_resources.gold = m_maxresources.gold;
	}
	if (troops)
	{
		add = m_maxtroops.archer/10;
		if (m_maxtroops.archer > (m_troops.archer + add))
			m_troops.archer += add;
		else
			m_troops.archer = m_maxtroops.archer;

		add = m_maxtroops.cavalry/10;
		if (m_maxtroops.cavalry > (m_troops.cavalry + add))
			m_troops.cavalry += add;
		else
			m_troops.cavalry = m_maxtroops.cavalry;

		add = m_maxtroops.pike/10;
		if (m_maxtroops.pike > (m_troops.pike + add))
			m_troops.pike += add;
		else
			m_troops.pike = m_maxtroops.pike;

		add = m_maxtroops.sword/10;
		if (m_maxtroops.sword > (m_troops.sword + add))
			m_troops.sword += add;
		else
			m_troops.sword = m_maxtroops.sword;

		add = m_maxtroops.warrior/10;
		if (m_maxtroops.warrior > (m_troops.warrior + add))
			m_troops.warrior += add;
		else
			m_troops.warrior = m_maxtroops.warrior;

		add = m_maxforts.abatis/10;
		if (m_maxforts.abatis > (m_forts.abatis + add))
			m_forts.abatis += add;
		else
			m_forts.abatis = m_maxforts.abatis;

		add = m_maxforts.logs/10;
		if (m_maxforts.logs > (m_forts.logs + add))
			m_forts.logs += add;
		else
			m_forts.logs = m_maxforts.logs;

		add = m_maxforts.towers/10;
		if (m_maxforts.towers > (m_forts.towers + add))
			m_forts.towers += add;
		else
			m_forts.towers = m_maxforts.towers;

		add = m_maxforts.traps/10;
		if (m_maxforts.traps > (m_forts.traps + add))
			m_forts.traps += add;
		else
			m_forts.traps = m_maxforts.traps;

		add = m_maxforts.trebs/10;
		if (m_maxforts.trebs > (m_forts.trebs + add))
			m_forts.trebs += add;
		else
			m_forts.trebs = m_maxforts.trebs;

	}
}

NpcCity::~NpcCity(void)
{
}

amf3object PlayerCity::ToObject()
{
	amf3object obj = amf3object();
	//obj["heroes"] = HeroArray();
	obj["buildingQueues"] = amf3array();
	obj["heros"] = HeroArray();
	obj["status"] = m_status;
	obj["allowAlliance"] = false;
	obj["resource"] = Resources();
	obj["logUrl"] = m_logurl;
	obj["fieldId"] = m_tileid;
	obj["usePACIFY_SUCCOUR_OR_PACIFY_PRAY"] = 1;//Unknown value (was set to 1)
	obj["transingTrades"] = amf3array();
	obj["troop"] = Troops();
	obj["id"] = m_castleid;
	obj["name"] = m_cityname;
	obj["buildings"] = Buildings();
	obj["fortification"] = Fortifications();
	obj["trades"] = amf3array();
	obj["fields"] = amf3array();//tiles owned?
	obj["goOutForBattle"] = false;
	obj["hasEnemy"] = false;
	return obj;
}

amf3array PlayerCity::HeroArray()
{
	amf3array array = amf3array();
	for (int i = 0; i < 10; ++i)
	{
		if (m_heroes[i])
		{
			amf3object temphero = m_heroes[i]->ToObject();
			array.Add(temphero);
		}
	}
	return array;
}

amf3object PlayerCity::Resources()
{
	amf3object obj = amf3object();
	obj["maxPopulation"] = m_maxpopulation;
	obj["taxIncome"] = m_production.gold;
	obj["support"] = m_loyalty;
	//obj["wood"] = amf3object();

	amf3object wood = amf3object();
	wood["amount"] = m_resources.wood;
	wood["storeRercent"] = (int)m_storepercent.wood;
	wood["workPeople"] = (int)m_workpopulation.wood;
	wood["max"] = (int)m_maxresources.wood;
	wood["increaseRate"] = (m_production.wood*(m_productionefficiency/100)) * (1 + (m_resourcemanagement/100)) + m_resourcebaseproduction;
	obj["wood"] = wood;

	amf3object stone = amf3object();
	stone["amount"] = m_resources.stone;
	stone["storeRercent"] = (int)m_storepercent.stone;
	stone["workPeople"] = (int)m_workpopulation.stone;
	stone["max"] = (int)m_maxresources.stone;
	stone["increaseRate"] = (m_production.stone*(m_productionefficiency/100)) * (1 + (m_resourcemanagement/100)) + m_resourcebaseproduction;
	obj["stone"] = stone;

	amf3object iron = amf3object();
	iron["amount"] = m_resources.iron;
	iron["storeRercent"] = (int)m_storepercent.iron;
	iron["workPeople"] = (int)m_workpopulation.iron;
	iron["max"] = (int)m_maxresources.iron;
	iron["increaseRate"] = (m_production.iron*(m_productionefficiency/100)) * (1 + (m_resourcemanagement/100)) + m_resourcebaseproduction;
	obj["iron"] = iron;

	amf3object food = amf3object();
	food["amount"] = m_resources.food;
	food["storeRercent"] = (int)m_storepercent.food;
	food["workPeople"] = (int)m_workpopulation.food;
	food["max"] = (int)m_maxresources.food;
	food["increaseRate"] = (m_production.food*(m_productionefficiency/100)) * (1 + (m_resourcemanagement/100)) - m_troopconsume + m_resourcebaseproduction;
	obj["food"] = food;

	obj["buildPeople"] = 0;// TODO: what is this
	obj["workPeople"] = int(m_workpopulation.food*m_workrate.food/100 + m_workpopulation.wood*m_workrate.wood/100 + m_workpopulation.stone*m_workrate.stone/100 + m_workpopulation.iron*m_workrate.iron/100);
	obj["curPopulation"] = m_population;
	int32_t herosalary = 0;
	for (int i = 0; i < 10; ++i)
		if (m_heroes[i])
			herosalary += m_heroes[i]->m_level * 20;
	obj["herosSalary"] = herosalary;
	obj["troopCostFood"] = m_troopconsume;
	obj["gold"] = m_resources.gold;
	obj["texRate"] = (int)m_workrate.gold;
	obj["complaint"] = m_grievance;
	int targetpopulation = ( m_maxpopulation * ( double( ( ( m_loyalty + m_grievance ) > 100 ) ? 100 : ( m_loyalty + m_grievance ) ) / 100 ) );
	//int targetpopulation = (m_maxpopulation * ((m_workrate.gold)/100));
	if (m_population > targetpopulation)
		obj["populationDirection"] = -1;
	else if (m_population < targetpopulation)
		obj["populationDirection"] = 1;
	else
		obj["populationDirection"] = 0;

	return obj;
}

amf3object PlayerCity::Troops()
{
	amf3object obj = amf3object();
	obj["peasants"] = m_troops.worker;
	obj["archer"] = m_troops.archer;
	obj["catapult"] = m_troops.catapult;
	obj["ballista"] = m_troops.ballista;
	obj["scouter"] = m_troops.scout;
	obj["carriage"] = m_troops.transporter;
	obj["heavyCavalry"] = m_troops.cataphract;
	obj["militia"] = m_troops.warrior;
	obj["lightCavalry"] = m_troops.cavalry;
	obj["swordsmen"] = m_troops.sword;
	obj["pikemen"] = m_troops.pike;
	obj["batteringRam"] = m_troops.ram;
	return obj;
}

amf3object PlayerCity::InjuredTroops()
{
	amf3object obj = amf3object();
	obj["peasants"] = m_injuredtroops.worker;
	obj["archer"] = m_injuredtroops.archer;
	obj["catapult"] = m_injuredtroops.catapult;
	obj["ballista"] = m_injuredtroops.ballista;
	obj["scouter"] = m_injuredtroops.scout;
	obj["carriage"] = m_injuredtroops.transporter;
	obj["heavyCavalry"] = m_injuredtroops.cataphract;
	obj["militia"] = m_injuredtroops.warrior;
	obj["lightCavalry"] = m_injuredtroops.cavalry;
	obj["swordsmen"] = m_injuredtroops.sword;
	obj["pikemen"] = m_injuredtroops.pike;
	obj["batteringRam"] = m_injuredtroops.ram;
	return obj;
}

amf3array PlayerCity::Buildings()
{
	amf3array array = amf3array();
	//for (int i = 0; i < m_buildings.Count; ++i)
	{
		amf3object obj = amf3object();
		//age2?
		//obj["appearance"] = 96;
		//obj["help"] = 1;

		for (int i = 0; i < 35; ++i)
		{
			if (m_innerbuildings[i].type > 0)
			{
				obj = amf3object();
				obj["startTime"] = m_innerbuildings[i].starttime;
				obj["endTime"] = m_innerbuildings[i].endtime;
				obj["level"] = m_innerbuildings[i].level;
				obj["status"] = m_innerbuildings[i].status;
				if (m_innerbuildings[i].id == 33)
					obj["positionId"] = -2;
				else if (m_innerbuildings[i].id == 32)
					obj["positionId"] = -1;
				else
					obj["positionId"] = m_innerbuildings[i].id;
				obj["name"] = GetBuildingName(m_innerbuildings[i].type);
				obj["typeId"] = m_innerbuildings[i].type;
				array.Add(obj);
			}
		}
		for (int i = 0; i <= 40; ++i)
		{
			if (m_outerbuildings[i].type > 0)
			{
				obj = amf3object();
				obj["startTime"] = m_outerbuildings[i].starttime;
				obj["endTime"] = m_outerbuildings[i].endtime;
				obj["level"] = m_outerbuildings[i].level;
				obj["status"] = m_outerbuildings[i].status;
				obj["positionId"] = m_outerbuildings[i].id;
				obj["name"] = GetBuildingName(m_outerbuildings[i].type);
				obj["typeId"] = m_outerbuildings[i].type;
				array.Add(obj);
			}
		}
	}
	return array;
}

amf3object PlayerCity::Fortifications()
{
	amf3object forts = amf3object();
	forts["trap"] = m_forts.traps;
	forts["rollingLogs"] = m_forts.logs;
	forts["rockfall"] = m_forts.trebs;
	forts["arrowTower"] = m_forts.towers;
	forts["abatis"] = m_forts.abatis;

	return forts;
}

void PlayerCity::ParseBuildings(char * str)
{
	if (str && strlen(str) > 0)
	{
		int type;
		int level;
		int position;
		int status;
		double starttime;
		double endtime;

		char * ch = 0, * cr = 0;
		char * tok;

		char * temp;
		temp = new char[strlen(str)+10];
		memset(temp, 0, strlen(str)+10);
		memcpy(temp, str, strlen(str));
		temp[strlen(str)+1] = 0;
		tok = strtok_s(temp, "|", &ch);
		do
		{
			tok = strtok_s(tok, ",", &cr);
			if (tok != 0)
				type = atoi(tok);
			tok = strtok_s(0, ",", &cr);
			if (tok != 0)
				level = atoi(tok);
			tok = strtok_s(0, ",", &cr);
			if (tok != 0)
				position = atoi(tok);
			tok = strtok_s(0, ",", &cr);
			if (tok != 0)
				status = atoi(tok);
			tok = strtok_s(0, ",", &cr);
			if (tok != 0)
				starttime = atof(tok);
			tok = strtok_s(0, ",", &cr);
			if (tok != 0)
				endtime = atof(tok);


			if (status == 1)
			{
				stBuildingAction * ba = new stBuildingAction;

				stTimedEvent te;
				ba->city = this;
				ba->client = this->m_client;
				ba->positionid = position;
				te.data = ba;
				te.type = DEF_TIMEDBUILDING;

				m_client->m_main->AddTimedEvent(te);
			}


			SetBuilding(type, level, position, status, starttime, endtime);
			tok = strtok_s(0, "|", &ch);
		} while (tok != 0);
		delete[] temp;
	}
}

void PlayerCity::CalculateStats()
{
	memset(&m_maxresources, 0, sizeof(m_maxresources));
	memset(&m_production, 0, sizeof(m_production));
	memset(&m_workpopulation, 0, sizeof(m_workpopulation));

	m_maxresources.food = 10000;
	m_maxresources.wood = 10000;
	m_maxresources.stone = 10000;
	m_maxresources.iron = 10000;
	m_maxpopulation = 50;

	for (int i = 0; i < 35; ++i)
	{
		if (m_innerbuildings[i].type > 0)
		{
			if (m_innerbuildings[i].type == 1)
			{
				switch (m_innerbuildings[i].level)
				{
				case 1:
					m_maxpopulation += 100;
					break;
				case 2:
					m_maxpopulation += 300;
					break;
				case 3:
					m_maxpopulation += 600;
					break;
				case 4:
					m_maxpopulation += 1000;
					break;
				case 5:
					m_maxpopulation += 1500;
					break;
				case 6:
					m_maxpopulation += 2100;
					break;
				case 7:
					m_maxpopulation += 2800;
					break;
				case 8:
					m_maxpopulation += 3600;
					break;
				case 9:
					m_maxpopulation += 4500;
					break;
				case 10:
					m_maxpopulation += 5500;
					break;
				}
			}
		}
	}
	for (int i = 0; i <= 40; ++i)
	{
		if (m_outerbuildings[i].type > 0)
		{
			double * pmax, * pprod, * pwork;
			if (m_outerbuildings[i].type == 7)
			{
				pmax = &m_maxresources.food;
				pprod = &m_production.food;
				pwork = &m_workpopulation.food;
			}
			else if (m_outerbuildings[i].type == 4)
			{
				pmax = &m_maxresources.wood;
				pprod = &m_production.wood;
				pwork = &m_workpopulation.wood;
			}
			else if (m_outerbuildings[i].type == 5)
			{
				pmax = &m_maxresources.stone;
				pprod = &m_production.stone;
				pwork = &m_workpopulation.stone;
			}
			else if (m_outerbuildings[i].type == 6)
			{
				pmax = &m_maxresources.iron;
				pprod = &m_production.iron;
				pwork = &m_workpopulation.iron;
			}
			else
			{
				continue;
			}
			switch (m_outerbuildings[i].level)
			{
			case 1:
				*pmax += 10000;
				*pprod += 100;
				*pwork += 10;
				break;
			case 2:
				*pmax += 30000;
				*pprod += 300;
				*pwork += 30;
				break;
			case 3:
				*pmax += 60000;
				*pprod += 600;
				*pwork += 60;
				break;
			case 4:
				*pmax += 100000;
				*pprod += 1000;
				*pwork += 100;
				break;
			case 5:
				*pmax += 150000;
				*pprod += 1500;
				*pwork += 150;
				break;
			case 6:
				*pmax += 210000;
				*pprod += 2100;
				*pwork += 210;
				break;
			case 7:
				*pmax += 280000;
				*pprod += 2800;
				*pwork += 280;
				break;
			case 8:
				*pmax += 360000;
				*pprod += 3600;
				*pwork += 360;
				break;
			case 9:
				*pmax += 450000;
				*pprod += 4500;
				*pwork += 450;
				break;
			case 10:
				*pmax += 550000;
				*pprod += 5500;
				*pwork += 550;
				break;
			}
		}
	}

	m_production.food *= (m_workrate.food/100);
	m_production.wood *= (m_workrate.wood/100);
	m_production.stone *= (m_workrate.stone/100);
	m_production.iron *= (m_workrate.iron/100);

	int32_t workpop = (m_workrate.food*m_workpopulation.food/100)+(m_workrate.stone*m_workpopulation.stone/100)+(m_workrate.wood*m_workpopulation.wood/100)+(m_workrate.iron*m_workpopulation.iron/100);
	m_availablepopulation = m_population-workpop;
	m_productionefficiency = (m_population>=workpop)?100:(m_population / workpop * 100);
	m_production.gold = double(m_population) * (m_workrate.gold/100);
}

void PlayerCity::CalculateResourceStats()
{
	// TODO add resource production modifiers (tech and valley)
	m_resourcebaseproduction = 100;
	m_resourcemanagement = m_mayor!=0?(m_mayor->m_management+m_mayor->m_managementadded+m_mayor->m_managementbuffadded):0;
	m_resourcetech = 0;
	m_resourcevalley = 0;
}

void PlayerCity::ParseMisc(string str)
{
	if (str.length() > 0)
	{
		char * str2 = new char[str.length()+1];
		memset(str2, 0, str.length()+1);
		memcpy(str2, str.c_str(), str.length());
		char * ch = 0, * cr = 0;
		char * tok;
		tok = strtok_s(str2, ",", &ch);
		assert(tok != 0);
		m_population = (uint16_t)atoi(tok);

		tok = strtok_s(0, ",", &ch);
		assert(tok != 0);
		m_workrate.gold = atof(tok);

		tok = strtok_s(0, ",", &ch);
		assert(tok != 0);
		m_workrate.food = atof(tok);

		tok = strtok_s(0, ",", &ch);
		assert(tok != 0);
		m_workrate.wood = atof(tok);

		tok = strtok_s(0, ",", &ch);
		assert(tok != 0);
		m_workrate.iron = atof(tok);

		tok = strtok_s(0, ",", &ch);
		assert(tok != 0);
		m_workrate.stone = atof(tok);

		tok = strtok_s(0, ",", &ch);
		assert(tok != 0);
		m_loyalty = (int8_t)atoi(tok);

		tok = strtok_s(0, ",", &ch);
		assert(tok != 0);
		m_grievance = (int8_t)atoi(tok);

		tok = strtok_s(0, ",", &ch);
		assert(tok != 0);
		m_timers.updateresources = atof(tok);

		delete[] str2;
	}
}
void PlayerCity::SetTroops(int8_t type, int64_t amount)
{
	if (type == TR_ARCHER)
		m_troops.archer += amount;
	else if (type == TR_WORKER)
		m_troops.worker += amount;
	else if (type == TR_WARRIOR)
		m_troops.warrior += amount;
	else if (type == TR_SCOUT)
		m_troops.scout += amount;
	else if (type == TR_PIKE)
		m_troops.pike += amount;
	else if (type == TR_SWORDS)
		m_troops.sword += amount;
	else if (type == TR_TRANSPORTER)
		m_troops.transporter += amount;
	else if (type == TR_RAM)
		m_troops.ram += amount;
	else if (type == TR_CATAPULT)
		m_troops.catapult += amount;
	else if (type == TR_CAVALRY)
		m_troops.cavalry += amount;
	else if (type == TR_CATAPHRACT)
		m_troops.cataphract += amount;
	TroopUpdate();
}

int64_t PlayerCity::GetTroops(int8_t type)
{
	if (type == TR_ARCHER)
		return m_troops.archer;
	else if (type == TR_WORKER)
		return m_troops.worker;
	else if (type == TR_WARRIOR)
		return m_troops.warrior;
	else if (type == TR_SCOUT)
		return m_troops.scout;
	else if (type == TR_PIKE)
		return m_troops.pike;
	else if (type == TR_SWORDS)
		return m_troops.sword;
	else if (type == TR_TRANSPORTER)
		return m_troops.transporter;
	else if (type == TR_RAM)
		return m_troops.ram;
	else if (type == TR_CATAPULT)
		return m_troops.catapult;
	else if (type == TR_CAVALRY)
		return m_troops.cavalry;
	else if (type == TR_CATAPHRACT)
		return m_troops.cataphract;
	return 0;
}

void PlayerCity::ParseTroops(char * str)
{
	if (str && strlen(str) > 0)
	{
		int amount;

		char * ch = 0, * cr = 0;
		char * tok;
		tok = strtok_s(str, "|", &ch);
		int64_t tr = 0;
		for (int i = 0; i < 12; ++i,tok = strtok_s(0, "|", &ch))
		{
			if (tok != 0)
			{
				tr = _atoi64(tok);
				if (tr < 0)
					SetTroops(i, 0);
				else
					SetTroops(i, tr);
			}
		}
	}
}

void PlayerCity::ParseFortifications(char * str)
{
	if (str && strlen(str) > 0)
	{
		int traps;
		int abatis;
		int towers;
		int logs;
		int trebs;

		char * cr = 0;
		char * tok;
		tok = strtok_s(str, ",", &cr);
		if (tok != 0)
			traps = atoi(tok);
		tok = strtok_s(0, ",", &cr);
		if (tok != 0)
			abatis = atoi(tok);
		tok = strtok_s(0, ",", &cr);
		if (tok != 0)
			towers = atoi(tok);
		tok = strtok_s(0, ",", &cr);
		if (tok != 0)
			logs = atoi(tok);
		tok = strtok_s(0, ",", &cr);
		if (tok != 0)
			trebs = atoi(tok);
		City::SetForts(traps, abatis, towers, logs, trebs);
	}
}

int16_t PlayerCity::GetBuildingLevel(int16_t id)
{
	int level = 0;
	for (int i = 0; i < 35; ++i)
		if (m_innerbuildings[i].type > 0)
			if (m_innerbuildings[i].type == id)
				if (level < m_innerbuildings[i].level)
					level = m_innerbuildings[i].level;
	for (int i = 0; i <= 40; ++i)
		if (m_outerbuildings[i].type > 0)
			if (m_outerbuildings[i].type == id)
				if (level < m_outerbuildings[i].level)
					level = m_outerbuildings[i].level;
	return level;
}
int16_t PlayerCity::GetBuildingCount(int16_t id)
{
	int32_t count = 0;
	for (int i = 0; i < 35; ++i)
		if (m_innerbuildings[i].type > 0)
			if (m_innerbuildings[i].type == id)
				count++;
	for (int i = 0; i <= 40; ++i)
		if (m_outerbuildings[i].type > 0)
			if (m_outerbuildings[i].type == id)
				count++;
	return count;
}
stBuilding * PlayerCity::GetBuilding(int16_t position)
{
	if (position > 1000)
	{
		if (position-1000 > 40)
		{
			return 0;
		}
		return &m_outerbuildings[position-1000];
	}
	else
	{
		if ((position < -2) || (position > 34))
		{
			return 0;
		}

		position += 2;

		return &m_innerbuildings[position];
	}
}
int16_t PlayerCity::GetTechLevel(int16_t id)
{
	int blevel = GetBuildingLevel(B_ACADEMY);//academy
	int research = m_client->GetResearchLevel(id);
	if (blevel != 0)
	{
		return blevel<=research?blevel:research;
	}
	return 0;
}
void PlayerCity::CalculateResources()
{
	CalculateResourceStats();
	uint64_t newtime, oldtime, diff;
	newtime = unixtime();

	oldtime = m_timers.updateresources;
	diff = newtime - oldtime;

	m_timers.updateresources = newtime;

	double add;
	add = ((((m_production.food + m_resourcebaseproduction)*(m_productionefficiency/100))/60/60/1000) * (1 + (m_resourcemanagement/100)) * diff) - m_troopconsume;
	if (m_maxresources.food > m_resources.food)
	{
		if (m_maxresources.food < m_resources.food+add)
		{
			m_resources.food = m_maxresources.food;
		}
		else
		{
			m_resources.food += add;
		}
	}
	add = ((((m_production.wood + m_resourcebaseproduction)*(m_productionefficiency/100))/60/60/1000) * (1 + (m_resourcemanagement/100)) * diff);
	if (m_maxresources.wood > m_resources.wood)
	{
		if (m_maxresources.wood < m_resources.wood+add)
		{
			m_resources.wood = m_maxresources.wood;
		}
		else
		{
			m_resources.wood += add;
		}
	}
	add = ((((m_production.stone + m_resourcebaseproduction)*(m_productionefficiency/100))/60/60/1000) * (1 + (m_resourcemanagement/100)) * diff);
	if (m_maxresources.stone > m_resources.stone)
	{
		if (m_maxresources.stone < m_resources.stone+add)
		{
			m_resources.stone = m_maxresources.stone;
		}
		else
		{
			m_resources.stone += add;
		}
	}
	add = ((((m_production.iron + m_resourcebaseproduction)*(m_productionefficiency/100))/60/60/1000) * (1 + (m_resourcemanagement/100)) * diff);
	if (m_maxresources.iron > m_resources.iron)
	{
		if (m_maxresources.iron < m_resources.iron+add)
		{
			m_resources.iron = m_maxresources.iron;
		}
		else
		{
			m_resources.iron += add;
		}
	}

	int32_t herosalary = 0;
	for (int i = 0; i < 10; ++i)
		if (m_heroes[i])
			herosalary += m_heroes[i]->m_level * 20;


	add = ((m_production.gold/60/60/1000) * diff) - herosalary;
	m_resources.gold += add;
	if (m_resources.gold < 0)
		m_resources.gold = 0;
}

void PlayerCity::CastleUpdate()
{
	if (!m_client->m_connected)
		return;
	amf3object obj = amf3object();
	obj["cmd"] = "server.CastleUpdate";
	obj["data"] = amf3object();
	amf3object & data = obj["data"];
	data["updateType"] = 2;
	data["castleBean"] = ToObject();

	m_client->m_main->SendObject(m_client->socket, obj);
}

void PlayerCity::ResourceUpdate()
{
	if (!m_client->m_connected)
		return;
	amf3object obj = amf3object();
	obj["cmd"] = "server.ResourceUpdate";
	obj["data"] = amf3object();
	amf3object & data = obj["data"];
	data["castleId"] = this->m_castleid;
	data["resource"] = Resources();

	m_client->m_main->SendObject(m_client->socket, obj);
}

void PlayerCity::HeroUpdate(Hero * hero, int16_t updatetype)
{
	if (!m_client->m_connected)
		return;
	amf3object obj = amf3object();
	obj["cmd"] = "server.HeroUpdate";
	obj["data"] = amf3object();
	amf3object & data = obj["data"];
	data["castleId"] = this->m_castleid;

	if (updatetype == 0)//hire hero
	{
		data["hero"] = hero->ToObject();
		data["updateType"] = updatetype;
	}
	else if (updatetype == 1)//fire hero
	{
		data["hero"] = hero->ToObject();
		data["updateType"] = updatetype;
	}
	else if (updatetype == 2)//update hero
	{
		data["hero"] = hero->ToObject();
		data["updateType"] = updatetype;
	}

	m_client->m_main->SendObject(m_client->socket, obj);
}

int16_t PlayerCity::GetReliefMultiplier()
{
	switch (GetBuildingLevel(B_RELIEFSTATION))
	{
		default:
			return 1;
		case 1:
		case 2:
		case 3:
			return 2;
		case 4:
			return 3;
		case 5:
		case 6:
		case 7:
			return 4;
		case 8:
		case 9:
			return 5;
		case 10:
			return 6;
	}
}

void PlayerCity::TroopUpdate()
{
	if (!m_client->m_connected)
		return;
	amf3object obj = amf3object();
	obj["cmd"] = "server.TroopUpdate";
	obj["data"] = amf3object();
	amf3object & data = obj["data"];
	data["caslteId"] = m_castleid;
	data["troop"] = Troops();

	m_client->m_main->SendObject(m_client->socket, obj);
}

void PlayerCity::FortUpdate()
{
	if (!m_client->m_connected)
		return;
	amf3object obj = amf3object();
	obj["cmd"] = "server.FortificationsUpdate";
	obj["data"] = amf3object();
	amf3object & data = obj["data"];
	data["castleId"] = m_castleid;
	data["fortification"] = Fortifications();

	m_client->m_main->SendObject(m_client->socket, obj);
}

void PlayerCity::RecacluateCityStats()
{
	int direction;
	int targetpopulation = ( m_maxpopulation * ( double( ( ( m_loyalty + m_grievance ) > 100 ) ? 100 : ( m_loyalty + m_grievance ) ) / 100 ) );
	int add = m_maxpopulation * 0.05;
	if (m_population > targetpopulation)
	{
		if (m_population - add < targetpopulation)
			m_population = targetpopulation;
		else if (m_population - add < 0)
			m_population = m_maxpopulation;
		else
			m_population -= add;
	}
	else if (m_population < targetpopulation)
	{
		if (m_population + add > targetpopulation)
			m_population = targetpopulation;
		else if (m_population + add > m_maxpopulation)
			m_population = m_maxpopulation;
		else
			m_population += add;
	}
	else
	{
		//nothing, pop is as it should be
	}

	int32_t targetloyalty = 100 - ( ( ( m_workrate.gold + m_grievance ) > 100 ) ? 100 : ( m_workrate.gold + m_grievance ) );
	if (targetloyalty < m_loyalty)
	{
		m_loyalty--;
	}
	else if (targetloyalty > m_loyalty)
	{
		m_loyalty++;
	}
	else if (targetloyalty == m_loyalty)
	{

	}

	m_production.gold = m_population * (m_workrate.gold/100);
	ResourceUpdate();
}

amf3array PlayerCity::ResourceProduceData()
{
	amf3array bean;
	amf3object obj = amf3object();

	obj["technologicalPercentage"] = m_resourcetech;
	obj["totalOutput"] = 0;
	obj["heroPercentage"] = (m_production.food*(m_productionefficiency/100)) * (m_resourcemanagement/100);
	obj["maxLabour"] = m_workpopulation.food;
	obj["commenceDemands"] = 0;
	obj["fieldPercentage"] = m_resourcevalley;
	obj["naturalPercentage"] = m_resourcebaseproduction;
	obj["armyPercentage"] = m_troopconsume;
	obj["productionCapacity"] = m_production.food;
	obj["typeid"] = 1;
	obj["commenceRate"] = m_workrate.food;
	obj["cimeliaPercentage"] = 0; //buff plus maybe?
	obj["basicOutput"] = 0;

	bean.Add(obj);

	obj = amf3object();
	obj["technologicalPercentage"] = m_resourcetech;
	obj["totalOutput"] = 0;
	obj["heroPercentage"] = (m_production.wood*(m_productionefficiency/100)) * (m_resourcemanagement/100);
	obj["maxLabour"] = m_workpopulation.wood;
	obj["commenceDemands"] = 0;
	obj["fieldPercentage"] = m_resourcevalley;
	obj["naturalPercentage"] = m_resourcebaseproduction;
	obj["armyPercentage"] = 0;
	obj["productionCapacity"] = m_production.wood;
	obj["typeid"] = 2;
	obj["commenceRate"] = m_workrate.wood;
	obj["cimeliaPercentage"] = 0; //buff plus maybe?
	obj["basicOutput"] = 0;

	bean.Add(obj);

	obj = amf3object();
	obj["technologicalPercentage"] = m_resourcetech;
	obj["totalOutput"] = 0;
	obj["heroPercentage"] = (m_production.stone*(m_productionefficiency/100)) * (m_resourcemanagement/100);
	obj["maxLabour"] = m_workpopulation.stone;
	obj["commenceDemands"] = 0;
	obj["fieldPercentage"] = m_resourcevalley;
	obj["naturalPercentage"] = m_resourcebaseproduction;
	obj["armyPercentage"] = 0;
	obj["productionCapacity"] = m_production.stone;
	obj["typeid"] = 3;
	obj["commenceRate"] = m_workrate.stone;
	obj["cimeliaPercentage"] = 0; //buff plus maybe?
	obj["basicOutput"] = 0;

	bean.Add(obj);

	obj = amf3object();
	obj["technologicalPercentage"] = m_resourcetech;
	obj["totalOutput"] = 0;
	obj["heroPercentage"] = (m_production.iron*(m_productionefficiency/100)) * (m_resourcemanagement/100);
	obj["maxLabour"] = m_workpopulation.iron;
	obj["commenceDemands"] = 0;
	obj["fieldPercentage"] = m_resourcevalley;
	obj["naturalPercentage"] = m_resourcebaseproduction;
	obj["armyPercentage"] = 0;
	obj["productionCapacity"] = m_production.iron;
	obj["typeid"] = 4;
	obj["commenceRate"] = m_workrate.iron;
	obj["cimeliaPercentage"] = 0; //buff plus maybe?
	obj["basicOutput"] = 0;

	bean.Add(obj);
	return bean;
}

int8_t PlayerCity::AddToBarracksQueue(int8_t position, int16_t troopid, int32_t count, bool isshare, bool isidle)
{
	stTroopQueue * queue = GetBarracksQueue(position);
	stBuilding * bldg = GetBuilding(position);
	if (queue->queue.size() >= bldg->level)
	{
		return -1;
	}
	stBuildingConfig * conf = &m_client->m_main->m_troopconfig[troopid];
	stTroopTrain troops;
	troops.troopid = troopid;
	troops.count = count;

	double costtime = conf->time;
	double mayorinf = 1;

	if (position == -2)
	{
		if (m_mayor)
			mayorinf = pow(0.995, m_mayor->GetManagement());

		costtime = ( costtime ) * ( mayorinf ) * ( pow(0.9, m_client->GetResearchLevel(T_CONSTRUCTION)) );
	}
	else
	{
		if (m_mayor)
			mayorinf = pow(0.995, m_mayor->GetPower());

		switch (troopid)
		{
		case TR_CATAPULT:
		case TR_RAM:
		case TR_TRANSPORTER:
		case TR_BALLISTA:
			costtime = ( costtime ) * ( mayorinf ) * ( pow(0.9, m_client->GetResearchLevel(T_METALCASTING)) );
		default:
			costtime = ( costtime ) * ( mayorinf ) * ( pow(0.9, m_client->GetResearchLevel(T_MILITARYSCIENCE)) );
		}
	}

	troops.costtime = floor(costtime) * count * 1000;
	troops.starttime = unixtime();
	troops.queueid = queue->nextqueueid++;

	if (queue->queue.size() > 0)
		troops.endtime = 0;
	else
		troops.endtime = unixtime() + troops.costtime;
	queue->queue.push_back(troops);
	return 1;
}
int16_t PlayerCity::HeroCount()
{
	int16_t cnt = 0;
	for (int i = 0; i < 10; ++i)
		if (m_heroes[i])
			++cnt;
	return cnt;
}

stTroopQueue * PlayerCity::GetBarracksQueue(int16_t position)
{
	for (int i = 0; i < m_troopqueue.size(); ++i)
	{
		if (m_troopqueue[i].positionid == position)
		{
			return &m_troopqueue[i];
		}
	}
	return 0;
}

bool PlayerCity::CheckBuildingPrereqs(int16_t type, int16_t level)
{
	if (type <= 0 || type > 35)
		return false;
	stBuildingConfig * cfg = &m_client->m_main->m_buildingconfig[type][level];


	for (int a = 0; a < 3; ++a)
	{
		if (cfg->buildings[a].id > 0)
		{
			if (GetBuildingLevel(cfg->buildings[a].id) < cfg->buildings[a].level)
				return false;
		}
	}
	for (int a = 0; a < 3; ++a)
	{
		if (cfg->items[a].id > 0)
		{
			if (m_client->GetItemCount(cfg->items[a].id) < cfg->items[a].level)
				return false;
		}
	}
	for (int a = 0; a < 3; ++a)
	{
		if (cfg->techs[a].id > 0)
		{
			if (m_client->GetResearchLevel(cfg->techs[a].id) < cfg->techs[a].level)
				return false;
		}
	}
	return true;
}

bool PlayerCity::HasTroops(stTroops & troops)
{
	if (m_troops.worker - troops.worker < 0)
		return false;
	if (m_troops.warrior - troops.warrior < 0)
		return false;
	if (m_troops.scout - troops.scout < 0)
		return false;
	if (m_troops.pike - troops.pike < 0)
		return false;
	if (m_troops.sword - troops.sword < 0)
		return false;
	if (m_troops.archer - troops.archer < 0)
		return false;
	if (m_troops.cavalry - troops.cavalry < 0)
		return false;
	if (m_troops.cataphract - troops.cataphract < 0)
		return false;
	if (m_troops.transporter - troops.transporter < 0)
		return false;
	if (m_troops.ballista - troops.ballista < 0)
		return false;
	if (m_troops.ram - troops.ram < 0)
		return false;
	if (m_troops.catapult - troops.catapult < 0)
		return false;
	return true;
}
Hero * PlayerCity::GetHero(uint64_t id)
{
	for (int i = 0; i < 10; ++i)
	{
		if (m_heroes[i]->m_id == id)
			return m_heroes[i];
	}
	return 0;
}


}
}