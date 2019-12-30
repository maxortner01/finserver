#include "finserver.h"

#include <iostream>
#include <errno.h>
#include <string.h>
#include <cstring>

using namespace finapi;

int Connection::push(const char* message, const unsigned int size) const
{
    LOG_S(INFO) << "Sending " << size << " bytes to client.";
    return send(_socket, message, size, 0);  
}

/* <-------------- PRIVATE METHODS ---------------> */

/* <--------------- STATIC METHODS ---------------> */

void Server::_process_connection(Connection* connection)
{
    char* data_buffer = (char*)std::malloc(_FIN_BUFFER_SIZE);
    unsigned int buffer_length = 0;
    do
    {
        std::memset(data_buffer, 0, _FIN_BUFFER_SIZE);
        buffer_length = recv(connection->_socket, data_buffer, _FIN_BUFFER_SIZE, 0);

        if (buffer_length > 0)
        {
            LOG_S(INFO) << "Received " << buffer_length << " bytes from " << connection->ip_addr << ": " << data_buffer;

            network::Parser::process_command(data_buffer, nullptr, connection);
        }

    } while (buffer_length > 0);
    std::free(data_buffer);

    LOG_S(WARNING) << "Closing connection from " << connection->ip_addr;
    if (shutdown(connection->_socket, SHUT_RDWR) < 0)
        LOG_S(ERROR) << "Shutdown failed with error code " << errno << ": " << strerror( errno );
        
    connection->finished = true;
}

void Server::_process_commandline(Server* server)
{
    while (server->_running)
        server->_parser.process_input((void*)server, nullptr);
}

/* <----------------------------------------------> */

void Server::_clean_connections() noexcept
{
    for (int i = 0; i < connections.size(); i++)
        if (connections[i]->finished)
        {
            connections[i]->thread->join();
            delete connections[i]->thread;
            delete connections[i];
            connections.erase(connections.begin() + i);
            i--;
        }
}

/* <----------------------------------------------> */

/* <-------------- PUBLIC METHODS ----------------> */

Server::Server()    
{
    _socket = network::make_socket();
    if (!network::set_options(_socket))
    {
        LOG_F(FATAL, "Set socket options failed!");
        exit(1);
        return;
    }
        
    _addr = network::bind_socket(_socket, 1420);
    if (!network::make_listen(_socket, 3))
    {
        LOG_F(FATAL, "Socket listen failed!");
        exit(1);
        return;
    }

    _running = true;
}

Server::~Server()
{
    // Turn off running which should stop the thread
    _running = false;
    command_line_thread->join();
    delete command_line_thread;
}

void Server::run() noexcept
{
    // Start the command line thread
    LOG_S(INFO) << "Starting Server.";
    LOG_S(INFO) << "Listening for connections...";
    command_line_thread = new std::thread(_process_commandline, this);

    while (_running)
    {
        // Create a new connection object and allocate the memory for the address object
        // and accept the next socket connection
        Connection* connection = new Connection;
        connection->_addr   = new sockaddr_in;
        connection->_socket = network::accept_socket(_socket, connection->_addr, &connection->addr_len);

        // At this point a connection has been established. Check whether or not an error occured.
        // Print a message if so and free up the previously allocated connection.
        if (connection->_socket < 0)
        {
            LOG_S(ERROR) << "Connection refused. Error: " << errno << ": " << strerror(errno);
            std::free(connection);
            continue;
        }

        // Otherwise, check the list real quick for other connections that have already finished.
        // Then allocate a string in the connection for the IP Address.
        _clean_connections();            
        connection->ip_addr = (char*)std::malloc(INET_ADDRSTRLEN);
        in_addr ip_addr     = connection->_addr->sin_addr;
        inet_ntop(AF_INET, &ip_addr, connection->ip_addr, INET_ADDRSTRLEN);

        LOG_S(WARNING) << "Connection accepted from " << connection->ip_addr << ".";

        // Create the thread, which will run automatically
        connection->thread = new std::thread(_process_connection, connection);
        connections.push_back(connection);
    }
}

/* <----------------------------------------------> */
