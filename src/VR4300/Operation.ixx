export module VR4300:Operation;

import NumericalTypes;

import <bit>;
import <cassert>;
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

		enum class ExternalInterruptSource
		{
			MI        = 0x04, /* ip2; MIPS Interface interrupt. Set to 1 when (MI_INTR_REG & MI_INTR_MASK_REG) != 0  */
			Cartridge = 0x08, /* ip3; This is connected to the cartridge slot. Cartridges with special hardware can trigger this interrupt. */
			Reset     = 0x10, /* ip4; Becomes 1 when the console's reset button is pressed. */
			IndyRead  = 0x20, /* ip5; Connected to the Indy dev kit’s RDB port. Set to 1 when a value is read. */
			IndyWrite = 0x40  /* ip6; Connected to the Indy dev kit’s RDB port. Set to 1 when a value is written. */
		};

		void CheckInterrupts();
		void ClearInterruptPending(ExternalInterruptSource);
		void Reset();
		void Run(uint cycles_to_run);
		void PowerOn(bool hle_pif);
		void SetInterruptPending(ExternalInterruptSource);

		uint p_cycle_counter = 0;
	}

	void DecodeExecuteInstruction(u32 instr_code);
	void FetchDecodeExecuteInstruction();
	void HlePif();
	void PrepareJump(u64 target_address);

	template<u64 number_of_cycles>
	void AdvancePipeline();

	template<u64 number_of_cycles>
	void IncrementCountRegister();

	bool pc_is_inside_branch_delay_slot = false;

	/* Debugging */
	u64 current_instr_pc;
	std::string_view current_instr_name;
	std::string current_instr_log_output;
}