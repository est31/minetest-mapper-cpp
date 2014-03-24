#include "db-sqlite3.h"
#include <stdexcept>
#include <unistd.h> // for usleep

DBSQLite3::DBSQLite3(const std::string &mapdir) :
	m_getBlockPosStatement(NULL),
	m_getBlocksOnZStatement(NULL),
	m_getBlocksOnPosStatement(NULL)
{
	
	std::string db_name = mapdir + "map.sqlite";
	if (sqlite3_open_v2(db_name.c_str(), &m_db, SQLITE_OPEN_READONLY | SQLITE_OPEN_PRIVATECACHE, 0) != SQLITE_OK) {
		throw std::runtime_error(std::string(sqlite3_errmsg(m_db)) + ", Database file: " + db_name);
	}
}

DBSQLite3::~DBSQLite3() {
	if (m_getBlockPosStatement) sqlite3_finalize(m_getBlockPosStatement);
	if (m_getBlocksOnZStatement) sqlite3_finalize(m_getBlocksOnZStatement);
	if (m_getBlocksOnPosStatement) sqlite3_finalize(m_getBlocksOnPosStatement);
	sqlite3_close(m_db);
}

std::vector<int64_t> DBSQLite3::getBlockPos() {
	std::vector<int64_t> vec;
	std::string sql = "SELECT pos FROM blocks";
	if (m_getBlockPosStatement || sqlite3_prepare_v2(m_db, sql.c_str(), sql.length(), &m_getBlockPosStatement, 0) == SQLITE_OK) {
		int result = 0;
		while (true) {
			result = sqlite3_step(m_getBlockPosStatement);
			if(result == SQLITE_ROW) {
				sqlite3_int64 blocknum = sqlite3_column_int64(m_getBlockPosStatement, 0);
				vec.push_back(blocknum);
			} else if (result == SQLITE_BUSY) // Wait some time and try again
				usleep(10000);
			else
				break;
		}
	} else {
		throw std::runtime_error("Failed to get list of MapBlocks");
	}
	return vec;
}

DBBlockList DBSQLite3::getBlocksOnZ(int zPos)
{
	std::string sql = "SELECT pos, data FROM blocks WHERE (pos >= ? AND pos <= ?)";
	if (!m_getBlocksOnZStatement && sqlite3_prepare_v2(m_db, sql.c_str(), sql.length(), &m_getBlocksOnZStatement, 0) != SQLITE_OK) {
		throw std::runtime_error("Failed to prepare statement");
	}
	DBBlockList blocks;

	sqlite3_int64 psMin;
	sqlite3_int64 psMax;

	psMin = (static_cast<sqlite3_int64>(zPos) * 16777216l) - 0x800000;
	psMax = (static_cast<sqlite3_int64>(zPos) * 16777216l) + 0x7fffff;
	sqlite3_bind_int64(m_getBlocksOnZStatement, 1, psMin);
	sqlite3_bind_int64(m_getBlocksOnZStatement, 2, psMax);

	int result = 0;
	while (true) {
		result = sqlite3_step(m_getBlocksOnZStatement);
		if(result == SQLITE_ROW) {
			sqlite3_int64 blocknum = sqlite3_column_int64(m_getBlocksOnZStatement, 0);
			const unsigned char *data = reinterpret_cast<const unsigned char *>(sqlite3_column_blob(m_getBlocksOnZStatement, 1));
			int size = sqlite3_column_bytes(m_getBlocksOnZStatement, 1);
			blocks.push_back(DBBlock(blocknum, std::basic_string<unsigned char>(data, size)));
		} else if (result == SQLITE_BUSY) { // Wait some time and try again
			usleep(10000);
		} else {
			break;
		}
	}
	sqlite3_reset(m_getBlocksOnZStatement);

	return blocks;
}

DBBlock DBSQLite3::getBlockOnPos(int64_t iPos)
{
	std::string sql = "SELECT pos, data FROM blocks WHERE pos == ?";
	if (!m_getBlocksOnPosStatement && sqlite3_prepare_v2(m_db, sql.c_str(), sql.length(), &m_getBlocksOnPosStatement, 0) != SQLITE_OK) {
		throw std::runtime_error("Failed to prepare statement");
	}
	DBBlock block(0,(const unsigned char *)"");

	sqlite3_int64 psPos = static_cast<sqlite3_int64>(iPos);
	sqlite3_bind_int64(m_getBlocksOnPosStatement, 1, psPos);

	int result = 0;
	while (true) {
		result = sqlite3_step(m_getBlocksOnPosStatement);
		if(result == SQLITE_ROW) {
			sqlite3_int64 blocknum = sqlite3_column_int64(m_getBlocksOnPosStatement, 0);
			const unsigned char *data = reinterpret_cast<const unsigned char *>(sqlite3_column_blob(m_getBlocksOnPosStatement, 1));
			int size = sqlite3_column_bytes(m_getBlocksOnPosStatement, 1);
			block = DBBlock(blocknum, std::basic_string<unsigned char>(data, size));
			break;
		} else if (result == SQLITE_BUSY) { // Wait some time and try again
			usleep(10000);
		} else {
			break;
		}
	}
	sqlite3_reset(m_getBlocksOnPosStatement);

	return block;
}

