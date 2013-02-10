//
// reply.cpp
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

#include "reply.hpp"
#include <string>
#include <boost/lexical_cast.hpp>

namespace spitfire {
namespace server {

std::vector<asio::const_buffer> reply::to_buffers()
{
  std::vector<asio::const_buffer> buffers;
  for (std::size_t i = 0; i < objects.size(); ++i)
  {
	  amf3writer * writer;

	  char tbuff[15000];
	  int length = 0;
	  writer = new amf3writer(tbuff+4);

	  writer->Write(objects[i]);

	  (*(int*)tbuff) = length = writer->position;
	  ByteSwap(*(int*)tbuff);

	  buffers.push_back(asio::buffer(tbuff, length+4));
	  delete writer;
  }
  return buffers;
}

} // namespace server
} // namespace http
