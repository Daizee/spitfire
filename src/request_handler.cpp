//
// request_handler.cpp
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

#include "request_handler.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include "reply.hpp"
#include "request.hpp"
#include "Client.h"
#include "server.hpp"

namespace spitfire {
namespace server {

void ShopUseGoods(amf3object & data, Client * client);
extern int DEF_MAPSIZE;
extern uint64_t unixtime();
extern spitfire::server::server * gserver;


bool ci_equal(char ch1, char ch2)
{
	return toupper((unsigned char)ch1) == toupper((unsigned char)ch2);
}

size_t ci_find(const string& str1, const string& str2)
{
	string::const_iterator pos = search(str1. begin ( ), str1. end ( ), str2.
		begin ( ), str2. end ( ), ci_equal);
	if (pos == str1. end ( ))
		return string::npos;
	else
		return pos - str1. begin ( );
}

string makesafe(string in)
{
	char * temp;
	temp = new char[in.length()*2+1];
	memset(temp, 0, in.length()*2+1);
	mysql_real_escape_string(gserver->accounts->mySQL, temp, in.c_str(), in.length());
	return (string)temp;
}

char * strtolower(char * x)
{
	for(int i = 0; i < strlen(x); ++i)
	{
		if(x[i] >= 65 && x[i] <= 90)
		{
			x[i] += 32;
		}
	}
	return x;
}



request_handler::request_handler()
{
}

void request_handler::handle_request(const request& req, reply& rep)
{
	//req.object
	//object received - process
// 	asio::async_write(socket_, reply_.to_buffers(),
// 		boost::bind(&connection::handle_write, shared_from_this(),
// 		asio::placeholders::error));
	//amf3object obj = req.object;
	//rep.objects.push_back(amf3object());

	uint64_t timestamp = unixtime();

	amf3object & obj = (amf3object)req.object;
	amf3object & data = obj["data"];
	string cmd = obj["cmd"];

	amf3object obj2 = amf3object();
	obj2["cmd"] = "";
	amf3object & data2 = obj2["data"];
	data2 = amf3object();

	Log("packet: size: %5.0"DBL2" - Command: %s", (double)req.size, cmd.c_str());

	char * ctx;

	char * temp = new char[cmd.length()+2];
	memset(temp, 0, cmd.length()+2);
	memcpy(temp, cmd.c_str(), cmd.length());
	temp[cmd.length()+1] = 0;

	string cmdtype, command;

	cmdtype = strtok_s(temp, ".", &ctx);
	if (*ctx != 0)
		command = strtok_s(NULL, ".", &ctx);

	delete[] temp;


	connection & c = *req.connection;
	Client * client = c.client_;

	if (cmdtype != "login")
		if (cmdtype == "" || command == "")
		{
			if (c.client_)
				Log("0 length command sent clientid: %d", c.client_->m_clientnumber);
			else
				Log("0 length command sent from nonexistent client");
			return;
		}

	CHECKCASTLEID();


	PlayerCity * pcity = 0;
	if (client && client->m_currentcityindex != -1)
		pcity = client->m_city[client->m_currentcityindex];


#pragma region common
	if ((cmdtype == "common"))
	{
		if ((command == "worldChat"))
		{
			obj2["cmd"] = "common.channelChat";
			data2["msg"] = data["msg"];
			data2["ok"] = 1;
			data2["channel"] = "world";

			gserver->SendObject(c, obj2);

			amf3object obj3;
			obj3["cmd"] = "server.ChannelChatMsg";
			obj3["data"] = amf3object();
			amf3object & data3 = obj3["data"];
			data3["msg"] = data["msg"];
			data3["languageType"] = 0;
			data3["ownitemid"] = client->m_icon;
			data3["fromUser"] = client->m_playername;
			data3["channel"] = "world";

			LOCK(M_CLIENTLIST);
			if (gserver->ParseChat(client, (char*)data["msg"]))
				for (int i = 0; i < DEF_MAXCLIENTS; ++i)
				{
					if (gserver->m_clients[i])
						gserver->SendObject(gserver->m_clients[i]->socket, obj3);
				}
			UNLOCK(M_CLIENTLIST);
			return;
		}
		if ((command == "privateChat"))
		{
			LOCK(M_CLIENTLIST);
			Client * clnt = gserver->GetClientByName((char*)data["targetName"]);
			if (!clnt)
			{
				obj2["cmd"] = "common.privateChat";
				data2["msg"] = data["msg"];
				data2["packageId"] = 0.0f;
				data2["ok"] = -41;
				stringstream ss;
				ss << "Player " << data["targetName"].c_str() << " doesn't exist.";
				data2["errorMsg"] = ss.str();

				gserver->SendObject(c, obj2);
				UNLOCK(M_CLIENTLIST);
				return;
			}
			UNLOCK(M_CLIENTLIST);

			obj2["cmd"] = "common.privateChat";
			data2["msg"] = data["msg"];
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;
			gserver->SendObject(c, obj2);

			amf3object obj3;
			obj3["cmd"] = "server.PrivateChatMessage";
			obj3["data"] = amf3object();
			amf3object & data3 = obj3["data"];
			data3["msg"] = data["msg"];
			data3["chatType"] = 0;
			data3["ownitemid"] = client->m_icon;
			data3["fromUser"] = client->m_playername;
			gserver->SendObject(clnt->socket, obj3);
			return;
		}
		if ((command == "channelChat"))
		{
			obj2["cmd"] = "common.channelChat";
			data2["msg"] = data["msg"];
			data2["ok"] = 1;
			data2["channel"] = "beginner";

			gserver->SendObject(c, obj2);

			amf3object obj3;
			obj3["cmd"] = "server.ChannelChatMsg";
			obj3["data"] = amf3object();
			amf3object & data3 = obj3["data"];
			data3["msg"] = data["sendMsg"];
			data3["languageType"] = 0;
			data3["ownitemid"] = client->m_icon;
			data3["fromUser"] = client->m_playername;
			data3["channel"] = "beginner";

			LOCK(M_CLIENTLIST);
			if (gserver->ParseChat(client, (char*)data["msg"]))
				for (int i = 0; i < DEF_MAXCLIENTS; ++i)
				{
					if ((gserver->m_clients[i]) && (gserver->m_clients[i]->socket))
						gserver->SendObject(gserver->m_clients[i]->socket, obj3);
				}
			UNLOCK(M_CLIENTLIST);
				return;
		}
		if ((command == "allianceChat"))
		{

			if (client->m_allianceid <= 0)
			{
				obj2["cmd"] = "common.allianceChat";
				data2["errorMsg"] = "To send an alliance message, you must be a member of an alliance";
				data2["ok"] = -99;
				data2["packageId"] = 0.0f;

				gserver->SendObject(c, obj2);
				return;
			}

			obj2["cmd"] = "common.allianceChat";
			data2["msg"] = data["msg"];
			data2["ok"] = 1;
			data2["channel"] = "alliance";

			gserver->SendObject(c, obj2);

			amf3object obj3;
			obj3["cmd"] = "server.AllianceChatMsg";
			obj3["data"] = amf3object();
			amf3object & data3 = obj3["data"];
			data3["msg"] = data["msg"];
			data3["languageType"] = 0;
			data3["ownitemid"] = client->m_icon;
			data3["fromUser"] = client->m_playername;
			data3["channel"] = "alliance";

			Alliance * alliance = client->GetAlliance();

			LOCK(M_CLIENTLIST);
			if (gserver->ParseChat(client, (char*)data["msg"]))
				for (int i = 0; i < alliance->m_members.size(); ++i)
				{
					gserver->SendObject(gserver->GetClient(alliance->m_members[i].clientid)->socket, obj3);
				}
			UNLOCK(M_CLIENTLIST);

			return;
		}
		if ((command == "mapInfoSimple"))
		{

			int x1 = data["x1"];
			int x2 = data["x2"];
			int y1 = data["y1"];
			int y2 = data["y2"];
			int castleId = data["castleId"];


			obj2["cmd"] = "common.mapInfoSimple";
			MULTILOCK(M_CLIENTLIST, M_MAP);
			try {
				obj2["data"] = gserver->m_map->GetTileRangeObject(c.client_->m_clientnumber, x1, x2, y1, y2);
				UNLOCK(M_CLIENTLIST);
				UNLOCK(M_MAP);
			}
			catch (...)
			{
				UNLOCK(M_CLIENTLIST);
				UNLOCK(M_MAP);
			}

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "zoneInfo"))
		{
			obj2["cmd"] = "common.zoneInfo";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;
			amf3array amfarray = amf3array();
			amf3object zone2;
			for (int i = 0; i < 16; ++i)
			{
				amf3object zone = amf3object();
				zone["id"] = i;
				zone["rate"] = gserver->m_map->m_stats[i].playerrate;
				zone["name"] = gserver->m_map->states[i];
				zone["playerCount"] = gserver->m_map->m_stats[i].players;
				zone["castleCount"] = gserver->m_map->m_stats[i].numbercities;
				amfarray.Add(zone);
			}
			data2["zones"] = amfarray;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "getPackageList"))
		{
			obj2["cmd"] = "common.getPackageList";
			data2["packages"] = amf3array();
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "getPackageNumber"))
		{
			obj2["cmd"] = "common.getPackageNumber";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;
			data2["number"] = 0;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "changeUserFace"))
		{
			obj2["cmd"] = "common.changeUserFace";
			data2["packageId"] = 0.0f;

			string faceurl = data["faceUrl"];
			int sex = data["sex"];

			if (client->m_changedface || faceurl.length() > 30 || faceurl.length() < 0 || sex < 0 || sex > 1)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Invalid setting.";

				gserver->SendObject(c, obj2);
				return;
			}
			data2["ok"] = 1;
			data2["msg"] = "\xE5\xA4\xB4\xE5\x83\x8F\xE8\xAE\xBE\xE7\xBD\xAE\xE6\x88\x90\xE5\x8A\x9F";

			client->m_faceurl = faceurl;
			client->m_sex = sex;

			client->m_changedface = true;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "delUniteServerPeaceStatus"))
		{
			obj2["cmd"] = "common.delUniteServerPeaceStatus";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;
			data2["number"] = 0;

			client->m_status = DEF_NORMAL;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "getItemDefXml"))
		{
			obj2["cmd"] = "common.getItemDefXml";

			data2["ok"] = 1;
			data2["packageId"] = 0.0f;
			string s = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
				<itemdef>\
				<items>\
				<itemEum id=\"player.box.compensation.e\" name=\"Compensation Package\" itemType=\"\xE5\xAE\x9D\xE7\xAE\xB1\" dayLimit=\"0\" userLimit=\"1\" desc=\"Includes: 10 amulets, 100 cents.\" itemDesc=\"This package was sent to every member of your server to apologize for extended downtime.\" iconUrl=\"images/items/chongzhidalibao.png\" price=\"0\" playerItem=\"true\"/>\
				<itemEum id=\"player.box.present.money.44\" name=\"Pamplona Prize Pack\" itemType=\"\xE5\xAE\x9D\xE7\xAE\xB1\" dayLimit=\"0\" userLimit=\"1\" desc=\"Includes: Wooden Bull Opener, Lion Medal, Rose Medal, Cross Medal, Primary Guidelines, Intermediate Guidelines, War Horn, Corselet, Holy Water, Hero Hunting, Truce Agreement, City Teleporter, Amulet.\" itemDesc=\"These packages are delivered as gifts to players for every $30 worth of purchases made during our Run with the Bulls promotion.\" iconUrl=\"images/icon/shop/PamplonaPrizePack.png\" price=\"0\" playerItem=\"true\"/>\
				<itemEum id=\"player.box.present.money.45\" name=\"Hollow Wooden Bull\" itemType=\"\xE5\xAE\x9D\xE7\xAE\xB1\" dayLimit=\"0\" userLimit=\"1\" desc=\"Includes: Chest A (Freedom Medal, Justice Medal, Nation Medal, Michelangelo's Script, Plowshares, Arch Saw, Quarrying Tools, Blower, War Ensign, Excalibur, The Wealth of Nations, Amulet) or Chest B (Primary Guidelines, Intermediate Guidelines, Hero Hunting, Merchant Fleet, Plowshares, Double Saw, Quarrying Tools, Blower, Michelangelo's Script, Tax Policy, The Wealth of Nations) or Chest C (Excalibur, War Horn, Corselet, Truce Agreement, War Ensign, Adv City Teleporter, Michelangelo's Script)\" itemDesc=\"These chests are sent to you as Run with the Bulls gifts from your friends in the game. They require a Wooden Bull Opener to open. You can obtain a Wooden Bull Opener for every $30 worth of purchases made during the Run with the Bulls promotion. When opened, you will receive the contents of Hollow Wooden Bull A, B or C at random.\" iconUrl=\"images/icon/shop/HollowWoodenBull.png\" price=\"300\" playerItem=\"true\"/>\
				<itemEum id=\"player.key.bull\" name=\"Wooden Bull Opener\" itemType=\"\xE5\xAE\x9D\xE7\xAE\xB1\" dayLimit=\"0\" userLimit=\"0\" desc=\"You can use this key to open one Hollow Wooden Bull sent to you by your friends. If you don’t have any Hollow Wooden Bull, you should ask your friends to send you some!\" itemDesc=\"You can open any Hollow Wooden Bull your friends gave you with this key once.\" iconUrl=\"images/icon/shop/WoodenBullOpener.png\" price=\"0\"/>\
				<itemEum id=\"player.running.shoes\" name=\"Extra-Fast Running Shoes\" itemType=\"\xE5\xAE\x9D\xE7\xAE\xB1\" dayLimit=\"0\" userLimit=\"0\" desc=\"A gift from your friends around Run with the Bulls. Use it to get 24 hours of 50% upkeep in ALL your cities any time from July 9th through July 13th. Extra-Fast Running Shoes is stackable (meaning if you already have this buff, using it again will add an additional 24 hours). Once July 14th comes, this item will expire if you haven't used it yet.\" itemDesc=\"Get a 24 hours 50% upkeep buff during July 9th and July 13th.\" iconUrl=\"images/icon/shop/RunningShoes.png\" price=\"0\" playerItem=\"true\"/>\
				<itemEum id=\"player.box.test.1\" name=\"Test Item\" itemType=\"\xE5\xAE\x9D\xE7\xAE\xB1\" dayLimit=\"0\" userLimit=\"0\" desc=\"Includes: test items.\" itemDesc=\"This package exists as a test.\" iconUrl=\"images/items/chongzhidalibao.png\" price=\"10\" playerItem=\"true\"/>\
				<itemEum id=\"alliance.ritual_of_pact.ultimate\" name=\"Ritual of Pact (Ultimate)\" itemType=\"\xE5\xAE\x9D\xE7\x89\xA9\" dayLimit=\"90\" userLimit=\"0\" desc=\"Ritual of Pact (Ultimate): member limit is 1,000; effective for 90 days; leeway period is 7 days.\" itemDesc=\"It allows alliance to increase member limit to 1,000 once applied, which is effective for 90 days, while multiple applications of this item can lengthen the effective period. Once the item effect is due, 7-day leeway is given to the alliance. During this time, new members are denied to be recruited to the alliance. If no further application of the item, the alliance disbands automatically once the 7-day leeway period passes.\" iconUrl=\"images/items/Ritual_of_Pact_Ultimate.png\" price=\"75\"/>\
				<itemEum id=\"player.speak.bronze_publicity_ambassador.permanent\" name=\"Bronze Publicity Ambassador (Permanent)\" itemType=\"\xE5\xAE\x9D\xE7\x89\xA9\" dayLimit=\"3650\" userLimit=\"0\" desc=\"Effect of Bronze Publicity Ambassador (Permanent) can only be replaced by Silver Publicity Ambassador (Permanent) or Gold Publicity Ambassador (Permanent).\" itemDesc=\"Once you apply this item, a special bronze icon will be displayed in front of your name when you speak in the chat box. While this item functions permanently, multiple applications make no difference to the duration of the effective period. Its effect can be replaced by Silver Publicity Ambassador (Permanent) or Gold Publicity Ambassador (Permanent).\" iconUrl=\"images/items/Bronze_Publicity_Ambassador_Permanentb.png\" price=\"75\"/>\
				<itemEum id=\"player.speak.bronze_publicity_ambassador.permanent.15\" name=\"Bronze Publicity Ambassador (15-day)\" itemType=\"\xE5\xAE\x9D\xE7\x89\xA9\" dayLimit=\"15\" userLimit=\"0\" desc=\"Once you apply this item, a special bronze icon will be displayed in front of your name when you speak in the chat box. It’s effective for 15 days, while multiple applications can lengthen the effective period. Its effect can be replaced by Bronze Publicity Ambassador (Permanent), Silver Publicity Ambassador (15-day), Silver Publicity Ambassador (Permanent), Gold Publicity Ambassador (15-day) or Gold Publicity Ambassador (Permanent).\" itemDesc=\"Once you apply this item, a special bronze icon will be displayed in front of your name when you speak in the chat box. It’s effective for 15 days, while multiple applications can lengthen the effective period. Its effect can be replaced by Bronze Publicity Ambassador (Permanent), Silver Publicity Ambassador (15-day), Silver Publicity Ambassador (Permanent), Gold Publicity Ambassador (15-day) or Gold Publicity Ambassador (Permanent).\" iconUrl=\"images/items/Bronze_Publicity_Ambassador_15b.png\" price=\"75\"/>\
				<itemEum id=\"player.speak.gold_publicity_ambassador.15\" name=\"Gold Publicity Ambassador (15-day)\" itemType=\"\xE5\xAE\x9D\xE7\x89\xA9\" dayLimit=\"15\" userLimit=\"0\" desc=\"Once you apply this item, a special gold icon will be displayed in front of your name when you speak in the chat box. It’s effective for 15 days, while multiple applications can lengthen the effective period. Its effect can be replaced by Gold Publicity Ambassador (Permanent).\" itemDesc=\"Effect of Gold Publicity Ambassador (15-day) can only be replaced by Gold Publicity Ambassador (Permanent).\" iconUrl=\"images/items/gold_publicity_ambassador_15b.png\" price=\"75\"/>\
				<itemEum id=\"player.speak.gold_publicity_ambassador.permanent\" name=\"Gold Publicity Ambassador (Permanent)\" itemType=\"\xE5\xAE\x9D\xE7\x89\xA9\" dayLimit=\"3650\" userLimit=\"0\" desc=\"Once you apply this item, a special gold icon will be displayed in front of your name when you speak in the chat box. While this item functions permanently, multiple applications make no difference to the duration of the effective period. \" itemDesc=\"You're the highest level Publicity Ambassador now.\" iconUrl=\"images/items/Gold_Publicity_Ambassador_Permanentb.png\" price=\"75\"/>\
				<itemEum id=\"player.speak.silver_publicity_ambassador.15\" name=\"Silver Publicity Ambassador (15-day)\" itemType=\"\xE5\xAE\x9D\xE7\x89\xA9\" dayLimit=\"15\" userLimit=\"0\" desc=\"Effect of Silver Publicity Ambassador (15-day) can only be replaced by Silver Publicity Ambassador (Permanent), Gold Publicity Ambassador (15-day) or Gold Publicity Ambassador (Permanent).\" itemDesc=\"Once you apply this item, a special silver icon will be displayed in front of your name when you speak in the chat box. It’s effective for 15 days, while multiple applications can lengthen the effective period. Its effect can be replaced by Silver Publicity Ambassador (Permanent), Gold Publicity Ambassador (15-day) or Gold Publicity Ambassador (Permanent).\" iconUrl=\"images/items/Silver_Publicity_Ambassador_15b.png\" price=\"75\"/>\
				<itemEum id=\"player.speak.silver_publicity_ambassador.permanent\" name=\"Silver Publicity Ambassador (Permanent)\" itemType=\"\xE5\xAE\x9D\xE7\x89\xA9\" dayLimit=\"3650\" userLimit=\"0\" desc=\"Once you apply this item, a special silver icon will be displayed in front of your name when you speak in the chat box. While this item functions permanently, multiple applications make no difference to the duration of the effective period. Its effect can be replaced by Silver Publicity Ambassador (Permanent) or Gold Publicity Ambassador (Permanent).\" itemDesc=\"Effect of Silver Publicity Ambassador (Permanent) can only be replaced by Gold Publicity Ambassador (Permanent).\" iconUrl=\"images/items/silver_publicity_ambassador_permanentb.png\" price=\"75\"/>\
				<itemEum id=\"alliance.ritual_of_pact.advanced\" name=\"Ritual of Pact (Advanced)\" itemType=\"\xE5\xAE\x9D\xE7\x89\xA9\" dayLimit=\"15\" userLimit=\"0\" desc=\"Ritual of Pact(Advanced):member limit is 1,000;effective for 15 days; leeway perod is 7 days.\" itemDesc=\"It allows alliance to increase member limit to 1,000 once applied, which is effective for 15 days, while multiple applications of this item can lengthen the effective period. Once the item effect is due, 7-day leeway is given to the alliance. During this time, new members are denied to be recruited to the alliance. If no further application of the item, the alliance disbands automatically once the 7-day leeway period passes.\" iconUrl=\"images/items/Ritual_of_Pact_Advanced.png\" price=\"75\"/>\
				<itemEum id=\"alliance.ritual_of_pact.premium\" name=\"Ritual of Pact (Premium)\" itemType=\"\xE5\xAE\x9D\xE7\x89\xA9\" dayLimit=\"30\" userLimit=\"0\" desc=\"Ritual of Pact (Premium): member limit is 1,000; effective for 30 days; leeway period is 7 days.\" itemDesc=\"It allows alliance to increase member limit to 1,000 once applied, which is effective for 30 days, while multiple applications of this item can lengthen the effective period. Once the item effect is due, 7-day leeway is given to the alliance. During this time, new members are denied to be recruited to the alliance. If no further application of the item, the alliance disbands automatically once the 7-day leeway period passes.\" iconUrl=\"images/items/Ritual_of_Pact_Premium.png\" price=\"75\"/>\
				<itemEum id=\"consume.1.c\" name=\"Speaker (100 pieces package)\" itemType=\"\xE5\xAE\x9D\xE7\x89\xA9\" dayLimit=\"0\" userLimit=\"0\" desc=\"Used while speaking in World channel and sending group message.\" itemDesc=\"It includes 100 Speakers. It costs one Speaker per sentence when chatting in World channel, while sending a group message costs two. Unpack automatically when purchased.\" iconUrl=\"images/items/biglaba.png\" price=\"200\" playerItem=\"true\"/>\
				</items>\
				<special>\
				<pack id=\"Special Christmas Chest\"/>\
				<pack id=\"Special New Year Chest\"/>\
				<pack id=\"Special Easter Chest\"/>\
				<pack id=\"Special Evony Happiness Chest\"/>\
				<pack id=\"Halloween Chest O'Treats\"/>\
				<pack id=\"Special Thanksgiving Package\"/>\
				<pack id=\"Secret Santa Chest\"/>\
				<pack id=\"Valentine's Day Chest \"/>\
				<pack id=\"St Patrick's Day Chest\"/>\
				<pack id=\"Special Easter Chest\"/>\
				<pack id=\"Hollow Wooden Bull\"/>\
				</special>\
				</itemdef>";
			data2["itemXml"] = s;
				
				
				//gserver->m_itemxml;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "createNewPlayer"))
		{
			char * captcha = data["captcha"];
			char * faceUrl = data["faceUrl"];
			char * castleName = data["castleName"];
			char * flag = data["flag"];
			int sex = data["sex"];
			char * userName = data["userName"];
			int zone = data["zone"];

			if (client->m_accountid > 0)
			{
				// already has a city
				amf3object obj;
				obj["cmd"] = "common.createNewPlayer";
				obj["data"] = amf3object();
				amf3object & data = obj["data"];
				data["packageId"] = 0.0f;
				data["ok"] = -86;
				data["errorMsg"] = "City/Account exists.";

				gserver->SendObject(c, obj);
				return;
			}


			//check for data error
			if (zone < 0 || zone > 15 || strlen(userName) < 1 || strlen(userName) > 20 || strlen(flag) < 1 || strlen(flag) > 5 || strlen(castleName) < 1 || strlen(castleName) > 10
				|| strlen(faceUrl) < 1 || strlen(faceUrl) > 30 || sex < 0 || sex > 1)
			{
				amf3object obj;
				obj["cmd"] = "common.createNewPlayer";
				obj["data"] = amf3object();
				amf3object & data = obj["data"];
				data["packageId"] = 0.0f;
				data["ok"] = -87;
				data["errorMsg"] = "Invalid data sent.";

				gserver->SendObject(c, obj);
				return;
			}

			gserver->msql->Select("SELECT * FROM `accounts` WHERE `username`='%s';", userName);
			gserver->msql->Fetch();
			if (gserver->msql->m_iRows > 0)
			{
				//player name exists
				amf3object obj;
				obj["cmd"] = "common.createNewPlayer";
				obj["data"] = amf3object();
				amf3object & data = obj["data"];
				data["packageId"] = 0.0f;
				data["ok"] = -88;
				data["errorMsg"] = "Player name taken";

				gserver->SendObject(c, obj);
				return;
			}
			else
			{
				//see if state can support a new city

				if (gserver->m_map->m_openflats[zone] > 0)
				{
					gserver->m_map->m_openflats[zone]--;
					//create new account, create new city, then send account details

					char tempc[50];
					int randomid = gserver->m_map->GetRandomOpenTile(zone);
					GETXYFROMID(randomid);
					int x = xfromid;
					int y = yfromid;
					if (gserver->m_map->m_tile[randomid].m_type != FLAT || gserver->m_map->m_tile[randomid].m_ownerid != -1)
					{
						Log("Error. Flat not empty!");
						amf3object obj;
						obj["cmd"] = "common.createNewPlayer";
						obj["data"] = amf3object();
						amf3object & data = obj["data"];
						data["packageId"] = 0.0f;
						data["ok"] = -25;
						data["errorMsg"] = "Error with account creation.";

						gserver->SendObject(c, obj);
						return;
					}


					char user[50];
					char flag2[50];
					char faceUrl2[50];
					char castlename2[50];
					mysql_real_escape_string(gserver->accounts->mySQL, user, userName, strlen(userName)<50?strlen(userName):50);
					mysql_real_escape_string(gserver->accounts->mySQL, flag2, flag, strlen(flag)<50?strlen(flag):50);
					mysql_real_escape_string(gserver->accounts->mySQL, faceUrl2, faceUrl, strlen(faceUrl)<50?strlen(faceUrl):50);
					mysql_real_escape_string(gserver->accounts->mySQL, castlename2, castleName, strlen(castleName)<50?strlen(castleName):50);
					gserver->msql->Insert("INSERT INTO `accounts` (`parentid`, `username`, `lastlogin`, `creation`, `ipaddress`, `status`, `reason`, `sex`, `flag`, `faceurl`) \
								 VALUES (%d, '%s', "XI64", "XI64", '%s', %d, '%s', %d, '%s', '%s');",
								 client->m_parentid, user, unixtime(), unixtime(), client->m_ipaddress, 0, "", sex, flag, faceUrl2);

					client->m_accountid = (int32_t)mysql_insert_id(gserver->msql->mySQL);

					string temp = "50,10.000000,100.000000,100.000000,100.000000,100.000000,90,0,";
					char temp2[200];
					sprintf_s(temp2, 200, DBL, unixtime());
					temp += temp2;

					gserver->msql->Insert("INSERT INTO `cities` (`accountid`,`misc`,`fieldid`,`name`,`buildings`,`gold`,`food`,`wood`,`iron`,`stone`,`creation`) \
								 VALUES (%d, '%s',%d, '%s', '%s',100000,100000,100000,100000,100000,"DBL");",
								 client->m_accountid, (char*)temp.c_str(), randomid, castleName, "31,1,-1,0,0.000000,0.000000", unixtime());

					client->m_playername = user;
					client->m_flag = flag2;
					client->m_faceurl = faceUrl2;
					client->m_sex = sex;

					LOCK(M_CASTLELIST);
					PlayerCity * city = (PlayerCity*)gserver->AddPlayerCity((int)client->m_clientnumber, randomid, (uint32_t)mysql_insert_id(gserver->msql->mySQL));
					UNLOCK(M_CASTLELIST);
					city->ParseBuildings("31,1,-1,0,0.000000,0.000000");
					city->m_logurl = "images/icon/cityLogo/citylogo_01.png";
					//city->m_accountid = client->m_accountid;
					city->m_cityname = castlename2;
					//city->m_tileid = randomid;
					client->m_currentcityid = city->m_castleid;
					city->m_creation = unixtime();
					client->m_currentcityindex = 0;
					city->SetResources(100000, 100000, 100000, 100000, 100000);
					city->CalculateResources();
					city->CalculateStats();

					/* Send new account data */

					client->m_cents = 500;

					amf3object obj3;
					obj3["cmd"] = "common.createNewPlayer";
					obj3["data"] = amf3object();
					amf3object & data3 = obj3["data"];
					data3["packageId"] = 0.0f;
					data3["ok"] = 1;
					data3["msg"] = "success";
					data3["player"] = client->ToObject();

					gserver->SendObject(c, obj3);

					client->m_connected = true;

					if (client->GetItemCount("consume.1.a") < 10000)
						client->SetItem("consume.1.a", 10000);


					gserver->m_map->CalculateOpenTiles();

					return;
				}
				else
				{
					//state is full
					amf3object obj;
					obj["cmd"] = "common.createNewPlayer";
					obj["data"] = amf3object();
					amf3object & data = obj["data"];
					data["packageId"] = 0.0f;
					data["ok"] = -25;
					data["errorMsg"] = "No open flats exist.";

					gserver->SendObject(c, obj);
					return;
				}

			}
			return;
		}
	}
