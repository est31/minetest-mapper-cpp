
#ifndef BLOCKPOS_H
#define BLOCKPOS_H

#include <cstdlib>
#include <limits>

struct BlockPos {
	int x;
	int y;
	int z;
#if SIZE_MAX > (1LL << 36)
	std::size_t hash(void) const { return databasePos(); }
#else
	std::size_t hash(void) const { return databasePos() % SIZE_MAX; }
#endif
	BlockPos() : x(0), y(0), z(0) {}
	BlockPos(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {}
	BlockPos(int64_t i) { setFromDBPos(i); }
	int64_t databasePos(void) const { return getDBPos(); }

	bool operator<(const BlockPos& p) const;
	bool operator==(const BlockPos& p) const;
	void operator=(const BlockPos &p) { x = p.x; y = p.y; z = p.z; }
	void operator=(int64_t i) { setFromDBPos(i); }

protected:
// Include code copied from the minetest codebase.
// Defines (at least) the following functions:
//	setFromDBPos(int64_t i);
//	int64_t getDBPos(void) const;
#include "minetest-database.h"

};

// operator< should order the positions in the
// order the corresponding pixels are generated:
// First (most significant): z coordinate, descending (i.e. reversed)
// Then                    : x coordinate, ascending
// Last (least significant): y coordinate, descending (i.e. reversed)
inline bool BlockPos::operator<(const BlockPos& p) const
{
	if (z > p.z)
		return true;
	if (z < p.z)
		return false;
	if (x < p.x)
		return true;
	if (x > p.x)
		return false;
	if (y > p.y)
		return true;
	if (y < p.y)
		return false;
	return false;
}

inline bool BlockPos::operator==(const BlockPos& p) const
{
	if (z != p.z)
		return false;
	if (y != p.y)
		return false;
	if (x != p.x)
		return false;
	return true;
}

#endif // BLOCKPOS_H
