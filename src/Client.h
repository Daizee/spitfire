//
// Client.h
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

#include <stdint.h>
#include <string>
#include <vector>
#include "amf3.h"
#include "Alliance.h"
#include "connection.hpp"
#include "City.h"

namespace spitfire {
namespace server {


using namespace std;

class server;





class Client
{
public:
	Client(server * core);
	~Client(void);

	connection * socket;

	amf3object ToObject();
	amf3array Items();
	amf3array SaleTypeItems();
	amf3array SaleItems();
	amf3array CastleArray();
	amf3array BuffsArray();
	amf3object PlayerInfo();

	void ParseBuffs(char * str);
	void ParseResearch(char * str);
	void ParseItems(char * str);
	void ParseMisc(char * str);

	void SetBuff(string type, string desc, int64_t endtime, int8_t param = 0);
	void RemoveBuff(string type);
	void SetItem(string type, int16_t dir);
	void SetResearch(uint16_t id, int16_t level, int16_t castleid, double starttime, double endtime);

	int16_t GetResearchLevel(int16_t id);
	int16_t GetItemCount(string type);
	int16_t GetItemCount(int16_t type);


	int m_accountid;
	int m_parentid;

	bool m_accountexists;

	server * m_main;
	int32_t m_playerid;// = 0;
	int32_t PACKETSIZE;// = 1024 * 150;
	int32_t m_socknum;
	bool m_loggedin;// = false;
	int32_t m_clientnumber;
	string m_playername;// = "Testname";
	string m_flag;// = "Evony";
	string m_faceurl;
	string m_alliancename;
	int16_t m_alliancerank;
	string m_office;
	char m_ipaddress[16];
	int32_t m_allianceid;
	string m_email;
	string m_password;
	double m_prestige;
	double m_honor;
	bool m_beginner;
	int32_t m_status;
	int32_t m_rank;
	int32_t m_title;
	double m_connected;
	double m_lastlogin;
	double m_creation;
	//char * readytoprocess = new char[PACKETSIZE];
	int32_t processoffset;
	int32_t m_citycount;
	int32_t m_sex;
	int32_t m_population;
	uint32_t m_prestigerank;
	uint32_t m_honorrank;
	vector<PlayerCity*> m_city;// = new ArrayList();
	int m_bdenyotherplayer;
	int m_tempvar;
	string m_allianceapply;
	bool m_changedface;
	uint16_t changeface;//new castle designs

	int8_t m_icon;

	int32_t m_cents;

	uint32_t m_currentcityindex;
	uint32_t m_currentcityid;

	double m_lag;



	vector<stBuff> m_buffs;
	stItem m_items[DEF_MAXITEMS];
	stResearch m_research[25];

	stBuff * GetBuff(string type)
	{
		for (int32_t i = 0; i < m_buffs.size(); ++i)
		{
			if (m_buffs[i].id == type)
				return &m_buffs[i];
		}
		return 0;
	}

	void CalculateResources();
	void PlayerUpdate();
	void ItemUpdate(char * itemname);
	void BuffUpdate(string name, string desc, int64_t endtime, int8_t type = 0);
	void HeroUpdate(int heroid, int castleid);
	void MailUpdate();
	bool HasAlliance()
	{
		if (m_allianceid > 0)
			return true;
		return false;
	}
	Alliance * GetAlliance();

	PlayerCity * GetCity(int32_t castleid);
	PlayerCity * GetFocusCity();
	bool Beginner() { return m_beginner; }
	void Beginner(bool set) {
		m_beginner = set;
		PlayerUpdate();
	}
	__forceinline void CheckBeginner()
	{
		if (Beginner())
		{
			if ((unixtime() - m_creation) > 1000*60*60*24*7)
			{
				Beginner(true);
				return;
			}
			for (int i = 0; i < this->m_city.size(); ++i)
			{
				if (m_city[i]->GetBuildingLevel(B_TOWNHALL) >= 5)
				{
					Beginner(true);
					return;
				}
			}
		}
	}
};


}
}