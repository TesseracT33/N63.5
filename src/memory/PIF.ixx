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

	struct JoypadStatus /* TODO: correct endianness? */
	{
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

	std::array<u8, rom_size + ram_size> memory{}; /* $0-$7BF: rom; $7C0-$7FF: ram */
}