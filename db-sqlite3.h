#ifndef _DB_SQLITE3_H
#define _DB_SQLITE3_H

#include "db.h"
#include <sqlite3.h>
#include <map>

class DBSQLite3 : public DB {
public:
	DBSQLite3(const std::string &mapdir);
	virtual std::vector<int64_t> getBlockPos();
	virtual DBBlockList getBlocksOnZ(int zPos);
	virtual DBBlock getBlockOnPos(int64_t iPos);
	~DBSQLite3();
private:
	sqlite3 *m_db;
	sqlite3_stmt *m_getBlockPosStatement;
	sqlite3_stmt *m_getBlocksOnZStatement;
	sqlite3_stmt *m_getBlocksOnPosStatement;
};

#endif // _DB_SQLITE3_H
