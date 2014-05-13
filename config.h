/*
 * =====================================================================
 *        Version:  1.0
 *        Created:  01.09.2012 12:58:02
 *         Author:  Miroslav Bendík
 *        Company:  LinuxOS.sk
 * =====================================================================
 */

#if MSDOS || __OS2__ || __NT__ || _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

#define BLOCK_SIZE	16
#define MAPBLOCK_MIN	(-2048)
#define MAPBLOCK_MAX	2047


#ifdef USE_CMAKE_CONFIG_H
#include "cmake_config.h"
#else
#define USE_LEVELDB 0
#endif

// List of possible database names (for usage message)
#if USE_SQLITE3
#define USAGE_NAME_SQLITE "sqlite3"
#else
#define USAGE_NAME_SQLITE
#endif
#if USE_SQLITE3 && USE_LEVELDB
#define USAGE_SEP_SQLITE_LEVELDB "/"
#else
#define USAGE_SEP_SQLITE_LEVELDB
#endif
#if USE_LEVELDB
#define USAGE_NAME_LEVELDB "leveldb"
#else
#define USAGE_NAME_LEVELDB
#endif
#if USE_LEVELDB && USE_REDIS
#define USAGE_SEP_LEVELDB_REDIS "/"
#else
#define USAGE_SEP_LEVELDB_REDIS
#endif
#if USE_REDIS
#define USAGE_NAME_REDIS "redis"
#else
#define USAGE_NAME_REDIS
#endif
#define USAGE_DATABASES USAGE_NAME_SQLITE USAGE_SEP_SQLITE_LEVELDB USAGE_NAME_LEVELDB USAGE_SEP_LEVELDB_REDIS USAGE_NAME_REDIS

// default database to use
#if USE_SQLITE3
#define DEFAULT_BACKEND "sqlite3"
#elif USE_LEVELDB
#define DEFAULT_BACKEND "leveldb"
#elif USE_REDIS
#define DEFAULT_BACKEND "redis"
#else
#error No database backends configured !
#endif

