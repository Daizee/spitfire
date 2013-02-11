//
// main.cpp
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


#include <iostream>
#include <string>
#include <asio.hpp>
#include <boost/bind.hpp>
#include "server.hpp"


#include "amf3.h"

namespace spitfire {
namespace server {
spitfire::server::server * gserver;
bool TimerThreadRunning = false;
}
}

int main(int argc, char* argv[])
{
	try
	{
		// Initialise the server.
		//spitfire::server::server s(argv[1], argv[2], argv[3]);
		spitfire::server::gserver = new spitfire::server::server;


		// Run the server until stopped.
		spitfire::server::gserver->run();

		while (spitfire::server::TimerThreadRunning)
		{
			Sleep(1);
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "exception: " << e.what() << "\n";
	}
	system("pause");
	delete spitfire::server::gserver;

	return 0;
}
