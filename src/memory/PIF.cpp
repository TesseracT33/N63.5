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
		// TODO: Reading from the PIF_ROM area will simply return 0 after boot is finished, because the PIF locks PIF_ROM accesses for security reasons.
		// Also for SI DMA
		Int ret;
		std::memcpy(&ret, memory.data() + (addr & 0x7FF), sizeof(Int));
		return std::byteswap(ret);
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
			std::memcpy(memory.data() + ram_start, &joypad_status, 4);
			break;

		case 0x02: /* Read Controller Accessory */
			break;

		case 0x03: /* Write Controller Accessory */
			break;

		case 0x04: /* Read EEPROM */
			break;

		case 0x05: /* Write EEPROM */
			break;
		}
	}


	template<size_t num_bytes>
	void WriteMemory(u32 addr, std::signed_integral auto data)
	{ /* CPU precondition: write does not go to the next boundary */
		addr &= 0x7FF;
		if (addr >= ram_start) { /* $0-$7BF: rom; $7C0-$7FF: ram */
			data = std::byteswap(data);
			std::memcpy(memory.data() + addr, &data, num_bytes);
			if (addr + num_bytes > 0x7FF) {
				if (memory[command_byte_index] & 1) {
					RunJoybusProtocol();
					memory[command_byte_index] &= ~1;
				}
				if (memory[command_byte_index] & 2) {
					ChallengeProtection();
					memory[command_byte_index] &= ~2;
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
	}


	template s8 ReadMemory<s8>(u32);
	template s16 ReadMemory<s16>(u32);
	template s32 ReadMemory<s32>(u32);
	template s64 ReadMemory<s64>(u32);
	template void WriteMemory<1>(u32, s8);
	template void WriteMemory<1>(u32, s16);
	template void WriteMemory<1>(u32, s32);
	template void WriteMemory<1>(u32, s64);
	template void WriteMemory<2>(u32, s16);
	template void WriteMemory<2>(u32, s32);
	template void WriteMemory<2>(u32, s64);
	template void WriteMemory<3>(u32, s32);
	template void WriteMemory<3>(u32, s64);
	template void WriteMemory<4>(u32, s32);
	template void WriteMemory<4>(u32, s64);
	template void WriteMemory<5>(u32, s64);
	template void WriteMemory<6>(u32, s64);
	template void WriteMemory<7>(u32, s64);
	template void WriteMemory<8>(u32, s64);
}