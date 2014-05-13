project(minetestmapper CXX)
cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0003 NEW)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}")

# try_compile does not remove the temporary build directory, so put it in CMakeFiles...
# Also: use absolute paths; cmake chokes on relative paths :-(
try_compile(CXX_C0X_SUPPORTED "${CMAKE_HOME_DIRECTORY}/CMakeFiles/CMakeTmp/c0x-test" "${CMAKE_HOME_DIRECTORY}/empty.cpp" CMAKE_FLAGS "-std=c++0x")
if(CXX_C0X_SUPPORTED)
	# Yes, I *know* c++0x is *not* an official C++ standard...
	message(STATUS "Compiler: C++ standard version 'c++0x' is supported")
elseif(CXX_C0X_SUPPORTED)
	message(FATAL_ERROR "Compiler: C++ standard version c++0x is not supported")
endif(CXX_C0X_SUPPORTED)
try_compile(CXX_C11_SUPPORTED "${CMAKE_HOME_DIRECTORY}/CMakeFiles/CMakeTmp/c11-test" "${CMAKE_HOME_DIRECTORY}/empty.cpp" CMAKE_FLAGS "-std=c++11")
if(CXX_C11_SUPPORTED)
	message(STATUS "Compiler: C++ standard version 'c++11' is supported")
else(CXX_C11_SUPPORTED)
	message(STATUS "Compiler: C++ standard version 'c++11' is not supported")
endif(CXX_C11_SUPPORTED)

# Determine C++ standard to use
# If CXX_CXX_STANDARD is empty, use autodetected c++ standard version
set(CXX_CXX_STANDARD "" CACHE STRING "C++ standard version to use. Leave empty, or set to c++11 or c++0x")
string(COMPARE NOTEQUAL "${CXX_CXX_STANDARD}" "" CXX_CXX_STANDARD_OVERRIDE)
if(CXX_CXX_STANDARD_OVERRIDE)
	string(COMPARE EQUAL "${CXX_CXX_STANDARD}" "c++11" CXX_C11_REQUESTED)
	string(COMPARE EQUAL "${CXX_CXX_STANDARD}" "c++0x" CXX_C0X_REQUESTED)
	if(CXX_C11_REQUESTED AND CXX_C11_SUPPORTED)
		message(STATUS "Compiler: C++ standard version 'c++11' selected (as requested)")
		set(CXX_USE_CXX_STANDARD "c++11")
	elseif(CXX_C11_REQUESTED AND NOT CXX_C11_SUPPORTED)
		message(FATAL_ERROR "Compiler: requested standard version '${CXX_CXX_STANDARD}' is not supported")
	elseif(CXX_C0X_REQUESTED)
		message(STATUS "Compiler: C++ standard version 'c++0x' selected (as requested)")
		set(CXX_USE_CXX_STANDARD "c++0x")
	else(CXX_C11_REQUESTED AND CXX_C11_SUPPORTED)
		message(FATAL_ERROR "Unrecognised c++ standard version requested: ${CXX_CXX_STANDARD}")
	endif(CXX_C11_REQUESTED AND CXX_C11_SUPPORTED)
else(CXX_CXX_STANDARD_OVERRIDE)
	if(CXX_C11_SUPPORTED)
		message(STATUS "Compiler: C++ standard version 'c++11' selected")
		set(CXX_USE_CXX_STANDARD "c++11")
	else(CXX_C11_SUPPORTED)
		message(STATUS "Compiler: C++ standard version 'c++0x' selected (c++11 not supported)")
		set(CXX_USE_CXX_STANDARD "c++0x")
	endif(CXX_C11_SUPPORTED)
endif(CXX_CXX_STANDARD_OVERRIDE)

# Clean targets after change in c++ standard, and remember current standard
string(COMPARE EQUAL "${CXX_CXX_STANDARD_LAST}" "" FIRST_CXX_STANDARD)
string(COMPARE NOTEQUAL "${CXX_CXX_STANDARD_LAST}" "${CXX_USE_CXX_STANDARD}" DIFFERENT_CXX_STANDARD)
set(CXX_CXX_STANDARD_LAST_TEMP "${CXX_CXX_STANDARD_LAST}")
set(CXX_CXX_STANDARD_LAST "${CXX_USE_CXX_STANDARD}" CACHE INTERNAL "Internal use - do not modify")
if(DIFFERENT_CXX_STANDARD AND NOT FIRST_CXX_STANDARD)
	execute_process(COMMAND "${CMAKE_BUILD_TOOL}" clean RESULT_VARIABLE CLEANING_RESULT OUTPUT_QUIET ERROR_QUIET)
	if(CLEANING_RESULT)
		message(STATUS "Clean previous build because of standard change (was: '${CXX_CXX_STANDARD_LAST_TEMP}') (exit: ${CLEANING_RESULT})")
	else(CLEANING_RESULT)
		message(STATUS "Clean previous build because of standard change (was: '${CXX_CXX_STANDARD_LAST_TEMP}')")
	endif(CLEANING_RESULT)
