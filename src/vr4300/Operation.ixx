export module VR4300:Operation;

import Util;

import <bit>;
import <cassert>;
import <cstring>;
import <format>;
import <iostream>;
import <string>;
import <string_view>;
import <utility>;

namespace VR4300
{
	export
	{
		enum class OperatingMode {
			User, Supervisor, Kernel
		} operating_mode;

		enum class ExternalInterruptSource {
			MI        = 1 << 2, /* ip2; MIPS Interface interrupt. Set to 1 when (MI_INTR_REG & MI_INTR_MASK_REG) != 0  */
			Cartridge = 1 << 3, /* ip3; This is connected to the cartridge slot. Cartridges with special hardware can trigger this interrupt. */
			Reset     = 1 << 4, /* ip4; Becomes 1 when the console's reset button is pressed. */
			IndyRead  = 1 << 5, /* ip5; Connected to the Indy dev kit’s RDB port. Set to 1 when a value is read. */
			IndyWrite = 1 << 6  /* ip6; Connected to the Indy dev kit’s RDB port. Set to 1 when a value is written. */
		};

		void AddInitialEvents();
		void CheckInterrupts();
		void ClearInterruptPending(ExternalInterruptSource);
		u64 GetElapsedCycles();
		void Reset();
		u64 Run(u64 cpu_cycles_to_run);
		void PowerOn(bool hle_pif);
		void SetInterruptPending(ExternalInterruptSource);
	}

	enum class Alignment {
		Aligned,
		UnalignedLeft, /* Load/Store (Double)Word Left instructions */
		UnalignedRight /* Load/Store (Double)Word Right instructions */
	};

	void AdvancePipeline(u64 cycles);
	void DecodeExecuteInstruction(u32 instr_code);
	void FetchDecodeExecuteInstruction();
	void InitializeRegisters();
	void HlePif();
	void NotifyIllegalInstrCode(u32 instr_code);
	void PrepareJump(u64 target_address);

	bool pc_is_inside_branch_delay_slot;
	u64 p_cycle_counter;
	u8* rdram_ptr;

	/* Debugging */
	u64 current_instr_pc;
	std::string_view current_instr_name;
	std::string current_instr_log_output;
}