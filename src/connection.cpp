//
// connection.cpp
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

#include "connection.hpp"
#include <vector>
#include <boost/bind.hpp>
#include "connection_manager.hpp"
#include "request_handler.hpp"
#include "Client.h"
#include "server.hpp"

namespace spitfire {
namespace server {

extern uint64_t unixtime();

connection::connection(asio::io_service& io_service,
	connection_manager& manager, request_handler& handler)
	: socket_(io_service),
	connection_manager_(manager),
	request_handler_(handler),
	uid(0),
	client_(0)
{
}

asio::ip::tcp::socket& connection::socket()
{
	return socket_;
}

void connection::start()
{
	uid = rand()*rand()*rand();

	asio::async_read(socket_, asio::buffer(buffer_, 4), boost::bind(&connection::handle_read_header, shared_from_this(),
		asio::placeholders::error,
		asio::placeholders::bytes_transferred));
// 			socket_.async_read_some(asio::buffer(buffer_),
// 				boost::bind(&connection::handle_read, shared_from_this(),
// 				asio::placeholders::error,
// 				asio::placeholders::bytes_transferred));
}

void connection::stop()
{
	socket_.close();
	if (client_)
	{
		if (!client_->m_accountexists)
		{
			int num = client_->m_clientnumber;
			server * s = client_->m_main;
			delete s->m_clients[num];
			s->m_clients[num] = 0;
			return;
		}
		client_->socket = 0;
		client_->m_socknum = 0;
		client_->m_lastlogin = unixtime();
		client_ = 0;
		//TODO: record last online time
	}
}

void connection::write(const char * data, const int32_t size)
{
	try {
		socket_.write_some(asio::buffer(data, size));
	}
	catch (std::exception& e)
	{
		std::cerr << "asio::write_some() exception: " << e.what() << "\n";
	}

//	asio::async_write(socket_, asio::buffer(data, size),
//		boost::bind(&connection::handle_write, shared_from_this(),
//		asio::placeholders::error));
}

void connection::handle_read_header(const asio::error_code& e,
	std::size_t bytes_transferred)
{
	if (!e)
	{
		if (bytes_transferred == 4)
		{
			if (!memcmp(buffer_.data(), "<c", 2) || !memcmp(buffer_.data(), "<p", 2))
			{
				//char buff[300];
				//socket_.read_some(asio::buffer(buff));
//						printf("%s", buff);
				asio::async_write(socket_, asio::buffer("<cross-domain-policy><allow-access-from domain=\"*\" to-ports=\"21-60000\" /></cross-domain-policy>\0"),
					boost::bind(&connection::handle_write, shared_from_this(),
					asio::placeholders::error));
				asio::async_read(socket_, asio::buffer(buffer_, size), boost::bind(&connection::handle_read_header, shared_from_this(),
					asio::placeholders::error,
					asio::placeholders::bytes_transferred));
			}
			else
			{
				size = *(int32_t*)buffer_.data();
				ByteSwap(size);

				asio::async_read(socket_, asio::buffer(buffer_, size), boost::bind(&connection::handle_read, shared_from_this(),
					asio::placeholders::error,
					asio::placeholders::bytes_transferred));
			}
		}
	}
	else if (e != asio::error::operation_aborted)
	{
		connection_manager_.stop(shared_from_this());
		return;
	}

}

void connection::handle_read(const asio::error_code& e,
									std::size_t bytes_transferred)
{
	if (!e)
	{
		if (bytes_transferred != size)
		{
			Log("Did not receive proper amount of bytes : %d", size);
			connection_manager_.stop(shared_from_this());
			return;
		}
		//printf("uid("XI64")\n", uid);
		// read object size
		if ((size > 8192*4) || (size <= 0))
		{
			//ERROR - object too large - close connection
			connection_manager_.stop(shared_from_this());
			return;
		}

		// parse packet
		request_.size = size;
		amf3parser * cparser = new amf3parser(buffer_.data());
		request_.object = cparser->ReadNextObject();
		request_.connection = this;
		request_handler_.handle_request(request_, reply_);
		delete cparser;
		if (reply_.objects.size() > 0)
		{
			// send reply packets
			try {
				socket_.write_some(reply_.to_buffers());
			}
			catch (std::exception& e)
			{
				std::cerr << "asio::write_some() exception: " << e.what() << "\n";
			}
			reply_.objects.clear();
// 			asio::async_write(socket_, reply_.to_buffers(),
// 				boost::bind(&connection::handle_write, shared_from_this(),
// 				asio::placeholders::error));
		}

		asio::async_read(socket_, asio::buffer(buffer_, 4), boost::bind(&connection::handle_read_header, shared_from_this(),
			asio::placeholders::error,
			asio::placeholders::bytes_transferred));
	}
	else if (e != asio::error::operation_aborted)
	{
		connection_manager_.stop(shared_from_this());
		return;
	}
}

void connection::handle_write(const asio::error_code& e)
{
	if (!e)
	{
// 		// Initiate graceful connection closure.
// 		asio::error_code ignored_ec;
// 		socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
	}

	if (e != asio::error::operation_aborted)
	{
		connection_manager_.stop(shared_from_this());
	}
}

} // namespace server
} // namespace http
