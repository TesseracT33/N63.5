module PIF;

import Memory;
import UserMessage;

import Util.Files;

#include "../Utils/EnumerateTemplateSpecializations.h"

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


	size_t GetNumberOfBytesUntilRAMEnd(const u32 offset)
	{
		return ram_size - (offset & (ram_size - 1)); /* Size is 0x40 bytes */
	}


	u8* GetPointerToRAM(const u32 address)
	{
		return memory.data() + rom_size + (address & (ram_size - 1)); /* Size is 0x40 bytes */
	}


	u8* GetPointerToMemory(const u32 address)
	{
		return memory.data() + (address & 0x7FF);
	}


	bool LoadIPL12(const std::string& path)
	{
		std::optional<std::array<u8, ram_size + rom_size>> optional_rom =
			FileUtils::LoadBinaryFileArray<ram_size + rom_size>(path);
		if (!optional_rom.has_value()) {
			UserMessage::Show("Failed to open boot rom (IPL) file.", UserMessage::Type::Warning);
			return false;
		}
		const auto& rom = optional_rom.value();
		std::memcpy(memory.data(), rom.data(), ram_size + rom_size);
		return true;
	}


	template<std::integral Int>
	Int ReadMemory(const u32 addr)
	{ /* CPU precondition: addr is aligned */
		Int ret;
		std::memcpy(&ret, memory.data() + (addr & 0x7FF), sizeof(Int));
		return std::byteswap(ret);
	}


	void RunJoybusProtocol()
	{
		switch (memory[ram_start]) {
		case 0x00:/* Info */
		case 0xFF:
			/* Device: controller */
			memory[ram_start] = 0x05;
			memory[ram_start + 1] = 0x00;
			/* Pak installed */
			memory[ram_start + 2] = 0x01;
			break;

		case 0x01: /* Controller State */
			std::memcpy(memory.data() + ram_start, &joypad_status, 4);
			break;

		case 0x02:
			break;

		case 0x03:
			break;

		case 0x04:
			break;

		case 0x05:
			break;
		}
	}


	template<size_t number_of_bytes>
	void WriteMemory(u32 addr, auto data)
	{ /* CPU precondition: write does not go to the next boundary */
		addr &= 0x7FF;
		if (addr >= ram_start) { /* $0-$7BF: rom; $7C0-$7FF: ram */
			data = std::byteswap(data);
			std::memcpy(memory.data() + addr, &data, number_of_bytes);
			if (addr + number_of_bytes > 0x7FF) {
				if (memory[command_byte_index] & 0x01) {
					RunJoybusProtocol();
				}
				if (memory[command_byte_index] & 0x02) {
					ChallengeProtection();
				}
				if (memory[command_byte_index] & 0x20) {
					ChecksumVerification();
				}
				if (memory[command_byte_index] & 0x40) {
					ClearRam();
				}
			}
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadMemory, u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteMemory, u32);
}