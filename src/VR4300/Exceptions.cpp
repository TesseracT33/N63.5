module VR4300:Exceptions;

import :COP0;
import :MMU;
import :Operation;
import :Registers;

namespace VR4300
{
	template<Exception exception, MemoryAccess::Operation operation>
	void SignalException()
	{
		constexpr int new_exception_priority = GetExceptionPriority<exception, operation>();
		if (exception_has_occurred)
		{
			/* Compare exception priorities; return if the new exception has a lower priority than an already occured one. */
			if (new_exception_priority < occurred_exception_priority)
				return;
		}
		exception_has_occurred = true;
		occurred_exception = exception;
		occurred_exception_priority = new_exception_priority;
		/* The below three assignments incur a run-time cost of three stores and one branch if a new exception occurs with a lower priority.
		   However, if we fetched this data once we knew which exception to handle, then the functions 'GetExceptionVector',
		   'GetExceptionCauseCode' and 'GetExceptionHandlerFun' could not take 'exception' as a template argument, and would
		   instead have to take it as a function argument. Then, several run-time branches would have to be taken over this argument. */
		exception_vector = GetExceptionVector<exception>();
		exception_handler_fun = GetExceptionHandlerFun<exception>();
		exception_cause_code = GetExceptionCauseCode<exception, operation>();
	}


	void HandleException()
	{
		cop0_reg.cause.bd = pc_is_inside_branch_delay_slot;
		cop0_reg.cause.exc_code = exception_cause_code;
		cop0_reg.cause.ce = coprocessor_unusable_source; /* Undefined if a Coprocessor Unusable exception did not occur. Thus, fine to always do this assignment. */
		if (cop0_reg.status.EXL == 0)
		{
			/* If the instruction was executing in a branch delay slot, the CPU loads the EPC register
			to the address of the branch instruction immediately preceding the branch delay slot. */
			cop0_reg.epc.value = pc_is_inside_branch_delay_slot ? pc - 4 : pc;
			cop0_reg.status.EXL = 1;
			SetActiveVirtualToPhysicalFunctions();
		}
		pc = exception_vector;

		std::invoke(exception_handler_fun);
		exception_has_occurred = false;
	}


	template<Exception exception, MemoryAccess::Operation operation>
	constexpr s32 GetExceptionCauseCode()
	{
		using enum Exception;
		using enum MemoryAccess::Operation;

		if constexpr (exception == AddressError) {
			if constexpr (operation == Write) return 5;
			else return 4;
		}
		else if constexpr (exception == Breakpoint) {
			return 9;
		}
		else if constexpr (exception == BusError) {
			if constexpr (operation == InstrFetch) return 6;
			else return 7;
		}
		else if constexpr (exception == ColdReset) {
			return -1; /* TODO: what should this return? */
		}
		else if constexpr (exception == CoprocessorUnusable) {
			return 11;
		}
		else if constexpr (exception == FloatingPoint) {
			return 15;
		}
		else if constexpr (exception == IntegerOverflow) {
			return 12;
		}
		else if constexpr (exception == Interrupt) {
			return 0;
		}
		else if constexpr (exception == NMI) {
			return -1;
		}
		else if constexpr (exception == ReservedInstruction) {
			return 10;
		}
		else if constexpr (exception == SoftReset) {
			return -1;
		}
		else if constexpr (exception == Syscall) {
			return 8;
		}
		else if constexpr (exception == TLB_Invalid) {
			return -1;
		}
		else if constexpr (exception == TLB_Miss || exception == XTLB_Miss) {
			if constexpr (operation == Write) return 3;
			else return 2;
		}
		else if constexpr (exception == TLB_Modification) {
			return 1;
		}
		else if constexpr (exception == Trap) {
			return 13;
		}
		else if constexpr (exception == Watch) {
			return 23;
		}
		else {
			static_assert(false);
		}
	}


