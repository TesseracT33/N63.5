export module VR4300:Exceptions;

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
			SoftReset,
			Syscall,
			TlbInvalid,
			TlbMiss,
			TlbModification,
			Trap,
			Watch,
			XtlbMiss
		};

		template<Exception exception, Memory::Operation operation = Memory::Operation::Read>
		void SignalException();

		template<Memory::Operation operation>
		void SignalAddressErrorException(u64 bad_virt_addr);

		void HandleException();
	}

	using ExceptionHandler = void(*)();

	template<Exception, Memory::Operation>
	constexpr ExceptionHandler GetExceptionHandlerFun();

	template<Exception, Memory::Operation>
	constexpr int GetExceptionPriority();

	template<Exception>
	u64 GetExceptionVector();

	constexpr std::string_view ExceptionToString(Exception exception);

	/* Exception handlers */
	template<Memory::Operation> void AddressErrorException();
	template<Memory::Operation> void BusErrorException();
	template<Memory::Operation> void TlbInvalidException();
	template<Memory::Operation> void TlbMissException();
	template<Memory::Operation> void XtlbMissException();
	void BreakPointException();
	void BusErrorException();
	void ColdResetException();
	void CoprocessorUnusableException();
	void FloatingpointException();
	void IntegerOverflowException();
	void InterruptException();
	void NmiException();
	void ReservedInstructionException();
	void SoftResetException();
	void SyscallException();
	void TlbModException();
	void TrapException();
	void WatchException();

	/* Details when a TLB Miss etc. or Address Error exception happens */
	struct AddressFailure {
		u64 bad_virt_addr;
		u64 bad_vpn2;
		u64 bad_asid;
	} address_failure;

	Exception occurred_exception;
	bool exception_has_occurred = false;
	int occurred_exception_priority = -1;
	u64 exception_vector;
	uint coprocessor_unusable_source; /* 0 if COP0 signaled the exception, 1 if COP1 did it. */
	ExceptionHandler exception_handler;
}