#pragma endregion
#pragma region server
	if ((cmdtype == "server"))
	{

	}
#pragma endregion
#pragma region castle
	if ((cmdtype == "castle"))
	{
		if ((command == "getAvailableBuildingBean"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			obj2["cmd"] = "castle.getAvailableBuildingBean";
			amf3array buildinglist = amf3array();


			for (int i = 0; i < 35; ++i)
			{
				if (gserver->m_buildingconfig[i][0].inside != 2)
					continue;
				if (gserver->m_buildingconfig[i][0].limit > 0 && gserver->m_buildingconfig[i][0].limit <= pcity->GetBuildingCount(i))
					continue;
				if (gserver->m_buildingconfig[i][0].time > 0)
				{
					amf3object parent;
					amf3object conditionbean;

					double costtime = gserver->m_buildingconfig[i][0].time*1000;
					double mayorinf = 1;
					if (pcity->m_mayor)
						mayorinf = pow(0.995, pcity->m_mayor->GetManagement());
					costtime = ( costtime ) * ( mayorinf ) * ( pow(0.9, client->GetResearchLevel(T_CONSTRUCTION)) );


					conditionbean["time"] = floor(costtime);
					conditionbean["destructTime"] = gserver->m_buildingconfig[i][0].destructtime;
					conditionbean["wood"] = gserver->m_buildingconfig[i][0].wood;
					conditionbean["food"] = gserver->m_buildingconfig[i][0].food;
					conditionbean["iron"] = gserver->m_buildingconfig[i][0].iron;
					conditionbean["gold"] = 0;
					conditionbean["stone"] = gserver->m_buildingconfig[i][0].stone;

					amf3array buildings = amf3array();
					amf3array items = amf3array();
					amf3array techs = amf3array();

					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_buildingconfig[i][0].buildings[a].id > 0)
						{
							amf3object ta = amf3object();
							ta["level"] = gserver->m_buildingconfig[i][0].buildings[a].level;
							int temp = pcity->GetBuildingLevel(gserver->m_buildingconfig[i][0].buildings[a].id);
							ta["curLevel"] = temp;
							ta["successFlag"] = temp>=gserver->m_buildingconfig[i][0].buildings[a].level?true:false;
							ta["typeId"] = gserver->m_buildingconfig[i][0].buildings[a].id;
							buildings.Add(ta);
						}
					}
					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_buildingconfig[i][0].items[a].id > 0)
						{
							amf3object ta = amf3object();
							int temp = client->m_items[gserver->m_buildingconfig[i][0].items[a].id].count;
							ta["curNum"] = temp;
							ta["num"] = gserver->m_buildingconfig[i][0].items[a].level;
							ta["successFlag"] = temp>=gserver->m_buildingconfig[i][0].items[a].level?true:false;
							ta["id"] = gserver->m_items[gserver->m_buildingconfig[i][0].items[a].id].name;
							items.Add(ta);
						}
					}
					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_buildingconfig[i][0].techs[a].id > 0)
						{
							amf3object ta = amf3object();
							ta["level"] = gserver->m_buildingconfig[i][0].techs[a].level;
							int temp = client->GetResearchLevel(gserver->m_buildingconfig[i][0].techs[a].id);
							ta["curLevel"] = temp;
							ta["successFlag"] = temp>=gserver->m_buildingconfig[i][0].techs[a].level?true:false;
							ta["id"] = gserver->m_buildingconfig[i][0].techs[a].id;
							techs.Add(ta);
						}
					}

					conditionbean["buildings"] = buildings;
					conditionbean["items"] = items;
					conditionbean["techs"] = techs;
					conditionbean["population"] = gserver->m_buildingconfig[i][0].population;
					parent["conditionBean"] = conditionbean;
					parent["typeId"] = i;
					buildinglist.Add(parent);
				}
			}


			data2["builingList"] = buildinglist;
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "getAvailableBuildingListInside"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			obj2["cmd"] = "castle.getAvailableBuildingListInside";
			amf3array buildinglist = amf3array();

			for (int i = 0; i < 35; ++i)
			{
				if (gserver->m_buildingconfig[i][0].inside != 1)
					continue;
				if (gserver->m_buildingconfig[i][0].limit > 0 && gserver->m_buildingconfig[i][0].limit <= pcity->GetBuildingCount(i))
					continue;
				if (gserver->m_buildingconfig[i][0].time > 0)
				{
					amf3object parent;
					amf3object conditionbean;

					double costtime = gserver->m_buildingconfig[i][0].time;//*1000;
					double mayorinf = 1;
					if (pcity->m_mayor)
						mayorinf = pow(0.995, pcity->m_mayor->GetManagement());
					costtime = ( costtime ) * ( mayorinf ) * ( pow(0.9, client->GetResearchLevel(T_CONSTRUCTION)) );


					conditionbean["time"] = floor(costtime);
					conditionbean["destructTime"] = gserver->m_buildingconfig[i][0].destructtime;
					conditionbean["wood"] = gserver->m_buildingconfig[i][0].wood;
					conditionbean["food"] = gserver->m_buildingconfig[i][0].food;
					conditionbean["iron"] = gserver->m_buildingconfig[i][0].iron;
					conditionbean["gold"] = 0;
					conditionbean["stone"] = gserver->m_buildingconfig[i][0].stone;

					amf3array buildings = amf3array();
					amf3array items = amf3array();
					amf3array techs = amf3array();

					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_buildingconfig[i][0].buildings[a].id > 0)
						{
							amf3object ta = amf3object();
							ta["level"] = gserver->m_buildingconfig[i][0].buildings[a].level;
							int temp = pcity->GetBuildingLevel(gserver->m_buildingconfig[i][0].buildings[a].id);
							ta["curLevel"] = temp;
							ta["successFlag"] = temp>=gserver->m_buildingconfig[i][0].buildings[a].level?true:false;
							ta["typeId"] = gserver->m_buildingconfig[i][0].buildings[a].id;
							buildings.Add(ta);
						}
					}
					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_buildingconfig[i][0].items[a].id > 0)
						{
							amf3object ta = amf3object();
							int temp = client->m_items[gserver->m_buildingconfig[i][0].items[a].id].count;
							ta["curNum"] = temp;
							ta["num"] = gserver->m_buildingconfig[i][0].items[a].level;
							ta["successFlag"] = temp>=gserver->m_buildingconfig[i][0].items[a].level?true:false;
							ta["id"] = gserver->m_items[gserver->m_buildingconfig[i][0].items[a].id].name;
							items.Add(ta);
						}
					}
					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_buildingconfig[i][0].techs[a].id > 0)
						{
							amf3object ta = amf3object();
							ta["level"] = gserver->m_buildingconfig[i][0].techs[a].level;
							int temp = client->GetResearchLevel(gserver->m_buildingconfig[i][0].techs[a].id);
							ta["curLevel"] = temp;
							ta["successFlag"] = temp>=gserver->m_buildingconfig[i][0].techs[a].level?true:false;
							ta["id"] = gserver->m_buildingconfig[i][0].techs[a].id;
							techs.Add(ta);
						}
					}

					conditionbean["buildings"] = buildings;
					conditionbean["items"] = items;
					conditionbean["techs"] = techs;
					conditionbean["population"] = gserver->m_buildingconfig[i][0].population;
					parent["conditionBean"] = conditionbean;
					parent["typeId"] = i;
					buildinglist.Add(parent);
				}
			}


			data2["builingList"] = buildinglist;
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			gserver->SendObject(c, obj2);

			
			

			/*amf3object obj2;
			obj2["cmd"] = "castle.getAvailableBuildingListInside";
			obj2["data"] = amf3object();
			amf3object & data2 = obj2["data"];
			data2["errorMsg"] = "com.evony.entity.tech.impl.FortificationTech";
			data2["ok"] = -99;
			data2["packageId"] = 0.0f;
			
			gserver->SendObject(c, obj2);*/
			
			return;
		}
		if ((command == "getAvailableBuildingListOutside"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			obj2["cmd"] = "castle.getAvailableBuildingListOutside";
			amf3array buildinglist = amf3array();

			for (int i = 0; i < 35; ++i)
			{
				if (gserver->m_buildingconfig[i][0].inside != 0)
					continue;
				if (gserver->m_buildingconfig[i][0].limit > 0 && gserver->m_buildingconfig[i][0].limit <= pcity->GetBuildingCount(i))
					continue;
				if (gserver->m_buildingconfig[i][0].time > 0)
				{
					amf3object parent;
					amf3object conditionbean;

					double costtime = gserver->m_buildingconfig[i][0].time*1000;
					double mayorinf = 1;
					if (pcity->m_mayor)
						mayorinf = pow(0.995, pcity->m_mayor->GetManagement());
					costtime = ( costtime ) * ( mayorinf ) * ( pow(0.9, client->GetResearchLevel(T_CONSTRUCTION)) );


					conditionbean["time"] = floor(costtime);
					conditionbean["destructTime"] = gserver->m_buildingconfig[i][0].destructtime;
					conditionbean["wood"] = gserver->m_buildingconfig[i][0].wood;
					conditionbean["food"] = gserver->m_buildingconfig[i][0].food;
					conditionbean["iron"] = gserver->m_buildingconfig[i][0].iron;
					conditionbean["gold"] = 0;
					conditionbean["stone"] = gserver->m_buildingconfig[i][0].stone;

					amf3array buildings = amf3array();
					amf3array items = amf3array();
					amf3array techs = amf3array();

					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_buildingconfig[i][0].buildings[a].id > 0)
						{
							amf3object ta = amf3object();
							ta["level"] = gserver->m_buildingconfig[i][0].buildings[a].level;
							int temp = pcity->GetBuildingLevel(gserver->m_buildingconfig[i][0].buildings[a].id);
							ta["curLevel"] = temp;
							ta["successFlag"] = temp>=gserver->m_buildingconfig[i][0].buildings[a].level?true:false;
							ta["typeId"] = gserver->m_buildingconfig[i][0].buildings[a].id;
							buildings.Add(ta);
						}
					}
					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_buildingconfig[i][0].items[a].id > 0)
						{
							amf3object ta = amf3object();
							int temp = client->m_items[gserver->m_buildingconfig[i][0].items[a].id].count;
							ta["curNum"] = temp;
							ta["num"] = gserver->m_buildingconfig[i][0].items[a].level;
							ta["successFlag"] = temp>=gserver->m_buildingconfig[i][0].items[a].level?true:false;
							ta["id"] = gserver->m_items[gserver->m_buildingconfig[i][0].items[a].id].name;
							items.Add(ta);
						}
					}
					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_buildingconfig[i][0].techs[a].id > 0)
						{
							amf3object ta = amf3object();
							ta["level"] = gserver->m_buildingconfig[i][0].techs[a].level;
							int temp = client->GetResearchLevel(gserver->m_buildingconfig[i][0].techs[a].id);
							ta["curLevel"] = temp;
							ta["successFlag"] = temp>=gserver->m_buildingconfig[i][0].techs[a].level?true:false;
							ta["id"] = gserver->m_buildingconfig[i][0].techs[a].id;
							techs.Add(ta);
						}
					}

					conditionbean["buildings"] = buildings;
					conditionbean["items"] = items;
					conditionbean["techs"] = techs;
					conditionbean["population"] = gserver->m_buildingconfig[i][0].population;
					parent["conditionBean"] = conditionbean;
					parent["typeId"] = i;
					buildinglist.Add(parent);
				}
			}


			data2["builingList"] = buildinglist;
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			gserver->SendObject(c, obj2);

			
			

			/*amf3object obj2;
			obj2["cmd"] = "castle.getAvailableBuildingListInside";
			obj2["data"] = amf3object();
			amf3object & data2 = obj2["data"];
			data2["errorMsg"] = "com.evony.entity.tech.impl.FortificationTech";
			data2["ok"] = -99;
			data2["packageId"] = 0.0f;
			
			gserver->SendObject(c, obj2);*/
			
			return;
		}
		if ((command == "newBuilding"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			obj2["cmd"] = "castle.newBuilding";

			int buildingtype = data["buildingType"];
			int positionid = data["positionId"];
			
			pcity->CalculateResources();
			pcity->CalculateStats();

			if ((buildingtype > 34 || buildingtype <= 0) || pcity->GetBuilding(positionid)->type || ((gserver->m_buildingconfig[buildingtype][0].limit > 0) && (gserver->m_buildingconfig[buildingtype][0].limit <= pcity->GetBuildingCount(buildingtype))))
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Can't build building.";
				data2["packageId"] = 0.0f;

				gserver->SendObject(c, obj2);
				return;
			}

			for (int i = 0; i < 35; ++i)
			{
				if (pcity->m_innerbuildings[i].status != 0)
				{
					data2["ok"] = -48;
					data2["errorMsg"] = "One building allowed to be built at a time.";
					data2["packageId"] = 0.0f;

					gserver->SendObject(c, obj2);
					return;
				}
			}
			for (int i = 0; i < 41; ++i)
			{
				if (pcity->m_outerbuildings[i].status != 0)
				{
					data2["ok"] = -48;
					data2["errorMsg"] = "One building allowed to be built at a time.";
					data2["packageId"] = 0.0f;

					gserver->SendObject(c, obj2);
					return;
				}
			}

			if (!pcity->CheckBuildingPrereqs(buildingtype, 0))
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Building Prerequisites not met.";
				data2["packageId"] = 0.0f;

				gserver->SendObject(c, obj2);
				return;
			}

			if ((gserver->m_buildingconfig[buildingtype][0].food > pcity->m_resources.food)
				|| (gserver->m_buildingconfig[buildingtype][0].wood > pcity->m_resources.wood)
				|| (gserver->m_buildingconfig[buildingtype][0].stone > pcity->m_resources.stone)
				|| (gserver->m_buildingconfig[buildingtype][0].iron > pcity->m_resources.iron)
				|| (gserver->m_buildingconfig[buildingtype][0].gold > pcity->m_resources.gold))
			{
				data2["ok"] = -1;
				data2["errorMsg"] = "Not enough resources.";
				data2["packageId"] = 0.0f;

				gserver->SendObject(c, obj2);
				return;
			}
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			gserver->SendObject(c, obj2);

			pcity->m_resources.food -= gserver->m_buildingconfig[buildingtype][0].food;
			pcity->m_resources.wood -= gserver->m_buildingconfig[buildingtype][0].wood;
			pcity->m_resources.stone -= gserver->m_buildingconfig[buildingtype][0].stone;
			pcity->m_resources.iron -= gserver->m_buildingconfig[buildingtype][0].iron;
			pcity->m_resources.gold -= gserver->m_buildingconfig[buildingtype][0].gold;


			MULTILOCK(M_CASTLELIST, M_TIMEDLIST);

			server::stBuildingAction * ba = new server::stBuildingAction;

			server::stTimedEvent te;
			ba->city = pcity;
			ba->client = client;
			ba->positionid = positionid;
			te.data = ba;
			te.type = DEF_TIMEDBUILDING;

			gserver->AddTimedEvent(te);

			double costtime = gserver->m_buildingconfig[buildingtype][0].time;
			double mayorinf = 1;
			if (pcity->m_mayor)
				mayorinf = pow(0.995, pcity->m_mayor->GetManagement());
			costtime = 1000 * ( costtime ) * ( mayorinf ) * ( pow(0.9, client->GetResearchLevel(T_CONSTRUCTION)) );

			ba->city->SetBuilding(buildingtype, 0, positionid, 1, timestamp, timestamp+floor(costtime));
		
			obj2["cmd"] = "server.BuildComplate";

			data2["buildingBean"] = ba->city->GetBuilding(positionid)->ToObject();
			data2["castleId"] = client->m_currentcityid;

			gserver->SendObject(c, obj2);

			pcity->ResourceUpdate();


			UNLOCK(M_CASTLELIST);
			UNLOCK(M_TIMEDLIST);
			return;
		}
		if ((command == "destructBuilding"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			obj2["cmd"] = "castle.destructBuilding";

			int buildingtype = data["buildingType"];
			int positionid = data["positionId"];
			stBuilding * bldg = pcity->GetBuilding(positionid);

			pcity->CalculateResources();
			pcity->CalculateStats();


			if ((bldg->type > 34 || bldg->type <= 0) || (bldg->level == 0))
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Can't destroy building.";
				data2["packageId"] = 0.0f;

				gserver->SendObject(c, obj2);
				return;
			}


			for (int i = 0; i < 35; ++i)
			{
				if (pcity->m_innerbuildings[i].status != 0)
				{
					data2["ok"] = -48;
					data2["errorMsg"] = "One building allowed to be built at a time.";
					data2["packageId"] = 0.0f;

					gserver->SendObject(c, obj2);
					return;
				}
			}
			for (int i = 0; i < 41; ++i)
			{
				if (pcity->m_outerbuildings[i].status != 0)
				{
					data2["ok"] = -48;
					data2["errorMsg"] = "One building allowed to be built at a time.";
					data2["packageId"] = 0.0f;

					gserver->SendObject(c, obj2);
					return;
				}
			}

			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			gserver->SendObject(c, obj2);

			MULTILOCK(M_CASTLELIST, M_TIMEDLIST);

			server::stBuildingAction * ba = new server::stBuildingAction;

			server::stTimedEvent te;
			ba->city = pcity;
			ba->client = client;
			ba->positionid = positionid;
			te.data = ba;
			te.type = DEF_TIMEDBUILDING;

			gserver->AddTimedEvent(te);

			double costtime = gserver->m_buildingconfig[bldg->type][bldg->level].destructtime*1000;
			double mayorinf = 1;
			if (pcity->m_mayor)
				mayorinf = pow(0.995, pcity->m_mayor->GetManagement());
			costtime = ( costtime ) * ( mayorinf ) * ( pow(0.9, client->GetResearchLevel(T_CONSTRUCTION)) );

			ba->city->SetBuilding(bldg->type, bldg->level, positionid, 2, timestamp, timestamp+floor(costtime));
			
			obj2["cmd"] = "server.BuildComplate";

			data2["buildingBean"] = ba->city->GetBuilding(positionid)->ToObject();
			data2["castleId"] = client->m_currentcityid;

			gserver->SendObject(c, obj2);

			pcity->ResourceUpdate();


			UNLOCK(M_CASTLELIST);
			UNLOCK(M_TIMEDLIST);
			return;
		}
		if ((command == "upgradeBuilding"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			pcity->CalculateResources();
			pcity->CalculateStats();

			int positionid = data["positionId"];
			stBuilding * bldg = pcity->GetBuilding(positionid);
			int buildingtype = bldg->type;
			int buildinglevel = bldg->level;

			obj2["cmd"] = "castle.upgradeBuilding";
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			if (bldg->status != 0)
			{
				data2["ok"] = -45;
				data2["errorMsg"] = "Invalid building status: Your network connection might have experienced delay or congestion. If this error message persists, please refresh or reload this page to login to the game again.";
				data2["packageId"] = 0.0f;

				gserver->SendObject(c, obj2);
				return;
			}

			for (int i = 0; i < 35; ++i)
			{
				if (pcity->m_innerbuildings[i].status != 0)
				{
					data2["ok"] = -48;
					data2["errorMsg"] = "One building allowed to be built at a time.";
					data2["packageId"] = 0.0f;

					gserver->SendObject(c, obj2);
					return;
				}
			}
			for (int i = 0; i < 41; ++i)
			{
				if (pcity->m_outerbuildings[i].status != 0)
				{
					data2["ok"] = -48;
					data2["errorMsg"] = "One building allowed to be built at a time.";
					data2["packageId"] = 0.0f;

					gserver->SendObject(c, obj2);
					return;
				}
			}

			if (!pcity->CheckBuildingPrereqs(buildingtype, buildinglevel))
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Building Prerequisites not met.";
				data2["packageId"] = 0.0f;

				gserver->SendObject(c, obj2);
				return;
			}

			if ((gserver->m_buildingconfig[buildingtype][buildinglevel].food > pcity->m_resources.food)
				|| (gserver->m_buildingconfig[buildingtype][buildinglevel].wood > pcity->m_resources.wood)
				|| (gserver->m_buildingconfig[buildingtype][buildinglevel].stone > pcity->m_resources.stone)
				|| (gserver->m_buildingconfig[buildingtype][buildinglevel].iron > pcity->m_resources.iron)
				|| (gserver->m_buildingconfig[buildingtype][buildinglevel].gold > pcity->m_resources.gold))
			{
				data2["ok"] = -1;
				data2["errorMsg"] = "Not enough resources.";
				data2["packageId"] = 0.0f;

				gserver->SendObject(c, obj2);
				return;
			}


			gserver->SendObject(c, obj2);

			pcity->m_resources.food -= gserver->m_buildingconfig[buildingtype][buildinglevel].food;
			pcity->m_resources.wood -= gserver->m_buildingconfig[buildingtype][buildinglevel].wood;
			pcity->m_resources.stone -= gserver->m_buildingconfig[buildingtype][buildinglevel].stone;
			pcity->m_resources.iron -= gserver->m_buildingconfig[buildingtype][buildinglevel].iron;
			pcity->m_resources.gold -= gserver->m_buildingconfig[buildingtype][buildinglevel].gold;

			MULTILOCK(M_CASTLELIST, M_TIMEDLIST);
			server::stBuildingAction * ba = new server::stBuildingAction;

			server::stTimedEvent te;
			ba->city = pcity;
			ba->client = client;
			ba->positionid = positionid;
			te.data = ba;
			te.type = DEF_TIMEDBUILDING;

			gserver->AddTimedEvent(te);

			double costtime = gserver->m_buildingconfig[buildingtype][buildinglevel].time;
			double mayorinf = 1;
			if (pcity->m_mayor)
				mayorinf = pow(0.995, pcity->m_mayor->GetManagement());
			costtime = ( costtime ) * ( mayorinf ) * ( pow(0.9, client->GetResearchLevel(T_CONSTRUCTION)) );

			pcity->ResourceUpdate();

			obj2["cmd"] = "server.BuildComplate";

			ba->city->SetBuilding(buildingtype, buildinglevel, positionid, 1, timestamp, timestamp+(floor(costtime)*1000));
			
			data2["buildingBean"] = ba->city->GetBuilding(positionid)->ToObject();
			data2["castleId"] = client->m_currentcityid;

			gserver->SendObject(c, obj2);

			obj2["cmd"] = "castle.upgradeBuilding";

			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			gserver->SendObject(c, obj2);


			UNLOCK(M_CASTLELIST);
			UNLOCK(M_TIMEDLIST);
			return;
		}
		if ((command == "checkOutUpgrade"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			obj2["cmd"] = "castle.checkOutUpgrade";

			int tileposition = data["positionId"];

	// 		if (tileposition == -1)
	// 			tileposition = 31;
	// 		if (tileposition == -2)
	// 			tileposition = 32;
			
			int level = pcity->GetBuilding(tileposition)->level;
			int id = pcity->GetBuilding(tileposition)->type;

			if (level >= 10)
			{
				data2["ok"] = 1;
				data2["packageId"] = 0.0f;

				amf3object conditionbean;

				conditionbean["time"] = 31536000;
				conditionbean["destructTime"] = gserver->m_buildingconfig[id][level].destructtime;
				conditionbean["wood"] = 50000000;
				conditionbean["food"] = 50000000;
				conditionbean["iron"] = 50000000;
				conditionbean["gold"] = 0;
				conditionbean["stone"] = 50000000;

				data2["conditionBean"] = conditionbean;

				gserver->SendObject(c, obj2);
				return;
			}

			amf3object conditionbean;

			double costtime = gserver->m_buildingconfig[id][level].time;

			double mayorinf = 1;
			if (pcity->m_mayor)
				mayorinf = pow(0.995, pcity->m_mayor->GetManagement());
			costtime = ( costtime ) * ( mayorinf ) * ( pow(0.9, client->GetResearchLevel(T_CONSTRUCTION)) );

			double desttime = gserver->m_buildingconfig[id][level].destructtime;
			mayorinf = 1;
			if (pcity->m_mayor)
				mayorinf = pow(0.995, pcity->m_mayor->GetManagement());
			desttime = ( desttime ) * ( mayorinf ) * ( pow(0.9, client->GetResearchLevel(T_CONSTRUCTION)) );

			conditionbean["time"] = floor(costtime);
			conditionbean["destructTime"] = desttime;
			conditionbean["wood"] = gserver->m_buildingconfig[id][level].wood;
			conditionbean["food"] = gserver->m_buildingconfig[id][level].food;
			conditionbean["iron"] = gserver->m_buildingconfig[id][level].iron;
			conditionbean["gold"] = 0;
			conditionbean["stone"] = gserver->m_buildingconfig[id][level].stone;

			amf3array buildings = amf3array();
			amf3array items = amf3array();
			amf3array techs = amf3array();

			for (int a = 0; a < 3; ++a)
			{
				if (gserver->m_buildingconfig[id][level].buildings[a].id > 0)
				{
					amf3object ta = amf3object();
					ta["level"] = gserver->m_buildingconfig[id][level].buildings[a].level;
					int temp = pcity->GetBuildingLevel(gserver->m_buildingconfig[id][level].buildings[a].id);
					ta["curLevel"] = temp;
					ta["successFlag"] = temp>=gserver->m_buildingconfig[id][level].buildings[a].level?true:false;
					ta["typeId"] = gserver->m_buildingconfig[id][level].buildings[a].id;
					buildings.Add(ta);
				}
			}
			for (int a = 0; a < 3; ++a)
			{
				if (gserver->m_buildingconfig[id][level].items[a].id > 0)
				{
					amf3object ta = amf3object();
					int temp = client->m_items[gserver->m_buildingconfig[id][level].items[a].level].count;
					ta["curNum"] = temp;
					ta["num"] = gserver->m_buildingconfig[id][level].items[a].level;
					ta["successFlag"] = temp>=gserver->m_buildingconfig[id][level].items[a].level?true:false;
					ta["id"] = gserver->m_items[gserver->m_buildingconfig[id][level].items[a].id].name;
					items.Add(ta);
				}
			}
			for (int a = 0; a < 3; ++a)
			{
				if (gserver->m_buildingconfig[id][level].techs[a].id > 0)
				{
					amf3object ta = amf3object();
					ta["level"] = gserver->m_buildingconfig[id][level].techs[a].level;
					int temp = client->GetResearchLevel(gserver->m_buildingconfig[id][level].techs[a].id);
					ta["curLevel"] = temp;
					ta["successFlag"] = temp>=gserver->m_buildingconfig[id][level].techs[a].level?true:false;
					ta["id"] = gserver->m_buildingconfig[id][level].techs[a].id;
					techs.Add(ta);
				}
			}

			conditionbean["buildings"] = buildings;
			conditionbean["items"] = items;
			conditionbean["techs"] = techs;
			conditionbean["population"] = gserver->m_buildingconfig[id][level].population;


			data2["conditionBean"] = conditionbean;
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			gserver->SendObject(c, obj2);


			return;
		}
		if ((command == "speedUpBuildCommand"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			char * speeditemid;
			int positionid = data["positionId"];
			int castleid = data["castleId"];

			speeditemid = data["itemId"];

			if (!strcmp(speeditemid, "free.speed"))
			{
				//check if under 5 mins

				stBuilding * building = pcity->GetBuilding(positionid);

				if ((building->endtime - building->starttime) <= 5*60*1000)
				{
					//under 5 mins
					obj2["cmd"] = "castle.speedUpBuildCommand";
					data2["packageId"] = 0.0f;
					data2["ok"] = 1;

					gserver->SendObject(c, obj2);

					building->endtime -= 5*60*1000;
				}
				else
				{
					//over 5 mins
					obj2["cmd"] = "castle.speedUpBuildCommand";
					data2["packageId"] = 0.0f;
					data2["ok"] = -99;
					data2["errorMsg"] = "Invalid speed up.";// TODO get 5 min speed up error - castle.speedUpBuildCommand

					gserver->SendObject(c, obj2);
				}
			}
			else
			{
				//is not under 5 mins, apply an item
				int itemcount = client->GetItemCount((string)speeditemid);

				amf3object obj3 = amf3object();
				obj3["cmd"] = "castle.speedUpBuildCommand";
				obj3["data"] = amf3object();
				amf3object & data3 = obj3["data"];
				data3["packageId"] = 0.0f;
				data3["ok"] = 99;// TODO find error value -- castle.speedUpBuildCommand
				data3["errorMsg"] = "Not enough cents.";

				int cents = 0;
				int reducetime = 0;

				obj2["cmd"] = "castle.speedUpBuildCommand";
				data2["packageId"] = 0.0f;
				data2["ok"] = 1;

				gserver->SendObject(c, obj2);

				// TODO reduce time on building being sped up -- castle.speedUpBuildCommand

				stBuilding * building = pcity->GetBuilding(positionid);
				if (!strcmp(speeditemid, "consume.2.a"))
				{
					cents = 5;
					reducetime = 15*60*1000;
				}
				else if (!strcmp(speeditemid, "consume.2.b"))
				{
					cents = 10;
					reducetime = 60*60*1000;
				}
				else if (!strcmp(speeditemid, "consume.2.b.1"))
				{
					cents = 20;
					reducetime = 150*60*1000;
				}
				else if (!strcmp(speeditemid, "consume.2.c"))
				{
					cents = 50;
					reducetime = 8*60*60*1000;
				}
				else if (!strcmp(speeditemid, "consume.2.c.1"))
				{
					cents = 80;
					reducetime = ((rand()%21)+10)*60*60*1000;
				}
				else if (!strcmp(speeditemid, "consume.2.d"))
				{
					cents = 120;
					reducetime = ((building->endtime - building->starttime)*0.30);
				}
				else if (!strcmp(speeditemid, "coins.speed"))
				{
					cents = 200;
					reducetime = (building->endtime - building->starttime);
				}

				if (itemcount <= 0)
				{
					if (client->m_cents < cents)
					{
						gserver->SendObject(c, obj3); // not enough item and not enough cents
						return;
					}
					//not enough item, but can buy with cents
					client->m_cents -= cents;
					client->PlayerUpdate();
				}
				else
				{ //has item
					client->SetItem((string)speeditemid, -1);
				}

				building->endtime -= reducetime;

				pcity->SetBuilding(building->type, building->level, building->id, building->status, building->starttime, building->endtime);
				
				obj2["cmd"] = "server.BuildComplate";

				data2["buildingBean"] = pcity->GetBuilding(positionid)->ToObject();
				data2["castleId"] = client->m_currentcityid;

				gserver->SendObject(c, obj2);

				return;
			}
			return;
		}
		if ((command == "getCoinsNeed"))
		{
			int positionid = data["positionId"];
			int castleid = data["castleId"];

			VERIFYCASTLEID();
			CHECKCASTLEID();

			obj2["cmd"] = "castle.getCoinsNeed";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;
			data2["coinsNeed"] = 200;// TODO calculate correct cents cost based on time

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "cancleBuildCommand"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];
			int positionid  = data["positionId"];

			obj2["cmd"] = "castle.cancleBuildCommand";
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			LOCK(M_TIMEDLIST);
			list<server::stTimedEvent>::iterator iter;
			if (gserver->buildinglist.size() > 0)
			for (iter = gserver->buildinglist.begin(); iter != gserver->buildinglist.end(); )
			{
				server::stBuildingAction * ba = (server::stBuildingAction *)iter->data;
				if (ba->positionid == positionid)
				{
					Client * client = ba->client;
					PlayerCity * city = ba->city;
					stBuilding * bldg = ba->city->GetBuilding(ba->positionid);
					if (bldg->status != 0)
					{
						bldg->status = 0;

						if (bldg->status == 1)
						{
							stResources res;
							res.food = gserver->m_buildingconfig[bldg->type][bldg->level].food;
							res.wood = gserver->m_buildingconfig[bldg->type][bldg->level].wood;
							res.stone = gserver->m_buildingconfig[bldg->type][bldg->level].stone;
							res.iron = gserver->m_buildingconfig[bldg->type][bldg->level].iron;
							res.gold = gserver->m_buildingconfig[bldg->type][bldg->level].gold;
							pcity->m_resources += res;
						}

						gserver->buildinglist.erase(iter++);

						if (bldg->level == 0)
							ba->city->SetBuilding(0, 0, ba->positionid, 0, 0.0, 0.0);
						else
							ba->city->SetBuilding(bldg->type, bldg->level, ba->positionid, 0, 0.0, 0.0);

						client->CalculateResources();
						city->CalculateStats();

						gserver->SendObject(client->socket, obj);

						obj2["cmd"] = "server.BuildComplate";

						data2["buildingBean"] = ba->city->GetBuilding(positionid)->ToObject();
						data2["castleId"] = client->m_currentcityid;

						gserver->SendObject(c, obj2);

						delete ba;

						UNLOCK(M_TIMEDLIST);

						return;
					}
					else
					{
						data2["ok"] = -99;
						data2["errorMsg"] = "Not being constructed.";
						data2["packageId"] = 0.0f;

						gserver->SendObject(c, obj2);
						UNLOCK(M_TIMEDLIST);
						return;
					}
				}
				++iter;
			}
			UNLOCK(M_TIMEDLIST);
			return;
		}
	}
