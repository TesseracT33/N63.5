export module VR4300:Operation;

import NumericalTypes;

namespace VR4300
{
	export
	{
		void Reset();
		void Run(const int cycles_to_run);
		void PowerOn();
	}

	enum class OperatingMode { User, Supervisor, Kernel } operating_mode;

	void ExecuteInstruction();
	void DecodeAndExecuteInstruction(const u32 instr_code);

	void EnterKernelMode();
	void DisableInterrupts();

	unsigned p_cycle_counter = 0;
}