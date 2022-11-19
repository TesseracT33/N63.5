module PIF;

import UserMessage;

namespace PIF
{
	void ChallengeProtection()
	{

	}


	void ChecksumVerification()
	{
		memory[command_byte_index] |= 0x80;
	}


	void ClearRam()
	{
		std::fill(memory.begin() + ram_start, memory.end(), 0);
	}


	size_t GetNumberOfBytesUntilMemoryEnd(u32 offset)
	{
		return memory_size - (offset & (memory_size - 1)); /* Size is 0x800 bytes */
	}


	size_t GetNumberOfBytesUntilRamStart(u32 offset)
	{
		offset &= memory_size - 1;
		return offset < ram_start ? ram_start - offset : 0;
	}


	u8* GetPointerToMemory(u32 address)
	{
		return memory.data() + (address & (memory_size - 1));
	}


	bool LoadIPL12(const std::string& path)
	{
		std::optional<std::array<u8, memory_size>> optional_rom = ReadFileIntoArray<memory_size>(path);
		if (!optional_rom.has_value()) {
			UserMessage::Show("Failed to open boot rom (IPL) file.", UserMessage::Type::Warning);
			return false;
		}
		memory = optional_rom.value();
		return true;
	}


	template<std::signed_integral Int>
	Int ReadMemory(u32 addr)
	{ /* CPU precondition: addr is aligned */
		Int ret;
		std::memcpy(&ret, memory.data() + (addr & 0x7FF), sizeof(Int));
		return std::byteswap(ret);
	}


	void RomLockout()
	{

	}


	void RunJoybusProtocol()
	{
		switch (memory[ram_start]) { /* joybus command */
		case 0x00:/* Info */
		case 0xFF: /* Reset & Info */
			/* Device: controller */
			memory[ram_start] = 0x05;
			memory[ram_start + 1] = 0x00;
			/* Pak installed */
			memory[ram_start + 2] = 0x01;
			break;

		case 0x01: /* Controller State */
			std::memcpy(&memory[ram_start], &joypad_status, 4);
			break;

		case 0x02: /* Read Controller Accessory */
			break;

		case 0x03: /* Write Controller Accessory */
			break;

		case 0x04: /* Read EEPROM */
			break;

		case 0x05: /* Write EEPROM */
			break;

		case 0x06: /* Real-Time Clock Info */
			std::memset(&memory[ram_start], 0, 3); /* clock does not exist */
			break;
		}
	}


	void TerminateBootProcess()
	{

	}


	template<size_t access_size>
	void WriteMemory(u32 addr, s64 data)
	{ /* CPU precondition: write does not go to the next boundary */
		addr &= 0x7FF;
		if (addr < ram_start) return;
		auto to_write = [&] { /* TODO: behavior is different from this */
			if constexpr (access_size == 1) return u8(data);
			if constexpr (access_size == 2) return std::byteswap(u16(data));
			if constexpr (access_size == 4) return std::byteswap(u32(data));
			if constexpr (access_size == 8) return std::byteswap(data);
		}();
		std::memcpy(memory.data() + addr, &to_write, access_size);
		
		if (addr + access_size >= command_byte_index) {
			if (memory[command_byte_index] & 1) {
				RunJoybusProtocol();
				memory[command_byte_index] &= ~1;
			}
			if (memory[command_byte_index] & 2) {
				ChallengeProtection();
				memory[command_byte_index] &= ~2;
			}
			if (memory[command_byte_index] & 8) {
				TerminateBootProcess();
				memory[command_byte_index] &= ~8;
			}
			if (memory[command_byte_index] & 0x10) {
				RomLockout();
				memory[command_byte_index] &= ~0x10;
			}
			if (memory[command_byte_index] & 0x20) {
				ChecksumVerification();
				memory[command_byte_index] &= ~0x20;
			}
			if (memory[command_byte_index] & 0x40) {
				ClearRam();
				memory[command_byte_index] &= ~0x40;
			}
		}
	}


	template s8 ReadMemory<s8>(u32);
	template s16 ReadMemory<s16>(u32);
	template s32 ReadMemory<s32>(u32);
	template s64 ReadMemory<s64>(u32);
	template void WriteMemory<1>(u32, s64);
	template void WriteMemory<2>(u32, s64);
	template void WriteMemory<4>(u32, s64);
	template void WriteMemory<8>(u32, s64);
}