	template<Exception exception, MemoryAccess::Operation operation>
	constexpr int GetExceptionPriority()
	{
		using enum Exception;
		using enum MemoryAccess::Operation;

		if constexpr (exception == AddressError) {
			if constexpr (operation == InstrFetch) return 17;
			else return 6;
		}
		else if constexpr (exception == Breakpoint) {
			return 12;
		}
		else if constexpr (exception == BusError) {
			if constexpr (operation == InstrFetch) return 14;
			else return 1;
		}
		else if constexpr (exception == ColdReset) {
			return 20;
		}
		else if constexpr (exception == CoprocessorUnusable) {
			return 11;
		}
		else if constexpr (exception == FloatingPoint) {
			return 7;
		}
		else if constexpr (exception == IntegerOverflow) {
			return 8;
		}
		else if constexpr (exception == Interrupt) {
			return 0;
		}
		else if constexpr (exception == NMI) {
			return 18;
		}
		else if constexpr (exception == ReservedInstruction) {
			return 10;
		}
		else if constexpr (exception == SoftReset) {
			return 19;
		}
		else if constexpr (exception == Syscall) {
			return 13;
		}
		else if constexpr (exception == TLB_Invalid) {
			if constexpr (operation == InstrFetch) return 15;
			else return 4;
		}
		else if constexpr (exception == TLB_Miss || exception == XTLB_Miss) {
			if constexpr (operation == InstrFetch) return 16;
			else return 5;
		}
		else if constexpr (exception == TLB_Modification) {
			return 3;
		}
		else if constexpr (exception == Trap) {
			return 9;
		}
		else if constexpr (exception == Watch) {
			return 2;
		}
		else {
			static_assert(false);
		}
	}


	template<Exception exception>
	constexpr ExceptionHandlerFun GetExceptionHandlerFun()
	{
		using enum Exception;
		using enum MemoryAccess::Operation;

		     if constexpr (exception == AddressError)        return AddressErrorException;
		else if constexpr (exception == Breakpoint)          return BreakPointException;
		else if constexpr (exception == BusError)            return BusErrorException;
		else if constexpr (exception == ColdReset)           return ColdResetException;
		else if constexpr (exception == CoprocessorUnusable) return CoprocessorUnusableException;
		else if constexpr (exception == FloatingPoint)       return FloatingpointException;
		else if constexpr (exception == IntegerOverflow)     return IntegerOverflowException;
		else if constexpr (exception == Interrupt)           return InterruptException;
		else if constexpr (exception == NMI)                 return NMI_Exception;
		else if constexpr (exception == ReservedInstruction) return ReservedInstructionException;
		else if constexpr (exception == SoftReset)           return SoftResetException;
		else if constexpr (exception == Syscall)             return SyscallException;
		else if constexpr (exception == TLB_Invalid)         return TLB_InvalidException;
		else if constexpr (exception == TLB_Miss)            return TLB_MissException;
		else if constexpr (exception == XTLB_Miss)           return XTLB_MissException;
		else if constexpr (exception == TLB_Modification)    return TLB_ModException;
		else if constexpr (exception == Trap)                return TrapException;
		else if constexpr (exception == Watch)               return WatchException;
		else static_assert(false);
	}


	template<Exception exception>
	u64 GetExceptionVector()
	{ /* See p. 181, Table 6-3 */
		using enum Exception;

		static constexpr std::array<u64, 2> vector_base_addr = { /* Indexed by status.BEV */
			0xFFFF'FFFF'8000'0000, 0xFFFF'FFFF'BFC0'0200 };

		if constexpr (exception == ColdReset || exception == SoftReset || exception == NMI)
			return vector_base_addr[1];
		else if constexpr (exception == TLB_Miss)
			return vector_base_addr[cop0_reg.status.BEV] | (cop0_reg.status.EXL ? 0x0180 : 0x0000);
		else if constexpr (exception == XTLB_Miss)
			return vector_base_addr[cop0_reg.status.BEV] | (cop0_reg.status.EXL ? 0x0180 : 0x0080);
		else
			return vector_base_addr[cop0_reg.status.BEV] | 0x0180;
	}


	struct ExceptionContext
	{
		OperatingMode operating_mode;
		bool interrupts_enabled;
		u64 pc;

		void SaveContext()
		{
			this->operating_mode = VR4300::operating_mode;
			this->pc = VR4300::pc;
		}
	} exception_context;


	static void EnterException()
	{
		cop0_reg.epc.value = pc;
		exception_context.SaveContext();
		EnterKernelMode();
		DisableInterrupts();
	}


	void AddressErrorException()
	{
		cop0_reg.bad_v_addr.value = bad_virt_addr;
		cop0_reg.epc.value = pc - 4;
		/* TODO The EPC register contains the address of the instruction that caused the exception,
unless this instruction is in a branch delay slot. If it is in a branch delay slot, the
EPC register contains the address of the preceding branch instruction and the BD
bit of the Cause register is set.*/
	}


	void BreakPointException()
	{

	}


	void BusErrorException()
	{

	}