#pragma endregion
#pragma region field
	if ((cmdtype == "field"))
	{
		if ((command == "getOtherFieldInfo"))
		{
			int fieldid = data["fieldId"];

			obj2["cmd"] = "field.getOtherFieldInfo";

			amf3object bean;

			MULTILOCK2(M_MAP, M_CLIENTLIST, M_ALLIANCELIST);
			data2["bean"] = gserver->m_map->GetMapCastle(fieldid, c.client_->m_clientnumber);
			UNLOCK(M_ALLIANCELIST);
			UNLOCK(M_CLIENTLIST);
			UNLOCK(M_MAP);

			//data2["errorMsg"] = "";
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			gserver->SendObject(c, obj2);
			return;
		}
	}
#pragma endregion
#pragma region quest
	if ((cmdtype == "quest"))
	{
		if ((command == "getQuestType")) //quest info requested every 3 mins
		{
			VERIFYCASTLEID();

			int castleid = data["castleId"];
			int questtype = data["type"];

			if (questtype == 1)
			{
				obj2["cmd"] = "quest.getQuestType";
				data2["ok"] = 1;
				data2["packageId"] = 0.0f;
				amf3array types = amf3array();

				amf3object dailygift = amf3object();
				dailygift["description"] = "Rebuild";
				dailygift["mainId"] = 1;
				dailygift["isFinish"] = false;
				dailygift["name"] = "Rebuild";
				dailygift["typeId"] = 66;
				types.Add(dailygift);

				dailygift["description"] = "Domain Expansion";
				dailygift["mainId"] = 1;
				dailygift["isFinish"] = false;
				dailygift["name"] = "Domain Expansion";
				dailygift["typeId"] = 72;
				types.Add(dailygift);

				data2["types"] = types;

				gserver->SendObject(c, obj2);
			}
			else if (questtype == 3)//dailies
			{
				obj2["cmd"] = "quest.getQuestType";
				data2["ok"] = 1;
				data2["packageId"] = 0.0f;
				amf3array types = amf3array();

				amf3object dailygift = amf3object();
				dailygift["description"] = "Daily Gift";
				dailygift["mainId"] = 3;
				dailygift["isFinish"] = false;
				dailygift["name"] = "Daily Gift";
				dailygift["typeId"] = 94;
				types.Add(dailygift);
				data2["types"] = types;

				gserver->SendObject(c, obj2);
			}

			return;
		}
		if ((command == "getQuestList"))
		{
			VERIFYCASTLEID();

			int castleid = data["castleId"];

			obj2["cmd"] = "quest.getQuestList";
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			amf3array quests = amf3array();

			amf3object questobject = amf3object();
			questobject["description"] = "quest.getQuestList DESCRIPTION";
			questobject["manual"] = "WIN THE GAME LA";
			questobject["isCard"] = false;
			questobject["isFinish"] = false;
			questobject["name"] = "CHAMPION OF SERVERS";
			questobject["award"] = "Gold 187,438,274,822,314";

			amf3array targets = amf3array();
			amf3object objective = amf3object();
			objective["name"] = "Winning the game.";
			objective["finished"] = false;
			targets.Add(objective);

			questobject["targets"] = targets;
			questobject["questId"] = 63;


			quests.Add(questobject);



			data2["quests"] = quests;



			gserver->SendObject(c, obj2);
			return;
		}
	}