endif(DIFFERENT_CXX_STANDARD AND NOT FIRST_CXX_STANDARD)

set(CMAKE_CXX_FLAGS_RELEASE "-std=${CXX_USE_CXX_STANDARD} -Wall -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -g -std=${CXX_USE_CXX_STANDARD} -Wall -Wextra -DDEBUG")


# Find libgd
find_library(LIBGD_LIBRARY gd)
find_path(LIBGD_INCLUDE_DIR gd.h)
message (STATUS "libgd library: ${LIBGD_LIBRARY}")
message (STATUS "libgd headers: ${LIBGD_INCLUDE_DIR}")
if(NOT LIBGD_LIBRARY OR NOT LIBGD_INCLUDE_DIR)
	message(FATAL_ERROR "libgd not found!")
endif(NOT LIBGD_LIBRARY OR NOT LIBGD_INCLUDE_DIR)

# Find zlib
find_library(ZLIB_LIBRARY z)
find_path(ZLIB_INCLUDE_DIR zlib.h)
message (STATUS "zlib library: ${ZLIB_LIBRARY}")
message (STATUS "zlib headers: ${ZLIB_INCLUDE_DIR}")
if(NOT ZLIB_LIBRARY OR NOT ZLIB_INCLUDE_DIR)
	message(FATAL_ERROR "zlib not found!")
endif(NOT ZLIB_LIBRARY OR NOT ZLIB_INCLUDE_DIR)

find_package(PkgConfig)
include(FindPackageHandleStandardArgs)

# Find database(s)
set(USE_SQLITE3 0)
set(USE_LEVELDB 0)
set(USE_REDIS 0)

OPTION(ENABLE_ANY_DATABASE "Enable any available database backends")
OPTION(ENABLE_SQLITE3 "Enable sqlite3 backend" True)
OPTION(ENABLE_LEVELDB "Enable LevelDB backend")
OPTION(ENABLE_REDIS "Enable redis backend")

MACRO(backend_message IS_FATAL text)
	if(S_FATAL)
		message(FATAL_ERROR text)
	else(IS_FATAL)
		message(STATUS text)
	endif(IS_FATAL)
ENDMACRO(backend_message)

# Find sqlite3
if(ENABLE_SQLITE3 OR ENABLE_ANY_DATABASE)
	find_library(SQLITE3_LIBRARY sqlite3)
	find_path(SQLITE3_INCLUDE_DIR zlib.h)
	message (STATUS "sqlite3 library: ${SQLITE3_LIBRARY}")
	message (STATUS "sqlite3 headers: ${SQLITE3_INCLUDE_DIR}")
	if(SQLITE3_LIBRARY AND SQLITE3_INCLUDE_DIR)
		set(USE_SQLITE3 1)
		message(STATUS "sqlite3 backend enabled")
		include_directories(${SQLITE3_INCLUDE_DIR})
	else(SQLITE3_LIBRARY AND SQLITE3_INCLUDE_DIR)
		set(USE_SQLITE3 0)
		backend_message((NOT ENABLE_ANY_DATABASE) "sqlite3 backend requested but sqlite3 libraries not found!")
	endif(SQLITE3_LIBRARY AND SQLITE3_INCLUDE_DIR)
endif(ENABLE_SQLITE3 OR ENABLE_ANY_DATABASE)

