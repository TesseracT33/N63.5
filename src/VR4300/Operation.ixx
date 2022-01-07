export module VR4300:Operation;

import <cassert>;

import NumericalTypes;

namespace VR4300
{
	export
	{
		void Reset();
		void Run(const int cycles);

		enum class OperatingMode { User, Supervisor, Kernel } operatingMode;
	}

	void ExecuteInstruction();
	void DecodeAndExecuteInstruction(const u32 instr_code);
}