#pragma endregion
#pragma region alliance
	if ((cmdtype == "alliance"))
	{
		if ((command == "isHasAlliance"))
		{
			obj2["cmd"] = "alliance.isHasAlliance";


			if (client->HasAlliance())
			{
				data2["ok"] = 1;
				data2["packageId"] = 0.0f;


				LOCK(M_ALLIANCELIST);
				Alliance * alliance = client->GetAlliance();
				Alliance * tempalliance;

				data2["serialVersionUID"] = 1.0f;

				amf3array middleList = amf3array();
				amf3array enemyList = amf3array();
				amf3array friendlyList = amf3array();

				vector<int32_t>::iterator iter;
				if (alliance->m_neutral.size() > 0)
				for ( iter = alliance->m_neutral.begin() ; iter != alliance->m_neutral.end(); ++iter )
				{
					tempalliance = client->m_main->m_alliances->AllianceById(*iter);
					if (tempalliance != (Alliance*)-1)
					{
						amf3object ta = amf3object();
						ta["rank"] = tempalliance->m_prestigerank;
						ta["honor"] = tempalliance->m_honor;// HACK might be 0 -- alliance.isHasAlliance
						ta["allianceName"] = tempalliance->m_name.c_str();
						ta["memberCount"] = tempalliance->m_currentmembers;
						ta["aPrestigeCount"] = tempalliance->m_prestige;// HACK might be 0 -- alliance.isHasAlliance
						middleList.Add(ta);
					}
				}
				if (alliance->m_enemies.size() > 0)
				for ( iter = alliance->m_enemies.begin() ; iter != alliance->m_enemies.end(); ++iter )
				{
					tempalliance = client->m_main->m_alliances->AllianceById(*iter);
					if (tempalliance != (Alliance*)-1)
					{
						amf3object ta = amf3object();
						ta["rank"] = tempalliance->m_prestigerank;
						ta["honor"] = tempalliance->m_honor;// HACK might be 0 -- alliance.isHasAlliance
						ta["allianceName"] = tempalliance->m_name.c_str();
						ta["memberCount"] = tempalliance->m_currentmembers;
						ta["aPrestigeCount"] = tempalliance->m_prestige;// HACK might be 0 -- alliance.isHasAlliance
						enemyList.Add(ta);
					}
				}
				if (alliance->m_allies.size() > 0)
				for ( iter = alliance->m_allies.begin() ; iter != alliance->m_allies.end(); ++iter )
				{
					tempalliance = client->m_main->m_alliances->AllianceById(*iter);
					if (tempalliance != (Alliance*)-1)
					{
						amf3object ta = amf3object();
						ta["rank"] = tempalliance->m_prestigerank;
						ta["honor"] = tempalliance->m_honor;// HACK might be 0 -- alliance.isHasAlliance
						ta["allianceName"] = tempalliance->m_name.c_str();
						ta["memberCount"] = tempalliance->m_currentmembers;
						ta["aPrestigeCount"] = tempalliance->m_prestige;// HACK might be 0 -- alliance.isHasAlliance
						friendlyList.Add(ta);
					}
				}
				data2["middleList"] = middleList;
				data2["enemyList"] = enemyList;
				data2["friendlyList"] = friendlyList;
				UNLOCK(M_ALLIANCELIST);


				data2["indexAllianceInfoBean"] = alliance->indexAllianceInfoBean();
			}
			else
			{
				data2["ok"] = -99;
				data2["packageId"] = 0.0f;
				data2["serialVersionUID"] = 0.0f;
				data2["errorMsg"] = "You are not in any alliance.";
			}

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "leaderWantUserInAllianceList"))
		{
			obj2["cmd"] = "alliance.leaderWantUserInAllianceList";
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			LOCK(M_ALLIANCELIST);
			data2["allianceAddPlayerByLeaderInfoBeanList"] = amf3array();

			//TODO: this is your list of alliances that have invited you?-- alliance.leaderWantUserInAllianceList

			amf3array & allianceinfo = *(amf3array*)data2["allianceAddPlayerByLeaderInfoBeanList"];

			for (int i = 0; i < DEF_MAXALLIANCES; ++i)
			{
				if (gserver->m_alliances->m_alliances[i])
				{
					Alliance * alliance = gserver->m_alliances->m_alliances[i];
					if (alliance->m_invites.size())
					{
						vector<Alliance::stInviteList>::iterator iter;
						for ( iter = alliance->m_invites.begin() ; iter != alliance->m_invites.end(); ++iter )
						{
							if (iter->client->m_accountid == client->m_accountid)
							{
								amf3object invite = amf3object();
								invite["prestige"] = alliance->m_prestige;
								invite["rank"] = alliance->m_prestigerank;
								invite["allianceName"] = alliance->m_name;
								invite["memberCount"] = alliance->m_currentmembers;
								allianceinfo.Add(invite);
							}
						}
					}
				}
			}


			UNLOCK(M_ALLIANCELIST);
			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "rejectComeinAlliance"))
		{
			obj2["cmd"] = "alliance.rejectComeinAlliance";
			data2["packageId"] = 0.0f;

			string alliancename = data["allianceName"];

			Alliance * alliance = gserver->m_alliances->AllianceByName(alliancename);
			if (alliance == (Alliance*)-1)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Alliance no longer exists.";

				gserver->SendObject(c, obj2);
				return;
			}

			LOCK(M_ALLIANCELIST);
			if (alliance->m_invites.size())
			{
				vector<Alliance::stInviteList>::iterator iter;
				for ( iter = alliance->m_invites.begin() ; iter != alliance->m_invites.end(); ++iter )
				{
					if (iter->client->m_accountid == client->m_accountid)
					{
						alliance->m_invites.erase(iter);
						data2["ok"] = 1;

						gserver->SendObject(c, obj2);
						return;
					}
				}
			}
			else
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Invite no longer exists.";

				gserver->SendObject(c, obj2);
				return;
			}
			UNLOCK(M_ALLIANCELIST);
			data2["ok"] = -99;
			data2["errorMsg"] = "An unknown error occurred.";

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "agreeComeinAllianceList"))
		{
			obj2["cmd"] = "alliance.agreeComeinAllianceList";
			data2["packageId"] = 0.0f;

			//TODO copy this permission check
			if ((client->m_allianceid == 0) || (client->m_alliancerank > DEF_ALLIANCEPRES))
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not enough rank.";

				gserver->SendObject(c, obj2);
				return;
			}

			LOCK(M_ALLIANCELIST);
			Alliance * alliance = gserver->m_alliances->AllianceById(client->m_allianceid);

			if (alliance == (Alliance*)-1)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Alliance does not exist.";

				gserver->SendObject(c, obj2);
				return;
			}

			amf3array allianceinfo = amf3array();

			for (int i = 0; i < alliance->m_invites.size(); ++i)
			{
				if (alliance->m_invites[i].inviteperson.length() == 0)
				{
					Client * pcl = alliance->m_invites[i].client;
					amf3object iobj = amf3object();
					iobj["prestige"] = pcl->m_prestige;
					iobj["rank"] = pcl->m_prestigerank;
					iobj["userName"] = pcl->m_playername;
					iobj["inviteTime"] = alliance->m_invites[i].invitetime;
					iobj["castleCount"] = pcl->m_citycount;
					allianceinfo.Add(iobj);
				}
			}
			UNLOCK(M_ALLIANCELIST);
			data2["ok"] = 1;

			data2["allianceAddPlayerByUserInfoBeanList"] = allianceinfo;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "resignForAlliance"))
		{
			obj2["cmd"] = "alliance.resignForAlliance";
			data2["packageId"] = 0.0f;

			if (!client->HasAlliance())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "You are not in an alliance.";

				gserver->SendObject(c, obj2);
				return;
			}
			Alliance * alliance = client->GetAlliance();
			if ((client->m_alliancerank == DEF_ALLIANCEHOST) && (alliance->m_currentmembers > 1))
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Resignation refused. Please transfer your host title to other before you resign.";

				gserver->SendObject(c, obj2);
				return;
			}
			else
			{
				//Only person left is host - disband alliance
				if (!gserver->m_alliances->RemoveFromAlliance(client->m_allianceid, client))
				{
					data2["ok"] = -99;
					data2["errorMsg"] = "Unable to leave alliance. Please contact support.";

					gserver->SendObject(c, obj2);
					return;
				}
				gserver->m_alliances->DeleteAlliance(alliance);
			}

			LOCK(M_ALLIANCELIST);
			if (!gserver->m_alliances->RemoveFromAlliance(client->m_allianceid, client))
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Unable to leave alliance. Please contact support.";

				gserver->SendObject(c, obj2);
				UNLOCK(M_ALLIANCELIST);
				return;
			}
			UNLOCK(M_ALLIANCELIST);

			data2["ok"] = 1;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "getAllianceMembers"))
		{
			obj2["cmd"] = "alliance.getAllianceMembers";
			data2["packageId"] = 0.0f;

			if (client->m_allianceid < 0)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not a member of an alliance.";

				gserver->SendObject(c, obj2);
				return;
			}
			MULTILOCK(M_ALLIANCELIST, M_CLIENTLIST);
			Alliance * alliance = gserver->m_alliances->AllianceById(client->m_allianceid);

			if (alliance == (Alliance*)-1)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Invalid Alliance.";

				gserver->SendObject(c, obj2);
				UNLOCK(M_CLIENTLIST);
				UNLOCK(M_ALLIANCELIST);
				return;
			}

			amf3array members = amf3array();

			for (int k = 0; k < alliance->m_members.size(); ++k)
			{
				Client * client = gserver->GetClient(alliance->m_members[k].clientid);
				if (client)
				{
					amf3object temp = amf3object();
					temp["createrTime"] = 0;
					temp["alliance"] = alliance->m_name;
					temp["office"] = client->m_office;
					temp["allianceLevel"] = AllianceCore::GetAllianceRank(client->m_alliancerank);
					temp["sex"] = client->m_sex;
					temp["levelId"] = client->m_alliancerank;
					temp["honor"] = client->m_honor;
					temp["bdenyotherplayer"] = client->m_bdenyotherplayer;
					temp["id"] = client->m_accountid;
					temp["accountName"] = "";
					temp["prestige"] = client->m_prestige;
					temp["faceUrl"] = client->m_faceurl;
					temp["flag"] = client->m_flag;
					temp["userId"] = client->m_parentid;
					temp["userName"] = client->m_playername;
					temp["castleCount"] = client->m_citycount;
					temp["titleId"] = client->m_title;
					temp["medal"] = 0;
					temp["ranking"] = client->m_prestigerank;
					temp["lastLoginTime"] = client->m_lastlogin;
					temp["population"] = client->m_population;
					members.Add(temp);
				}
			}

			data2["ok"] = 1;
			data2["members"] = members;

			UNLOCK(M_CLIENTLIST);
			UNLOCK(M_ALLIANCELIST);
			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "getAllianceWanted"))
		{
			obj2["cmd"] = "alliance.getAllianceWanted";
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			if (client->m_allianceapply.length() > 0)
				data2["allianceName"] = client->m_allianceapply;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "getAllianceInfo"))
		{
			obj2["cmd"] = "alliance.getAllianceInfo";
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			string alliancename = data["allianceName"];

			LOCK(M_ALLIANCELIST);
			Alliance * alliance = gserver->m_alliances->AllianceByName(alliancename);

			if (alliance == (Alliance*)-1)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Alliance does not exist.";

				gserver->SendObject(c, obj2);
				return;
			}

			data2["leader"] = alliance->m_owner;
			data2["prestigeCount"] = alliance->m_prestige;
			data2["ranking"] = alliance->m_prestigerank;
			data2["memberCount"] = alliance->m_currentmembers;
			data2["allinaceInfo"] = alliance->m_intro;
			data2["creator"] = alliance->m_founder;
			UNLOCK(M_ALLIANCELIST);


			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "userWantInAlliance"))
		{
			obj2["cmd"] = "alliance.userWantInAlliance";
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			string alliancename = data["allianceName"];

			Alliance * alliance = gserver->m_alliances->AllianceByName(alliancename);

			if (alliance == (Alliance*)-1)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Alliance does not exist.";

				gserver->SendObject(c, obj2);
				return;
			}

			if (client->m_allianceapply.length() != 0)
			{
				gserver->m_alliances->AllianceByName(client->m_allianceapply)->UnRequestJoin(client);
			}

			alliance->RequestJoin(client, timestamp);


			client->m_allianceapply = alliancename;

			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "cancelUserWantInAlliance"))
		{
			obj2["cmd"] = "alliance.cancelUserWantInAlliance";
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			string alliancename = data["allianceName"];

			if (client->m_allianceapply.length() == 0)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not applied.";

				gserver->SendObject(c, obj2);
				return;
			}

			LOCK(M_ALLIANCELIST);
			Alliance * alliance = gserver->m_alliances->AllianceByName(alliancename);

			if (alliance == (Alliance*)-1)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Alliance does not exist.";

				gserver->SendObject(c, obj2);
				return;
			}

			alliance->UnRequestJoin(client);
			UNLOCK(M_ALLIANCELIST);


			client->m_allianceapply = "";

			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "getMilitarySituationList"))
		{
			int pagesize = data["pageSize"];
			int pageno = data["pageNo"];

			obj2["cmd"] = "alliance.getMilitarySituationList";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "addUsertoAllianceList"))
		{
			//TODO permission check
			obj2["cmd"] = "alliance.addUsertoAllianceList";
			data2["packageId"] = 0.0f;

			if (!client->HasAlliance())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not a member of an alliance.";

				gserver->SendObject(c, obj2);
				return;
			}

			data2["ok"] = 1;

			LOCK(M_ALLIANCELIST);
			amf3array listarray = amf3array();

			Alliance * alliance = client->GetAlliance();

			for (int i = 0; i < alliance->m_invites.size(); ++i)
			{
				if (alliance->m_invites[i].inviteperson.length() == 0)
				{
					Client * pcl = alliance->m_invites[i].client;
					amf3object iobj = amf3object();
					iobj["prestige"] = pcl->m_prestige;
					iobj["rank"] = pcl->m_prestigerank;
					iobj["invitePerson"] = alliance->m_invites[i].inviteperson;
					iobj["userName"] = pcl->m_playername;
					iobj["inviteTime"] = alliance->m_invites[i].invitetime;
					listarray.Add(iobj);
				}
			}
			UNLOCK(M_ALLIANCELIST);

			data2["allianceAddPlayerInfoBeanList"] = listarray;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "addUsertoAlliance"))
		{
			//TODO permission check
			obj2["cmd"] = "alliance.addUsertoAlliance";
			data2["packageId"] = 0.0f;

			if (!client->HasAlliance())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not a member of an alliance.";

				gserver->SendObject(c, obj2);
				return;
			}

			//TODO copy 24h alliance join cooldown?
			/*
			["data"] Type: Object - Value: Object
				["packageId"] Type: Number - Value: 0.000000
				["ok"] Type: Integer - Value: -301
				["errorMsg"] Type: String - Value: Can not join the same Alliance again in 23 hours.
			 */

			MULTILOCK(M_ALLIANCELIST, M_CLIENTLIST);
			string username = data["userName"];
			Client * invitee = gserver->GetClientByName(username);

			if (invitee == 0)
			{
				data2["ok"] = -41;
				string msg;
				msg = "Player ";
				msg += username;
				msg += " doesn't exist.";
				data2["errorMsg"] = msg;

				gserver->SendObject(c, obj2);
				UNLOCK(M_ALLIANCELIST);
				UNLOCK(M_CLIENTLIST);
				return;
			}
			if (invitee->HasAlliance())
			{
				data2["ok"] = -99;
				string msg;
				msg = username;
				msg += "is already a member of other alliance.";
				data2["errorMsg"] = msg;

				gserver->SendObject(c, obj2);
				UNLOCK(M_ALLIANCELIST);
				UNLOCK(M_CLIENTLIST);
				return;
			}
			data2["ok"] = 1;

			Alliance * alliance = client->GetAlliance();

			Alliance::stInviteList invite;
			invite.inviteperson = client->m_playername;
			invite.invitetime = timestamp;
			invite.client = invitee;

			alliance->m_invites.push_back(invite);
			UNLOCK(M_ALLIANCELIST);
			UNLOCK(M_CLIENTLIST);

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "canceladdUsertoAlliance"))
		{
			//TODO permission check
			obj2["cmd"] = "alliance.canceladdUsertoAlliance";
			data2["packageId"] = 0.0f;

			if (!client->HasAlliance())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not a member of an alliance.";

				gserver->SendObject(c, obj2);
				return;
			}

			MULTILOCK(M_ALLIANCELIST, M_CLIENTLIST);
			string username = data["userName"];

			data2["ok"] = 1;

			Alliance * alliance = client->GetAlliance();

			alliance->UnRequestJoin(username);

			UNLOCK(M_ALLIANCELIST);
			UNLOCK(M_CLIENTLIST);

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "cancelagreeComeinAlliance"))
		{
			obj2["cmd"] = "alliance.cancelagreeComeinAlliance";
			data2["packageId"] = 0.0f;

			if (!client->HasAlliance())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not a member of an alliance.";

				gserver->SendObject(c, obj2);
				return;
			}

			MULTILOCK(M_ALLIANCELIST, M_CLIENTLIST);
			string username = data["userName"];

			data2["ok"] = 1;

			Alliance * alliance = client->GetAlliance();

			alliance->UnRequestJoin(username);

			UNLOCK(M_ALLIANCELIST);
			UNLOCK(M_CLIENTLIST);

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "setAllianceFriendship"))
		{
			//TODO permission check
			obj2["cmd"] = "alliance.setAllianceFriendship";
			data2["packageId"] = 0.0f;

			if (!client->HasAlliance())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not a member of an alliance.";

				gserver->SendObject(c, obj2);
				return;
			}

			//1 friendly
			//2 neutral
			//3 enemy
			
			int alliancetype = data["type"];
			string otheralliancename = data["targetAllianceName"];

			Alliance * otheralliance = gserver->m_alliances->AllianceByName(otheralliancename);
			if (otheralliance == (Alliance*)-1)
			{
				//doesn't exist
				data2["ok"] = -99;
				data2["errorMsg"] = "Alliance does not exist.";

				gserver->SendObject(c, obj2);
				return;
			}

			Alliance * alliance = client->GetAlliance();

			if (alliancetype == 1)
			{
				if (alliance->IsAlly(otheralliance->m_allianceid))
				{
					//already allied
					data2["ok"] = -99;
					data2["errorMsg"] = "Alliance is already an ally.";

					gserver->SendObject(c, obj2);
					return;
				}
				alliance->Ally(otheralliance->m_allianceid);
			}
			else if (alliancetype == 2)
			{
				if (alliance->IsNeutral(otheralliance->m_allianceid))
				{
					//already neutral
					data2["ok"] = -99;
					data2["errorMsg"] = "Alliance is already neutral.";

					gserver->SendObject(c, obj2);
					return;
				}
				alliance->Neutral(otheralliance->m_allianceid);
			}
			else //alliancetype = 3
			{
				if (alliance->IsEnemy(otheralliance->m_allianceid))
				{
					//already enemy
					data2["ok"] = -99;
					data2["errorMsg"] = "Alliance is already an enemy.";

					gserver->SendObject(c, obj2);
					return;
				}
				if (unixtime() < alliance->enemyactioncooldown)
				{
					//Declared war too soon
					data2["ok"] = -99;
					data2["errorMsg"] = "You have already declared war recently.";

					gserver->SendObject(c, obj2);
					return;
				}

				alliance->Enemy(otheralliance->m_allianceid);
			}
			data2["ok"] = 1;
			gserver->SendObject(c, obj2);
		}
		if ((command == "getAllianceFriendshipList"))
		{
			// TODO War reports -- alliance.getMilitarySituationList
			obj2["cmd"] = "alliance.getAllianceFriendshipList";
			data2["packageId"] = 0.0f;

			if (!client->HasAlliance())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not a member of an alliance.";

				gserver->SendObject(c, obj2);
				return;
			}

			data2["ok"] = 1;

			LOCK(M_ALLIANCELIST);
			Alliance * tempalliance = (Alliance*)-1;
			Alliance * alliance = client->GetAlliance();

			amf3array middleList = amf3array();
			amf3array enemyList = amf3array();
			amf3array friendlyList = amf3array();

			vector<int32_t>::iterator iter;
			if (alliance->m_neutral.size() > 0)
			for ( iter = alliance->m_neutral.begin() ; iter != alliance->m_neutral.end(); ++iter )
			{
				tempalliance = client->m_main->m_alliances->AllianceById(*iter);
				if (tempalliance != (Alliance*)-1)
				{
					amf3object ta = amf3object();
					ta["rank"] = tempalliance->m_prestigerank;
					ta["honor"] = tempalliance->m_honor;// HACK might be 0 -- alliance.isHasAlliance
					ta["allianceName"] = tempalliance->m_name.c_str();
					ta["memberCount"] = tempalliance->m_currentmembers;
					ta["aPrestigeCount"] = tempalliance->m_prestige;// HACK might be 0 -- alliance.isHasAlliance
					middleList.Add(ta);
				}
			}
			if (alliance->m_enemies.size() > 0)
			for ( iter = alliance->m_enemies.begin() ; iter != alliance->m_enemies.end(); ++iter )
			{
				tempalliance = client->m_main->m_alliances->AllianceById(*iter);
				if (tempalliance != (Alliance*)-1)
				{
					amf3object ta = amf3object();
					ta["rank"] = tempalliance->m_prestigerank;
					ta["honor"] = tempalliance->m_honor;// HACK might be 0 -- alliance.isHasAlliance
					ta["allianceName"] = tempalliance->m_name.c_str();
					ta["memberCount"] = tempalliance->m_currentmembers;
					ta["aPrestigeCount"] = tempalliance->m_prestige;// HACK might be 0 -- alliance.isHasAlliance
					enemyList.Add(ta);
				}
			}
			if (alliance->m_allies.size() > 0)
			for ( iter = alliance->m_allies.begin() ; iter != alliance->m_allies.end(); ++iter )
			{
				tempalliance = client->m_main->m_alliances->AllianceById(*iter);
				if (tempalliance != (Alliance*)-1)
				{
					amf3object ta = amf3object();
					ta["rank"] = tempalliance->m_prestigerank;
					ta["honor"] = tempalliance->m_honor;// HACK might be 0 -- alliance.isHasAlliance
					ta["allianceName"] = tempalliance->m_name.c_str();
					ta["memberCount"] = tempalliance->m_currentmembers;
					ta["aPrestigeCount"] = tempalliance->m_prestige;// HACK might be 0 -- alliance.isHasAlliance
					friendlyList.Add(ta);
				}
			}
			data2["middleList"] = middleList;
			data2["enemyList"] = enemyList;
			data2["friendlyList"] = friendlyList;

			UNLOCK(M_ALLIANCELIST);
			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "createAlliance"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			obj2["cmd"] = "alliance.createAlliance";
			data2["packageId"] = 0.0f;

			char * alliancename = data["allianceName"];

			if (pcity->m_resources.gold < 10000)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not enough gold.";

				gserver->SendObject(c, obj2);
				return;
			}

			if (!gserver->m_alliances->CheckName(alliancename) || strlen(alliancename) < 2)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Illegal naming, please choose another name.";

				gserver->SendObject(c, obj2);
				return;
			}

			LOCK(M_ALLIANCELIST);
			if (gserver->m_alliances->AllianceByName(alliancename) != (Alliance*)-1)
			{
				data2["ok"] = -12;
				string error = "Alliance already existed: ";
				error += alliancename;
				error += ".";
				data2["errorMsg"] = error;

				gserver->SendObject(c, obj2);
				UNLOCK(M_ALLIANCELIST);
				return;
			}
			Alliance * alliance = 0;
			if (alliance = gserver->m_alliances->CreateAlliance(alliancename, client->m_accountid))
			{
				data2["ok"] = 1;

				string error = "Establish alliance ";
				error += alliancename;
				error += " successfully.";
				data2["msg"] = error;

				alliance->m_founder = client->m_playername;

				pcity->m_resources.gold -= 10000;

				if (!gserver->m_alliances->JoinAlliance(alliance->m_allianceid, client))
				{
					client->PlayerUpdate();
					pcity->ResourceUpdate();

					data2["ok"] = -99;
					data2["errorMsg"] = "Alliance created but cannot join. Please contact support.";

					gserver->SendObject(c, obj2);
					UNLOCK(M_ALLIANCELIST);
					return;
				}
				if (!gserver->m_alliances->SetRank(alliance->m_allianceid, client, DEF_ALLIANCEHOST))
				{
					client->PlayerUpdate();
					pcity->ResourceUpdate();

					data2["ok"] = -99;
					data2["errorMsg"] = "Alliance created but cannot set rank to host. Please contact support.";

					gserver->SendObject(c, obj2);
					UNLOCK(M_ALLIANCELIST);
					return;
				}

				client->PlayerUpdate();
				pcity->ResourceUpdate();



				gserver->m_alliances->SortAlliances();

				gserver->SendObject(c, obj2);
				UNLOCK(M_ALLIANCELIST);
				return;
			}
			else
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Cannot create alliance. Please contact support.";

				gserver->SendObject(c, obj2);
				UNLOCK(M_ALLIANCELIST);
				return;
			}
		}
		if ((command == "setAllInfoForAlliance"))
		{
			//TODO permission check
			string notetext = data["noteText"];
			string infotext = data["infoText"];
			string alliancename = data["allianceName"];

			obj2["cmd"] = "alliance.setAllInfoForAlliance";
			data2["packageId"] = 0.0f;

			if (!client->HasAlliance())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not a member of an alliance.";

				gserver->SendObject(c, obj2);
				return;
			}
			LOCK(M_ALLIANCELIST);
			Alliance * aliance = client->GetAlliance();
			if (aliance->m_name != alliancename)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Error.";

				gserver->SendObject(c, obj2);
				return;
			}

			aliance->m_note = makesafe(notetext);
			aliance->m_intro = makesafe(infotext);

			UNLOCK(M_ALLIANCELIST);

			data2["ok"] = 1;

			gserver->SendObject(c, obj2);
			return;
		}
		if (command == "getPowerFromAlliance")
		{
			obj2["cmd"] = "alliance.getPowerFromAlliance";
			data2["packageId"] = 0.0f;

			if (!client->HasAlliance())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not a member of an alliance.";

				gserver->SendObject(c, obj2);
				return;
			}
			data2["ok"] = 1;
			data2["level"] = client->m_alliancerank;

			gserver->SendObject(c, obj2);
			return;
		}
		if (command == "resetTopPowerForAlliance")
		{
			obj2["cmd"] = "alliance.resetTopPowerForAlliance";
			data2["packageId"] = 0.0f;

			if (!client->HasAlliance())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not a member of an alliance.";

				gserver->SendObject(c, obj2);
				return;
			}

			if (client->m_alliancerank != DEF_ALLIANCEHOST)
			{
				//you're not the host.. what are you doing?
				data2["ok"] = -42;
				data2["errorMsg"] = "You are not entitled to operate.";

				gserver->SendObject(c, obj2);
				return;
			}

			string passtoname = data["userName"];

			Alliance * alliance = client->GetAlliance();
			if (alliance->HasMember(passtoname))
			{
				//member found
				Client * tclient = gserver->GetClientByName(passtoname);
				if (tclient->m_alliancerank != DEF_ALLIANCEVICEHOST)
				{
					data2["ok"] = -88;
					data2["errorMsg"] = "The Host title of the Alliance can only be transferred to Vice Host. You need to promote this player first.";

					gserver->SendObject(c, obj2);
					return;
				}
				//everything checks out. target is vice host and you are host
				tclient->m_alliancerank = DEF_ALLIANCEHOST;
				client->m_alliancerank = DEF_ALLIANCEVICEHOST;
				client->PlayerUpdate();
				tclient->PlayerUpdate();
				alliance->m_owner = tclient->m_playername;

				data2["ok"] = 1;

				gserver->SendObject(c, obj2);
				return;
			}
			else
			{
				data2["ok"] = -41;
				data2["errorMsg"] = "Player " + passtoname + " doesn't exist.";

				gserver->SendObject(c, obj2);
				return;
			}
		}
		if ((command == "agreeComeinAllianceByLeader"))
		{
			//TODO permission check?
			obj2["cmd"] = "alliance.agreeComeinAllianceByLeader";
			data2["packageId"] = 0.0f;

			if (!client->HasAlliance())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not a member of an alliance.";

				gserver->SendObject(c, obj2);
				return;
			}

			MULTILOCK(M_ALLIANCELIST, M_CLIENTLIST);
			string username = data["userName"];
			Client * invitee = gserver->GetClientByName(username);

			if (invitee == 0)
			{
				data2["ok"] = -41;
				string msg;
				msg = "Player ";
				msg += username;
				msg += " doesn't exist.";
				data2["errorMsg"] = msg;

				gserver->SendObject(c, obj2);
				UNLOCK(M_ALLIANCELIST);
				UNLOCK(M_CLIENTLIST);
				return;
			}
			if (invitee->HasAlliance())
			{
				data2["ok"] = -99;
				string msg;
				msg = username;
				msg += " is already a member of other alliance.";
				data2["errorMsg"] = msg;

				gserver->SendObject(c, obj2);
				UNLOCK(M_ALLIANCELIST);
				UNLOCK(M_CLIENTLIST);
				return;
			}
			data2["ok"] = 1;

			Alliance * alliance = client->GetAlliance();

			alliance->UnRequestJoin(username);
			gserver->m_alliances->JoinAlliance(alliance->m_allianceid, invitee);

			UNLOCK(M_ALLIANCELIST);
			UNLOCK(M_CLIENTLIST);

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "setPowerForUserByAlliance"))
		{
			obj2["cmd"] = "alliance.setPowerForUserByAlliance";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			int8_t type = data["typeId"];
			string username = data["userName"];

			if (!client->HasAlliance())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not a member of an alliance.";

				gserver->SendObject(c, obj2);
				return;
			}

			MULTILOCK(M_ALLIANCELIST, M_CLIENTLIST);
			Client * tar = gserver->GetClientByName(username);

			if (!tar || !client->GetAlliance()->HasMember(username))
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Member does not exist.";

				gserver->SendObject(c, obj2);
				UNLOCK(M_ALLIANCELIST);
				UNLOCK(M_CLIENTLIST);
				return;
			}
			//TODO: Set limits to rank counts?
			gserver->m_alliances->SetRank(client->m_allianceid, tar, type);

			UNLOCK(M_ALLIANCELIST);
			UNLOCK(M_CLIENTLIST);

			gserver->SendObject(c, obj2);

			//send alliance message
			string msg = client->m_playername + " promotes " + tar->m_playername + " to " + AllianceCore::GetAllianceRank(type) + ".";
			client->GetAlliance()->SendAllianceMessage(msg, false, false);
			return;
		}
		if ((command == "kickOutMemberfromAlliance"))
		{
			//TODO permission check
			obj2["cmd"] = "alliance.kickOutMemberfromAlliance";
			data2["packageId"] = 0.0f;

			if (!client->HasAlliance())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not a member of an alliance.";

				gserver->SendObject(c, obj2);
				return;
			}

			string username = data["userName"];

			MULTILOCK(M_ALLIANCELIST, M_CLIENTLIST);
			Client * tar = gserver->GetClientByName(username);

			if (!tar || !client->GetAlliance()->HasMember(username))
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Member does not exist.";

				gserver->SendObject(c, obj2);
				UNLOCK(M_ALLIANCELIST);
				UNLOCK(M_CLIENTLIST);
				return;
			}

			if (!gserver->m_alliances->RemoveFromAlliance(client->m_allianceid, tar))
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Could not kick out member.";

				gserver->SendObject(c, obj2);
				UNLOCK(M_ALLIANCELIST);
				UNLOCK(M_CLIENTLIST);
				return;
			}
			
			UNLOCK(M_ALLIANCELIST);
			UNLOCK(M_CLIENTLIST);

			gserver->SendObject(c, obj2);
			return;
		}
	}
