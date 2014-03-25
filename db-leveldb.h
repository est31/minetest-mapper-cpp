#ifndef _DB_LEVELDB_H
#define _DB_LEVELDB_H

#include "db.h"
#include <leveldb/db.h>
#include <set>

class DBLevelDB : public DB {
public:
	DBLevelDB(const std::string &mapdir);
	virtual int getBlocksUnCachedCount(void);
	virtual int getBlocksCachedCount(void);
	virtual int getBlocksReadCount(void);
	virtual std::vector<int64_t> getBlockPos();
	virtual DBBlock getBlockOnPos(int x, int y, int z);
	~DBLevelDB();
private:
	int m_blocksReadCount;
	int m_blocksCachedCount;
	int m_blocksUnCachedCount;
	leveldb::DB *m_db;
	std::set<int64_t> m_bpcache;
};

#endif // _DB_LEVELDB_H
