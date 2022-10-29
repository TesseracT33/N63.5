export module VR4300:Cache;

import Memory;
import Util;

import <algorithm>;
import <array>;
import <bit>;
import <concepts>;
import <cstring>;
import <format>;

namespace VR4300
{
	struct DCacheLine
	{
		u8 data[16];
		u32 ptag;
		bool valid;
		bool dirty;
	};

	struct ICacheLine
	{
		u8 data[32];
		u32 ptag;
		bool valid;
	};

	void Cache(u32 instr_code);
	void FillCacheLine(auto& cache_line, u32 phys_addr);
	void WritebackCacheLine(auto& cache_line, u32 new_phys_addr);

	template<std::signed_integral Int, Memory::Operation>
	Int ReadCacheableArea(u32 phys_addr);

	template<size_t num_bytes>
	void WriteCacheableArea(u32 phys_addr, std::signed_integral auto data);

	/* TODO: making these anything but 0 makes NM64 not get to the title screen */
	constexpr uint cache_hit_read_cycle_delay = 0;
	constexpr uint cache_hit_write_cycle_delay = 0;
	constexpr uint cache_miss_cycle_delay = 0; /* Magic number gathered from ares / CEN64 */

	std::array<DCacheLine, 512> d_cache; /* 8 KB */
	std::array<ICacheLine, 512> i_cache; /* 16 KB */
}