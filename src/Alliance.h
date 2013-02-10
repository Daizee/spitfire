//
// Alliance.h
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
#include <list>
#include "funcs.h"
#include "amf3.h"


namespace spitfire {
namespace server {

	
using namespace std;

class server;
class Alliance;

class Client;

#define DEF_ALLIANCE 0
#define DEF_ALLY 1
#define DEF_NEUTRAL 2
#define DEF_ENEMY 3
#define DEF_NORELATION 4
#define DEF_SELFRELATION 5

// white = 2
// green = 0?
// teal (ally) = 1
// red = 3
// white = 4
// 
struct stAlliance
{
	int32_t id;
	union
	{
		int32_t members;
		double honor;
		double prestige;
	};
	uint32_t rank;
	Alliance * ref;
};

class AllianceCore
{
public:
	AllianceCore(server * server);
	~AllianceCore();
	int16_t GetRelation(int32_t client1, int32_t client2);
	Alliance * CreateAlliance(char * name, int32_t ownerid, int32_t allianceid = 0);
	void DeleteAlliance(Alliance * alliance);
	bool CheckName(char * name)
	{
		char * tempname;
		tempname = new char[strlen(name)+1];
		memset(tempname, 0, strlen(name)+1);
		for (int i = 0; i < strlen(name); ++i)
			tempname[i] = tolower(name[i]);

		if (strstr(tempname, "penis"))
			return false;
		if (strstr(tempname, "cock"))
			return false;
		if (strstr(tempname, "dick"))
			return false;
		if (strstr(tempname, "asshole"))
			return false;
		if (strstr(tempname, "fuck"))
			return false;
		if (strstr(tempname, "shit"))
			return false;
		if (strstr(tempname, "bitch"))
			return false;
		if (strstr(tempname, "tits"))
			return false;
		if (strstr(tempname, "cunt"))
			return false;
		if (strstr(tempname, "testicle"))
			return false;
		if (strstr(tempname, "queer"))
			return false;
		if (strstr(tempname, "fag"))
			return false;
		if (strstr(tempname, "gay"))
			return false;
		if (strstr(tempname, "homo"))
			return false;
		if (strstr(tempname, "whore"))
			return false;
		if (strstr(tempname, "nigger"))
			return false;
		return true;
	}
	static string GetAllianceRank(int16_t rank)
	{
		if (rank == DEF_ALLIANCEHOST)
		{
			return "Host";
		}
		else if (rank == DEF_ALLIANCEVICEHOST)
		{
			return "Vice Host";
		}
		else if (rank == DEF_ALLIANCEPRES)
		{
			return "Presbyter";
		}
		else if (rank == DEF_ALLIANCEOFFICER)
		{
			return "Officer";
		}
		else if (rank == DEF_ALLIANCEMEMBER)
		{
			return "Member";
		}
		return "No Rank";
	};
	// 	string GetAllianceRank(int16_t rank)
	// 	{
	// 		if (rank == 1)
	// 		{
	// 			return "Leader";
	// 		}
	// 		else if (rank == 2)
	// 		{
	// 			return "Vice Host";
	// 		}
	// 		else if (rank == 3)
	// 		{
	// 			return "Presbyter";
	// 		}
	// 		else if (rank == 4)
	// 		{
	// 			return "Officer";
	// 		}
	// 		else if (rank == 5)
	// 		{
	// 			return "Member";
	// 		}
	// 		return "No Rank";
	// 	};

	void SortAlliances();
	bool JoinAlliance(uint32_t allianceid, Client * client);
	bool RemoveFromAlliance(uint32_t allianceid, Client * client);
	bool SetRank(uint32_t allianceid, Client * client, int8_t rank);

	Alliance * AllianceById(uint32_t id);
	Alliance * AllianceByName(string name);

	server * m_main;

	Alliance * m_alliances[DEF_MAXALLIANCES];

	list<stAlliance> m_membersrank;
	list<stAlliance> m_prestigerank;
	list<stAlliance> m_honorrank;
};


class Alliance
{
public:
	Alliance(server * main, char * name, int32_t ownerid);
	~Alliance();

	server * m_main;

	amf3object ToObject();

	bool HasMember(string username);
	bool HasMember(uint32_t clientid);
	bool IsEnemy(int32_t allianceid);
	bool IsAlly(int32_t allianceid);
	bool IsNeutral(int32_t allianceid);
	void Ally(int32_t allianceid);
	void Neutral(int32_t allianceid);
	void Enemy(int32_t allianceid, bool skip = false);
	void UnAlly(int32_t allianceid);
	void UnNeutral(int32_t allianceid);
	void UnEnemy(int32_t allianceid);

	bool AddMember(uint32_t clientid, uint8_t rank);
	bool RemoveMember(uint32_t clientid);
	void ParseMembers(char * str);
	void ParseRelation(vector<int32_t> * list, char * str);

	void RequestJoin(Client * client, uint64_t timestamp);
	void UnRequestJoin(Client * client);
	void UnRequestJoin(string client);

	void SendAllianceMessage(string msg, bool tv, bool nosender);


	amf3object indexAllianceInfoBean();

	uint64_t enemyactioncooldown;

	int16_t m_currentmembers;
	int16_t m_maxmembers;

	int64_t m_prestige;
	int64_t m_honor;
	int16_t m_prestigerank;
	int16_t m_honorrank;
	int16_t m_membersrank;
	int32_t m_citycount;

	struct stMember
	{
		int32_t clientid;
		int8_t rank;
	};

	struct stInviteList
	{
		Client * client;
		string inviteperson;
		double invitetime;
	};

	vector<stInviteList> m_invites;

	vector<stMember> m_members;
	vector<int32_t> m_enemies;
	vector<int32_t> m_allies;
	vector<int32_t> m_neutral;

	int32_t m_ownerid;
	string m_owner;
	string m_name;
	string m_founder;
	string m_note;
	string m_intro;
	string m_motd;

	int32_t m_allianceid;
};


}
}