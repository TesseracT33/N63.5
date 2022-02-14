export module VR4300:Exceptions;

import <array>;
import <functional>;

import MemoryAccess;
import NumericalTypes;

namespace VR4300
{
	export
	{
		enum class Exception
		{
			AddressError,
			Breakpoint,
			BusError,
			ColdReset,
			CoprocessorUnusable,
			FloatingPoint,
			IntegerOverflow,
			Interrupt,
			NMI,
			ReservedInstruction,
			SoftReset,
			Syscall,
			TLB_Invalid,
			TLB_Miss,
			TLB_Modification,
			Trap,
			Watch,
			XTLB_Miss
		};

		template<Exception exception, MemoryAccess::Operation operation = MemoryAccess::Operation::Read>
		void SignalException();

		template<MemoryAccess::Operation operation>
		void SignalAddressErrorException(const u64 bad_virt_addr);

		void HandleException();

		bool exception_has_occurred = false;
	}

	typedef void(*ExceptionHandlerFun)();
	ExceptionHandlerFun exception_fun_to_call;

	/* Exception handlers */
	template<MemoryAccess::Operation operation> void AddressErrorException();
	template<MemoryAccess::Operation operation> void BusErrorException();
	template<MemoryAccess::Operation operation> void TLB_InvalidException();
	template<MemoryAccess::Operation operation> void TLB_MissException();
	template<MemoryAccess::Operation operation> void XTLB_MissException();
	void BreakPointException();
	void BusErrorException();
	void ColdResetException();
	void CoprocessorUnusableException();
	void FloatingpointException();
	void IntegerOverflowException();
	void InterruptException();
	void NMI_Exception();
	void ReservedInstructionException();
	void SoftResetException();
	void SyscallException();
	void TLB_ModException();
	void TrapException();
	void WatchException();

	template<Exception exception, MemoryAccess::Operation operation>
	constexpr ExceptionHandlerFun GetExceptionHandlerFun();

	template<Exception exception, MemoryAccess::Operation operation>
	constexpr int GetExceptionPriority();

	template<Exception exception>
	u64 GetExceptionVector();

	Exception occurred_exception;
	int occurred_exception_priority = -1;
	u64 exception_vector;
	unsigned coprocessor_unusable_source; /* 0 if COP0 signaled the exception, 1 if COP1 did it. */
	u64 bad_virt_addr; /* Assigned to when an Address Error exception is signalled. */
	ExceptionHandlerFun exception_handler_fun;
}