#pragma endregion
#pragma region tech
	if ((cmdtype == "tech"))
	{
		if ((command == "getResearchList"))
		{
			VERIFYCASTLEID();

			int castleid = data["castleId"];

			obj2["cmd"] = "tech.getResearchList";
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			amf3array researchbeans = amf3array();

			for (int i = 0; i < 25; ++i)
			{
				if (gserver->m_researchconfig[i][0].time > 0)
				{
					int level = client->GetResearchLevel(i);


					amf3object parent;
					amf3object conditionbean;

					double costtime = gserver->m_researchconfig[i][level].time;
					double mayorinf = 1;
					if (pcity->m_mayor)
						mayorinf = pow(0.995, pcity->m_mayor->GetStratagem());

					costtime = ( costtime ) * ( mayorinf );

					conditionbean["time"] = floor(costtime);
					conditionbean["destructTime"] = 0;
					conditionbean["wood"] = gserver->m_researchconfig[i][level].wood;
					conditionbean["food"] = gserver->m_researchconfig[i][level].food;
					conditionbean["iron"] = gserver->m_researchconfig[i][level].iron;
					conditionbean["gold"] = gserver->m_researchconfig[i][level].gold;
					conditionbean["stone"] = gserver->m_researchconfig[i][level].stone;

					amf3array buildings = amf3array();
					amf3array items = amf3array();
					amf3array techs = amf3array();


					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_researchconfig[i][level].buildings[a].id > 0)
						{
							amf3object ta = amf3object();
							ta["level"] = gserver->m_researchconfig[i][level].buildings[a].level;
							int temp = ((PlayerCity*)client->m_city.at(client->m_currentcityindex))->GetBuildingLevel(gserver->m_researchconfig[i][level].buildings[a].id);
							ta["curLevel"] = temp;
							ta["successFlag"] = temp>=gserver->m_researchconfig[i][level].buildings[a].level?true:false;
							ta["typeId"] = gserver->m_researchconfig[i][level].buildings[a].id;
							buildings.Add(ta);
						}
					}
					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_researchconfig[i][level].items[a].id > 0)
						{
							amf3object ta = amf3object();
							int temp = client->m_items[gserver->m_researchconfig[i][level].items[a].level].count;
							ta["curNum"] = temp;
							ta["num"] = gserver->m_researchconfig[i][level].items[a].level;
							ta["successFlag"] = temp>=gserver->m_researchconfig[i][level].items[a].level?true:false;
							ta["id"] = gserver->m_items[gserver->m_researchconfig[i][level].items[a].id].name;
							items.Add(ta);
						}
					}
					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_researchconfig[i][level].techs[a].id > 0)
						{
							amf3object ta = amf3object();
							ta["level"] = gserver->m_researchconfig[i][level].techs[a].level;
							int temp = client->GetResearchLevel(gserver->m_researchconfig[i][level].techs[a].id);
							ta["curLevel"] = temp;
							ta["successFlag"] = temp>=gserver->m_researchconfig[i][level].techs[a].level?true:false;
							ta["typeId"] = gserver->m_researchconfig[i][level].techs[a].id;
							techs.Add(ta);
						}
					}

					conditionbean["buildings"] = buildings;
					conditionbean["items"] = items;
					conditionbean["techs"] = techs;
					conditionbean["population"] = gserver->m_researchconfig[i][level].population;
					parent["startTime"] = (double)client->m_research[i].starttime;
					parent["castleId"] = (double)client->m_research[i].castleid;
					// TODO verify if works with multiple academies
					if ((client->m_research[i].endtime != 0) && (client->m_research[i].endtime < timestamp))
					{
						 //if research was sped up and isn't processed through the timer thread yet, emulate it being completed
						int research = level+1;
						parent["level"] = research;
						parent["upgradeing"] = false;
						parent["endTime"] = 0;
						int blevel = pcity->GetBuildingLevel(B_ACADEMY);//academy
						if (blevel != 0)
						{
							parent["avalevel"] = blevel<=research?blevel:research;
						}
						parent["permition"] = true;
					}
					else
					{
						parent["level"] = level;
						parent["upgradeing"] = ((client->m_research[i].endtime != 0) && (client->m_research[i].endtime > timestamp));
						parent["endTime"] = (client->m_research[i].endtime > 0)?(client->m_research[i].endtime-client->m_lag):(client->m_research[i].endtime);// HACK: attempt to fix "lag" issues
						parent["avalevel"] = pcity->GetTechLevel(i);
						parent["permition"] = !pcity->m_researching;
					}
					parent["conditionBean"] = conditionbean;
					parent["typeId"] = i;
					researchbeans.Add(parent);
				}
			}


			data2["acailableResearchBeans"] = researchbeans;
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;


			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "research"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];
			int techid = data["techId"];

			if (techid < 0 || techid > 25 || client->m_research[techid].level >= 10 || gserver->m_researchconfig[techid][client->m_research[techid].level].time == 0)
			{
				obj2["cmd"] = "tech.research";
				data2["ok"] = -99;
				data2["packageId"] = 0.0f;
				data2["errorMsg"] = "Invalid technology.";

				gserver->SendObject(c, obj2);
				return;
			}

			Client::stResearch * research;
			server::stBuildingConfig * researchconfig;

			research = &client->m_research[techid];
			researchconfig = &gserver->m_researchconfig[techid][research->level];


			if (!pcity->m_researching)
			{
				if ((researchconfig->food > pcity->m_resources.food)
					|| (researchconfig->wood > pcity->m_resources.wood)
					|| (researchconfig->stone > pcity->m_resources.stone)
					|| (researchconfig->iron > pcity->m_resources.iron)
					|| (researchconfig->gold > pcity->m_resources.gold))
				{
					obj2["cmd"] = "tech.research";
					data2["ok"] = -99;
					data2["packageId"] = 0.0f;
					data2["errorMsg"] = "Not enough resources.";

					gserver->SendObject(c, obj2);
					return;
				}
				obj2["cmd"] = "tech.research";
				data2["ok"] = 1;
				data2["packageId"] = 0.0f;

				pcity->m_resources.food -= researchconfig->food;
				pcity->m_resources.wood -= researchconfig->wood;
				pcity->m_resources.stone -= researchconfig->stone;
				pcity->m_resources.iron -= researchconfig->iron;
				pcity->m_resources.gold -= researchconfig->gold;

				research->castleid = castleid;


				double costtime = researchconfig->time;
				double mayorinf = 1;
				if (pcity->m_mayor)
					mayorinf = pow(0.995, pcity->m_mayor->GetStratagem());

				costtime = ( costtime ) * ( mayorinf );

				research->endtime = timestamp + floor(costtime)*1000;

				research->starttime = timestamp;

				server::stResearchAction * ra = new server::stResearchAction;

				server::stTimedEvent te;
				ra->city = pcity;
				ra->client = client;
				ra->researchid = techid;
				te.data = ra;
				te.type = DEF_TIMEDRESEARCH;
				pcity->m_researching = true;

				gserver->AddTimedEvent(te);

				amf3object parent;
				amf3object conditionbean;

				conditionbean["time"] = floor(costtime);
				conditionbean["destructTime"] = 0.0f;
				conditionbean["wood"] = researchconfig->wood;
				conditionbean["food"] = researchconfig->food;
				conditionbean["iron"] = researchconfig->iron;
				conditionbean["gold"] = researchconfig->gold;
				conditionbean["stone"] = researchconfig->stone;

				amf3array buildings = amf3array();
				amf3array items = amf3array();
				amf3array techs = amf3array();


				for (int a = 0; a < 3; ++a)
				{
					if (researchconfig->buildings[a].id > 0)
					{
						amf3object ta = amf3object();
						ta["level"] = researchconfig->buildings[a].level;
						int temp = ((PlayerCity*)client->m_city.at(client->m_currentcityindex))->GetBuildingLevel(researchconfig->buildings[a].id);
						ta["curLevel"] = temp;
						ta["successFlag"] = temp>=researchconfig->buildings[a].level?true:false;
						ta["typeId"] = researchconfig->buildings[a].id;
						buildings.Add(ta);
					}
				}
				for (int a = 0; a < 3; ++a)
				{
					if (researchconfig->items[a].id > 0)
					{
						amf3object ta = amf3object();
						int temp = client->m_items[researchconfig->items[a].level].count;
						ta["curNum"] = temp;
						ta["num"] = researchconfig->items[a].level;
						ta["successFlag"] = temp>=researchconfig->items[a].level?true:false;
						ta["id"] = gserver->m_items[researchconfig->items[a].id].name;
						items.Add(ta);
					}
				}
				for (int a = 0; a < 3; ++a)
				{
					if (researchconfig->techs[a].id > 0)
					{
						amf3object ta = amf3object();
						ta["level"] = researchconfig->techs[a].level;
						int temp = client->GetResearchLevel(researchconfig->techs[a].id);
						ta["curLevel"] = temp;
						ta["successFlag"] = temp>=researchconfig->techs[a].level?true:false;
						ta["typeId"] = researchconfig->techs[a].id;
						techs.Add(ta);
					}
				}

				conditionbean["buildings"] = buildings;
				conditionbean["items"] = items;
				conditionbean["techs"] = techs;
				conditionbean["population"] = researchconfig->population;
				parent["startTime"] = (double)research->starttime;
				parent["castleId"] = (double)research->castleid;
				parent["level"] = client->GetResearchLevel(techid);
				parent["conditionBean"] = conditionbean;
				parent["avalevel"] = pcity->GetTechLevel(techid);
				parent["upgradeing"] = (bool)(research->starttime != 0);
				parent["endTime"] = (research->endtime > 0)?(research->endtime-client->m_lag):(research->endtime);// HACK: attempt to fix "lag" issues
				parent["typeId"] = techid;
				parent["permition"] = !pcity->m_researching;

				data2["tech"] = parent;

				gserver->SendObject(c, obj2);

				pcity->ResourceUpdate();
				return;
			}
			else
			{
				obj2["cmd"] = "tech.research";
				data2["ok"] = -99;
				data2["packageId"] = 0.0f;
				data2["errorMsg"] = "Research already in progress.";

				gserver->SendObject(c, obj2);
				return;
			}
		}
		if ((command == "cancelResearch"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];
			PlayerCity * pcity = client->GetCity(castleid);
			uint16_t techid = 0;

			for (int i = 0; i < 25; ++i)
			{
				if (client->m_research[i].castleid == castleid)
				{
					techid = i;
					break;
				}
			}

			if (!pcity || !pcity->m_researching || techid == 0)
			{
				obj2["cmd"] = "tech.cancelResearch";
				data2["ok"] = -99;
				data2["packageId"] = 0.0f;
				data2["errorMsg"] = "Invalid city.";

				gserver->SendObject(c, obj2);
				return;
			}

			Client::stResearch * research;
			server::stBuildingConfig * researchconfig;


			research = &client->m_research[techid];
			researchconfig = &gserver->m_researchconfig[techid][research->level];


			if (pcity->m_researching)
			{
				obj2["cmd"] = "tech.cancelResearch";
				data2["ok"] = 1;
				data2["packageId"] = 0.0f;

				pcity->m_resources.food += double(researchconfig->food)/3;
				pcity->m_resources.wood += double(researchconfig->wood)/3;
				pcity->m_resources.stone += double(researchconfig->stone)/3;
				pcity->m_resources.iron += double(researchconfig->iron)/3;
				pcity->m_resources.gold += double(researchconfig->gold)/3;

				list<server::stTimedEvent>::iterator iter;


				LOCK(M_TIMEDLIST);
				for (iter = gserver->researchlist.begin(); iter != gserver->researchlist.end(); )
				{
					server::stResearchAction * ra = (server::stResearchAction *)iter->data;
					PlayerCity * city = ra->city;
					if (city->m_castleid == castleid)
					{
						ra->researchid = 0;
						break;
					}
					++iter;
				}
				UNLOCK(M_TIMEDLIST);

				research->castleid = 0;
				research->endtime = 0.0f;
				research->starttime = 0.0f;

				pcity->m_researching = false;

				gserver->SendObject(c, obj2);

				pcity->ResourceUpdate();
				return;
			}
		}
		if ((command == "speedUpResearch"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			char * speeditemid;
			int castleid = data["castleId"];

			speeditemid = data["itemId"];

			Client::stResearch * research = 0;
			for (int i = 0; i < 25; ++i)
			{
				if (client->m_research[i].castleid == pcity->m_castleid)
				{
					research = &client->m_research[i];
					break;
				}
			}
			if (research == 0)
			{
				obj2["cmd"] = "tech.speedUpResearch";
				data2["packageId"] = 0.0f;
				data2["ok"] = -99;
				data2["errorMsg"] = "Invalid tech.";// TODO get error message for city not having research -- tech.speedUpResearch

				gserver->SendObject(c, obj2);
				return;
			}

			if (!strcmp(speeditemid, "free.speed"))
			{
				//check if under 5 mins

				if ((research->endtime - research->starttime) <= 5*60*1000)
				{
					//under 5 mins
					obj2["cmd"] = "tech.speedUpResearch";
					data2["packageId"] = 0.0f;
					data2["ok"] = 1;

					gserver->SendObject(c, obj2);

					research->endtime -= 5*60*1000;
				}
				else
				{
					//over 5 mins
					obj2["cmd"] = "tech.speedUpResearch";
					data2["packageId"] = 0.0f;
					data2["ok"] = -99;
					data2["errorMsg"] = "Invalid speed up.";// TODO get 5 min speed up error - tech.speedUpResearch

					gserver->SendObject(c, obj2);
					return;
				}
			}
			else
			{
				//is not under 5 mins, apply an item
				int itemcount = client->GetItemCount((string)speeditemid);

				amf3object obj3 = amf3object();
				obj3["cmd"] = "tech.speedUpResearch";
				obj3["data"] = amf3object();
				amf3object & data3 = obj3["data"];
				data3["packageId"] = 0.0f;
				data3["ok"] = 99;// TODO find error value -- tech.speedUpResearch
				data3["errorMsg"] = "Not enough cents.";

				int cents = 0;
				int reducetime = 0;

				obj2["cmd"] = "tech.speedUpResearch";
				data2["packageId"] = 0.0f;
				data2["ok"] = 1;

				gserver->SendObject(c, obj2);

				// TODO reduce time on building being sped up -- tech.speedUpResearch

				if (!strcmp(speeditemid, "consume.2.a"))
				{
					cents = 5;
					reducetime = 15*60*1000;
				}
				else if (!strcmp(speeditemid, "consume.2.b"))
				{
					cents = 10;
					reducetime = 60*60*1000;
				}
				else if (!strcmp(speeditemid, "consume.2.b.1"))
				{
					cents = 20;
					reducetime = 150*60*1000;
				}
				else if (!strcmp(speeditemid, "consume.2.c"))
				{
					cents = 50;
					reducetime = 8*60*60*1000;
				}
				else if (!strcmp(speeditemid, "consume.2.c.1"))
				{
					cents = 80;
					reducetime = ((rand()%21)+10)*60*60*1000;
				}
				else if (!strcmp(speeditemid, "consume.2.d"))
				{
					cents = 120;
					reducetime = ((research->endtime - research->starttime)*0.30);
				}
				else if (!strcmp(speeditemid, "coins.speed"))
				{
					cents = 200;
					reducetime = (research->endtime - research->starttime);
				}

				if (itemcount <= 0)
				{
					if (client->m_cents < cents)
					{
						gserver->SendObject(c, obj3); // not enough item and not enough cents
						return;
					}
					//not enough item, but can buy with cents
					client->m_cents -= cents;
					client->PlayerUpdate();
				}
				else
				{ //has item
					client->SetItem((string)speeditemid, -1);
				}

				research->endtime -= reducetime;
				return;
			}
			return;
		}
		if ((command == "getCoinsNeed"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int positionid = data["positionId"];
			int castleid = data["castleId"];

			obj2["cmd"] = "castle.getCoinsNeed";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;
			data2["coinsNeed"] = 200;// TODO calculate correct cents cost based on time

			gserver->SendObject(c, obj2);
			return;
		}
	}
#pragma endregion
#pragma region rank
	if ((cmdtype == "rank"))
	{
		if ((command == "getPlayerRank"))
		{
			int pagesize = data["pageSize"];
			string key = data["key"];
			int sorttype = data["sortType"];
			int pageno = data["pageNo"];

			obj2["cmd"] = "rank.getPlayerRank";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;
			list<server::stClientRank> * ranklist;
			amf3array beans = amf3array();
			LOCK(M_RANKEDLIST);
			switch (sorttype)
			{
			case 1:
				ranklist = &gserver->m_titlerank;
				break;
			case 2:
				ranklist = &gserver->m_prestigerank;
				break;
			case 3:
				ranklist = &gserver->m_honorrank;
				break;
			case 4:
				ranklist = &gserver->m_citiesrank;
				break;
			case 5:
				ranklist = &gserver->m_populationrank;
				break;
			default:
				ranklist = &gserver->m_prestigerank;
				break;
			}
			list<server::stClientRank>::iterator iter;
			if (pagesize <= 0 || pagesize > 20 || pageno < 0 || pageno > 100000)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Invalid data.";
				gserver->SendObject(c, obj2);
				return;
			}

			if (key.length() > 0)
			{
				//search term given
				ranklist = (list<server::stClientRank>*)gserver->DoRankSearch(key, 1, ranklist, pageno, pagesize);//1 = client
			}

			if ((pageno-1)*pagesize > ranklist->size())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Invalid page.";
				gserver->SendObject(c, obj2);
				UNLOCK(M_RANKEDLIST);
				return;
			}
			iter = ranklist->begin();
			for (int i = 0; i < (pageno-1)*pagesize; ++i)
				iter++;

			int rank = (pageno-1)*pagesize + 1;
			data2["pageNo"] = pageno;
			data2["pageSize"] = pagesize;
			if ((ranklist->size()%pagesize) == 0)
				data2["totalPage"] = (ranklist->size()/pagesize);
			else
				data2["totalPage"] = (ranklist->size()/pagesize)+1;
			for (  ; iter != ranklist->end() && pagesize != 0; ++iter )
			{
				pagesize--;
				amf3object temp = amf3object();
				temp["createrTime"] = 0;
				if (iter->client->m_allianceid > 0)
				{
					temp["alliance"] = iter->client->GetAlliance()->m_name;
					temp["allianceLevel"] = AllianceCore::GetAllianceRank(iter->client->m_alliancerank);
					temp["levelId"] = iter->client->m_alliancerank;
				}
				temp["office"] = iter->client->m_office;
				temp["sex"] = iter->client->m_sex;
				temp["honor"] = iter->client->m_honor;
				temp["bdenyotherplayer"] = iter->client->m_bdenyotherplayer;
				temp["id"] = iter->client->m_accountid;
				temp["accountName"] = "";
				temp["prestige"] = iter->client->m_prestige;
				temp["faceUrl"] = iter->client->m_faceurl;
				temp["flag"] = iter->client->m_flag;
				temp["userId"] = iter->client->m_parentid;
				temp["userName"] = iter->client->m_playername;
				temp["castleCount"] = iter->client->m_citycount;
				temp["titleId"] = iter->client->m_title;
				temp["medal"] = 0;
				temp["ranking"] = iter->rank;
				temp["lastLoginTime"] = 0;
				temp["population"] = iter->client->m_population;
				beans.Add(temp);
			}
			UNLOCK(M_RANKEDLIST);

			data2["beans"] = beans;
			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "getAllianceRank"))
		{
			int pagesize = data["pageSize"];
			string key = data["key"];
			int sorttype = data["sortType"];
			int pageno = data["pageNo"];

			obj2["cmd"] = "rank.getAllianceRank";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;
			list<stAlliance> * ranklist;
			amf3array beans = amf3array();
			LOCK(M_RANKEDLIST);
			switch (sorttype)
			{
			case 1:
				ranklist = &gserver->m_alliances->m_membersrank;
				break;
			case 2:
				ranklist = &gserver->m_alliances->m_prestigerank;
				break;
			case 3:
				ranklist = &gserver->m_alliances->m_honorrank;
				break;
			default:
				ranklist = &gserver->m_alliances->m_membersrank;
				break;
			}
			list<stAlliance>::iterator iter;
			if (pagesize <= 0 || pagesize > 20 || pageno < 0 || pageno > 100000)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Invalid data.";
				gserver->SendObject(c, obj2);
				UNLOCK(M_RANKEDLIST);
				return;
			}

			if (key.length() > 0)
			{
				//search term given
				ranklist = (list<stAlliance>*)gserver->DoRankSearch(key, 4, ranklist, pageno, pagesize);//1 = client
			}

			if ((pageno-1)*pagesize > ranklist->size())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Invalid page.";
				gserver->SendObject(c, obj2);
				UNLOCK(M_RANKEDLIST);
				return;
			}
			iter = ranklist->begin();
			for (int i = 0; i < (pageno-1)*pagesize; ++i)
				iter++;

			int rank = (pageno-1)*pagesize + 1;
			data2["pageNo"] = pageno;
			data2["pageSize"] = pagesize;
			if ((ranklist->size()%pagesize) == 0)
				data2["totalPage"] = (ranklist->size()/pagesize);
			else
				data2["totalPage"] = (ranklist->size()/pagesize)+1;
			for (  ; iter != ranklist->end() && pagesize != 0; ++iter )
			{
				pagesize--;
				amf3object temp = amf3object();
				temp["member"] = iter->ref->m_currentmembers;
				temp["prestige"] = iter->ref->m_prestige;
				temp["rank"] = iter->rank;
				temp["playerName"] = iter->ref->m_owner;
				temp["honor"] = iter->ref->m_honor;
				temp["description"] = iter->ref->m_intro;
				temp["createrName"] = iter->ref->m_founder;
				temp["name"] = iter->ref->m_name;
				temp["city"] = iter->ref->m_citycount;
				beans.Add(temp);
			}
			UNLOCK(M_RANKEDLIST);

			data2["beans"] = beans;
			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "getHeroRank"))
		{
			int pagesize = data["pageSize"];
			string key = data["key"];
			int sorttype = data["sortType"];
			int pageno = data["pageNo"];

			obj2["cmd"] = "rank.getHeroRank";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;
			list<server::stHeroRank> * ranklist;
			amf3array beans = amf3array();
			LOCK(M_RANKEDLIST);
			switch (sorttype)
			{
				case 1:
					ranklist = &gserver->m_herorankgrade;
					break;
				case 2:
					ranklist = &gserver->m_herorankmanagement;
					break;
				case 3:
					ranklist = &gserver->m_herorankpower;
					break;
				case 4:
					ranklist = &gserver->m_herorankstratagem;
					break;
				default:
					ranklist = &gserver->m_herorankgrade;
					break;
			}
			list<server::stHeroRank>::iterator iter;
			if (pagesize <= 0 || pagesize > 20 || pageno < 0 || pageno > 100000)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Invalid data.";
				gserver->SendObject(c, obj2);
				UNLOCK(M_RANKEDLIST);
				return;
			}

			if (key.length() > 0)
			{
				//search term given
				ranklist = (list<server::stHeroRank>*)gserver->DoRankSearch(key, 2, ranklist, pageno, pagesize);//1 = client
			}

			if ((pageno-1)*pagesize > ranklist->size())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Invalid page.";
				gserver->SendObject(c, obj2);
				UNLOCK(M_RANKEDLIST);
				return;
			}
			iter = ranklist->begin();
			for (int i = 0; i < (pageno-1)*pagesize; ++i)
				iter++;

			int rank = (pageno-1)*pagesize + 1;
			data2["pageNo"] = pageno;
			data2["pageSize"] = pagesize;
			if ((ranklist->size()%pagesize) == 0)
				data2["totalPage"] = (ranklist->size()/pagesize);
			else
				data2["totalPage"] = (ranklist->size()/pagesize)+1;
			for (  ; iter != ranklist->end() && pagesize != 0; ++iter )
			{
				pagesize--;
				amf3object temp = amf3object();
				temp["rank"] = iter->rank;
				temp["stratagem"] = iter->stratagem;
				temp["name"] = iter->name;
				temp["power"] = iter->power;
				temp["grade"] = iter->grade;
				temp["management"] = iter->management;
				temp["kind"] = iter->kind;
				beans.Add(temp);
			}
			UNLOCK(M_RANKEDLIST);

			data2["beans"] = beans;
			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "getCastleRank"))
		{
			int pagesize = data["pageSize"];
			string key = data["key"];
			int sorttype = data["sortType"];
			int pageno = data["pageNo"];

			obj2["cmd"] = "rank.getCastleRank";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;
			list<server::stCastleRank> * ranklist;
			amf3array beans = amf3array();
			LOCK(M_RANKEDLIST);
			switch (sorttype)
			{
			case 1:
				ranklist = &gserver->m_castlerankpopulation;
				break;
			case 2:
				ranklist = &gserver->m_castleranklevel;
				break;
			default:
				ranklist = &gserver->m_castlerankpopulation;
				break;
			}
			list<server::stCastleRank>::iterator iter;
			if (pagesize <= 0 || pagesize > 20 || pageno < 0 || pageno > 100000)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Invalid data.";
				gserver->SendObject(c, obj2);
				UNLOCK(M_RANKEDLIST);
				return;
			}

			if (key.length() > 0)
			{
				//search term given
				ranklist = (list<server::stCastleRank>*)gserver->DoRankSearch(key, 3, ranklist, pageno, pagesize);//1 = client
			}

			if ((pageno-1)*pagesize > ranklist->size())
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Invalid page.";
				gserver->SendObject(c, obj2);
				UNLOCK(M_RANKEDLIST);
				return;
			}
			iter = ranklist->begin();
			for (int i = 0; i < (pageno-1)*pagesize; ++i)
				iter++;

			int rank = (pageno-1)*pagesize + 1;
			data2["pageNo"] = pageno;
			data2["pageSize"] = pagesize;
			if ((ranklist->size()%pagesize) == 0)
				data2["totalPage"] = (ranklist->size()/pagesize);
			else
				data2["totalPage"] = (ranklist->size()/pagesize)+1;
			for (  ; iter != ranklist->end() && pagesize != 0; ++iter )
			{
				pagesize--;
				amf3object temp = amf3object();
				temp["alliance"] = iter->alliance;
				temp["rank"] = iter->rank;
				temp["level"] = iter->level;
				temp["name"] = iter->name;
				temp["grade"] = iter->grade;
				temp["kind"] = iter->kind;
				temp["population"] = iter->population;
				beans.Add(temp);
			}
			UNLOCK(M_RANKEDLIST);

			data2["beans"] = beans;
			gserver->SendObject(c, obj2);
			return;
		}
	}
