module VR4300:Exceptions;

import :COP0;
import :CPU;
import :MMU;
import :Operation;

namespace VR4300
{
	struct
	{
		OperatingMode operating_mode;
		bool interrupts_enabled;
		u64 PC;

		void save_context()
		{
			this->operating_mode = VR4300::operating_mode;
			this->PC = VR4300::PC;
		}
	} exception_context;

	static void EnterException()
	{
		CP0_reg.epc = PC;
		exception_context.save_context();
		EnterKernelMode();
		DisableInterrupts();
	}

	void AddressErrorException()
	{

	}


	void ColdResetException()
	{
		PC = 0xFFFF'FFFF'BFC0'0000;
		CP0_reg.status.RP = CP0_reg.status.SR = CP0_reg.status.TS = 0;
		CP0_reg.status.ERL = CP0_reg.status.BEV = 1;
		CP0_reg.config.EP = 0;
		CP0_reg.config.BE = 1;
		CP0_reg.random = 31;
		/* TODO The EC(2:0) bits of the Config register are set to the contents of the DivMode(1:0)* pins */
	}


	void DivisionByZeroException()
	{

	}


	void IntegerOverflowException()
	{

	}


	void InexactOperationException()
	{

	}


	void InvalidOperationException()
	{

	}


	void OverflowException()
	{

	}


	void NMI_Exception()
	{
		PC = 0xFFFF'FFFF'BFC0'0000;

	}


	void ReservedInstructionException()
	{

	}


	void SoftResetException()
	{
		if (CP0_reg.status.ERL == 0)
			PC = CP0_reg.error_epc;
		else
			PC = 0xFFFF'FFFF'BFC0'0000;
		CP0_reg.status.RP = CP0_reg.status.TS = 0;
		CP0_reg.status.BEV = CP0_reg.status.ERL = CP0_reg.status.SR = 1;
	}


	void TLB_InvalidException()
	{

	}


	template<MemoryAccessOperation operation>
	void TLB_MissException(const u32 bad_virt_addr)
	{
		CP0_reg.cause.exc_code = static_cast<u32>([&] {
			if constexpr (operation == MemoryAccessOperation::Read)
				return CauseExcCode::TLBL;
			else /* Write */
				return CauseExcCode::TLBS;
		}());
		CP0_reg.bad_v_addr = bad_virt_addr; /* TODO arg is u32, dest is u64 */
	}


	void TLB_ModException()
	{

	}


	void TrapException()
	{

	}


	void UnderflowException()
	{

	}


	void UnimplementedOperationException()
	{

	}


	void XTLB_MissException()
	{

	}


	template void TLB_MissException<MemoryAccessOperation::Read>(const u32 bad_virt_addr);
	template void TLB_MissException<MemoryAccessOperation::Write>(const u32 bad_virt_addr);
}