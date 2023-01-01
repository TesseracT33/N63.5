module Cart;

import UserMessage;

namespace Cart
{
	void AllocateSram()
	{
		sram.resize(sram_size);
		std::ranges::fill(sram, 0xFF);
	}


	size_t GetNumberOfBytesUntilRomEnd(u32 addr)
	{
		static constexpr u32 addr_rom_start = 0x1000'0000;
		return original_rom_size - (addr - addr_rom_start) % original_rom_size;
	}


	u8* GetPointerToRom(u32 addr)
	{
		return rom.empty() ? nullptr : rom.data() + (addr & rom_access_mask);
	}


	u8* GetPointerToSram(u32 addr)
	{
		return sram.empty() ? nullptr : sram.data() + (addr & (sram_size - 1));
	}


	bool LoadRom(const std::filesystem::path& rom_path)
	{
		std::optional<std::vector<u8>> optional_rom = ReadFileIntoVector(rom_path);
		if (!optional_rom.has_value()) {
			UserMessage::Error("Failed to open rom file.");
			return false;
		}
		rom = optional_rom.value();
		if (rom.empty()) {
			UserMessage::Error("Rom file has size 0.");
			return false;
		}
		if (rom.size() > rom_region_size) {
			UserMessage::Warning(std::format("Rom file has size larger than the maximum allowed ({} bytes). "
				"Truncating to the maximum allowed.", rom_region_size));
			rom.resize(rom_region_size);
		}
		original_rom_size = rom.size();
		ResizeRomToPowerOfTwo();
		rom_access_mask = u32(rom.size() - 1);
		AllocateSram();
		return true;
	}


	bool LoadSram(const std::filesystem::path& sram_path)
	{
		std::optional<std::vector<u8>> optional_sram = ReadFileIntoVector(sram_path);
		if (!optional_sram.has_value()) {
			return false;
		}
		sram = optional_sram.value();
		if (sram.empty()) {
			UserMessage::Error("Error: sram file has size 0.");
			return false;
		}
		if (sram.size() != sram_size) {
			UserMessage::Warning(std::format("Sram file has size different than the allowed ({} bytes). ", sram_size));
			sram.resize(sram_size);
		}
		return true;
	}


	template<std::signed_integral Int>
	Int ReadRom(u32 addr)
	{
		if constexpr (sizeof(Int) < 4) {
			addr += addr & 2; /* PI external bus glitch */
		}
		Int ret;
		std::memcpy(&ret, GetPointerToRom(addr), sizeof(Int));
		return std::byteswap(ret);
	}


	template<std::signed_integral Int>
	Int ReadSram(u32 addr)
	{ /* CPU precondition: addr is always aligned */
		if constexpr (sizeof(Int) < 4) {
			addr += addr & 2; /* PI external bus glitch */
		}
		Int ret;
		std::memcpy(&ret, GetPointerToSram(addr), sizeof(Int));
		return std::byteswap(ret);
	}


	void ResizeRomToPowerOfTwo()
	{
		size_t actual_size = rom.size();
		size_t pow_two_size = std::bit_ceil(actual_size);
		size_t diff = pow_two_size - actual_size;
		if (diff > 0) {
			rom.resize(pow_two_size);
			std::copy(rom.begin(), rom.begin() + diff, rom.begin() + actual_size);
		}
	}


	template<size_t access_size>
	void WriteSram(u32 addr, s64 data)
	{ /* CPU precondition: addr + number_of_bytes does not go beyond the next alignment boundary */
		if constexpr (access_size < 4) {
			addr += addr & 2; /* PI external bus glitch */     /* TODO: not sure how it works for writes */
		}
		data = std::byteswap(data);
		std::memcpy(GetPointerToSram(addr), &data, access_size);
	}


	template<size_t access_size>
	void WriteRom(u32 addr, s64 data)
	{
		/* TODO */
	}


	template s8 ReadRom<s8>(u32);
	template s16 ReadRom<s16>(u32);
	template s32 ReadRom<s32>(u32);
	template s64 ReadRom<s64>(u32);
	template s8 ReadSram<s8>(u32);
	template s16 ReadSram<s16>(u32);
	template s32 ReadSram<s32>(u32);
	template s64 ReadSram<s64>(u32);
	template void WriteSram<1>(u32, s64);
	template void WriteSram<2>(u32, s64);
	template void WriteSram<4>(u32, s64);
	template void WriteSram<8>(u32, s64);
	template void WriteRom<1>(u32, s64);
	template void WriteRom<2>(u32, s64);
	template void WriteRom<4>(u32, s64);
	template void WriteRom<8>(u32, s64);
}