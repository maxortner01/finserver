#pragma once

#include <finapi/finapi.h>

#include "Parser.h"

namespace finapi
{

    struct Connection : public network::object
    {
        network::Permission permission = network::Guest;
        bool                finished   = false;
        
        int   addr_len;
        char* ip_addr;

        std::thread* thread;

        int push(const char* message, const unsigned int length) const;
        ~Connection() { std::free(ip_addr); }
    };

    class Server : protected network::object
    {
        bool         _running;

        network::Parser _parser;
        std::thread* command_line_thread;
        std::vector<Connection*> connections;

        static void _process_connection (Connection* connection);
        static void _process_commandline(Server* server);

        void _clean_connections() noexcept;

    public:
        Server();
        ~Server();

        void run() noexcept;

        const     int  running_connections() noexcept { _clean_connections(); return connections.size(); }
        constexpr bool running() const noexcept { return _running; }
    };
}