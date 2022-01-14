export module VR4300:Exceptions;

import :MMU;

import NumericalTypes;

namespace VR4300
{
	enum class Exception
	{
		TLB_Invalid,
		TLB_Miss,
		TLB_Modification
	};

	template<Exception exception>
	void SignalException()
	{

	}

	bool exception_has_occurred = false;

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
	template<MemoryAccessOperation operation> void TLB_MissException(const u32 bad_virt_addr);
	void TLB_ModException();
	void TrapException();
	void UnderflowException();
	void UnimplementedOperationException();
	void XTLB_MissException();
}