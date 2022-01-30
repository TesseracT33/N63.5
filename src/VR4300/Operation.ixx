export module VR4300:Operation;

import NumericalTypes;

import <bit>;

namespace VR4300
{
	export
	{
		enum class OperatingMode { User, Supervisor, Kernel } operating_mode;

		std::endian endianness;

		void Run(const unsigned cycles_to_run);
		void Reset();
		void PowerOn(const bool hle_pif);
	}

	void ExecuteInstruction();
	void DecodeAndExecuteInstruction(const u32 instr_code);

	void SetNewEndianness();
	void EnterKernelMode();
	void DisableInterrupts();

	unsigned p_cycle_counter = 0;

	void HLE_PIF();
}