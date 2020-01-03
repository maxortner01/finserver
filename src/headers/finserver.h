#pragma once

#define FIN_SERVER_VERSION_MAJOR "1"
#define FIN_SERVER_VERSION_MINOR "1"
#define FIN_SERVER_VERSION_FIX   "0"
#define FIN_SERVER_VERSION_STR\
    FIN_SERVER_VERSION_MAJOR "." FIN_SERVER_VERSION_MINOR "." FIN_SERVER_VERSION_FIX

#include <finapi/finapi.h>

#include "Server.h"
#include "FileManip.h"
#include "Parser.h"
#include "loguru.hpp"