#ifndef _DB_H
#define _DB_H

#include <stdint.h>
#include <vector>
#include <list>
#include <string>
#include <utility>

// we cannot use ... char>> here because mingw-gcc is f**king retarded (caring about whitespace and shit)
typedef std::pair<int64_t, std::basic_string<unsigned char> > DBBlock;
typedef std::list<DBBlock> DBBlockList;

class DB {
public:
	virtual std::vector<int64_t> getBlockPos()=0;
	virtual int getBlocksUnCachedCount(void)=0;
	virtual int getBlocksCachedCount(void)=0;
	virtual int getBlocksReadCount(void)=0;
	virtual DBBlock getBlockOnPos(int x, int y, int z)=0;
};

#endif // _DB_H