#pragma endregion
#pragma region trade
	if ((cmdtype == "trade"))
	{
		if ((command == "searchTrades"))
		{
			int restype = data["resType"];

			if (restype < 0 || restype > 3)
			{
				obj2["cmd"] = "trade.searchTrades";
				data2["packageId"] = 0.0f;
				data2["ok"] = -99;
				data2["errorMsg"] = "Not a valid resource type.";

				gserver->SendObject(c, obj2);
				return;
			}


			obj2["cmd"] = "trade.searchTrades";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			gserver->SendObject(c, obj2);
			return;
		}
	}
#pragma endregion
#pragma region shop
	if ((cmdtype == "shop"))
	{
		if ((command == "buy"))
		{
			int amount = data["amount"];
			string itemid = data["itemId"];

			for (int i = 0; i < DEF_MAXITEMS; ++i)
			{
				if (itemid == gserver->m_items[i].name)
				{
					if (client->m_cents < gserver->m_items[i].cost*amount)
					{
						obj2["cmd"] = "shop.buy";
						data2["packageId"] = 0.0f;
						data2["ok"] = -28;
						data2["errorMsg"] = "Insufficient game coins.";

						gserver->SendObject(c, obj2);
						return;
					}
					client->m_cents -= gserver->m_items[i].cost*amount;

					client->SetItem(itemid, amount);

					client->PlayerUpdate();

					obj2["cmd"] = "shop.buy";
					data2["packageId"] = 0.0f;
					data2["ok"] = 1;

					gserver->SendObject(c, obj2);
					return;
				}
			}
			obj2["cmd"] = "shop.buy";
			data2["packageId"] = 0.0f;
			data2["ok"] = -99;
			data2["errorMsg"] = "Item does not exist.";

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "getBuyResourceInfo"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();


			int amount = data["amount"];
			string itemid = data["itemId"];

			obj2["cmd"] = "shop.getBuyResourceInfo";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			amf3object buyresource = amf3object();
			buyresource["woodRemain"] = 10000000;
			buyresource["forWood"] = 100000;
			buyresource["stoneRemain"] = 10000000;
			buyresource["forStone"] = 50000;
			buyresource["ironRemain"] = 10000000;
			buyresource["forIron"] = 40000;
			buyresource["foodRemain"] = 10000000;
			buyresource["forFood"] = 100000;

			data2["buyResourceBean"] = buyresource;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "buyResource"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int fooduse = data["foodUse"];
			int wooduse = data["woodUse"];
			int stoneuse = data["stoneUse"];
			int ironuse = data["ironUse"];

			if (fooduse + wooduse + stoneuse + ironuse > client->m_cents)
			{
				obj2["cmd"] = "shop.buyResource";
				data2["packageId"] = 0.0f;
				data2["ok"] = -24;
				data2["errorMsg"] = "Insufficient game coins.";


				gserver->SendObject(c, obj2);
				return;
			}

			client->m_cents -= fooduse + wooduse + stoneuse + ironuse;
			pcity->m_resources.food += fooduse*100000;
			pcity->m_resources.wood += wooduse*100000;
			pcity->m_resources.stone += stoneuse*50000;
			pcity->m_resources.iron += ironuse*40000;

			obj2["cmd"] = "shop.buyResource";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			client->PlayerUpdate();
			pcity->ResourceUpdate();

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "useGoods"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int num = data["num"];
			string itemid = data["itemId"];

			if (client->GetItemCount(itemid) < num)
			{
				obj2["cmd"] = "shop.useGoods";
				data2["packageId"] = 0.0f;
				data2["ok"] = -24;
				data2["errorMsg"] = "Insufficient items.";


				gserver->SendObject(c, obj2);
				return;
			}
			ShopUseGoods(data, client);
			return;
		}
	}
#pragma endregion
#pragma region mail
	if ((cmdtype == "mail"))
	{

	}
#pragma endregion
#pragma region fortifications
	if ((cmdtype == "fortifications"))
	{
		if ((command == "getProduceQueue"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];
			
			obj2["cmd"] = "fortifications.getProduceQueue";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			amf3array producequeue = amf3array();
			PlayerCity::stTroopQueue * train = pcity->GetBarracksQueue(-2);
			amf3object producequeueobj = amf3object();
			amf3array producequeueinner = amf3array();
			amf3object producequeueinnerobj = amf3object();

			list<PlayerCity::stTroopTrain>::iterator iter;

			if (train->queue.size() > 0)
				for (iter = train->queue.begin(); iter != train->queue.end(); ++iter)
				{
					producequeueinnerobj["num"] = iter->count;
					producequeueinnerobj["queueId"] = iter->queueid;
					producequeueinnerobj["endTime"] = (iter->endtime > 0)?(iter->endtime-client->m_lag):(iter->endtime);// HACK: attempt to fix "lag" issues
					producequeueinnerobj["type"] = iter->troopid;
					producequeueinnerobj["costTime"] = iter->costtime/1000;
					producequeueinner.Add(producequeueinnerobj);
				}
				producequeueobj["allProduceQueue"] = producequeueinner;
				producequeueobj["positionId"] = -2;
				producequeue.Add(producequeueobj);

			data2["allProduceQueue"] = producequeue;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "getFortificationsProduceList"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];

			obj2["cmd"] = "fortifications.getFortificationsProduceList";
			amf3array fortlist = amf3array();


			for (int i = 0; i < 20; ++i)
			{
				if (gserver->m_troopconfig[i].inside != 2)
					continue;
				if (gserver->m_troopconfig[i].time > 0)
				{
					amf3object parent;
					amf3object conditionbean;

					double costtime = gserver->m_troopconfig[i].time;
					double mayorinf = 1;
					if (pcity->m_mayor)
						mayorinf = pow(0.995, pcity->m_mayor->GetManagement());

					costtime = ( costtime ) * ( mayorinf ) * ( pow(0.9, client->GetResearchLevel(T_CONSTRUCTION)) );

					conditionbean["time"] = floor(costtime);
					conditionbean["destructTime"] = 0;
					conditionbean["wood"] = gserver->m_troopconfig[i].wood;
					conditionbean["food"] = gserver->m_troopconfig[i].food;
					conditionbean["iron"] = gserver->m_troopconfig[i].iron;
					conditionbean["gold"] = gserver->m_troopconfig[i].gold;
					conditionbean["stone"] = gserver->m_troopconfig[i].stone;

					amf3array buildings = amf3array();
					amf3array items = amf3array();
					amf3array techs = amf3array();

					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_troopconfig[i].buildings[a].id > 0)
						{
							amf3object ta = amf3object();
							ta["level"] = gserver->m_troopconfig[i].buildings[a].level;
							int temp = pcity->GetBuildingLevel(gserver->m_troopconfig[i].buildings[a].id);
							ta["curLevel"] = temp;
							ta["successFlag"] = temp>=gserver->m_troopconfig[i].buildings[a].level?true:false;
							ta["typeId"] = gserver->m_troopconfig[i].buildings[a].id;
							buildings.Add(ta);
						}
					}
					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_troopconfig[i].items[a].id > 0)
						{
							amf3object ta = amf3object();
							int temp = client->m_items[gserver->m_troopconfig[i].items[a].level].count;
							ta["curNum"] = temp;
							ta["num"] = gserver->m_troopconfig[i].items[a].level;
							ta["successFlag"] = temp>=gserver->m_troopconfig[i].items[a].level?true:false;
							ta["id"] = gserver->m_items[gserver->m_troopconfig[i].items[a].id].name;
							items.Add(ta);
						}
					}
					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_troopconfig[i].techs[a].id > 0)
						{
							amf3object ta = amf3object();
							ta["level"] = gserver->m_troopconfig[i].techs[a].level;
							int temp = client->GetResearchLevel(gserver->m_troopconfig[i].techs[a].id);
							ta["curLevel"] = temp;
							ta["successFlag"] = temp>=gserver->m_troopconfig[i].techs[a].level?true:false;
							ta["id"] = gserver->m_troopconfig[i].techs[a].id;
							techs.Add(ta);
						}
					}

					conditionbean["buildings"] = buildings;
					conditionbean["items"] = items;
					conditionbean["techs"] = techs;
					conditionbean["population"] = 0;
					parent["conditionBean"] = conditionbean;
					parent["permition"] = false;
					parent["typeId"] = i;
					fortlist.Add(parent);
				}
			}


			data2["fortList"] = fortlist;
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "produceWallProtect"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];


			obj2["cmd"] = "fortifications.produceWallProtect";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			int trooptype = data["wallProtectType"];
			int num = data["num"];

			stResources res;
			res.food = gserver->m_troopconfig[trooptype].food * num;
			res.wood = gserver->m_troopconfig[trooptype].wood * num;
			res.stone = gserver->m_troopconfig[trooptype].stone * num;
			res.iron = gserver->m_troopconfig[trooptype].iron * num;
			res.gold = gserver->m_troopconfig[trooptype].gold * num;


			if ((res.food > pcity->m_resources.food)
				|| (res.wood > pcity->m_resources.wood)
				|| (res.stone > pcity->m_resources.stone)
				|| (res.iron > pcity->m_resources.iron)
				|| (res.gold > pcity->m_resources.gold))
			{
				data2["ok"] = -1;
				data2["errorMsg"] = "Not enough resources.";
				data2["packageId"] = 0.0f;

				gserver->SendObject(c, obj2);
				return;
			}

			pcity->m_resources -= res;
			pcity->ResourceUpdate();

			if (pcity->AddToBarracksQueue(-2, trooptype, num, false, false) == -1)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Troops could not be trained.";
				data2["packageId"] = 0.0f;

				gserver->SendObject(c, obj2);
				return;
			}

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "cancelFortificationProduce"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];
			int positionid = data["positionId"];
			int queueid = data["queueId"];


			obj2["cmd"] = "fortifications.cancelFortificationProduce";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;


			PlayerCity::stTroopQueue * tq = pcity->GetBarracksQueue(-2);
			list<PlayerCity::stTroopTrain>::iterator iter;

			for (iter = tq->queue.begin(); iter != tq->queue.end(); )
			{
				if (iter->queueid == queueid)
				{
					if (iter->endtime > 0)
					{
						//in progress
						//refund 1/3 resources and set next queue to run
						stResources res;
						res.food = (gserver->m_troopconfig[iter->troopid].food * iter->count)/3;
						res.wood = (gserver->m_troopconfig[iter->troopid].wood * iter->count)/3;
						res.stone = (gserver->m_troopconfig[iter->troopid].stone * iter->count)/3;
						res.iron = (gserver->m_troopconfig[iter->troopid].iron * iter->count)/3;
						res.gold = (gserver->m_troopconfig[iter->troopid].gold * iter->count)/3;

						pcity->m_resources += res;
						pcity->ResourceUpdate();
						tq->queue.erase(iter++);

						iter->endtime = unixtime() + iter->costtime;
					}
					else
					{
						//not in progress
						//refund all resources
						stResources res;
						res.food = gserver->m_troopconfig[iter->troopid].food * iter->count;
						res.wood = gserver->m_troopconfig[iter->troopid].wood * iter->count;
						res.stone = gserver->m_troopconfig[iter->troopid].stone * iter->count;
						res.iron = gserver->m_troopconfig[iter->troopid].iron * iter->count;
						res.gold = gserver->m_troopconfig[iter->troopid].gold * iter->count;

						pcity->m_resources += res;
						tq->queue.erase(iter++);
						pcity->ResourceUpdate();
					}

					gserver->SendObject(c, obj2);
					return;
				}
				++iter;
			}
		}
	}
#pragma endregion
#pragma region troop
	if ((cmdtype == "troop"))
	{
		if ((command == "getProduceQueue"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];

			obj2["cmd"] = "troop.getProduceQueue";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			amf3array producequeue = amf3array();
			for (int i = 0; i < 35; ++i)
			{
				stBuilding * building = pcity->GetBuilding(i);
				if (building->type == B_BARRACKS)
				{
					PlayerCity::stTroopQueue * train = pcity->GetBarracksQueue(i);

					if (train == 0)
					{
						Log("Crash! PlayerCity::stTroopQueue * train = pcity->GetBarracksQueue(%d); - %d", i, __LINE__);
						return;
					}

					amf3object producequeueobj = amf3object();
					amf3array producequeueinner = amf3array();
					amf3object producequeueinnerobj = amf3object();

					list<PlayerCity::stTroopTrain>::iterator iter;

					if (train->queue.size() > 0)
					for (iter = train->queue.begin(); iter != train->queue.end(); ++iter)
					{
						producequeueinnerobj["num"] = iter->count;
						producequeueinnerobj["queueId"] = iter->queueid;
						producequeueinnerobj["endTime"] = (iter->endtime > 0)?(iter->endtime-client->m_lag):(iter->endtime);// HACK: attempt to fix "lag" issues
						producequeueinnerobj["type"] = iter->troopid;
						producequeueinnerobj["costTime"] = iter->costtime/1000;
						producequeueinner.Add(producequeueinnerobj);
					}
					producequeueobj["allProduceQueue"] = producequeueinner;
					producequeueobj["positionId"] = building->id;
					producequeue.Add(producequeueobj);
				}
			}

			data2["allProduceQueue"] = producequeue;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "getTroopProduceList"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];

			obj2["cmd"] = "troop.getTroopProduceList";
			amf3array trooplist = amf3array();


			for (int i = 0; i < 20; ++i)
			{
				if (gserver->m_troopconfig[i].inside != 1)
					continue;
				if (gserver->m_troopconfig[i].time > 0)
				{
					amf3object parent;
					amf3object conditionbean;

					double costtime = gserver->m_troopconfig[i].time;
					double mayorinf = 1;
					if (pcity->m_mayor)
						mayorinf = pow(0.995, pcity->m_mayor->GetPower());

					switch (i)
					{
					case TR_CATAPULT:
					case TR_RAM:
					case TR_TRANSPORTER:
					case TR_BALLISTA:
						costtime = ( costtime ) * ( mayorinf ) * ( pow(0.9, client->GetResearchLevel(T_METALCASTING)) );
						break;
					default:
						costtime = ( costtime ) * ( mayorinf ) * ( pow(0.9, client->GetResearchLevel(T_MILITARYSCIENCE)) );
						break;
					}

					conditionbean["time"] = floor(costtime);
					conditionbean["destructTime"] = 0;
					conditionbean["wood"] = gserver->m_troopconfig[i].wood;
					conditionbean["food"] = gserver->m_troopconfig[i].food;
					conditionbean["iron"] = gserver->m_troopconfig[i].iron;
					conditionbean["gold"] = gserver->m_troopconfig[i].gold;
					conditionbean["stone"] = gserver->m_troopconfig[i].stone;

					amf3array buildings = amf3array();
					amf3array items = amf3array();
					amf3array techs = amf3array();

					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_troopconfig[i].buildings[a].id > 0)
						{
							amf3object ta = amf3object();
							ta["level"] = gserver->m_troopconfig[i].buildings[a].level;
							int temp = pcity->GetBuildingLevel(gserver->m_troopconfig[i].buildings[a].id);
							ta["curLevel"] = temp;
							ta["successFlag"] = temp>=gserver->m_troopconfig[i].buildings[a].level?true:false;
							ta["typeId"] = gserver->m_troopconfig[i].buildings[a].id;
							buildings.Add(ta);
						}
					}
					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_troopconfig[i].items[a].id > 0)
						{
							amf3object ta = amf3object();
							int temp = client->m_items[gserver->m_troopconfig[i].items[a].level].count;
							ta["curNum"] = temp;
							ta["num"] = gserver->m_troopconfig[i].items[a].level;
							ta["successFlag"] = temp>=gserver->m_troopconfig[i].items[a].level?true:false;
							ta["id"] = gserver->m_items[gserver->m_troopconfig[i].items[a].id].name;
							items.Add(ta);
						}
					}
					for (int a = 0; a < 3; ++a)
					{
						if (gserver->m_troopconfig[i].techs[a].id > 0)
						{
							amf3object ta = amf3object();
							ta["level"] = gserver->m_troopconfig[i].techs[a].level;
							int temp = client->GetResearchLevel(gserver->m_troopconfig[i].techs[a].id);
							ta["curLevel"] = temp;
							ta["successFlag"] = temp>=gserver->m_troopconfig[i].techs[a].level?true:false;
							ta["id"] = gserver->m_troopconfig[i].techs[a].id;
							techs.Add(ta);
						}
					}

					conditionbean["buildings"] = buildings;
					conditionbean["items"] = items;
					conditionbean["techs"] = techs;
					conditionbean["population"] = gserver->m_troopconfig[i].population;
					parent["conditionBean"] = conditionbean;
					parent["permition"] = false;
					parent["typeId"] = i;
					trooplist.Add(parent);
				}
			}


			data2["troopList"] = trooplist;
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "produceTroop"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];


			obj2["cmd"] = "troop.produceTroop";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			int trooptype = data["troopType"];
			bool isshare = data["isShare"];
			bool toidle = data["toIdle"];
			int positionid = data["positionId"];
			int num = data["num"];


			stResources res;
			res.food = gserver->m_troopconfig[trooptype].food * num;
			res.wood = gserver->m_troopconfig[trooptype].wood * num;
			res.stone = gserver->m_troopconfig[trooptype].stone * num;
			res.iron = gserver->m_troopconfig[trooptype].iron * num;
			res.gold = gserver->m_troopconfig[trooptype].gold * num;


			if ((res.food > pcity->m_resources.food)
				|| (res.wood > pcity->m_resources.wood)
				|| (res.stone > pcity->m_resources.stone)
				|| (res.iron > pcity->m_resources.iron)
				|| (res.gold > pcity->m_resources.gold))
			{
				data2["ok"] = -1;
				data2["errorMsg"] = "Not enough resources.";
				data2["packageId"] = 0.0f;

				gserver->SendObject(c, obj2);
				return;
			}

			if (isshare || toidle)
			{
				data2["ok"] = -99;
				data2["errorMsg"] = "Not supported action.";
				data2["packageId"] = 0.0f;

				gserver->SendObject(c, obj2);
				return;
			}

			pcity->m_resources -= res;
			pcity->ResourceUpdate();

			LOCK(M_TIMEDLIST);
			if (pcity->AddToBarracksQueue(positionid, trooptype, num, isshare, toidle) == -1)
			{
				UNLOCK(M_TIMEDLIST);
				data2["ok"] = -99;
				data2["errorMsg"] = "Troops could not be trained.";
				data2["packageId"] = 0.0f;

				gserver->SendObject(c, obj2);
				return;
			}
			UNLOCK(M_TIMEDLIST);




			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "cancelTroopProduce"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];
			int positionid = data["positionId"];
			int queueid = data["queueId"];


			obj2["cmd"] = "troop.cancelTroopProduce";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;


			PlayerCity::stTroopQueue * tq = pcity->GetBarracksQueue(positionid);
			list<PlayerCity::stTroopTrain>::iterator iter;
			
			for (iter = tq->queue.begin(); iter != tq->queue.end(); )
			{
				if (iter->queueid == queueid)
				{
					if (iter->endtime > 0)
					{
						//in progress
						//refund 1/3 resources and set next queue to run
						stResources res;
						res.food = (gserver->m_troopconfig[iter->troopid].food * iter->count)/3;
						res.wood = (gserver->m_troopconfig[iter->troopid].wood * iter->count)/3;
						res.stone = (gserver->m_troopconfig[iter->troopid].stone * iter->count)/3;
						res.iron = (gserver->m_troopconfig[iter->troopid].iron * iter->count)/3;
						res.gold = (gserver->m_troopconfig[iter->troopid].gold * iter->count)/3;

						pcity->m_resources += res;
						pcity->ResourceUpdate();
						tq->queue.erase(iter++);

						iter->endtime = unixtime() + iter->costtime;
					}
					else
					{
						//not in progress
						//refund all resources
						stResources res;
						res.food = gserver->m_troopconfig[iter->troopid].food * iter->count;
						res.wood = gserver->m_troopconfig[iter->troopid].wood * iter->count;
						res.stone = gserver->m_troopconfig[iter->troopid].stone * iter->count;
						res.iron = gserver->m_troopconfig[iter->troopid].iron * iter->count;
						res.gold = gserver->m_troopconfig[iter->troopid].gold * iter->count;

						pcity->m_resources += res;
						tq->queue.erase(iter++);
						pcity->ResourceUpdate();
					}

					gserver->SendObject(c, obj2);
					return;
				}
				++iter;
			}
		}
	}
