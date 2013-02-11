--[[
    This file contains the configuration settings for Spitfire
--]]
 
--[[
bindip/port = ip and port to host on (typically 0.0.0.0 and 443)

maxplayersloaded = Maximum amount of active accounts allowed (accounts are
		created by logging in once and creating a city) - WARNING: Setting
		this value to an unreasonably high number will negatively impact the
		server's performance. As little as 500 should work for most situations.
		Adjust as needed.
		
maxplayersonline = Maximum amount of players connected to the server at once
		(typically set to the same or more than `maxplayersloaded`)
		
mysqlhost/user/pass = mysql credentials
--]]
server =
{
	ipaddress = "0.0.0.0",
	port = 443,
	maxplayersloaded = 2000,
	maxplayersonline = 2000,
	mysqlhost = "192.168.112.129",
	mysqluser = "evo",
	mysqlpass = "evo",
	--[[ other config options to be added later such as multipliers to affect in game actions
	like building, training, research, resource gain, travel speeds, etc ]]
}
