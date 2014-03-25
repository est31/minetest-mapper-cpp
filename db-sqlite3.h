#ifndef _DB_SQLITE3_H
#define _DB_SQLITE3_H

#include "db.h"
#include <sqlite3.h>
#if _cplusplus == 201103L
#include <unordered_map>
#else
#include <map>
#endif
#include <string>
#include <sstream>

class DBSQLite3 : public DB {
#if _cplusplus == 201103L
	typedef std::unordered_map<uint64_t, DBBlock>  BlockCache;
#else
	typedef std::map<uint64_t, DBBlock>  BlockCache;
#endif
public:
	bool cacheWorldRow;
	DBSQLite3(const std::string &mapdir);
	virtual int getBlocksUnCachedCount(void);
	virtual int getBlocksCachedCount(void);
	virtual int getBlocksReadCount(void);
	virtual std::vector<int64_t> getBlockPos();
	virtual DBBlock getBlockOnPos(int x, int y, int z);
	~DBSQLite3();
private:
	int m_blocksReadCount;
	int m_blocksCachedCount;
	int m_blocksUnCachedCount;
	sqlite3 *m_db;
	sqlite3_stmt *m_blockPosListStatement;
	sqlite3_stmt *m_blocksOnZStatement;
	sqlite3_stmt *m_blockOnPosStatement;
	sqlite3_stmt *m_blocksYRangeStatement;
	std::ostringstream  m_getBlockSetStatementBlocks;
	BlockCache  m_blockCache;

	void prepareBlocksOnZStatement(void);
	void prepareBlockOnPosStatement(void);
	void prepareBlocksYRangeStatement(void);
	void cacheBlocksYRangeRaw(int x, int y, int z);
	void cacheBlocksOnZRaw(int zPos);
	DBBlock getBlockOnPosRaw(sqlite3_int64 psPos);
	void cacheBlocks(sqlite3_stmt *SQLstatement);
};

#endif // _DB_SQLITE3_H
