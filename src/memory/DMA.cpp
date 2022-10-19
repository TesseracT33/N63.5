module DMA;

import Cartridge;
import DebugOptions;
import Logging;
import MI;
import PI;
import PIF;
import RDRAM;
import RSP;
import SI;
import Scheduler;
import VR4300;

namespace DMA
{
	template<Location location>
	size_t GetNumberOfBytesUntilMemoryEnd(u32 addr)
	{
		if constexpr (location == Location::Cartridge)
			return Cartridge::GetNumberOfBytesUntilROMEnd(addr);
		else if constexpr (location == Location::PIF)
			return PIF::GetNumberOfBytesUntilRAMEnd(addr);
		else if constexpr (location == Location::RDRAM)
			return RDRAM::GetNumberOfBytesUntilMemoryEnd(addr);
		else
			static_assert(location != location, "Unknown location given as argument to function \"GetNumberOfBytesUntilRegionEnd\"");
	}


	template<Location location>
	u8* GetPointerFromAddress(u32 addr)
	{
		if constexpr (location == Location::Cartridge)
			return Cartridge::GetPointerToROM(addr);
		else if constexpr (location == Location::PIF)
			return PIF::GetPointerToRAM(addr);
		else if constexpr (location == Location::RDRAM)
			return RDRAM::GetPointerToMemory(addr);
		else
			static_assert(location != location, "Unknown location given as argument to function \"GetPointerFromAddress\"");
	}


	template<Type type, Location src, Location dst>
	void Init(size_t length, s32 src_start_addr, s32 dst_start_addr)
	{
		u8* src_start_ptr = GetPointerFromAddress<src>(src_start_addr);
		u8* dst_start_ptr = GetPointerFromAddress<dst>(dst_start_addr);

		size_t bytes_until_source_end = GetNumberOfBytesUntilMemoryEnd<src>(src_start_addr);
		size_t bytes_until_dest_end = GetNumberOfBytesUntilMemoryEnd<dst>(dst_start_addr);
		size_t num_bytes_to_copy = std::min(length, std::min(bytes_until_source_end, bytes_until_dest_end));

		std::memcpy(dst_start_ptr, src_start_ptr, num_bytes_to_copy);

		static constexpr auto cycles_per_byte_dma = 18;
		auto cycles_until_finish = num_bytes_to_copy * cycles_per_byte_dma;
		Scheduler::EventType event = [] {
			if constexpr (type == DMA::Type::PI) return Scheduler::EventType::PiDmaFinish;
			else if constexpr (type == DMA::Type::SI) return Scheduler::EventType::SiDmaFinish;
			else static_assert("Unknown DMA type given as argument to \"DMA::Init\"");
		}();
		Scheduler::AddEvent(event, cycles_until_finish, [] {
			if constexpr (type == DMA::Type::PI) {
				MI::SetInterruptFlag(MI::InterruptType::PI);
				PI::SetStatusFlag(PI::StatusFlag::DmaCompleted);
				PI::ClearStatusFlag(PI::StatusFlag::DmaBusy);
				VR4300::CheckInterrupts();
			}
			else if constexpr (type == DMA::Type::SI) {
				MI::SetInterruptFlag(MI::InterruptType::SI);
				SI::SetStatusFlag(SI::StatusFlag::Interrupt);
				SI::ClearStatusFlag(SI::StatusFlag::DmaBusy);
				VR4300::CheckInterrupts();
			}
			else static_assert("Unknown DMA type");
		});

		if constexpr (log_dma) {
			std::string output = std::format("From {} ${:X} to {} ${:X}; ${:X} bytes",
				location_to_string_table[std::to_underlying(src)], src_start_addr,
				location_to_string_table[std::to_underlying(dst)], dst_start_addr, num_bytes_to_copy);
			Logging::LogDMA(output);
		}
	}


	template void Init <Type::PI, Location::Cartridge, Location::RDRAM> (size_t, s32, s32);
	template void Init <Type::PI, Location::RDRAM, Location::Cartridge>(size_t, s32, s32);
	template void Init <Type::SI, Location::PIF, Location::RDRAM>(size_t, s32, s32);
	template void Init <Type::SI, Location::RDRAM, Location::PIF>(size_t, s32, s32);
}