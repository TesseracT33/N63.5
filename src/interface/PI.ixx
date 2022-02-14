export module PI; /* Peripheral Interface */

import NumericalTypes;

import <array>;
import <concepts>;

namespace PI
{
	std::array<u8, 0x34> mem{};

	template<std::size_t start, std::size_t number_of_bytes /* how many bytes we want to write */>
	void WriteToDramCartAddr(const auto data);

	template<std::size_t start, std::size_t number_of_bytes>
	void WriteToRdLen(const auto data);

	template<std::size_t start, std::size_t number_of_bytes>
	void WriteToWrLen(const auto data);

	void WriteToStatus(const u8 data);

	export
	{
		enum class StatusFlag : u8
		{
			DMA_BUSY = 1 << 0,
			IO_BUSY = 1 << 1,
			DMA_ERROR = 1 << 2,
			DMA_COMPLETED = 1 << 3
		};

		template<StatusFlag status_flag>
		void SetStatusFlag();

		template<StatusFlag status_flag>
		void ClearStatusFlag();

		template<std::integral Int>
		Int Read(const u32 addr);

		template<std::size_t number_of_bytes>
		void Write(const u32 addr, const auto data);
	}
}