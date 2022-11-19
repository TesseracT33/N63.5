export module VR4300:Exceptions;

import :MMU;

import Memory;
import Util;

import <array>;
import <cassert>;
import <format>;
import <string_view>;
import <utility>;

namespace VR4300
{
	export
	{
		enum class Exception {
			AddressError,
			Breakpoint,
			BusError,
			ColdReset,
			CoprocessorUnusable,
			FloatingPoint,
			IntegerOverflow,
			Interrupt,
			Nmi,
			ReservedInstruction,
			ReservedInstructionCop2,
			SoftReset,
			Syscall,
			TlbInvalid,
			TlbMiss,
			TlbModification,
			Trap,
			Watch,
			XtlbMiss
		};

		template<Exception exception, MemOp mem_op = MemOp::Read>
		void SignalException();

		template<MemOp mem_op>
		void SignalAddressErrorException(u64 bad_virt_addr);

		void HandleException();
	}

	using ExceptionHandler = void(*)();

	template<Exception, MemOp>
	constexpr ExceptionHandler GetExceptionHandlerFun();

	template<Exception, MemOp>
	constexpr int GetExceptionPriority();

	template<Exception>
	u64 GetExceptionVector();

	constexpr std::string_view ExceptionToString(Exception exception);

	void SignalCoprocessorUnusableException(int co);

	/* Exception handlers */
	template<MemOp> void AddressErrorException();
	template<MemOp> void BusErrorException();
	template<MemOp> void TlbInvalidException();
	template<MemOp> void TlbMissException();
	template<MemOp> void XtlbMissException();
	void BreakPointException();
	void BusErrorException();
	void ColdResetException();
	void CoprocessorUnusableException();
	void FloatingpointException();
	void IntegerOverflowException();
	void InterruptException();
	void NmiException();
	void ReservedInstructionException();
	void ReservedInstructionExceptionCop2();
	void SoftResetException();
	void SyscallException();
	void TlbModException();
	void TrapException();
	void WatchException();

	Exception occurred_exception;
	bool exception_has_occurred = false;
	int occurred_exception_priority = -1;
	u64 exception_bad_virt_addr;
	u64 exception_vector;
	uint coprocessor_unusable_source; /* 0 if COP0 signaled the exception, 1 if COP1 did it. */
	ExceptionHandler exception_handler;
}