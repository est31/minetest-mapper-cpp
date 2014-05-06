#ifndef _DB_SQLITE3_H
#define _DB_SQLITE3_H

#include "db.h"
#include <sqlite3.h>
#if _cplusplus >= 201103L
#include <unordered_map>
#else
#include <map>
#endif
#include <string>
#include <sstream>

#include "types.h"

class DBSQLite3 : public DB {
#if _cplusplus >= 201103L
	typedef std::unordered_map<int64_t, ustring>  BlockCache;
#else
	typedef std::map<int64_t, ustring>  BlockCache;
#endif
public:
	bool cacheWorldRow;
	DBSQLite3(const std::string &mapdir);
	virtual int getBlocksUnCachedCount(void);
	virtual int getBlocksCachedCount(void);
	virtual int getBlocksReadCount(void);
	virtual const BlockPosList &getBlockPos();
	virtual Block getBlockOnPos(const BlockPos &pos);
	~DBSQLite3();
private:
	int m_blocksReadCount;
	int m_blocksCachedCount;
	int m_blocksUnCachedCount;
	sqlite3 *m_db;
	sqlite3_stmt *m_blockPosListStatement;
	sqlite3_stmt *m_blocksOnZStatement;
	sqlite3_stmt *m_blockOnPosStatement;
	std::ostringstream  m_getBlockSetStatementBlocks;
	BlockCache  m_blockCache;
	BlockPosList m_BlockPosList;

	void prepareBlocksOnZStatement(void);
	void prepareBlockOnPosStatement(void);
	void cacheBlocksOnZRaw(int zPos);
	Block getBlockOnPosRaw(const BlockPos &pos);
	void cacheBlocks(sqlite3_stmt *SQLstatement);
};

#endif // _DB_SQLITE3_H