#pragma endregion
#pragma region interior
	if ((cmdtype == "interior"))
	{
		if ((command == "modifyTaxRate"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];

			int temp = data["tax"];
			if (temp < 0 || temp > 100)
			{
				pcity->m_workrate.gold = 0;
				// TODO error reporting - interior.modifyTaxRate
			}
			else
			{
				pcity->m_workrate.gold = data["tax"];
			}

			obj2["cmd"] = "interior.modifyTaxRate";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			gserver->SendObject(c, obj2);

			pcity->CalculateStats();
			pcity->CalculateResources();
			pcity->ResourceUpdate();
			return;
		}
		if ((command == "pacifyPeople"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];

			if (timestamp - pcity->m_lastcomfort < 15*60*1000)
			{
				obj2["cmd"] = "interior.pacifyPeople";
				data2["packageId"] = 0.0f;
				data2["ok"] = -34;
				string str = "";

				{
					stringstream ss;
					double timediff = pcity->m_lastcomfort + 15*60*1000 - timestamp;
					int min = timediff/1000/60;
					int sec = int( ( ( ( ( timediff ) / 1000 ) / 60 ) - min ) * 60 );
					ss << min << "m ";
					ss << sec << "s ";
					ss << "interval needed for next comforting.";
					str = ss.str();
				}

				data2["errorMsg"] = (char*)str.c_str();

				gserver->SendObject(c, obj2);
				return;
			}

			int itypeid = data["typeId"];
			pcity->m_lastcomfort = timestamp;

			obj2["cmd"] = "interior.pacifyPeople";
			data2["packageId"] = 0.0f;
			data2["ok"] = -99;

			switch (itypeid)
			{
				case 1://Disaster Relief 100% pop limit in food for cost, increases loyalty by 5 reduces grievance by 15
					if (pcity->m_resources.food < pcity->m_maxpopulation)
					{
						data2["errorMsg"] = "Not enough food.";

						gserver->SendObject(c, obj2);
						return;
					}
					pcity->m_resources.food -= pcity->m_maxpopulation;
					pcity->m_loyalty += 5;
					pcity->m_grievance -= 15;
					if (pcity->m_loyalty > 100)
						pcity->m_loyalty = 100;
					if (pcity->m_grievance < 0)
						pcity->m_grievance = 0;
					pcity->ResourceUpdate();
					client->PlayerUpdate();
					break;
				case 2://Praying 100% pop limit in food for cost, increases loyalty by 25 reduces grievance by 5
					if (pcity->m_resources.food < pcity->m_maxpopulation)
					{
						data2["errorMsg"] = "Not enough food.";

						gserver->SendObject(c, obj2);
						return;
					}
					pcity->m_resources.food -= pcity->m_maxpopulation;
					pcity->m_loyalty += 25;
					pcity->m_grievance -= 5;
					if (pcity->m_loyalty > 100)
						pcity->m_loyalty = 100;
					if (pcity->m_grievance < 0)
						pcity->m_grievance = 0;
					pcity->ResourceUpdate();
					client->PlayerUpdate();
					break;
				case 3://Blessing 10% pop limit in gold for cost, increases food by 100% pop limit - chance for escaping disaster?
					if (pcity->m_resources.gold < (pcity->m_maxpopulation/10))
					{
						data2["errorMsg"] = "Not enough gold.";

						gserver->SendObject(c, obj2);
						return;
					}
					pcity->m_resources.gold -= (pcity->m_maxpopulation/10);
					pcity->m_resources.food += pcity->m_maxpopulation;
					if (rand()%10 == 1)
					{
						pcity->m_resources.food += pcity->m_maxpopulation;
						data2["errorMsg"] = "Free blessing!";

						gserver->SendObject(c, obj2);

						pcity->ResourceUpdate();
						client->PlayerUpdate();
						return;
					}
					pcity->ResourceUpdate();
					client->PlayerUpdate();
					break;
				case 4://Population Raising 500% pop limit in food for cost, increases population by 5%
					if (pcity->m_resources.food < (pcity->m_maxpopulation*5))
					{
						data2["errorMsg"] = "Not enough food.";

						gserver->SendObject(c, obj2);
						return;
					}
					pcity->m_resources.food -= (pcity->m_maxpopulation*5);
					pcity->m_population += double(pcity->m_maxpopulation)/20;
					if (pcity->m_population >= pcity->m_maxpopulation)
						pcity->m_population = pcity->m_maxpopulation;
					pcity->ResourceUpdate();
					client->PlayerUpdate();
					break;
			}

			// TODO finish - interior.pacifyPeople
			obj2["cmd"] = "interior.pacifyPeople";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "taxation"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];

			if (timestamp - pcity->m_lastlevy < 15*60*1000)
			{
				obj2["cmd"] = "interior.taxation";
				data2["packageId"] = 0.0f;
				data2["ok"] = -34;
				string str = "";

				{
					stringstream ss;
					double timediff = pcity->m_lastlevy + 15*60*1000 - timestamp;
					int min = timediff/1000/60;
					int sec = int( ( ( ( ( timediff ) / 1000 ) / 60 ) - min ) * 60 );
					ss << min << "m ";
					ss << sec << "s ";
					ss << "interval needed for next levy.";
					str = ss.str();
				}

				data2["errorMsg"] = (char*)str.c_str();

				gserver->SendObject(c, obj2);
				return;
			}

			int itypeid = data["typeId"];
			pcity->m_lastlevy = timestamp;

			obj2["cmd"] = "interior.taxation";
			data2["packageId"] = 0.0f;
			data2["ok"] = -99;

			if (pcity->m_loyalty <= 20) //not enough loyalty to levy
			{
				data2["errorMsg"] = "Loyalty too low. Please comfort first.";

				gserver->SendObject(c, obj2);
				return;
			}
			pcity->m_loyalty -= 20;

			switch (itypeid)
			{
			case 1://Gold 10% current pop
				pcity->m_resources.gold += (pcity->m_population/10);
				break;
			case 2://Food 100% current pop
				pcity->m_resources.food += pcity->m_population;
				break;
			case 3://Wood 100% current pop 
				pcity->m_resources.wood += pcity->m_population;
				break;
			case 4://Stone 50% current pop 
				pcity->m_resources.stone += (pcity->m_population/2);
				break;
			case 5://Iron 40% current pop 
				pcity->m_resources.iron += (pcity->m_population*0.40);
				break;
			}
			pcity->ResourceUpdate();
			client->PlayerUpdate();

			// TODO finish - interior.taxation
			obj2["cmd"] = "interior.taxation";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "modifyCommenceRate"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];

			int temp = data["foodrate"];
			
			pcity->CalculateStats();
			pcity->CalculateResources();

			if (temp < 0 || temp > 100)
			{
				pcity->m_workrate.food = 0;
				// TODO error reporting - interior.modifyCommenceRate
			}
			else
			{
				pcity->m_workrate.food = data["foodrate"];
			}
			temp = data["woodrate"];
			if (temp < 0 || temp > 100)
			{
				pcity->m_workrate.wood = 0;
				// TODO error reporting - interior.modifyCommenceRate
			}
			else
			{
				pcity->m_workrate.wood = data["woodrate"];
			}
			temp = data["ironrate"];
			if (temp < 0 || temp > 100)
			{
				pcity->m_workrate.iron = 0;
				// TODO error reporting - interior.modifyCommenceRate
			}
			else
			{
				pcity->m_workrate.iron = data["ironrate"];
			}
			temp = data["stonerate"];
			if (temp < 0 || temp > 100)
			{
				pcity->m_workrate.stone = 0;
				// TODO error reporting - interior.modifyCommenceRate
			}
			else
			{
				pcity->m_workrate.stone = data["stonerate"];
			}

			obj2["cmd"] = "interior.modifyCommenceRate";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			gserver->SendObject(c, obj2);

			pcity->CalculateStats();
			pcity->ResourceUpdate();

			return;
		}
		if ((command == "getResourceProduceData"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];

			obj2["cmd"] = "interior.getResourceProduceData";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;
			data2["resourceProduceDataBean"] = pcity->ResourceProduceData();

			gserver->SendObject(c, obj2);
			return;
		}
	}
#pragma endregion
#pragma region hero
	if ((cmdtype == "hero"))
	{
		if ((command == "getHerosListFromTavern"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int innlevel = pcity->GetBuildingLevel(B_INN);

			if (innlevel > 0)
			{
				obj2["cmd"] = "hero.getHerosListFromTavern";
				data2["ok"] = 1;
				data2["packageId"] = 0.0f;
				int tempnum = 0;
				for (int i = 0; i < 10; ++i)
					if (pcity->m_heroes[i])
						tempnum++;
				data2["posCount"] = pcity->GetBuildingLevel(B_FEASTINGHALL) - tempnum;

				amf3array heroes = amf3array();

				for (int i = 0; i < innlevel; ++i)
				{
					if (!pcity->m_innheroes[i])
					{
						pcity->m_innheroes[i] = gserver->CreateRandomHero(innlevel);
					}

					amf3object temphero = pcity->m_innheroes[i]->ToObject();
					heroes.Add(temphero);
				}

				data2["heros"] = heroes;
				
				gserver->SendObject(c, obj2);
			}
			else
			{
				obj2["cmd"] = "hero.getHerosListFromTavern";
				data2["ok"] = -1; // TODO find error (no inn exists) - hero.getHerosListFromTavern
				data2["packageId"] = 0.0f;

				gserver->SendObject(c, obj2);
			}

			return;
		}
		if ((command == "hireHero"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();


			char * heroname = data["heroName"];
			int blevel = pcity->GetBuildingLevel(B_FEASTINGHALL);

			if (blevel <= pcity->HeroCount())
			{
				obj2["cmd"] = "hero.hireHero";
				data2["ok"] = -99;
				data2["packageId"] = 0.0f;
				data2["errorMsg"] = "Insufficient vacancies in Feasting Hall.";

				gserver->SendObject(c, obj2);
				return;
			}

			for (int i = 0; i < 10; ++i)
			{
				if (!strcmp(pcity->m_innheroes[i]->m_name.c_str(), heroname))
				{
					int32_t hirecost = pcity->m_innheroes[i]->m_level * 1000;
					if (hirecost > pcity->m_resources.gold)
					{
						obj2["cmd"] = "hero.hireHero";
						data2["ok"] = -99;// TODO Get proper not enough gold to hire error code - hero.hireHero
						data2["packageId"] = 0.0f;
						data2["errorMsg"] = "Not enough gold!";

						gserver->SendObject(c, obj2);
						return;
					}

					for (int x = 0; x < 10; ++x)
					{
						if (!pcity->m_heroes[x])
						{
							pcity->m_resources.gold -= hirecost;
							pcity->CalculateResourceStats();
							pcity->CalculateStats();
							pcity->CalculateResources();
							pcity->m_innheroes[i]->m_id = gserver->m_heroid++;
							LOCK(M_HEROLIST);
							pcity->HeroUpdate(pcity->m_innheroes[i], 0);
							pcity->m_heroes[x] = pcity->m_innheroes[i];
							pcity->m_innheroes[i] = 0;
							UNLOCK(M_HEROLIST);
							pcity->ResourceUpdate();
							break;
						}
					}

					obj2["cmd"] = "hero.hireHero";
					data2["ok"] = 1;
					data2["packageId"] = 0.0f;

					gserver->SendObject(c, obj2);
					return;
				}
			}
			obj2["cmd"] = "hero.hireHero";
			data2["ok"] = -99;
			data2["packageId"] = 0.0f;
			data2["errorMsg"] = "Hero does not exist!";

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "fireHero"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int heroid = data["heroId"];

			for (int i = 0; i < 10; ++i)
			{
				if (pcity->m_heroes[i]->m_id == heroid)
				{
					if (pcity->m_heroes[i]->m_status != 0)
					{
						obj2["cmd"] = "hero.fireHero";
						data2["ok"] = -80;
						data2["packageId"] = 0.0f;
						data2["errorMsg"] = "Status of this hero is not Idle!";

						gserver->SendObject(c, obj2);
						return;
					}
					pcity->CalculateResourceStats();
					pcity->CalculateStats();
					pcity->CalculateResources();
					pcity->ResourceUpdate();
					pcity->HeroUpdate(pcity->m_heroes[i], 1);
					
					MULTILOCK(M_HEROLIST, M_DELETELIST);
					gserver->m_deletedhero.push_back(pcity->m_heroes[i]->m_id);

					delete pcity->m_heroes[i];
					pcity->m_heroes[i] = 0;
					UNLOCK(M_HEROLIST);
					UNLOCK(M_DELETELIST);

					obj2["cmd"] = "hero.fireHero";
					data2["ok"] = 1;
					data2["packageId"] = 0.0f;

					gserver->SendObject(c, obj2);
					return;
				}
			}

			obj2["cmd"] = "hero.fireHero";
			data2["ok"] = -99;
			data2["packageId"] = 0.0f;
			data2["errorMsg"] = "Hero does not exist!";

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "promoteToChief"))
		{// BUG : Correct hero not being promoted
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int heroid = data["heroId"];

			for (int i = 0; i < 10; ++i)
			{
				if (pcity->m_heroes[i]->m_id == heroid)
				{
					pcity->CalculateResourceStats();
					pcity->CalculateStats();
					pcity->CalculateResources();
					if (pcity->m_mayor)
						pcity->m_mayor->m_status = 0;
					pcity->m_mayor = pcity->m_heroes[i];
					pcity->m_heroes[i]->m_status = 1;

					pcity->HeroUpdate(pcity->m_mayor, 2);
					pcity->CalculateResourceStats();
					pcity->CalculateResources();
					pcity->ResourceUpdate();

					obj2["cmd"] = "hero.promoteToChief";
					data2["ok"] = 1;
					data2["packageId"] = 0.0f;

					gserver->SendObject(c, obj2);
					return;
				}
			}
			obj2["cmd"] = "hero.promoteToChief";
			data2["ok"] = -99;
			data2["packageId"] = 0.0f;
			data2["errorMsg"] = "TODO error message - hero.promoteToChief";

			gserver->SendObject(c, obj2);
			return;
			// TODO needs an error message - hero.promoteToChief
		}
		if ((command == "dischargeChief"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			if (!pcity->m_mayor)
			{
				obj2["cmd"] = "hero.dischargeChief";
				data2["ok"] = -99;
				data2["packageId"] = 0.0f;
				data2["errorMsg"] = "Castellan is not appointed yet.";

				gserver->SendObject(c, obj2);
				return;
			}

			pcity->CalculateResourceStats();
			pcity->CalculateStats();
			pcity->CalculateResources();
			pcity->m_mayor->m_status = 0;
			pcity->HeroUpdate(pcity->m_mayor, 2);
			pcity->m_mayor = 0;
			pcity->CalculateResourceStats();
			pcity->CalculateResources();
			pcity->ResourceUpdate();

			obj2["cmd"] = "hero.dischargeChief";
			data2["ok"] = 1;
			data2["packageId"] = 0.0f;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "resetPoint"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int heroid = data["heroId"];

			// TODO require holy water? - hero.resetPoint

			for (int i = 0; i < 10; ++i)
			{
				if (pcity->m_heroes[i]->m_id == heroid)
				{
					if (pcity->m_heroes[i]->m_status > 1)
					{
						obj2["cmd"] = "hero.resetPoint";
						data2["ok"] = -80;
						data2["packageId"] = 0.0f;
						data2["errorMsg"] = "Status of this hero is not Idle!";

						gserver->SendObject(c, obj2);
						return;
					}
					pcity->CalculateResourceStats();
					pcity->CalculateStats();
					pcity->CalculateResources();
					pcity->ResourceUpdate();

					Hero * hero = pcity->m_heroes[i];
					hero->m_power = hero->m_basepower;
					hero->m_management = hero->m_basemanagement;
					hero->m_stratagem = hero->m_basestratagem;
					hero->m_remainpoint = hero->m_level;

					pcity->HeroUpdate(pcity->m_heroes[i], 2);
					pcity->CalculateResourceStats();
					pcity->CalculateResources();

					obj2["cmd"] = "hero.resetPoint";
					data2["ok"] = 1;
					data2["packageId"] = 0.0f;

					gserver->SendObject(c, obj2);
					return;
				}
			}

			obj2["cmd"] = "hero.resetPoint";
			data2["ok"] = -99;
			data2["packageId"] = 0.0f;
			data2["errorMsg"] = "Hero does not exist!";

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "addPoint"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int heroid = data["heroId"];
			int stratagem = data["stratagem"];
			int power = data["power"];
			int management = data["management"];

			// TODO require holy water? - hero.resetPoint

			for (int i = 0; i < 10; ++i)
			{
				if (pcity->m_heroes[i] && pcity->m_heroes[i]->m_id == heroid)
				{
					pcity->CalculateResourceStats();
					pcity->CalculateStats();
					pcity->CalculateResources();
					pcity->ResourceUpdate();

					Hero * hero = pcity->m_heroes[i];
					if (((stratagem + power + management) > (hero->m_basemanagement + hero->m_basepower + hero->m_basestratagem + hero->m_level))
						|| (stratagem < hero->m_stratagem) || (power < hero->m_power) || (management < hero->m_management))
					{
						obj2["cmd"] = "hero.resetPoint";
						data2["ok"] = -99;
						data2["packageId"] = 0.0f;
						data2["errorMsg"] = "Invalid action.";

						gserver->SendObject(c, obj2);
						return;
					}
					hero->m_power = power;
					hero->m_management = management;
					hero->m_stratagem = stratagem;
					hero->m_remainpoint = (hero->m_basemanagement + hero->m_basepower + hero->m_basestratagem + hero->m_level) - (stratagem + power + management);

					pcity->HeroUpdate(pcity->m_heroes[i], 2);
					pcity->CalculateResourceStats();
					pcity->CalculateResources();
					pcity->ResourceUpdate();

					obj2["cmd"] = "hero.resetPoint";
					data2["ok"] = 1;
					data2["packageId"] = 0.0f;

					gserver->SendObject(c, obj2);
					return;
				}
			}

			obj2["cmd"] = "hero.resetPoint";
			data2["ok"] = -99;
			data2["packageId"] = 0.0f;
			data2["errorMsg"] = "Hero does not exist!";

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "levelUp"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int heroid = data["heroId"];

			obj2["cmd"] = "hero.levelUp";
			data2["packageId"] = 0.0f;

			for (int i = 0; i < 10; ++i)
			{
				if (pcity->m_heroes[i] && pcity->m_heroes[i]->m_id == heroid)
				{
					Hero * hero = pcity->m_heroes[i];
					if (hero->m_experience < hero->m_upgradeexp)
					{
						data2["ok"] = -99;
						data2["errorMsg"] = "Not enough experience.";

						gserver->SendObject(c, obj2);
						return;
					}

					hero->m_level++;
					hero->m_remainpoint++;

					hero->m_experience -= hero->m_upgradeexp;
					hero->m_upgradeexp = hero->m_level * hero->m_level * 100;

					pcity->HeroUpdate(hero, 2);

					obj2["cmd"] = "hero.levelUp";
					data2["ok"] = 1;

					gserver->SendObject(c, obj2);
					return;
				}
			}

			data2["ok"] = -99;
			data2["errorMsg"] = "Hero does not exist!";

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "refreshHerosListFromTavern"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int innlevel = pcity->GetBuildingLevel(B_INN);

			if (innlevel > 0)
			{
				obj2["cmd"] = "hero.getHerosListFromTavern";
				data2["ok"] = 1;
				data2["packageId"] = 0.0f;
				int tempnum = 0;
				for (int i = 0; i < 10; ++i)
					if (pcity->m_heroes[i])
						tempnum++;
				data2["posCount"] = pcity->GetBuildingLevel(B_FEASTINGHALL) - tempnum;

				amf3array heroes = amf3array();

				for (int i = 0; i < innlevel; ++i)
				{
					if (pcity->m_innheroes[i])
					{
						delete pcity->m_innheroes[i];
					}

					pcity->m_innheroes[i] = gserver->CreateRandomHero(innlevel);
					amf3object temphero = pcity->m_innheroes[i]->ToObject();
					heroes.Add(temphero);
				}

				data2["heros"] = heroes;

				gserver->SendObject(c, obj2);
			}
			else
			{
				obj2["cmd"] = "hero.refreshHerosListFromTavern";
				data2["ok"] = -99; // TODO find error (not enough cents) - hero.refreshHerosListFromTavern
				data2["packageId"] = 0.0f;
				data2["errorMsg"] = "Not enough cents.";

				gserver->SendObject(c, obj2);
			}

			return;
		}
	}
#pragma endregion
#pragma region friend
	if ((cmdtype == "friend"))
	{

	}
#pragma endregion
#pragma region city
	if ((cmdtype == "city"))
	{
		if ((command == "modifyCastleName"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];
			char * logurl = data["logUrl"];
			char * name = data["name"];

			pcity->m_cityname = name;
			pcity->m_logurl = logurl;
			// TODO check valid name and error reporting - city.modifyCastleName

			obj2["cmd"] = "city.modifyCastleName";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "modifyFlag"))
		{
			char * flag = data["newFlag"];

			client->m_flag = flag;
			// TODO check valid name and error reporting - city.modifyFlag

			client->PlayerUpdate();

			obj2["cmd"] = "city.modifyFlag";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "setStopWarState"))
		{
			string pass = data["passWord"];
			string itemid = data["ItemId"];

			if (client->m_password != pass)
			{
				obj2["cmd"] = "city.setStopWarState";
				data2["packageId"] = 0.0f;
				data2["ok"] = -50;
				data2["errorMsg"] = "Incorrect account or password.";

				gserver->SendObject(c, obj2);
				return;
			}

			if (client->GetItemCount(itemid) > 0)
			{
				client->SetItem(itemid, -1);
				client->m_status = DEF_TRUCE;
			}
			else
			{
				int32_t cost = gserver->GetItem(itemid)->cost;
				if (client->m_cents < cost)
				{
					obj2["cmd"] = "city.setStopWarState";
					data2["packageId"] = 0.0f;
					data2["ok"] = -99;
					data2["errorMsg"] = "Not enough cents.";

					gserver->SendObject(c, obj2);
					return;
				}
				client->m_cents -= cost;
			}
			if (itemid == "player.peace.1")
			{
				client->SetBuff("PlayerPeaceBuff", "Truce Agreement activated", timestamp + (12*60*60*1000));
			}
			else
			{
				obj2["cmd"] = "city.setStopWarState";
				data2["packageId"] = 0.0f;
				data2["ok"] = -99;
				data2["errorMsg"] = "Invalid state.";

				gserver->SendObject(c, obj2);
				return;
			}

			obj2["cmd"] = "city.setStopWarState";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			gserver->SendObject(c, obj2);
			return;
		}
	}
#pragma endregion
#pragma region furlough
	if ((cmdtype == "furlough"))
	{
		if ((command == "isFurlought"))
		{
			int32_t playerid = data["playerId"];
			string password = data["password"];
			bool autofurlough = data["isAutoFurlough"];
			int32_t day = data["day"];

			if (client->m_password != password)
			{
				obj2["cmd"] = "furlough.isFurlought";
				data2["packageId"] = 0.0f;
				data2["ok"] = -50;
				data2["errorMsg"] = "Incorrect account or password.";

				gserver->SendObject(c, obj2);
				return;
			}

			if (client->m_cents < day*10)
			{
				obj2["cmd"] = "furlough.isFurlought";
				data2["packageId"] = 0.0f;
				data2["ok"] = -99;
				data2["errorMsg"] = "Not enough cents.";

				gserver->SendObject(c, obj2);
				return;
			}
			client->m_cents -= day*10;

			client->SetBuff("FurloughBuff", "", timestamp + (day*24*60*60*1000));

			obj2["cmd"] = "furlough.isFurlought";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;
			data2["playerBean"] = client->ToObject();

			gserver->SendObject(c, obj2);
			return;
		}
		if ((command == "cancelFurlought"))
		{
			client->SetBuff("FurloughBuff", "", 0);

			obj2["cmd"] = "furlough.cancelFurlought";
			data2["packageId"] = 0.0f;
			data2["ok"] = 1;

			gserver->SendObject(c, obj2);
			return;
		}
	}
#pragma endregion
#pragma region army
	if (cmdtype, "army")
	{
		if ((command == "getInjuredTroop"))
		{
			VERIFYCASTLEID();
			CHECKCASTLEID();

			int castleid = data["castleId"];

			amf3object obj3;
			obj3["cmd"] = "server.InjuredTroopUpdate";
			obj3["data"] = amf3object();
			amf3object & data3 = obj3["data"];
			data3["castleId"] = 0.0f;
			data3["troop"] = client->GetFocusCity()->InjuredTroops();
			rep.objects.push_back(obj3);
		}
	}

