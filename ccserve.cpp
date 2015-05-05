/*
    ccserve: credit-card generator server
    Copyright (C) 2015 Graham King

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <memory>
#include <unordered_map>
#include <ctime>
#include <string>

#include <asio.hpp>

#include "ccgen.h"

#define PORT 1212

const std::string headers {R"MARKER(HTTP/1.0 200 OK
Connection: close
Access-Control-Allow-Origin: *
Content-type: text/html
Content-length: )MARKER"};

void handle_in(unsigned int, const asio::error_code&);
void handle_out(unsigned int conn_id, const asio::error_code& e, std::size_t size);
void done(const unsigned int, const asio::error_code&, std::size_t);

const char* time_format {"%Y-%m-%dT%H:%M:%S"};
std::string now() {
	std::time_t t = std::time(nullptr);
	std::tm tm = *std::localtime(&t);
	char* now = new char[20];
	std::strftime(now, 20, time_format, &tm);
	std::string s {now};
	delete[] now;
	return s;
}

class Conn {
public:
	Conn(unsigned int conn_id, asio::io_service& io) : sock(io), id(conn_id) { };
	void read();
	void write();
	void close() { sock.close(); };
	asio::ip::tcp::socket& get_socket() { return sock; };
private:
	asio::ip::tcp::socket sock;
	unsigned int id;
	asio::streambuf b;
};

void Conn::write()
{
	std::string out {headers};
	std::string ccbody {ccgen()};
	out += std::to_string(ccbody.size());
	out += +"\r\n\r\n";
	out += ccbody;

	asio::async_write(
		sock,
		asio::buffer(out),
		std::bind(&done, id, std::placeholders::_1, std::placeholders::_2)
	);
}

void Conn::read()
{
	asio::async_read_until(
		sock,
		b,
		"\r\n\r\n",
		std::bind(&handle_out, id, std::placeholders::_1, std::placeholders::_2)
	);
}

class Tcp_server
{
public:
	Tcp_server(asio::io_service& io)
		: acceptor(io)
	{
		std::cout << '[' << now() << "] ccserve start\n";

		asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), PORT);
		acceptor.open(endpoint.protocol());
		acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
		acceptor.bind(endpoint);
		acceptor.listen();

		accept_next();
	}

	void accept_next() {
		unsigned int cid = next_conn_id;
		next_conn_id = ++next_conn_id % std::numeric_limits<int>::max();
		auto res = conns.emplace(
			cid,
			new Conn(cid, acceptor.get_io_service())
		);
		auto iter = res.first;
		acceptor.async_accept(
			iter->second->get_socket(),
			std::bind(&handle_in, cid, std::placeholders::_1)
		);
	}

	void read_conn(const unsigned int conn_id) { conns[conn_id]->read(); }

	void write_conn(const unsigned int conn_id) { conns[conn_id]->write(); }

	void close(const unsigned int conn_id) {
		Conn* c = conns[conn_id];
		c->close();
		delete c;
		conns.erase(conn_id);
	}

private:
	asio::ip::tcp::acceptor acceptor;
	std::unordered_map<unsigned int, Conn*> conns;
	unsigned int next_conn_id;
};

std::unique_ptr<Tcp_server> server;

// completion handler
void done(
		const unsigned int conn_id,
		const asio::error_code& error,
		std::size_t bytes_transferred)
{
	server->close(conn_id);
}

// accept handler
void handle_in(unsigned int conn_id, const asio::error_code& error)
{
	server->read_conn(conn_id);
	server->accept_next();
}

// called when we have consumed input
void handle_out(unsigned int conn_id, const asio::error_code& e, std::size_t size)
{
	server->write_conn(conn_id);
}

int main()
{
	try
	{
		asio::io_service io;
		server = std::make_unique<Tcp_server>(io);
		io.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Global exception: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
