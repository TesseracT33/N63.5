export module VR4300:Exceptions;

import <array>;
import <functional>;

import MemoryAccess;
import NumericalTypes;

namespace VR4300
{
	/* Exception handlers */
	void AddressErrorException();
	void BreakPointException();
	void BusErrorException();
	void ColdResetException();
	void CoprocessorUnusableException();
	void DivisionByZeroException();
	void FloatingpointException();
	void IntegerOverflowException();
	void InexactOperationException();
	void InterruptException();
	void InvalidOperationException();
	void OverflowException();
	void NMI_Exception();
	void ReservedInstructionException();
	void SoftResetException();
	void SyscallException();
	void TLB_InvalidException();
	void TLB_MissException();
	void TLB_ModException();
	void TrapException();
	void UnderflowException();
	void UnimplementedOperationException();
	void WatchException();
	void XTLB_MissException();

	typedef void(*ExceptionHandlerFun)();
	ExceptionHandlerFun exception_fun_to_call;

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

		void HandleException();

		bool exception_has_occurred = false;
	}

	Exception occurred_exception;
	int occurred_exception_priority = -1;
	u64 exception_vector;
	u32 exception_cause_code;
	ExceptionHandlerFun exception_handler_fun;

	template<Exception exception, MemoryAccess::Operation operation>
	constexpr s32 GetExceptionCauseCode();

	template<Exception exception>
	constexpr ExceptionHandlerFun GetExceptionHandlerFun();

	template<Exception exception, MemoryAccess::Operation operation>
	constexpr int GetExceptionPriority();

	template<Exception exception>
	u64 GetExceptionVector();
}