# Find leveldb
if(ENABLE_LEVELDB OR ENABLE_ANY_DATABASE)
	find_library(LEVELDB_LIBRARY leveldb)
	find_path(LEVELDB_INCLUDE_DIR db.h PATH_SUFFIXES leveldb)
	message (STATUS "LevelDB library: ${LEVELDB_LIBRARY}")
	message (STATUS "LevelDB headers: ${LEVELDB_INCLUDE_DIR}")
	if(LEVELDB_LIBRARY AND LEVELDB_INCLUDE_DIR)
		set(USE_LEVELDB 1)
		message(STATUS "LevelDB backend enabled")
		include_directories(${LEVELDB_INCLUDE_DIR})
	else(LEVELDB_LIBRARY AND LEVELDB_INCLUDE_DIR)
		set(USE_LEVELDB 0)
		backend_message((NOT ENABLE_ANY_DATABASE) "LevelDB backend requested but LevelDB libraries not found!")
	endif(LEVELDB_LIBRARY AND LEVELDB_INCLUDE_DIR)
endif(ENABLE_LEVELDB OR ENABLE_ANY_DATABASE)

# Find redis
if(ENABLE_REDIS OR ENABLE_ANY_DATABASE)
	find_library(REDIS_LIBRARY hiredis)
	find_path(REDIS_INCLUDE_DIR hiredis.h PATH_SUFFIXES hiredis)
	message (STATUS "redis library: ${REDIS_LIBRARY}")
	message (STATUS "redis headers: ${REDIS_INCLUDE_DIR}")
	if(REDIS_LIBRARY AND REDIS_INCLUDE_DIR)
		set(USE_REDIS 1)
		message(STATUS "redis backend enabled")
		include_directories(${REDIS_INCLUDE_DIR})
	else(REDIS_LIBRARY AND REDIS_INCLUDE_DIR)
		set(USE_REDIS 0)
		backend_message((NOT ENABLE_ANY_DATABASE) "redis backend requested but redis libraries not found!")
	endif(REDIS_LIBRARY AND REDIS_INCLUDE_DIR)
endif(ENABLE_REDIS OR ENABLE_ANY_DATABASE)

if(NOT ENABLE_SQLITE3 AND NOT ENABLE_LEVELDB AND NOT ENABLE_REDIS)
	message(FATAL_ERROR "No database backends are configured, or none could be found")
endif(NOT ENABLE_SQLITE3 AND NOT ENABLE_LEVELDB AND NOT ENABLE_REDIS)

include_directories(
	"${PROJECT_BINARY_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_BINARY_DIR}"
	${LIBGD_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
)

configure_file(
	"${PROJECT_SOURCE_DIR}/cmake_config.h.in"
	"${PROJECT_BINARY_DIR}/cmake_config.h"
)
add_definitions ( -DUSE_CMAKE_CONFIG_H )

set(mapper_SRCS
	PixelAttributes.cpp
	PlayerAttributes.cpp
	TileGenerator.cpp
	ZlibDecompressor.cpp
	Color.cpp
	mapper.cpp
)

if(USE_SQLITE3)
	set(mapper_SRCS ${mapper_SRCS} db-sqlite3.cpp)
endif(USE_SQLITE3)

if(USE_LEVELDB)
	set(mapper_SRCS ${mapper_SRCS} db-leveldb.cpp)
endif(USE_LEVELDB)

if(USE_REDIS)
	set(mapper_SRCS ${mapper_SRCS} db-redis.cpp)
endif(USE_REDIS)

add_executable(minetestmapper
	${mapper_SRCS}
)

target_link_libraries(
	minetestmapper
	${SQLITE3_LIBRARY}
	${LEVELDB_LIBRARY}
	${REDIS_LIBRARY}
	${LIBGD_LIBRARY}
	${ZLIB_LIBRARY}
)

install(FILES "AUTHORS" DESTINATION ".")
install(FILES "COPYING" DESTINATION ".")
install(FILES "README.rst" DESTINATION ".")
install(FILES "colors.txt" DESTINATION ".")

# CPack

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Overview mapper for Minetest")
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VENDOR "celeron55")
set(CPACK_PACKAGE_CONTACT "Perttu Ahola <celeron55@gmail.com>")

if(WIN32)
	install(FILES "${PROJECT_BINARY_DIR}/minetestmapper.exe" DESTINATION ".")
	set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VERSION_STRING}-win32")
	set(CPACK_GENERATOR ZIP)
else()
	install(FILES "${PROJECT_BINARY_DIR}/minetestmapper" DESTINATION ".")
	set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VERSION_STRING}-linux")
	set(CPACK_GENERATOR TGZ)
	set(CPACK_SOURCE_GENERATOR TGZ)
endif()

include(CPack)
