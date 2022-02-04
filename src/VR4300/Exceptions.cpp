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
		COP0_reg.epc = PC;
		exception_context.save_context();
		EnterKernelMode();
		DisableInterrupts();
	}


	void AddressErrorException()
	{

	}


	void ColdResetException()
	{
		PC = GetExceptionVector<Exception::ColdReset>();
		COP0_reg.status.RP = COP0_reg.status.SR = COP0_reg.status.TS = 0;
		COP0_reg.status.ERL = COP0_reg.status.BEV = 1;
		COP0_reg.config.EP = 0;
		COP0_reg.config.BE = 1;
		COP0_reg.random.random = 31;

		COP0_reg.status.NotifyCpuAfterWrite();
		COP0_reg.config.NotifyCpuAfterWrite();

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

	}


	void ReservedInstructionException()
	{

	}


	void SoftResetException()
	{
		PC = COP0_reg.status.ERL == 0 ? COP0_reg.error_epc : GetExceptionVector<Exception::SoftReset>();
		COP0_reg.status.RP = COP0_reg.status.TS = 0;
		COP0_reg.status.BEV = COP0_reg.status.ERL = COP0_reg.status.SR = 1;
	}


	void TLB_InvalidException()
	{

	}


	template<MemoryAccess::Operation operation>
	void TLB_MissException(const u32 bad_virt_addr, const u32 bad_VPN2)
	{
		COP0_reg.context.bad_vpn2 = bad_VPN2;
		/* TODO: context.PTEBase */

		COP0_reg.bad_v_addr = bad_virt_addr; /* TODO arg is u32, dest is u64 */

		COP0_reg.cause.exc_code = static_cast<u32>([] {
			if constexpr (operation == MemoryAccess::Operation::Read)
				return ExceptionCode::TLBL;
			else return ExceptionCode::TLBS; /* Write */
		}());
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


	template void TLB_MissException<MemoryAccess::Operation::Read>(const u32 bad_virt_addr, const u32 bad_VPN2);
	template void TLB_MissException<MemoryAccess::Operation::Write>(const u32 bad_virt_addr, const u32 bad_VPN2);
}