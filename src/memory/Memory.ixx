export module Memory;

import Util;

import <bit>;
import <concepts>;
import <format>;

export namespace Memory
{
	template<std::signed_integral Int>
	Int Read(u32 addr);

	template<size_t access_size, typename... MaskT>
	void Write(u32 addr, s64 data, MaskT... mask);
}