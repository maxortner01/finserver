#pragma once

namespace finapi
{
namespace network
{
    enum Permission
    {
        Guest,
        Admin,
        Root
    };

    /**
     * @brief Basic container for a function pointer and an associated string.
     */
    struct _command
    {
        void (*func)(const char**, const int, void*, void*);
        const char* verb;
        Permission  permission;
    };

    /**
     * @brief Class for handling and parsing of input as well as command execution.
     */
    class Parser
    {
        static _command commands[];
    public:

        /**
         * @brief Parses a given command string and executes its associated method.
         * 
         * @param command String of input from the user.
         */
        static void process_command(const char* command, void* server, void* connection);

        /**
         * @brief Gets the input and executes the command.
         */
        void process_input(void* server, void* connection);
    };
}
}