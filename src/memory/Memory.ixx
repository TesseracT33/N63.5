export module Memory;

import <array>;
import <bit>;
import <cassert>;
import <concepts>;
import <functional>;

import Cartridge;
import HostSystem;
import NumericalTypes;
import UserMessage;

namespace Memory
{
	template<std::integral T>
	T TruncateAndConvertEndian(const u64 value);

	u64 read_pif(const u32 addr);
	u64 read_rdram(const u32 addr);
	u64 invalid_read(const u32 addr);

	typedef u64(*read_fun_t)(const u32);

	constexpr std::array<read_fun_t, 0x1000> read_table = [] {
		std::array<read_fun_t, 0x1000> table{};
		unsigned addr = 0;
		for (auto& fun : table) {
			fun = [&] {
				if (addr <= 0x003) return read_rdram;
				if (addr <= 0x007) return invalid_read;
				if (addr <= 0x03E) return invalid_read;
				if (addr == 0x03F) return invalid_read;
				if (addr == 0x040) return invalid_read;
				if (addr == 0x041) return invalid_read;
				if (addr == 0x042) return invalid_read;
				if (addr == 0x043) return invalid_read;
				if (addr == 0x044) return invalid_read;
				if (addr == 0x045) return invalid_read;
				if (addr == 0x046) return invalid_read;
				if (addr == 0x047) return invalid_read;
				if (addr == 0x048) return invalid_read;
				if (addr == 0x049) return invalid_read;
				if (addr <= 0x04F) return invalid_read;
				if (addr <= 0x05F) return invalid_read;
				if (addr <= 0x07F) return invalid_read;
				if (addr <= 0x0FF) return invalid_read;
				if (addr <= 0x1FB) return Cartridge::read;
				if (addr == 0x1FC) return read_pif;
				if (addr <= 0x7FF) return invalid_read;
				return invalid_read;
			}();
			addr++;
		}
		return table;
	}();

	export
	template<std::integral T>
	T ReadPhysical(const u32 physical_address)
	{
		const auto page = physical_address >> 20;
		const u64 value = std::invoke(read_table[page], physical_address);
		/* Every read function always reads 64 bits. We convert it to the width of T, and reverse bytes if the host and n64 endianness are different. */
		const T ret_value = TruncateAndConvertEndian<T>(value); 
		return ret_value;
	}

	u64 invalid_read(const u32 addr)
	{
		assert(false);
		return 0;
	}

	u64 read_pif(const u32 addr)
	{
		assert(false);
		return 0;
	}

	u64 read_rdram(const u32 addr)
	{
		assert(false);
		return 0;
	}
}