#pragma endregion
#pragma region login
	if (cmdtype == "login")
	{
		string username = data["user"];
		string password = data["pwd"];


// 		if (!client)
// 		{
// 			client = NewClient();
// 			client->socket = req.connection;
// 			client->m_socknum = 0;
// 			req.connection->client_ = client;
// 			//TODO: set client stuff
// 		}

		char newuser[50];
		char newpass[50];
		mysql_real_escape_string(gserver->accounts->mySQL, newuser, username.c_str(), username.length()<50?username.length():50);
		mysql_real_escape_string(gserver->accounts->mySQL, newpass, password.c_str(), password.length()<50?password.length():50);
		gserver->accounts->Select("SELECT COUNT(*) AS a FROM `account` WHERE `email`='%s';", newuser);
		gserver->accounts->Fetch();
		if (gserver->accounts->GetInt(0, "a") <= 0)
		{
			gserver->accounts->Insert("INSERT INTO `account` (`name`, `email`, `ip`, `lastlogin`, `creation`, `password`, `status`, `reason`) \
								VALUES ('null', '%s', '', "XI64", "XI64", '%s', 0, '');",
								newuser, unixtime(), unixtime(), newpass);

		}

		gserver->accounts->Reset();
		gserver->accounts->Select("SELECT * FROM `account` WHERE `email`='%s' AND `password`='%s';", newuser, newpass);
		gserver->accounts->Fetch();
		if (gserver->accounts->m_iRows <= 0)
		{
			//account doesn't exist or password is wrong
			amf3object obj;
			obj["cmd"] = "server.LoginResponse";
			obj["data"] = amf3object();
			amf3object & data = obj["data"];
			data["packageId"] = 0.0f;
			data["ok"] = -2;
			data["errorMsg"] = "Incorrect account or password.";

			rep.objects.push_back(obj);
			//SendObject(*req.connection, obj);
			//req.connection->stop();
			gserver->accounts->Reset();
			return;
		}
		else
		{
			int parentid = gserver->accounts->GetInt(0, "id");
			client = gserver->GetClientByParent(parentid);

			bool banned = false;

			//are they banned? if so, globally or for this server?
			gserver->msql->Select("SELECT * FROM `accounts` WHERE `parentid`='%d';", parentid);
			gserver->msql->Fetch();

			if (gserver->accounts->GetInt(0, "status") == -99)
				banned = true;

			if (gserver->msql->m_iRows > 0 && gserver->msql->GetInt(0, "status") == -99)
				banned = true;

			if (banned)
			{
				amf3object obj;
				obj["cmd"] = "server.LoginResponse";
				obj["data"] = amf3object();
				amf3object & data = obj["data"];
				data["packageId"] = 0.0f;
				data["ok"] = -99;
				string errormsg = "You are banned. Reason: ";
				errormsg += strlen(gserver->accounts->GetString(0, "reason"))>0?gserver->accounts->GetString(0, "reason"):gserver->msql->GetString(0, "reason");
				data["errorMsg"] = errormsg.c_str();

				rep.objects.push_back(obj);
				//SendObject(*req.connection, obj);
				//req.connection->stop();
				gserver->accounts->Reset();
				gserver->msql->Reset();
				return;
			}

			//LOCK(M_CLIENTLIST);
			//client = gserver->GetClientByParent(parentid);
			if (client == 0)
			{
				client = gserver->NewClient();
				client->m_parentid = parentid;
				client->m_socknum = req.connection->uid;
				client->socket = req.connection;
				req.connection->client_ = client;
			}
			else
			{
				client->socket = req.connection;
				client->m_socknum = req.connection->uid;
				req.connection->client_ = client;
				Log("Already established client found # %d", (uint32_t)client->m_clientnumber);
			}

			if (client == 0)
			{
				//UNLOCK(M_CLIENTLIST);
				//error creating client object
				Log("Error creating client object @ %s:%d", __FILE__, __LINE__);
				amf3object obj;
				obj["cmd"] = "server.LoginResponse";
				obj["data"] = amf3object();
				amf3object & data = obj["data"];
				data["packageId"] = 0.0f;
				data["ok"] = -99;
				data["errorMsg"] = "Error with connecting. Please contact support.";

				rep.objects.push_back(obj);
				//SendObject(*req.connection, obj);
				//req.connection->stop();
				gserver->accounts->Reset();
				gserver->msql->Reset();
				return;
			}

			gserver->msql->Reset();

			//account exists
			gserver->msql->Select("SELECT * FROM `accounts` WHERE `parentid`='%d';", parentid);
			gserver->msql->Fetch();
			if (gserver->msql->m_iRows <= 0)
			{
				//UNLOCK(M_CLIENTLIST);
				//does not have an account on server
				amf3object obj;
				obj["cmd"] = "server.LoginResponse";
				obj["data"] = amf3object();
				amf3object & data = obj["data"];
				data["packageId"] = 0.0f;
				data["ok"] = -4;
				data["errorMsg"] = "need create player";

				rep.objects.push_back(obj);
				//SendObject(*req.connection, obj);
				client->m_loggedin = true;

				gserver->accounts->Reset();
				gserver->msql->Reset();
				return;
			}
			else
			{
				int accountid = gserver->msql->GetInt(0, "accountid");
				//client->m_accountid = accountid;

				//has an account, what about cities?
				gserver->msql->Select("SELECT * FROM `cities` WHERE `accountid`=%d;", accountid);
				gserver->msql->Fetch();
				if (gserver->msql->m_iRows <= 0)
				{
					//UNLOCK(M_CLIENTLIST);
					//does not have any cities on server - theoretically should never happen. if it does, delete the account row.
					//may happen if a new account is attempted to be made yet a city is not made for it or if idle city deletion happens but account is not removed
					gserver->msql->Delete("DELETE FROM `accounts` WHERE `parentid`=%d", parentid);
					amf3object obj;
					obj["cmd"] = "server.LoginResponse";
					obj["data"] = amf3object();
					amf3object & data = obj["data"];
					data["packageId"] = 0.0f;
					data["ok"] = -4;
					data["errorMsg"] = "need create player";

					rep.objects.push_back(obj);
					//SendObject(*req.connection, obj);
					client->m_loggedin = true;
					gserver->accounts->Reset();
					gserver->msql->Reset();
					return;
				}
				else
				{
					//has an account and cities. process the list and send account info

					amf3object obj;
					obj["cmd"] = "server.LoginResponse";
					obj["data"] = amf3object();
					amf3object & data = obj["data"];
					data["packageId"] = 0.0f;

					double tslag = unixtime();

					data["player"] = client->ToObject();
					//UNLOCK(M_CLIENTLIST);

					if (client->m_city.size() == 0)
					{
						//problem
						Log("Error client has no cities @ %s:%d", __FILE__, __LINE__);
						amf3object obj;
						obj["cmd"] = "server.LoginResponse";
						obj["data"] = amf3object();
						amf3object & data = obj["data"];
						data["packageId"] = 0.0f;
						data["ok"] = -99;
						data["errorMsg"] = "Error with connecting. Please contact support.";

						rep.objects.push_back(obj);
						//SendObject(*req.connection, obj);
						//req.connection->stop();
						gserver->accounts->Reset();
						gserver->msql->Reset();
						return;
					}
					client->m_currentcityid = ((PlayerCity*)client->m_city.at(0))->m_castleid;
					client->m_currentcityindex = 0;


					//check for holiday status
					Client::stBuff * holiday = client->GetBuff("FurloughBuff");
					if (holiday && holiday->endtime > tslag)
					{
						//is in holiday - send holiday info too

						string s;
						{
							int32_t hours;
							int32_t mins;
							int32_t secs = (holiday->endtime - tslag)/1000;

							hours = secs/60/60;
							mins = secs/60 - hours*60;
							secs = secs - mins*60 - hours*60*60;

							stringstream ss;
							ss << hours << "," << mins << "," << secs;

							s = ss.str();
						}

						data["ok"] = -100;
						data["msg"] = s;
						data["errorMsg"] = s;
					}
					else
					{
						data["ok"] = 1;
						data["msg"] = "success";
					}



					rep.objects.push_back(obj);
					//SendObject(*req.connection, obj);

					client->m_lag = unixtime() - tslag;

					client->m_connected = true;

					if (client->GetItemCount("consume.1.a") < 10000)
						client->SetItem("consume.1.a", 10000);

					client->m_loggedin = true;
					return;
				}
				gserver->msql->Reset();
				gserver->accounts->Reset();
			}
		}
		return;
	}
#pragma endregion
#pragma region gameClient
	else if (cmdtype == "gameClient")
	{
		if (command == "version")
		{
			if (data == "091103_11")
			{
				//pass
				return;
			}
			else
			{
// 				obj2["cmd"] = "gameClient.kickout";
// 				obj2["data"] = amf3object();
// 				data2["msg"] = "You suck.";
// 
// 				gserver->SendObject(c, obj2);
// 				//"other" version
// 				return;
				
				amf3object obj2 = amf3object();
				obj2["cmd"] = "";
				amf3object & data2 = obj2["data"];
				data2 = amf3object();

				obj2["cmd"] = "gameClient.errorVersion";
				data2["version"] = "091103_11";
				data2["msg"] = "Invalid Version.";

				rep.objects.push_back(obj2);
				//"other" version
				return;
			}
		}
		return;
	}
#pragma endregion
}
int32_t GetGambleCount(string item);
amf3object GenerateGamble();

void ShopUseGoods(amf3object & data, Client * client)
{
	amf3object obj2 = amf3object();
	obj2["cmd"] = "";
	amf3object & data2 = obj2["data"];

	int num = data["num"];
	string itemid = data["itemId"];

	//Maybe tokenize instead?
// 	char * cmdtype, * command, * ctx;
// 	cmdtype = strtok_s(cmd, ".", &ctx);
// 	command = strtok_s(NULL, ".", &ctx);
// 
// 	if ((cmdtype == "gameClient"))
// 	{
// 		if ((command == "version"))
// 		{
// 		}
// 	}

	/*if (itemid == "player.speak.bronze_publicity_ambassador.permanent")
	{

	}
	if (itemid == "player.speak.bronze_publicity_ambassador.permanent.15")
	{

	}
	if (itemid == "player.speak.gold_publicity_ambassador.15")
	{

	}
	if (itemid == "player.speak.gold_publicity_ambassador.permanent"){}
	if (itemid == "player.speak.silver_publicity_ambassador.15"){}
	if (itemid == "player.speak.silver_publicity_ambassador.permanent"){}
	if (itemid == "alliance.ritual_of_pact.advanced"){}
	if (itemid == "alliance.ritual_of_pact.premium"){}
	if (itemid == "alliance.ritual_of_pact.ultimate"){}
	if (itemid == "consume.1.a"){}
	if (itemid == "consume.1.b"){}
	if (itemid == "consume.1.c"){}
	if (itemid == "consume.2.a"){}
	if (itemid == "consume.2.b"){}
	if (itemid == "consume.2.b.1"){}
	if (itemid == "consume.2.c"){}
	if (itemid == "consume.2.c.1"){}
	if (itemid == "consume.2.d"){}
	if (itemid == "consume.blueprint.1"){}
	if (itemid == "consume.changeflag.1"){}
	if (itemid == "consume.hegemony.1"){}
	if (itemid == "consume.key.1"){}
	if (itemid == "consume.key.2"){}
	if (itemid == "consume.key.3"){}
	if (itemid == "consume.move.1"){}
	if (itemid == "consume.refreshtvern.1"){}
	if (itemid == "consume.transaction.1"){}
	if (itemid == "hero.intelligence.1"){}
	if (itemid == "hero.loyalty.1"){}
	if (itemid == "hero.loyalty.2"){}
	if (itemid == "hero.loyalty.3"){}
	if (itemid == "hero.loyalty.4"){}
	if (itemid == "hero.loyalty.5"){}
	if (itemid == "hero.loyalty.6"){}
	if (itemid == "hero.loyalty.7"){}
	if (itemid == "hero.loyalty.8"){}
	if (itemid == "hero.loyalty.9"){}
	if (itemid == "hero.management.1"){}
	if (itemid == "hero.power.1"){}
	if (itemid == "hero.reset.1"){}
	if (itemid == "hero.reset.1.a"){}
	if (itemid == "player.attackinc.1"){}
	if (itemid == "player.attackinc.1.b"){}
	if (itemid == "player.box.1"){}
	if (itemid == "player.box.2"){}
	if (itemid == "player.box.3"){}
	if (itemid == "player.box.currently.1"){}
	if (itemid == "player.box.gambling.1"){}
	if (itemid == "player.box.gambling.10"){}
	if (itemid == "player.box.gambling.11"){}
	if (itemid == "player.box.gambling.12"){}
	if (itemid == "player.box.gambling.2"){}
	if (itemid == "player.box.gambling.3"){}
	if (itemid == "player.box.gambling.4"){}
	if (itemid == "player.box.gambling.5"){}
	if (itemid == "player.box.gambling.6"){}
	if (itemid == "player.box.gambling.7"){}
	if (itemid == "player.box.gambling.8"){}
	if (itemid == "player.box.gambling.9"){}
	if (itemid == "player.box.gambling.food"){}
	if (itemid == "player.box.gambling.gold"){}
	if (itemid == "player.box.gambling.iron"){}
	if (itemid == "player.box.gambling.medal.10"){}
	if (itemid == "player.box.gambling.medal.300"){}
	if (itemid == "player.box.gambling.stone"){}
	if (itemid == "player.box.gambling.wood"){}
	if (itemid == "player.box.hero.a"){}
	if (itemid == "player.box.hero.b"){}
	if (itemid == "player.box.hero.c"){}
	if (itemid == "player.box.hero.d"){}
	if (itemid == "player.box.present.1"){}
	if (itemid == "player.box.present.10"){}
	if (itemid == "player.box.present.11"){}
	if (itemid == "player.box.present.2"){}
	if (itemid == "player.box.present.3"){}
	if (itemid == "player.box.present.4"){}
	if (itemid == "player.box.present.5"){}
	if (itemid == "player.box.present.6"){}
	if (itemid == "player.box.present.7"){}
	if (itemid == "player.box.present.8"){}
	if (itemid == "player.box.present.9"){}
	if (itemid == "player.box.resource.1"){}
	if (itemid == "player.box.special.1"){}
	if (itemid == "player.box.special.2"){}
	if (itemid == "player.box.special.3"){}
	if (itemid == "player.box.troop.1"){}
	if (itemid == "player.box.troop.a"){}
	if (itemid == "player.box.troop.b"){}
	if (itemid == "player.box.troop.c"){}
	if (itemid == "player.box.troop.d"){}
	if (itemid == "player.box.wood.1"){}
	if (itemid == "player.defendinc.1"){}
	if (itemid == "player.defendinc.1.b"){}
	if (itemid == "player.destroy.1.a"){}
	if (itemid == "player.experience.1.a"){}
	if (itemid == "player.experience.1.b"){}
	if (itemid == "player.experience.1.c"){}
	if (itemid == "player.fort.1.c"){}
	if (itemid == "player.gold.1.a"){}
	if (itemid == "player.gold.1.b"){}
	if (itemid == "player.heart.1.a"){}
	if (itemid == "player.more.castle.1.a"){}
	if (itemid == "player.name.1.a"){}
	if (itemid == "player.peace.1"){}
	if (itemid == "player.pop.1.a"){}
	if (itemid == "player.relive.1"){}
	if (itemid == "player.resinc.1"){}
	if (itemid == "player.resinc.1.b"){}
	if (itemid == "player.resinc.2"){}
	if (itemid == "player.resinc.2.b"){}
	if (itemid == "player.resinc.3"){}
	if (itemid == "player.resinc.3.b"){}
	if (itemid == "player.resinc.4"){}
	if (itemid == "player.resinc.4.b"){}
	if (itemid == "player.troop.1.a"){}
	if (itemid == "player.troop.1.b"){}
	if (itemid == "player.box.present.medal.50"){}
	if (itemid == "player.box.present.12"){}
	if (itemid == "player.box.present.13"){}
	if (itemid == "player.box.present.14"){}
	if (itemid == "player.box.present.15"){}
	if (itemid == "player.box.present.16"){}
	if (itemid == "player.box.present.17"){}
	if (itemid == "player.box.present.18"){}
	if (itemid == "player.box.present.19"){}
	if (itemid == "player.box.present.20"){}
	if (itemid == "player.box.present.21"){}
	if (itemid == "player.box.present.22"){}
	if (itemid == "player.box.present.medal.3500"){}
	if (itemid == "player.box.present.medal.500"){}
	if (itemid == "player.box.present.23"){}
	if (itemid == "player.box.present.24"){}
	if (itemid == "player.box.present.25"){}
	if (itemid == "player.box.present.26"){}
	if (itemid == "player.box.present.27"){}
	if (itemid == "player.box.present.28"){}
	if (itemid == "player.box.present.29"){}
	if (itemid == "player.box.present.3"){}
	if (itemid == "player.box.present.30"){}
	if (itemid == "player.box.present.31"){}
	if (itemid == "player.box.present.32"){}
	if (itemid == "player.box.present.33"){}
	if (itemid == "player.box.present.34"){}
	if (itemid == "player.box.present.35"){}
	if (itemid == "player.box.present.36"){}
	if (itemid == "player.box.present.37"){}
	if (itemid == "player.box.present.38"){}
	if (itemid == "player.box.present.recall.a"){}
	if (itemid == "player.box.present.recall.b"){}
	if (itemid == "player.box.present.recall.c"){}
	if (itemid == "player.box.present.money.3"){}
	if (itemid == "player.box.present.money.4"){}
	if (itemid == "player.box.present.money.5"){}
	if (itemid == "player.box.present.money.6"){}
	if (itemid == "player.box.present.money.7"){}
	if (itemid == "player.box.present.money.8"){}
	if (itemid == "player.box.present.money.9"){}
	if (itemid == "player.box.present.money.10"){}
	if (itemid == "player.box.present.money.11"){}
	if (itemid == "player.box.present.money.12"){}
	if (itemid == "player.box.present.money.13"){}
	if (itemid == "player.box.present.money.14"){}
	if (itemid == "player.box.present.money.15"){}
	if (itemid == "player.box.present.money.16"){}
	if (itemid == "player.box.present.money.buff.17"){}
	if (itemid == "player.box.present.money.buff.18"){}
	if (itemid == "player.box.present.money.buff.19"){}
	if (itemid == "player.box.present.money.20"){}
	if (itemid == "player.box.present.money.21"){}
	if (itemid == "player.box.present.money.22"){}
	if (itemid == "player.box.present.money.23"){}
	if (itemid == "player.key.santa"){}
	if (itemid == "player.santa.stoptoopsupkeep"){}
	if (itemid == "player.box.present.christmas.a"){}
	if (itemid == "player.box.present.christmas.b"){}
	if (itemid == "player.box.present.christmas.c"){}
	if (itemid == "player.box.present.money.24"){}
	if (itemid == "player.box.present.money.25"){}
	if (itemid == "player.key.newyear"){}
	if (itemid == "player.newyear.stoptoopsupkeep"){}
	if (itemid == "player.truce.dream"){}
	if (itemid == "player.box.present.money.27"){}
	if (itemid == "player.move.castle.1.b"){}
	if (itemid == "player.box.present.40"){}
	if (itemid == "player.key.easter_package"){}
	if (itemid == "player.box.present.money.28"){}
	if (itemid == "player.box.present.money.29"){}
	if (itemid == "player.reduce.troops.upkeep.1"){}
	if (itemid == "player.key.special.chest"){}
	if (itemid == "player.box.present.money.30"){}
	if (itemid == "player.box.present.money.31"){}
	if (itemid == "player.box.merger.compensation"){}
	if (itemid == "player.box.present.money.32"){}
	if (itemid == "player.box.present.money.33"){}
	if (itemid == "player.key.halloween"){}
	if (itemid == "player.halloween.candy"){}
	if (itemid == "player.box.present.money.34"){}
	if (itemid == "player.box.present.money.35"){}
	if (itemid == "player.box.evony.birthday"){}
	if (itemid == "player.box.present.money.36"){}
	if (itemid == "player.box.present.money.37"){}
	if (itemid == "player.box.evony.subscription"){}
	if (itemid == "player.box.toolbarbonus1.greater"){}
	if (itemid == "player.box.toolbarbonus1.lesser"){}
	if (itemid == "player.box.toolbarbonus1.medium"){}
	if (itemid == "player.box.toolbarbonus1.superior"){}
	if (itemid == "player.box.present.money.38"){}
	if (itemid == "player.box.present.money.39"){}
	if (itemid == "player.key.valentine"){}
	if (itemid == "player.cupid.chocolate"){}
	if (itemid == "player.queue.building"){}
	if (itemid == "player.key.patrick"){}
	if (itemid == "player.box.present.money.40"){}
	if (itemid == "player.box.present.money.41"){}
	if (itemid == "player.irish.whiskey"){}
	if (itemid == "player.box.present.money.42"){}
	if (itemid == "player.box.present.money.43"){}
	if (itemid == "player.key.easter"){}
	if (itemid == "player.easter.egg"){}
	if (itemid == "player.box.compensation.a"){}
	if (itemid == "player.box.compensation.b"){}
	if (itemid == "player.box.compensation.c"){}
	if (itemid == "player.box.compensation.d"){}
	if (itemid == "player.box.compensation.e"){}
	if (itemid == "player.box.present.money.44"){}
	if (itemid == "player.box.present.money.45"){}
	if (itemid == "player.key.bull"){}
	if (itemid == "player.running.shoes"){}*/

	if (itemid == "player.box.gambling.3")
	{
		obj2["cmd"] = "shop.useGoods";

		amf3array itembeans = amf3array();
		amf3array gamblingbeans = amf3array();

		server::stItem * randitem = &gserver->m_items[rand()%gserver->m_itemcount];

		amf3object item = amf3object();
		item["id"] = randitem->name;
		item["count"] = 1;
		item["minCount"] = 0;
		item["maxCount"] = 0;

		itembeans.Add(item);

		randitem = &gserver->m_items[rand()%gserver->m_itemcount];

		obj2["data"] = GenerateGamble();

		amf3object announceitem = obj2["data"];
		amf3array & array = *(amf3array*)announceitem["itemBeans"];
		amf3object itemobj = array.Get(0);

		gserver->MassMessage("Lord <font color='#FF0000'><b><u>" + client->m_playername + "</u></b></font> gained <b><font color='#00A2FF'>" + (string)itemobj["count"] + " " + (string)itemobj["id"] + "</font></b> (worth <b><font color='#FF0000'>100</font></b> Cents) from <b><font color='#FF0000'>Amulet</font></b>!");

		gserver->SendObject(client->socket, obj2);
		return;
	}
}
int32_t GetGambleCount(string item)
{
	if (item == "consume.1.a")
		return (rand()%5==1)?5:1;
	if (item == "consume.2.a")
		return (rand()%5==1)?5:1;
	if (item == "consume.2.b")
		return (rand()%5==1)?5:1;
	if (item == "consume.2.b.1")
		return (rand()%5==1)?5:1;
	if (item == "consume.blueprint.1")
		return (rand()%5==1)?5:1;
	if (item == "consume.refreshtavern.1")
		return (rand()%5==1)?10:5;
	if (item == "consume.transaction.1")
		return (rand()%5==1)?10:5;
	if (item == "hero.loyalty.1")
		return (rand()%5!=1)?1:5;
	if (item == "hero.loyalty.2")
		return (rand()%5!=1)?1:5;
	if (item == "hero.loyalty.3")
		return (rand()%5!=1)?1:5;
	if (item == "hero.loyalty.4")
		return (rand()%5!=1)?1:5;
	if (item == "hero.loyalty.5")
		return (rand()%5!=1)?1:5;
	if (item == "hero.loyalty.6")
		return (rand()%5!=1)?1:5;
	if (item == "hero.loyalty.7")
		return (rand()%5!=1)?1:4;
	if (item == "hero.loyalty.8")
		return (rand()%5!=1)?1:3;
	if (item == "hero.loyalty.9")
		return (rand()%5!=1)?1:2;
	if (item == "player.box.gambling.food")
		return (rand()%2==1)?250000:500000;
	if (item == "player.box.gambling.wood")
		return (rand()%2==1)?250000:500000;
	if (item == "player.box.gambling.stone")
		return (rand()%2==1)?150000:300000;
	if (item == "player.box.gambling.iron")
		return (rand()%2==1)?100000:200000;
	if (item == "player.box.gambling.gold")
		return (rand()%2==1)?150000:300000;
	if (item == "player.heart.1.a")
		return (rand()%5==1)?10:5;
	if (item == "player.queue.building")
		return (rand()%5==1)?10:5;
	if (item == "player.gold.1.a")
		return (rand()%5==1)?10:5;
	if (item == "player.gold.1.b")
		return (rand()%5==1)?10:5;
	
	return 1;
}
amf3object GenerateGamble()
{
	amf3array itemBeans = amf3array();
	amf3array gamblingItemsBeans = amf3array();
	amf3object data = amf3object();

	//16 normals
	//4 corners
	//3 rares (left right bottom)
	//1 super rare (top middle)

	// 24 total


	// Item rarity:
	// 5 = ultra rare (chance within super rare to appear)
	// 4 = super rare
	// 3 = semi rare
	// 2 = special
	// 1 = common


	for (int i = 0; i < 16; ++i)
	{
		amf3object obj = amf3object();
		server::stItem * item = gserver->m_gambleitems.common.at(rand()%gserver->m_gambleitems.common.size());
		obj["id"] = item->name;
		obj["count"] = GetGambleCount(item->name);
		obj["kind"] = item->rarity-1;
		gamblingItemsBeans.Add(obj);
		Log("Item: %s", item->name.c_str());
	}
	for (int i = 0; i < 4; ++i)
	{
		amf3object obj = amf3object();
		server::stItem * item = gserver->m_gambleitems.special.at(rand()%gserver->m_gambleitems.special.size());
		obj["id"] = item->name;
		obj["count"] = GetGambleCount(item->name);
		obj["kind"] = item->rarity-1;
		gamblingItemsBeans.Add(obj);
		Log("Item: %s", item->name.c_str());
	}
	for (int i = 0; i < 3; ++i)
	{
		amf3object obj = amf3object();
		server::stItem * item = gserver->m_gambleitems.rare.at(rand()%gserver->m_gambleitems.rare.size());
		obj["id"] = item->name;
		obj["count"] = GetGambleCount(item->name);
		obj["kind"] = item->rarity-1;
		gamblingItemsBeans.Add(obj);
		Log("Item: %s", item->name.c_str());
	}
	{
		amf3object obj = amf3object();
		server::stItem * item;
		if (rand()%100 < 95)
		{
			item = gserver->m_gambleitems.superrare.at(rand()%gserver->m_gambleitems.superrare.size());
			obj["kind"] = item->rarity-1;
		}
		else
		{
			item = gserver->m_gambleitems.ultrarare.at(rand()%gserver->m_gambleitems.ultrarare.size());
			obj["kind"] = item->rarity;
		}

		obj["id"] = item->name;
		obj["count"] = GetGambleCount(item->name);
		gamblingItemsBeans.Add(obj);
		Log("Item: %s", item->name.c_str());
	}
	int get = 0;
	int value = (rand()%100000);
	if ((value >= 0) && (value < 60000))
	{
		get = rand()%16;
	}
	else if ((value >= 60000) && (value < 85000))
	{
		get = rand()%4 + 16;
	}
	else if ((value >= 85000) && (value < 95000))
	{
		get = rand()%3 + 16 + 4;
	}
	else if ((value >= 95000) && (value < 100000))
	{
		get = 23;
	}

	itemBeans.Add(gamblingItemsBeans.Get(get));

	data["itemBeans"] = itemBeans;
	data["gamblingItemsBeans"] = gamblingItemsBeans;
	data["packageId"] = 0.0f;
	data["ok"] = 1;

	return data;
}

} // namespace server
} // namespace http
