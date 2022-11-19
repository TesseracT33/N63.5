export module PIF;

import Util;

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
		size_t GetNumberOfBytesUntilMemoryEnd(u32 offset);
		size_t GetNumberOfBytesUntilRamStart(u32 offset);
		u8* GetPointerToMemory(u32 address);
		bool LoadIPL12(const std::string& path);

		template<std::signed_integral Int>
		Int ReadMemory(u32 addr);

		template<size_t access_size>
		void WriteMemory(u32 addr, s64 data);
	}

	void ChallengeProtection();
	void ChecksumVerification();
	void ClearRam();
	void RomLockout();
	void RunJoybusProtocol();
	void TerminateBootProcess();

	constexpr size_t command_byte_index = 0x7FF;
	constexpr size_t ram_size = 0x40;
	constexpr size_t rom_size = 0x7C0;
	constexpr size_t ram_start = rom_size;
	constexpr size_t memory_size = ram_size + rom_size;

	struct JoypadStatus { /* TODO: wrong endianess if ram kept in BE */
		u32 a : 1;
		u32 b : 1;
		u32 z : 1;
		u32 s : 1;
		u32 dU : 1;
		u32 dD : 1;
		u32 dL : 1;
		u32 dR : 1;
		u32 rst : 1;
		u32 : 1;
		u32 lt : 1;
		u32 rt : 1;
		u32 cU : 1;
		u32 cD : 1;
		u32 cL : 1;
		u32 cR : 1;
		u32 x_axis : 8;
		u32 y_axis : 8;
	} joypad_status;

	std::array<u8, memory_size> memory; /* $0-$7BF: rom; $7C0-$7FF: ram */
}