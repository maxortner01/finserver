#pragma once

#include <fstream>
#include <sys/stat.h> // stat

namespace finapi
{
namespace file
{
    static bool exists(const char* filename)
    {
        struct stat buffer;
        return (stat(filename, &buffer) == 0);
    }

    static long int filesize(const char* filename)
    {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        return file.tellg();
    }

    static void readsection(const char* filename, char** buffer, const unsigned int first, const unsigned int last)
    {
        char*& buff = *(buffer);

        std::ifstream file(filename, std::ios::binary);
        file.seekg(first);
        file.read(buff, last - first);
        file.close();
    }

    static unsigned int chunks(const char* filename, const unsigned int size)
    {
        return (unsigned int)(filesize(filename) / size) + 1;
    }
}
}