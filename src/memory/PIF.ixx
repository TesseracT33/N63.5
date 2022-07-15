export module PIF;

import NumericalTypes;

import <algorithm>;
import <array>;
import <bit>;
import <concepts>;
import <cstring>;
import <optional>;
import <string>;

namespace PIF
{
	export
	{
		size_t GetNumberOfBytesUntilRAMEnd(u32 offset);
		u8* GetPointerToRAM(u32 address);
		u8* GetPointerToMemory(u32 address);
		bool LoadIPL12(const std::string& path);

		template<std::integral Int>
		Int ReadMemory(u32 addr);

		template<size_t number_of_bytes>
		void WriteMemory(u32 addr, auto data);
	}

	void ChallengeProtection();
	void ChecksumVerification();
	void ClearRam();
	void RunJoybusProtocol();

	constexpr size_t command_byte_index = 0x7FF;
	constexpr size_t ram_size = 0x40;
	constexpr size_t rom_size = 0x7C0;
	constexpr size_t ram_start = rom_size;

	s32 joypad_status;

	std::array<u8, rom_size + ram_size> memory{}; /* $0-$7BF: rom; $7C0-$7FF: ram */
}