	void ColdResetException()
	{
		pc = exception_vector;
		cop0_reg.status.RP = cop0_reg.status.SR = cop0_reg.status.TS = 0;
		cop0_reg.status.ERL = cop0_reg.status.BEV = 1;
		cop0_reg.config.EP = 0;
		cop0_reg.config.BE = 1;
		cop0_reg.random.random = 31;

		cop0_reg.status.NotifyCpuAfterWrite();
		cop0_reg.config.NotifyCpuAfterWrite();

		/* TODO The EC(2:0) bits of the Config register are set to the contents of the DivMode(1:0)* pins */
	}


	void CoprocessorUnusableException()
	{

	}


	void DivisionByZeroException()
	{

	}


	void FloatingpointException()
	{

	}


	void IntegerOverflowException()
	{

	}


	void InexactOperationException()
	{

	}


	void InterruptException()
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
		pc = cop0_reg.status.ERL == 0 ? cop0_reg.error_epc.value : exception_vector;
		cop0_reg.status.RP = cop0_reg.status.TS = 0;
		cop0_reg.status.BEV = cop0_reg.status.ERL = cop0_reg.status.SR = 1;
	}


	void SyscallException()
	{

	}


	void TLB_InvalidException()
	{

	}


	void TLB_MissException()
	{
		u32 bad_VPN2 = 0; /* TEMP */
		u32 bad_virt_addr = 0; /* TEMP */

		cop0_reg.context.bad_vpn2 = bad_VPN2;
		/* TODO: context.PTEBase */

		cop0_reg.bad_v_addr.value = bad_virt_addr; /* TODO arg is u32, dest is u64 */

		cop0_reg.cause.exc_code = exception_cause_code;
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


	void WatchException()
	{

	}


	void XTLB_MissException()
	{

	}


//#define ENUMERATE_EXCEPTION_SIGNAL_SPECIALIZATION(MEMORY_OPERATION)
//	template void SignalException<Exception::AddressError, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::Breakpoint, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::BusError, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::ColdReset, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::CoprocessorUnusable, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::FloatingPoint, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::IntegerOverflow, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::Interrupt, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::NMI, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::ReservedInstruction, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::SoftReset, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::Syscall, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::TLB_Invalid, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::TLB_Miss, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::TLB_Modification, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::Trap, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::Watch, MEMORY_OPERATION>(); \
//	template void SignalException<Exception::XTLB_Miss, MEMORY_OPERATION>();
//
//	ENUMERATE_EXCEPTION_SIGNAL_SPECIALIZATION(MemoryAccess::Operation::Read)
//	ENUMERATE_EXCEPTION_SIGNAL_SPECIALIZATION(MemoryAccess::Operation::Write)
//	ENUMERATE_EXCEPTION_SIGNAL_SPECIALIZATION(MemoryAccess::Operation::InstrFetch)

	template void SignalException<Exception::AddressError, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::Breakpoint, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::BusError, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::ColdReset, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::CoprocessorUnusable, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::FloatingPoint, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::IntegerOverflow, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::Interrupt, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::NMI, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::ReservedInstruction, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::SoftReset, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::Syscall, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::TLB_Invalid, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::TLB_Miss, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::TLB_Modification, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::Trap, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::Watch, MemoryAccess::Operation::Read>();
	template void SignalException<Exception::XTLB_Miss, MemoryAccess::Operation::Read>();

	template void SignalException<Exception::AddressError, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::Breakpoint, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::BusError, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::ColdReset, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::CoprocessorUnusable, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::FloatingPoint, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::IntegerOverflow, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::Interrupt, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::NMI, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::ReservedInstruction, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::SoftReset, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::Syscall, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::TLB_Invalid, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::TLB_Miss, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::TLB_Modification, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::Trap, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::Watch, MemoryAccess::Operation::Write>();
	template void SignalException<Exception::XTLB_Miss, MemoryAccess::Operation::Write>();

	template void SignalException<Exception::AddressError, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::Breakpoint, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::BusError, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::ColdReset, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::CoprocessorUnusable, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::FloatingPoint, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::IntegerOverflow, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::Interrupt, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::NMI, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::ReservedInstruction, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::SoftReset, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::Syscall, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::TLB_Invalid, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::TLB_Miss, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::TLB_Modification, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::Trap, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::Watch, MemoryAccess::Operation::InstrFetch>();
	template void SignalException<Exception::XTLB_Miss, MemoryAccess::Operation::InstrFetch>();
}