#include "db-sqlite3.h"
#include <stdexcept>
#include <unistd.h> // for usleep


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

std::vector<int64_t> DBSQLite3::getBlockPos() {
	std::vector<int64_t> vec;
	std::string sql = "SELECT pos FROM blocks";
	if (m_blockPosListStatement || sqlite3_prepare_v2(m_db, sql.c_str(), sql.length(), &m_blockPosListStatement, 0) == SQLITE_OK) {
		int result = 0;
		while (true) {
			result = sqlite3_step(m_blockPosListStatement);
			if(result == SQLITE_ROW) {
				sqlite3_int64 blocknum = sqlite3_column_int64(m_blockPosListStatement, 0);
				vec.push_back(blocknum);
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
	return vec;
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

// Apparently, this attempt at being smart, is actually quite inefficient for sqlite.
//
// For larger subsections of the map, it performs much worse than caching an entire
// world row (i.e. z coordinate).  In cases where it *is* more efficient, no caching is
// much more efficient still.
//
// It seems that any computation on pos severely affects the performance (?)...
//
// For the moment, this function is not used.
void DBSQLite3::prepareBlocksYRangeStatement(void)
{
	// This one seems to perform best:
	std::string sql = "SELECT pos, data FROM blocks WHERE (pos BETWEEN ?1 AND ?2) AND (pos-?3)&4095 == 0 AND (pos-?3 BETWEEN ?4 AND ?5)";
	// These perform worse:
	//std::string sql = "SELECT pos, data FROM blocks WHERE (pos BETWEEN ?1 AND ?2) AND (pos-?3 BETWEEN ?4 AND ?5) AND (pos-?3)&4095 == 0";
	//std::string sql = "SELECT pos, data FROM blocks WHERE (pos BETWEEN ?1 AND ?2) AND (pos-?3 BETWEEN ?4 AND ?5)";
	//std::string sql = "SELECT pos, data FROM (select pos, data FROM blocks WHERE (pos BETWEEN ?1 AND ?2) ) WHERE (pos-?3 BETWEEN ?4 AND ?5) AND (pos-?3)&4095 == 0";
	//std::string sql = "SELECT pos, data FROM (select pos, (pos-?3) AS pos3, data FROM blocks WHERE (pos BETWEEN ?1 AND ?2) ) WHERE (pos3 BETWEEN ?4 AND ?5) AND (pos3)&4095 == 0";
	//std::string sql = "SELECT pos, data FROM (select pos, (pos-?3) AS pos3, data FROM blocks WHERE (pos BETWEEN ?1 AND ?2) ) WHERE (pos3 BETWEEN ?4 AND ?5)";
	if (!m_blocksYRangeStatement && sqlite3_prepare_v2(m_db, sql.c_str(), sql.length(), &m_blocksYRangeStatement, 0) != SQLITE_OK) {
		throw std::runtime_error("Failed to prepare SQL statement (blocksYRangeStatement)");
	}
}

void DBSQLite3::cacheBlocksOnZRaw(int zPos)
{
	prepareBlocksOnZStatement();

	DBBlockList blocks;

	sqlite3_int64 psMin;
	sqlite3_int64 psMax;

	psMin = (static_cast<sqlite3_int64>(zPos) * 16777216l) - 0x800000;
	psMax = (static_cast<sqlite3_int64>(zPos) * 16777216l) + 0x7fffff;

	sqlite3_bind_int64(m_blocksOnZStatement, 1, psMin);
	sqlite3_bind_int64(m_blocksOnZStatement, 2, psMax);

	cacheBlocks(m_blocksOnZStatement);
}

DBBlock DBSQLite3::getBlockOnPosRaw(sqlite3_int64 psPos)
{
	prepareBlockOnPosStatement();

	DBBlock block(0,(const unsigned char *)"");
	int result = 0;

	sqlite3_bind_int64(m_blockOnPosStatement, 1, psPos);

	while (true) {
		result = sqlite3_step(m_blockOnPosStatement);
		if(result == SQLITE_ROW) {
			sqlite3_int64 blocknum = sqlite3_column_int64(m_blockOnPosStatement, 0);
			const unsigned char *data = reinterpret_cast<const unsigned char *>(sqlite3_column_blob(m_blockOnPosStatement, 1));
			int size = sqlite3_column_bytes(m_blockOnPosStatement, 1);
			block = DBBlock(blocknum, std::basic_string<unsigned char>(data, size));
			m_blocksUnCachedCount++;
			//std::cerr << "Read block " << blocknum << " from database" << std::endl;
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

void DBSQLite3::cacheBlocksYRangeRaw(int x, int y, int z)
{
	prepareBlocksYRangeStatement();

	sqlite3_int64 psZPosFrom = (static_cast<sqlite3_int64>(z) << 24) - 0x800000;
	sqlite3_int64 psZPosTo   = (static_cast<sqlite3_int64>(z) << 24) + 0x7fffff;
	sqlite3_int64 psPosZero  =  static_cast<sqlite3_int64>(x);
	              psPosZero  += static_cast<sqlite3_int64>(z) << 24;
	sqlite3_int64 psYPosFrom =  0;
	sqlite3_int64 psYPosTo   =  static_cast<sqlite3_int64>(y) << 12;

	sqlite3_bind_int64(m_blocksYRangeStatement, 1, psZPosFrom);
	sqlite3_bind_int64(m_blocksYRangeStatement, 2, psZPosTo);
	sqlite3_bind_int64(m_blocksYRangeStatement, 3, psPosZero);
	sqlite3_bind_int64(m_blocksYRangeStatement, 4, psYPosFrom);
	sqlite3_bind_int64(m_blocksYRangeStatement, 5, psYPosTo);

	cacheBlocks(m_blocksYRangeStatement);
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
			m_blockCache[blocknum] = DBBlock(blocknum, std::basic_string<unsigned char>(data, size));
			m_blocksCachedCount++;
			//std::cerr << "Cache block " << blocknum << " from database" << std::endl;
		} else if (result == SQLITE_BUSY) { // Wait some time and try again
			usleep(10000);
		} else {
			break;
		}
	}
	sqlite3_reset(SQLstatement);
}

DBBlock DBSQLite3::getBlockOnPos(int x, int y, int z)
{
	sqlite3_int64 psPos;
	psPos =  static_cast<sqlite3_int64>(x);
	psPos += static_cast<sqlite3_int64>(y) << 12;
	psPos += static_cast<sqlite3_int64>(z) << 24;
	//std::cerr << "Block " << x << "," << y << "," << z << " -> " << psPos << std::endl;

	m_blocksReadCount++;

	BlockCache::const_iterator DBBlockSearch;
	DBBlockSearch = m_blockCache.find(psPos);
	if (DBBlockSearch == m_blockCache.end()) {
		if (cacheWorldRow) {
			m_blockCache.clear();
			cacheBlocksOnZRaw(z);
			DBBlockSearch = m_blockCache.find(psPos);
			if (DBBlockSearch != m_blockCache.end()) {
				return DBBlockSearch->second;
			}
			else {
				return DBBlock(0, (const unsigned char *)"");
			}
		}
		else {
			return getBlockOnPosRaw(psPos);
		}
	}
	else {
		return DBBlockSearch->second;
	}
}

