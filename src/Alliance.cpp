//
// Alliance.cpp
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

#include "Alliance.h"
#include "server.hpp"
#include "Client.h"

namespace spitfire {
namespace server {

extern uint64_t unixtime();

AllianceCore::AllianceCore(server * server)
{
	m_main = server;
	for (int i = 0; i < DEF_MAXALLIANCES; ++i)
	{
		m_alliances[i] = 0;
	}
}

AllianceCore::~AllianceCore()
{
	for (int i = 0; i < DEF_MAXALLIANCES; ++i)
	{
		if (m_alliances[i])
			delete m_alliances[i];
	}
}

void AllianceCore::DeleteAlliance(Alliance * alliance)
{
	int32_t found = 0;
	for (int i = 0; i < DEF_MAXALLIANCES; ++i)
	{
		if (m_alliances[i])
		{
			if (m_alliances[i]->m_allianceid != alliance->m_allianceid)
			{
				if (m_alliances[i]->IsAlly(alliance->m_allianceid))
					m_alliances[i]->UnAlly(alliance->m_allianceid);
				if (m_alliances[i]->IsNeutral(alliance->m_allianceid))
					m_alliances[i]->UnNeutral(alliance->m_allianceid);
				if (m_alliances[i]->IsEnemy(alliance->m_allianceid))
					m_alliances[i]->UnEnemy(alliance->m_allianceid);


				for (int x = 0; x < DEF_MAXCLIENTS; ++x)
				{
					if ((m_main->m_clients[x]) && (m_main->m_clients[x]->m_allianceapply == alliance->m_name))
						m_main->m_clients[x]->m_allianceapply = "";
				}
			}
			else
			{
				found = i;
			}
		}
	}
	alliance->m_invites.clear();
	delete m_alliances[found];
	m_alliances[found] = 0;
}

Alliance * AllianceCore::CreateAlliance(char * name, int32_t ownerid, int32_t allianceid)
{
	for (int i = 0; i < DEF_MAXALLIANCES; ++i)
	{
		if (m_alliances[i] == 0)
		{
			m_alliances[i] = new Alliance(m_main, name, ownerid);
			if (allianceid == 0)
				m_alliances[i]->m_allianceid = m_main->m_allianceid++;
			else
				m_alliances[i]->m_allianceid = allianceid;
			return m_alliances[i];
		}
	}
	return 0;
}

bool AllianceCore::JoinAlliance(uint32_t allianceid, Client * client)
{
	if (client->m_allianceid > 0)
	{
		return false; //already in an alliance
	}
	Alliance * alliance = AllianceById(allianceid);
	if (alliance == (Alliance*)-1)
	{
		return false; //alliance doesn't exist
	}


	if (alliance->AddMember(client->m_accountid, DEF_ALLIANCEMEMBER))
	{
		client->m_allianceid = alliance->m_allianceid;
		client->m_alliancename = alliance->m_name;
		client->m_alliancerank = DEF_ALLIANCEMEMBER;
		client->PlayerUpdate();
		return true;
	}
	return false;
}

bool AllianceCore::RemoveFromAlliance(uint32_t allianceid, Client * client)
{
	if (client->m_allianceid <= 0)
	{
		return false; //not in an alliance
	}
	Alliance * alliance = AllianceById(allianceid);
	if (alliance == (Alliance*)-1)
	{
		return false; //alliance doesn't exist
	}

	alliance->RemoveMember(client->m_accountid);

	client->m_allianceid = 0;
	client->m_alliancename = "";
	client->m_alliancerank = 0;

	client->PlayerUpdate();
	return true;
}

bool AllianceCore::SetRank(uint32_t allianceid, Client * client, int8_t rank)
{
	if (client->m_allianceid <= 0)
	{
		return false; //no alliance id given
	}
	Alliance * alliance = AllianceById(allianceid);
	if (alliance == (Alliance*)-1)
	{
		return false; //alliance doesn't exist
	}


	for (int i = 0; i < alliance->m_members.size(); ++i)
	{
		if (alliance->m_members[i].clientid == client->m_accountid)
		{
			client->m_alliancerank = rank;
			alliance->m_members[i].rank = rank;
			return true;
		}
	}
	return false;
}

void Alliance::SendAllianceMessage(string msg, bool tv, bool nosender)
{
	amf3object obj = amf3object();
	amf3object data = amf3object();

	obj["cmd"] = "server.SystemInfoMsg";
	data["alliance"] = true;
	data["sender"] = "Alliance Msg";
	data["tV"] = tv;
	data["msg"] = msg;
	data["noSenderSystemInfo"] = nosender;

	obj["data"] = data;

	for (int i = 0; i < m_members.size(); ++i)
	{
		m_main->SendObject(m_main->GetClient(m_members[i].clientid)->socket, obj);
	}
}

int16_t AllianceCore::GetRelation(int32_t client1, int32_t client2)
{
	if (client1 == client2)
		return DEF_SELFRELATION;

	Client * c1 = m_main->m_clients[client1];
	Client * c2 = m_main->m_clients[client2];

	if (!c1 || !c2 || c1->m_allianceid < 0 || c2->m_allianceid < 0)
		return DEF_NORELATION;

	if (c1->m_allianceid == c2->m_allianceid)
		return DEF_ALLIANCE;

	if (c1->m_allianceid <= 0 || c2->m_allianceid <= 0)
		return DEF_NORELATION;

	Alliance * a1 = m_alliances[c1->m_allianceid];
	Alliance * a2 = m_alliances[c2->m_allianceid];

	if (a1->IsEnemy(c2->m_allianceid))
		return DEF_ENEMY;
	else if (a1->IsAlly(c2->m_allianceid))
		return DEF_ALLY;
	else if (a1->IsNeutral(c2->m_allianceid))
		return DEF_NEUTRAL;
	else
		return DEF_NORELATION;
}

bool comparemembers(stAlliance first, stAlliance second)
{
	if (first.members < second.members)
		return true;
	else
		return false;
}
bool compareprestige(stAlliance first, stAlliance second)
{
	if (first.prestige > second.prestige)
		return true;
	else
		return false;
}
bool comparehonor(stAlliance first, stAlliance second)
{
	if (first.honor > second.honor)
		return true;
	else
		return false;
}

void AllianceCore::SortAlliances()
{
	m_membersrank.clear();
	m_prestigerank.clear();
	m_honorrank.clear();

	stAlliance stmem;
	stAlliance stpre;
	stAlliance sthon;
	for (int i = 0; i < DEF_MAXALLIANCES; ++i)
	{
		if (m_alliances[i])
		{
			sthon.honor = stpre.prestige = 0;
			stmem.id = stpre.id = sthon.id = m_alliances[i]->m_allianceid;
			stmem.members = m_alliances[i]->m_members.size();
			m_alliances[i]->m_citycount = 0;
			for (int k = 0; k < m_alliances[i]->m_members.size(); ++k)
			{
				Client * client = m_main->GetClient(m_alliances[i]->m_members[k].clientid);
				if (client)
				{
					sthon.honor += client->m_honor;
					stpre.prestige += client->m_prestige;
					m_alliances[i]->m_citycount += client->m_citycount;
				}
			}
			stmem.ref = m_alliances[i];
			stpre.ref = m_alliances[i];
			sthon.ref = m_alliances[i];
			m_membersrank.push_back(stmem);
			m_prestigerank.push_back(stpre);
			m_honorrank.push_back(sthon);
		}
	}

	m_membersrank.sort(comparemembers);
	m_prestigerank.sort(compareprestige);
	m_honorrank.sort(comparehonor);


	list<stAlliance>::iterator iter;
	int num = 1;
	for ( iter = m_membersrank.begin() ; iter != m_membersrank.end(); ++iter )
	{
		iter->rank = num;
		iter->ref->m_membersrank = num++;
	}
	num = 1;
	for ( iter = m_prestigerank.begin() ; iter != m_prestigerank.end(); ++iter )
	{
		iter->rank = num;
		iter->ref->m_prestigerank = num++;
		iter->ref->m_prestige = iter->prestige;
	}
	num = 1;
	for ( iter = m_honorrank.begin() ; iter != m_honorrank.end(); ++iter )
	{
		iter->rank = num;
		iter->ref->m_honorrank = num++;
		iter->ref->m_honor = iter->honor;
	}
}

Alliance * AllianceCore::AllianceById(uint32_t id)
{
	for (int i = 0; i < DEF_MAXALLIANCES; ++i)
	{
		if (m_alliances[i] && m_alliances[i]->m_allianceid == id)
		{
			return m_alliances[i];
		}
	}
	return (Alliance*)-1;
}

Alliance * AllianceCore::AllianceByName(string name)
{
	for (int i = 0; i < DEF_MAXALLIANCES; ++i)
	{
		if (m_alliances[i] && m_alliances[i]->m_name == name)
		{
			return m_alliances[i];
		}
	}
	return (Alliance*)-1;
}


Alliance::Alliance(server * main, char * name, int32_t ownerid)
{
	enemyactioncooldown = 0;
	m_ownerid = ownerid;
	m_name = name;
	m_members.clear();
	m_enemies.clear();
	m_allies.clear();
	m_neutral.clear();
	m_currentmembers = 0;
	m_maxmembers = 500;
	//m_members.push_back(ownerid);
	m_prestige = 0;
	m_honor = 0;
	m_prestigerank = 0;
	m_honorrank = 0;
	m_membersrank = 0;
	m_main = main;
	m_owner = m_main->GetClient(ownerid)->m_playername;
	m_citycount = 0;
}

Alliance::~Alliance()
{

}

amf3object Alliance::ToObject()
{
	amf3object obj = amf3object();

	return obj;
}

bool Alliance::IsEnemy(int32_t allianceid)
{
	vector<int32_t>::const_iterator iter;
	if (m_enemies.size())
	for ( iter = m_enemies.begin() ; iter < m_enemies.end(); ++iter )
	{
		if (*iter == allianceid)
		{
			return true;
		}
	}
	return false;
}

bool Alliance::IsAlly(int32_t allianceid)
{
	vector<int32_t>::const_iterator iter;
	if (m_allies.size())
	for ( iter = m_allies.begin() ; iter < m_allies.end(); ++iter )
	{
		if (*iter == allianceid)
		{
			return true;
		}
	}
	return false;
}

bool Alliance::IsNeutral(int32_t allianceid)
{
	vector<int32_t>::const_iterator iter;
	if (m_neutral.size())
	for ( iter = m_neutral.begin() ; iter < m_neutral.end(); ++iter )
	{
		if (*iter == allianceid)
		{
			return true;
		}
	}
	return false;
}

bool Alliance::AddMember(uint32_t clientid, uint8_t rank)
{
	if (m_currentmembers >= m_maxmembers)
	{
		return false;
	}
	stMember member;
	member.clientid = clientid;
	member.rank = rank;
	m_members.push_back(member);
	m_currentmembers++;
	return true;
}

bool Alliance::HasMember(string username)
{
	Client * client = m_main->GetClientByName(username);
	if (!client)
		return false;
	HasMember(client->m_accountid);
}

bool Alliance::HasMember(uint32_t clientid)
{
	vector<Alliance::stMember>::iterator iter;
	for ( iter = m_members.begin() ; iter != m_members.end(); ++iter)
	{
		if (iter->clientid == clientid)
		{
			return true;
		}
	}
	return false;
}

bool Alliance::RemoveMember(uint32_t clientid)
{
	vector<Alliance::stMember>::iterator iter;
	for ( iter = m_members.begin() ; iter != m_members.end(); )
	{
		if (iter->clientid == clientid)
		{
			m_members.erase(iter++);
			m_currentmembers--;
			return true;
		}
		++iter;
	}
	return true;
}

void Alliance::RequestJoin(Client * client, uint64_t timestamp)
{
	stInviteList invite;
	invite.invitetime = timestamp;
	invite.client = client;
	m_invites.push_back(invite);
}

void Alliance::UnRequestJoin(Client * client)
{
	vector<stInviteList>::iterator iter;
	if (m_invites.size())
	{
		for ( iter = m_invites.begin() ; iter != m_invites.end(); )
		{
			if (iter->client->m_accountid == client->m_accountid)
			{
				m_invites.erase(iter++);
				return;
			}
			 ++iter;
		}
	}
}

void Alliance::UnRequestJoin(string client)
{
	vector<stInviteList>::iterator iter;
	if (m_invites.size())
	{
		for ( iter = m_invites.begin() ; iter < m_invites.end(); )
		{
			if (iter->client->m_playername == client)
			{
				m_invites.erase(iter++);
				return;
			}
			++iter;
		}
	}
}

void Alliance::ParseMembers(char * str)
{
	if (str && strlen(str) > 0)
	{
		uint32_t clientid;
		uint8_t rank;

		char * ch = 0, * cr = 0;
		char * tok;
		tok = strtok_s(str, "|", &ch);
		do
		{
			tok = strtok_s(tok, ",", &cr);
			if (tok != 0)
				clientid = atoi(tok);
			tok = strtok_s(0, ",", &cr);
			if (tok != 0)
				rank = atoi(tok);

			AddMember(clientid, rank);
			tok = strtok_s(0, "|", &ch);
		} while (tok != 0);
	}
}

void Alliance::ParseRelation(vector<int32_t> * list, char * str)
{
	if (str && strlen(str) > 0)
	{
		uint32_t allianceid;

		char * ch = 0, * cr = 0;
		char * tok;
		tok = strtok_s(str, "|", &ch);
		do
		{
			tok = strtok_s(tok, ",", &cr);
			if (tok != 0)
				allianceid = atoi(tok);

			list->push_back(allianceid);

			tok = strtok_s(0, "|", &ch);
		} while (tok != 0);
	}
}

amf3object Alliance::indexAllianceInfoBean()
{
	amf3object allianceinfo;
	allianceinfo["prestige"] = m_prestige;
	allianceinfo["rank"] = m_prestigerank;
	allianceinfo["creatorName"] = m_founder.c_str();
	allianceinfo["allianceNote"] = m_note.c_str();
	allianceinfo["allianceInfo"] = m_intro.c_str();
	allianceinfo["allianceName"] = m_name.c_str();
	allianceinfo["memberCount"] = m_currentmembers;
	allianceinfo["memberLimit"] = m_maxmembers;
	allianceinfo["leaderName"] = m_owner.c_str();
	return allianceinfo;
};

void Alliance::Ally(int32_t allianceid)
{
	if (IsAlly(allianceid))
		return;
	if (IsNeutral(allianceid))
		UnNeutral(allianceid);
	if (IsEnemy(allianceid))
		UnEnemy(allianceid);
	m_allies.push_back(allianceid);
	Alliance * temp = m_main->m_alliances->AllianceById(allianceid);
	temp->SendAllianceMessage("Alliance [" + temp->m_name + "] recognizes Diplomatic Relationship with us as Ally.", false, false);
}
void Alliance::Neutral(int32_t allianceid)
{
	if (IsNeutral(allianceid))
		return;
	if (IsAlly(allianceid))
		UnAlly(allianceid);
	if (IsEnemy(allianceid))
		UnEnemy(allianceid);
	m_neutral.push_back(allianceid);
	Alliance * temp = m_main->m_alliances->AllianceById(allianceid);
	temp->SendAllianceMessage("Alliance [" + temp->m_name + "] recognizes Diplomatic Relationship with us as Neutral.", false, false);
}
void Alliance::Enemy(int32_t allianceid, bool skip /* = false*/)
{
	if (IsEnemy(allianceid))
		return;
	enemyactioncooldown = unixtime() + 1000*60*60*24;
	if (IsNeutral(allianceid))
		UnNeutral(allianceid);
	if (IsAlly(allianceid))
		UnAlly(allianceid);
	m_enemies.push_back(allianceid);
	if (skip)
		return;
	//send global message
	Alliance * temp = m_main->m_alliances->AllianceById(allianceid);
	m_main->MassMessage("Alliance " + this->m_name + " declares war against alliance " + temp->m_name + ". Diplomatic Relationship between each other alters to Hostile automatically.");
	temp->Enemy(m_allianceid, true);
	temp->SendAllianceMessage("Alliance [" + m_name + "] recognizes Diplomatic Relationship with us as Enemy.", false, false);
}
void Alliance::UnAlly(int32_t allianceid)
{
	vector<int32_t>::iterator iter;
	if (m_allies.size())
	{
		for ( iter = m_allies.begin() ; iter < m_allies.end(); )
		{
			if (*iter == allianceid)
			{
				m_allies.erase(iter++);
				return;
			}
			++iter;
		}
	}
}
void Alliance::UnNeutral(int32_t allianceid)
{
	vector<int32_t>::iterator iter;
	if (m_neutral.size())
	{
		for ( iter = m_neutral.begin() ; iter < m_neutral.end(); )
		{
			if (*iter == allianceid)
			{
				m_neutral.erase(iter++);
				return;
			}
			++iter;
		}
	}
}
void Alliance::UnEnemy(int32_t allianceid)
{
	vector<int32_t>::iterator iter;
	if (m_enemies.size())
	{
		for ( iter = m_enemies.begin() ; iter < m_enemies.end(); )
		{
			if (*iter == allianceid)
			{
				m_enemies.erase(iter++);
				return;
			}
			++iter;
		}
	}
}

/*
struct stHero
{
	int32_t m_id;
	int16_t m_level;
	int8_t m_loyalty;
	int16_t m_management;
	int16_t m_managementbuffadded;
	int16_t m_power;
	int16_t m_powerbuffadded;
	int16_t m_stratagem;
	int16_t m_stratagembuffadded;
	int8_t m_status;

};
struct stResources
{
	double food;
	double stone;
	double iron;
	double wood;
	double gold;
};
struct stForts
{
	int32_t traps;
	int32_t abatis;
	int32_t towers;
	int32_t logs;
	int32_t trebs;
};
struct stTroops
{
	int32_t worker;
	int32_t warrior;
	int32_t scout;
	int32_t pike;
	int32_t sword;
	int32_t archer;
	int32_t cavalry;
	int32_t cataphract;
	int32_t transporter;
	int32_t ballista;
	int32_t ram;
	int32_t catapult;
};
struct stResult
{
	stResources resources;
	bool attackerwon;
};
struct stBuff
{
	string id;
	double endtime;
} m_buffs;
struct stResearch
{
	int16_t level;
	double endtime;
} m_research;
struct stAttacker
{
	stTroops troops;
	stBuff ** buffs;
	stResearch ** techs;
	PlayerCity * city;
	stHero hero;
};
struct stDefender
{
	stTroops troops;
	stForts forts;
	stBuff ** buffs;
	stResearch ** techs;
	PlayerCity * city;
	stHero hero;
};
stResult * CalculateBattle(stAttacker & attacker, stDefender & defender)
{
	
}*/
}
}