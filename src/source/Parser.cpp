#include "finserver.h"

#include <iostream>          // std::cin
#include <cstring>           // strlen, memset
#include <cstdlib>           // malloc 
#include <cassert>           // assert

#define _BUFFER_SIZE BUFFER_SIZE
#undef  BUFFER_SIZE
#define BUFFER_SIZE 256

#define ALLOC_LIST(type, count) (type*)std::calloc(count, sizeof(void*))
#define MAKE_BUFF (char*)std::calloc(1, BUFFER_SIZE)
#define ASSN_STRING(name) name = MAKE_BUFF
#define STR_BUFFER(name) char* ASSN_STRING(name)
#define STR_LIST(name, count) char** name = ALLOC_LIST(char*, count)
#define FREE_BUFF(name) std::free(name)

#define COMMAND_FUNCTION(name) void name ( const char** args, const int argc, void* s, void* con )
#define COMMAND_METHOD(name) static COMMAND_FUNCTION(name)
#define NO_ARGS 0, 0, 0
#define MAKE_SERVER finapi::Server* server = (finapi::Server*)s
#define MAKE_CONNECTION finapi::Connection* connection = (finapi::Connection*)con;

#define ERROR_MSG(msg) LOG_S(ERROR) << msg
#define MUST_HAVE_ARGS(count, command)\
    ERROR_MSG("There must be " << count << " argument(s) for command '" << command << "'");\
    if (connection) connection->push("ARG:"#count, strlen("ARG:"#count))

using namespace finapi;
using namespace network;

COMMAND_METHOD(exists)
{
    MAKE_CONNECTION;

    if (argc == 1)
    {
        MUST_HAVE_ARGS(1, "exists");
        return;
    }

    bool exists = file::exists(args[1]);

    char* msg = (char*)std::malloc(BUFFER_SIZE);

    // If it exists send true otherwise send false
    if (exists) std::memcpy(msg, "T", 1);
    else        std::memcpy(msg, "F", 1);

    // If a connection is passed, send the response
    if (connection) connection->push(msg, 1);
    
    // Print the message to the server log
    LOG_S(INFO) << "File '" << args[1] << "' exists: " << msg;

    // Free the message
    FREE_BUFF(msg);
}

COMMAND_METHOD(request)
{
    MAKE_CONNECTION;

    if (argc == 1)
    {
        MUST_HAVE_ARGS(1, "REQ");
        return;
    }

    const char*    filename = args[1];
    const long int filesize = file::filesize(filename);
    unsigned int lower_bound, upper_bound;

    if (filesize == 0)
    {
        if (connection) connection->push("DNE", 3);
        LOG_S(ERROR) << "File '" << filename << "' does not exist.";
        return;
    }

    if (argc == 2)
    {
        lower_bound = 0;
        upper_bound = filesize;
    }
    else if (argc == 3)
    {
        unsigned int index = std::stoi( args[2] );

        lower_bound = _FIN_BUFFER_SIZE * index;
        upper_bound = lower_bound + _FIN_BUFFER_SIZE;
    }
    else if (argc == 4)
    {
        unsigned int frac  = std::stoi( args[2] );
        unsigned int index = std::stoi( args[3] );

        if (index >= frac) 
        {
            if (connection) connection->push("OOB", 3);
            LOG_S(ERROR) << "Command index is out of bounds";
            return;
        }

        lower_bound = (filesize / frac) * index;
        upper_bound = lower_bound + (filesize / frac);

        if (index == frac - 1)
            upper_bound += filesize - upper_bound;
    }

    char* file_data = (char*)std::malloc(upper_bound - lower_bound);

    file::readsection(filename, &file_data, lower_bound, upper_bound);
    if (connection) connection->push(file_data, upper_bound - lower_bound);
    LOG_S(INFO) << "Sending: " << upper_bound - lower_bound;
    std::memset(file_data, 0, upper_bound - lower_bound);
    FREE_BUFF(file_data);
}

COMMAND_METHOD(chunk)
{
    MAKE_CONNECTION;

    if (argc == 1)
    {
        MUST_HAVE_ARGS(1, "CHK");
        return;
    }

    if (!file::exists(args[1]))
    {
        if (connection) connection->push("NOFILE", 6);
        LOG_S(ERROR) << "File '" << args[1] << "' does not exist";
        return;
    }

    unsigned int chunks = file::chunks(args[1], _FIN_BUFFER_SIZE);
    if (connection) connection->push((const char*)&chunks, sizeof(unsigned int));
    LOG_S(INFO) << "The file '" << args[1] << "' has " << chunks << " chunks."; 
}

COMMAND_METHOD(size)
{
    MAKE_CONNECTION;

    if (argc == 1)
    {
        MUST_HAVE_ARGS(1, "SZE");
        return;
    }

    if (!file::exists(args[1]))
    {
        if (connection) connection->push("NOFILE", 6);
        LOG_S(ERROR) << "File '" << args[1] << "' does not exist";
        return;
    }

    unsigned int size = file::filesize(args[1]);
    if (connection) connection->push((const char*)&size, sizeof(unsigned int));
    LOG_S(INFO) << "The file '" << args[1] << "' is " << size << " bytes."; 
}

COMMAND_METHOD(login)
{
    MAKE_CONNECTION;
    if (!connection)
    {
        LOG_S(WARNING) << "Only external connections can perform a login.";
        return;
    }

    if (argc != 3)
    {
        LOG_S(ERROR) << "Required syntax: LOGIN 'username' 'password'";
        MUST_HAVE_ARGS(2, "LOGIN");
        return;
    }

    const char* USERNAME = "ADMIN";
    const char* PASSWORD = "ADMIN123";

    if (!strcmp(USERNAME, args[1]) && !strcmp(PASSWORD, args[2]))
    {
        if (connection) connection->push("OK", 2);
        LOG_S(WARNING) << "Connection logged in.";

        connection->permission = Admin;

        return;
    }

    if (connection) connection->push("LOG_FAIL", 8);
    LOG_S(ERROR) << "Connection submitted invalid credentials.";
}

COMMAND_METHOD(stream)
{
    MAKE_CONNECTION;

    if (argc <= 3)
    {
        MUST_HAVE_ARGS(4, "STRM");
        return;
    }

    unsigned int bytes = std::stoi(args[2]);
    unsigned int first = std::stoi(args[3]);

    char* buffer = (char*)std::malloc(bytes);
    std::memset(buffer, 0, bytes);

    std::ifstream file(args[1]);

    file.seekg(first);
    file.read(buffer, bytes);

    file.close();

    connection->push(buffer, bytes);

    std::free(buffer);
}

COMMAND_METHOD(connections)
{
    MAKE_SERVER;
    LOG_S(INFO) << "There are " << server->running_connections() << " connections.";
}

COMMAND_METHOD(exit_server)
{
    LOG_S(FATAL) << "Shutting down server.";
    exit(0);
}

COMMAND_METHOD(unknown)
{
    MAKE_CONNECTION;
    LOG_S(WARNING) << "Unknown command '" << args[0] << "'.";

    if (connection) connection->push("UNK", strlen("UNK"));
}

COMMAND_METHOD(no_permissions)
{
    MAKE_CONNECTION;
    LOG_S(ERROR) << "You must have higher permissions!";

    if (connection) connection->push("PRM", 3);
}

_command Parser::commands[] = {
    { connections, "connections", Root  },
    { exit_server, "exit",        Root  },
    { exists,      "exists",      Guest },
    { request,     "REQ",         Admin },
    { chunk,       "CHK",         Guest },
    { size,        "SZE",         Guest },
    { login,       "LOGIN",       Guest },
    { stream,      "STRM",        Admin }
};

void Parser::process_command(const char* command, void* server, void* connection)
{
    const unsigned int size = strlen(command);
    if (size == 0) return;

    // Count the spaces in the command
    unsigned int spaces = 1;
    for (int i = 0; i < size; i++)
        if (command[i] == ' ') spaces++;

    // Allocate space for the words
    STR_LIST(words, spaces);

    // Create indicies for the last space index, the current word list index
    // and an offset to apply when the very last index is reached
    unsigned int last_index = 0, words_index = 0, offset = 0;
    for (int i = 0; i < size; i++)
        if (command[i] == ' ' || i == size - 1)
        {
            // If the last index is reached, add one to the offset
            if (i == size - 1)
                offset = 1;

            // Allocate room for a word, then add the string terminator character at the string's end,
            // then copy the memory over from the command string over to the character buffer.
            *(words + words_index) = MAKE_BUFF;
            *(*(words + words_index) + (i - last_index)) = '\0';
            std::memcpy(*(words + words_index), command + last_index, i - last_index + offset);

            // Reset the offset, set the last space index, and increment the word index
            offset = 0;
            last_index = i + 1;
            words_index++;
        }

    // Search through the commands and find the matching function
    bool found = false;
    for (int i = 0; i < sizeof(commands) / sizeof(_command); i++)
        if (!strcmp(commands[i].verb, *(words)))
        {
            found = true;
            if (connection && ((finapi::Connection*)connection)->permission < commands[i].permission )
            {
                no_permissions( (const char**)words, spaces, server, connection );
                break;
            }

            commands[i].func( (const char**)words, spaces, server, connection );
            break;
        }

    if (!found) unknown( (const char**)words, spaces, server, connection );

    // Free each word buffer
    for (int i = 0; i < spaces; i++) FREE_BUFF(*(words + i));

    FREE_BUFF(words);
}

void Parser::process_input(void* server, void* connection)
{
    // Create a string buffer and get the current input
    STR_BUFFER(input);
    //get_input(input);
    std::cin.getline(input, BUFFER_SIZE);

    LOG_S(INFO) << "Received input (" << strlen(input) << "): " << input;

    /* Find the last character in the buffer
    unsigned int last_char = BUFFER_SIZE;
    for (int i = last_char - 1; i > -1; i--)
        if (input[i] > 32)
        {
            last_char = i + 1;
            break;
        } */

    // Finally, pass the input to the command processor
    process_command(input, server, connection);
    FREE_BUFF(input);
}

#undef  BUFFER_SIZE
#define BUFFER_SIZE _BUFFER_SIZE
#undef  _BUFFER_SIZE