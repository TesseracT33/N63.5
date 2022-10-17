export module RDP;

import NumericalTypes;

import <array>;
import <concepts>;
import <cstring>;

namespace RDP
{
	export
	{
		template<std::integral Int> Int ReadReg(u32 addr);
		template<std::integral Int> void WriteReg(u32 addr, Int data);
	}

	enum class CommandLocation {
		DMEM, RDRAM
	};

	struct StatusReg
	{
		u32 dmem_dma_status : 1;
		u32 freeze_status : 1;
		u32 flush_status : 1;
		u32 start_gclk : 1;
		u32 tmem_busy : 1;
		u32 pipe_busy : 1;
		u32 command_busy : 1;
		u32 command_buffer_busy : 1;
		u32 dma_busy : 1;
		u32 end_valid : 1;
		u32 start_valid : 1;
		u32 : 21;
	};

	struct
	{
		u32 start_reg, end_reg, current_reg;
		StatusReg status_reg;
		u32 clock_reg, bufbusy_reg, pipebusy_reg, tmem_reg;
	} dp;

	template<CommandLocation command_location> u64 LoadCommandDword(u32 addr);
	template<CommandLocation> void LoadDecodeExecuteCommand();

	/* Commands */
	void FillRectangle(u64 command){};
	void LoadTile(u64 command){};
	void SetColorImage(u64 command){};
	void SetFillColor(u64 command){};
	void SetOtherModes(u64 command){};
	void SetScissor(u64 command){};
	void SetTextureImage(u64 command){};
	void SetTile(u64 command){};
	void SetZImage(u64 command){};
	void SyncFull(u64 command){};
	void SyncLoad(u64 command){};
	void SyncPipe(u64 command){};
	void SyncTile(u64 command){};
	void TextureRectangle(u64 command_low, u64 command_high){};

	std::array<u8, 0x1000> tmem;
}