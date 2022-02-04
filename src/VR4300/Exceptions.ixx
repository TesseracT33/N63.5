export module VR4300:Exceptions;

import <array>;
import <functional>;

import :COP0;
import :Registers;

import MemoryAccess;
import NumericalTypes;

namespace VR4300
{
	enum class Exception
	{
		AddressErrorInstructionFetch,
		AddressErrorDataAccess,
		ColdReset,
		NMI,
		SoftReset,
		TLB_Invalid = 7,
		TLB_Miss = 7,
		TLB_Modification,
		XTLB_Miss
	};

	void AddressErrorException();
	void ColdResetException();
	void DivisionByZeroException();
	void IntegerOverflowException();
	void InexactOperationException();
	void InvalidOperationException();
	void OverflowException();
	void NMI_Exception();
	void ReservedInstructionException();
	void SoftResetException();
	void TLB_InvalidException();
	void TLB_ModException();
	void TrapException();
	void UnderflowException();
	void UnimplementedOperationException();
	void XTLB_MissException();

	typedef void(*ExceptionHandlerFun)();
	ExceptionHandlerFun exception_fun_to_call;

	struct Exc
	{
		const unsigned exception_code, priority;
		const ExceptionHandlerFun handler_fun;
	};

	constexpr Exc cold_reset = { 0, 5, ColdResetException };
	constexpr Exc soft_reset = { 1, 5, SoftResetException };
	constexpr Exc nmi = { 2, 5, AddressErrorException };
	constexpr Exc address_error_instruction_fetch = { 3, 5, AddressErrorException };
	constexpr Exc tlb_miss = { 4, 5, AddressErrorException };
	constexpr Exc xtlb_miss = { 4, 5, AddressErrorException };
	constexpr Exc tlb_invalid = { 5, 5, AddressErrorException };
	constexpr Exc bus_error_instruction_fetch = { 6, 5, AddressErrorException };
	constexpr Exc system_call = { 7, 5, AddressErrorException };
	constexpr Exc breakpoint = { 8, 5, AddressErrorException };


	/* The CP0 'cause' register is set to these values during an exception (p. 172) */
	enum class ExceptionCode
	{
		Int, /* Interrupt */
		Mod, /* TLB Modification exception */
		TLBL, /* TLB Miss exception (load or instruction fetch) */
		TLBS, /* TLB Miss exception (store) */
		AdEL, /* Address Error exception (load or instruction fetch) */
		AdES, /* Address Error exception (store) */
		IBE, /* Bus Error exception (instruction fetch) */
		DBE, /* Bus Error exception (data reference: load or store) */
		Sys, /* Syscall exception */
		Bp, /* Breakpoint exception */
		RI, /* Reserved Instruction exception */
		CpU, /* Coprocessor Unusable exception */
		Ov, /* Arithmetic Overflow exception */
		Tr, /* Trap exception */
		FPE = 15, /* Floating-Point exception */
		WATCH = 23 /* Watch exception */
	};

	bool exception_has_occurred = false;
	Exception occurred_exception;
	Exc occ_exc = cold_reset;

	template<Exception exception>
	u64 GetExceptionVector()
	{ /* See p. 181, Table 6-3 */
		using enum Exception;

		/* Indexed by status.BEV */
		static constexpr std::array<u64, 2> vector_base_addr = {
			0xFFFF'FFFF'8000'0000, 0xFFFF'FFFF'BFC0'0200 };

		if constexpr (exception == ColdReset || exception == SoftReset || exception == NMI)
			return vector_base_addr[1];
		else if constexpr (exception == TLB_Miss)
			return vector_base_addr[COP0_reg.status.BEV] | (COP0_reg.status.EXL ? 0x0180 : 0x0000);
		else if constexpr (exception == XTLB_Miss)
			return vector_base_addr[COP0_reg.status.BEV] | (COP0_reg.status.EXL ? 0x0180 : 0x0080);
		else
			return vector_base_addr[COP0_reg.status.BEV] | 0x0180;
	}


	template<Exception exception>
	void SignalException()
	{
		if (exception_has_occurred)
		{
			/* Compare exception priorities; return if the new exception has a lower priority than an already occured one. */
			if (static_cast<unsigned>(exception) > static_cast<unsigned>(occurred_exception))
				return;
		}
		exception_has_occurred = true;
		occurred_exception = exception;
	}

	void HandleException()
	{
		//const auto exception_handler_fun = exception_handler_fun_table[static_cast<unsigned>(occurred_exception)];
		//std::invoke(exception_handler_fun);
		std::invoke(occ_exc.handler_fun);
		exception_has_occurred = false;
	}



	template<MemoryAccess::Operation operation>
	void TLB_MissException(const u32 bad_virt_addr, const u32 bad_VPN2);
}