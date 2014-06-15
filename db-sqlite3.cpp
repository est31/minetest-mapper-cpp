#include "db-sqlite3.h"
#include <stdexcept>
#include <unistd.h> // for usleep
#include "types.h"


DBSQLite3::DBSQLite3(const std::string &mapdir) :
	cacheWorldRow(false),
	m_blocksReadCount(0),
	m_blocksCachedCount(0),
	m_blocksUnCachedCount(0),
	m_blockPosListStatement(NULL),
	m_blocksOnZStatement(NULL),
	m_blockOnPosStatement(NULL)
{
	
	std::string db_name = mapdir + "map.sqlite";
	if (sqlite3_open_v2(db_name.c_str(), &m_db, SQLITE_OPEN_READONLY | SQLITE_OPEN_PRIVATECACHE, 0) != SQLITE_OK) {
		throw std::runtime_error(std::string(sqlite3_errmsg(m_db)) + ", Database file: " + db_name);
	}
}

DBSQLite3::~DBSQLite3() {
	if (m_blockPosListStatement) sqlite3_finalize(m_blockPosListStatement);
	if (m_blocksOnZStatement) sqlite3_finalize(m_blocksOnZStatement);
	if (m_blockOnPosStatement) sqlite3_finalize(m_blockOnPosStatement);
	sqlite3_close(m_db);
}

int DBSQLite3::getBlocksReadCount(void)
{
	return m_blocksReadCount;
}

int DBSQLite3::getBlocksCachedCount(void)
{
	return m_blocksCachedCount;
}

int DBSQLite3::getBlocksUnCachedCount(void)
{
	return m_blocksUnCachedCount;
}

const DB::BlockPosList &DBSQLite3::getBlockPos() {
	m_BlockPosList.clear();
	std::string sql = "SELECT pos FROM blocks";
	if (m_blockPosListStatement || sqlite3_prepare_v2(m_db, sql.c_str(), sql.length(), &m_blockPosListStatement, 0) == SQLITE_OK) {
		int result = 0;
		while (true) {
			result = sqlite3_step(m_blockPosListStatement);
			if(result == SQLITE_ROW) {
				sqlite3_int64 blocknum = sqlite3_column_int64(m_blockPosListStatement, 0);
				m_BlockPosList.push_back(blocknum);
			} else if (result == SQLITE_BUSY) // Wait some time and try again
				usleep(10000);
			else
				break;
		}
	} else {
		sqlite3_reset(m_blockPosListStatement);
		throw std::runtime_error("Failed to get list of MapBlocks");
	}
	sqlite3_reset(m_blockPosListStatement);
	return m_BlockPosList;
}

void DBSQLite3::prepareBlocksOnZStatement(void)
{
	//std::string sql = "SELECT pos, data FROM blocks WHERE (pos >= ? AND pos <= ?)";
	std::string sql = "SELECT pos, data FROM blocks WHERE (pos BETWEEN ? AND ?)";
	if (!m_blocksOnZStatement && sqlite3_prepare_v2(m_db, sql.c_str(), sql.length(), &m_blocksOnZStatement, 0) != SQLITE_OK) {
		throw std::runtime_error("Failed to prepare statement (blocksOnZStatement)");
	}
}

void DBSQLite3::prepareBlockOnPosStatement(void)
{
	std::string sql = "SELECT pos, data FROM blocks WHERE pos == ?";
	if (!m_blockOnPosStatement && sqlite3_prepare_v2(m_db, sql.c_str(), sql.length(), &m_blockOnPosStatement, 0) != SQLITE_OK) {
		throw std::runtime_error("Failed to prepare SQL statement (blockOnPosStatement)");
	}
}

void DBSQLite3::cacheBlocksOnZRaw(int zPos)
{
	prepareBlocksOnZStatement();

	sqlite3_int64 psMin;
	sqlite3_int64 psMax;

	psMin = (static_cast<sqlite3_int64>(zPos) * 0x1000000L) - 0x800000;
	psMax = (static_cast<sqlite3_int64>(zPos) * 0x1000000L) + 0x7fffff;

	sqlite3_bind_int64(m_blocksOnZStatement, 1, psMin);
	sqlite3_bind_int64(m_blocksOnZStatement, 2, psMax);

	cacheBlocks(m_blocksOnZStatement);
}

DB::Block DBSQLite3::getBlockOnPosRaw(const BlockPos &pos)
{
	prepareBlockOnPosStatement();

	Block block(pos,reinterpret_cast<const unsigned char *>(""));
	int result = 0;

	sqlite3_bind_int64(m_blockOnPosStatement, 1, pos.databasePosI64());

	while (true) {
		result = sqlite3_step(m_blockOnPosStatement);
		if(result == SQLITE_ROW) {
			const unsigned char *data = reinterpret_cast<const unsigned char *>(sqlite3_column_blob(m_blockOnPosStatement, 1));
			int size = sqlite3_column_bytes(m_blockOnPosStatement, 1);
			block = Block(pos, ustring(data, size));
			m_blocksUnCachedCount++;
			break;
		} else if (result == SQLITE_BUSY) { // Wait some time and try again
			usleep(10000);
		} else {
			break;
		}
	}
	sqlite3_reset(m_blockOnPosStatement);

	return block;
}

void DBSQLite3::cacheBlocks(sqlite3_stmt *SQLstatement)
{
	int result = 0;
	while (true) {
		result = sqlite3_step(SQLstatement);
		if(result == SQLITE_ROW) {
			sqlite3_int64 blocknum = sqlite3_column_int64(SQLstatement, 0);
			const unsigned char *data = reinterpret_cast<const unsigned char *>(sqlite3_column_blob(SQLstatement, 1));
			int size = sqlite3_column_bytes(SQLstatement, 1);
			m_blockCache[blocknum] = ustring(data, size);
			m_blocksCachedCount++;
		} else if (result == SQLITE_BUSY) { // Wait some time and try again
			usleep(10000);
		} else {
			break;
		}
	}
	sqlite3_reset(SQLstatement);
}

DB::Block DBSQLite3::getBlockOnPos(const BlockPos &pos)
{
	m_blocksReadCount++;

	BlockCache::const_iterator DBBlockSearch;
	DBBlockSearch = m_blockCache.find(pos.databasePosI64());
	if (DBBlockSearch == m_blockCache.end()) {
		if (cacheWorldRow) {
			m_blockCache.clear();
			cacheBlocksOnZRaw(pos.z);
			DBBlockSearch = m_blockCache.find(pos.databasePosI64());
			if (DBBlockSearch != m_blockCache.end()) {
				return Block(pos, DBBlockSearch->second);
			}
			else {
				return Block(pos,reinterpret_cast<const unsigned char *>(""));
			}
		}
		else {
			return getBlockOnPosRaw(pos);
		}
	}
	else {
		return Block(pos, DBBlockSearch->second);
	}
}

