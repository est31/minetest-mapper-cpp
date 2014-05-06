#include "db-leveldb.h"
#include <stdexcept>
#include <sstream>
#include "types.h"

inline int64_t stoi64(const std::string &s) {
	std::stringstream tmp(s);
	long long t;
	tmp >> t;
	return t;
}

inline std::string i64tos(int64_t i) {
	std::ostringstream o;
	o<<i;
	return o.str();
}

DBLevelDB::DBLevelDB(const std::string &mapdir) :
	m_blocksReadCount(0),
	m_blocksCachedCount(0),
	m_blocksUnCachedCount(0)
{
	leveldb::Options options;
	options.create_if_missing = false;
	leveldb::Status status = leveldb::DB::Open(options, mapdir + "map.db", &m_db);
	if(!status.ok())
		throw std::runtime_error("Failed to open Database");
}

DBLevelDB::~DBLevelDB() {
	delete m_db;
}

int DBLevelDB::getBlocksReadCount(void)
{
	return m_blocksReadCount;
}

int DBLevelDB::getBlocksCachedCount(void)
{
	return m_blocksCachedCount;
}

int DBLevelDB::getBlocksUnCachedCount(void)
{
	return m_blocksUnCachedCount;
}

std::vector<int64_t> DBLevelDB::getBlockPos() {
	std::vector<int64_t> vec;
	leveldb::Iterator* it = m_db->NewIterator(leveldb::ReadOptions());
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		vec.push_back(stoi64(it->key().ToString()));
	}
	delete it;
	return vec;
}

DBBlock DBLevelDB::getBlockOnPos(int x, int y, int z)
{
	int64_t iPos;
	DBBlock block(0,(const unsigned char *)"");
	std::string datastr;
	leveldb::Status status;

	iPos =  static_cast<int64_t>(x);
	iPos += static_cast<int64_t>(y) << 12;
	iPos += static_cast<int64_t>(z) << 24;

	status = m_db->Get(leveldb::ReadOptions(), i64tos(iPos), &datastr);
	if(status.ok()) {
		block = DBBlock( iPos, ustring( (const unsigned char*) datastr.c_str(), datastr.size() ) );
		m_blocksReadCount++;
		m_blocksUnCachedCount++;
	}

	return block;
}

