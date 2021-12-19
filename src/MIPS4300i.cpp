#include "MIPS4300i.h"


#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_mul128)
#pragma intrinsic(_umul128)
#endif


void MIPS4300i::Run(const int cycles)
{

}


void MIPS4300i::ExecuteInstruction()
{
	const u32 instr_code = mmu->cpu_read_mem<u32>(PC);
	PC += 4;

	const u8 op_code = instr_code >> 26; /* (0-63) */
	switch (op_code)
	{
	case 0b000000: /* "SPECIAL" instructions */
	{
		const u8 sec_op_code = instr_code & 0x3F;
		switch (sec_op_code)
		{
		case 0b100000: ALU_ThreeOperand<Instr::ADD>(instr_code); break;
		case 0b100001: ALU_ThreeOperand<Instr::ADDU>(instr_code); break;
		case 0b100100: ALU_ThreeOperand<Instr::AND>(instr_code); break;
		case 0b101100: ALU_ThreeOperand<Instr::DADD>(instr_code); break;
		case 0b101101: ALU_ThreeOperand<Instr::DADDU>(instr_code); break;
		case 0b101110: ALU_ThreeOperand<Instr::DSUB>(instr_code); break;
		case 0b101111: ALU_ThreeOperand<Instr::DSUBU>(instr_code); break;
		case 0b100111: ALU_ThreeOperand<Instr::NOR>(instr_code); break;
		case 0b100101: ALU_ThreeOperand<Instr::OR>(instr_code); break;
		case 0b101010: ALU_ThreeOperand<Instr::SLT>(instr_code); break;
		case 0b101011: ALU_ThreeOperand<Instr::SLTU>(instr_code); break;
		case 0b100010: ALU_ThreeOperand<Instr::SUB>(instr_code); break;
		case 0b100011: ALU_ThreeOperand<Instr::SUBU>(instr_code); break;
		case 0b100110: ALU_ThreeOperand<Instr::XOR>(instr_code); break;

		case 0b111000: ALU_Shift<Instr::DSLL>(instr_code); break;
		case 0b010100: ALU_Shift<Instr::DSLLV>(instr_code); break;
		case 0b111100: ALU_Shift<Instr::DSLL32>(instr_code); break;
		case 0b111011: ALU_Shift<Instr::DSRA>(instr_code); break;
		case 0b010111: ALU_Shift<Instr::DSRAV>(instr_code); break;
		case 0b111111: ALU_Shift<Instr::DSRA32>(instr_code); break;
		case 0b111010: ALU_Shift<Instr::DSRL>(instr_code); break;
		case 0b010110: ALU_Shift<Instr::DSRLV>(instr_code); break;
		case 0b111110: ALU_Shift<Instr::DSRL32>(instr_code); break;
		case 0b000000: ALU_Shift<Instr::SLL>(instr_code); break;
		case 0b000100: ALU_Shift<Instr::SLLV>(instr_code); break;
		case 0b000011: ALU_Shift<Instr::SRA>(instr_code); break;
		case 0b000111: ALU_Shift<Instr::SRAV>(instr_code); break;
		case 0b000010: ALU_Shift<Instr::SRL>(instr_code); break;
		case 0b000110: ALU_Shift<Instr::SRLV>(instr_code); break;

		case 0b011110: ALU_MulDiv<Instr::DDIV>(instr_code); break;
		case 0b011111: ALU_MulDiv<Instr::DDIVU>(instr_code); break;
		case 0b011010: ALU_MulDiv<Instr::DIV>(instr_code); break;
		case 0b011011: ALU_MulDiv<Instr::DIVU>(instr_code); break;
		case 0b011100: ALU_MulDiv<Instr::DMULT>(instr_code); break;
		case 0b011101: ALU_MulDiv<Instr::DMULTU>(instr_code); break;
		case 0b011000: ALU_MulDiv<Instr::MULT>(instr_code); break;
		case 0b011001: ALU_MulDiv<Instr::MULTU>(instr_code); break;

		case 0b001001: Jump<Instr::JALR>(instr_code); break;
		case 0b001000: Jump<Instr::JR>(instr_code); break;

		case 0b110100: Trap_ThreeOperand<Instr::TEQ>(instr_code); break;
		case 0b110000: Trap_ThreeOperand<Instr::TGE>(instr_code); break;
		case 0b110001: Trap_ThreeOperand<Instr::TGEU>(instr_code); break;
		case 0b110010: Trap_ThreeOperand<Instr::TLT>(instr_code); break;
		case 0b110011: Trap_ThreeOperand<Instr::TLTU>(instr_code); break;
		case 0b110110: Trap_ThreeOperand<Instr::TNE>(instr_code); break;

		case 0b010000: MFHI(instr_code); break;
		case 0b010010: MFLO(instr_code); break;
		case 0b010001: MTHI(instr_code); break;
		case 0b010011: MTLO(instr_code); break;

		case 0b001101: BREAK(instr_code); break;
		case 0b001111: SYNC(instr_code); break;
		case 0b001100: SYSCALL(instr_code); break;
		}
		break;
	}

	case 0b000001: /* "REGIMM" instructions */
	{
		const u8 sec_op_code = instr_code >> 16 & 0x1F;
		switch (sec_op_code)
		{
		case 0b00001: Branch<Instr::BGEZ>(instr_code); break;
		case 0b10001: Branch<Instr::BGEZAL>(instr_code); break;
		case 0b10011: Branch<Instr::BGEZALL>(instr_code); break;
		case 0b00011: Branch<Instr::BGEZL>(instr_code); break;
		case 0b00000: Branch<Instr::BLTZ>(instr_code); break;
		case 0b10000: Branch<Instr::BLTZAL>(instr_code); break;
		case 0b10010: Branch<Instr::BLTZALL>(instr_code); break;
		case 0b00010: Branch<Instr::BLTZL>(instr_code); break;

		case 0b01100: Trap_Immediate<Instr::TEQI>(instr_code); break;
		case 0b01000: Trap_Immediate<Instr::TGEI>(instr_code); break;
		case 0b01001: Trap_Immediate<Instr::TGEIU>(instr_code); break;
		case 0b01010: Trap_Immediate<Instr::TLTI>(instr_code); break;
		case 0b01011: Trap_Immediate<Instr::TLTIU>(instr_code); break;
		case 0b01110: Trap_Immediate<Instr::TNEI>(instr_code); break;
		}
		break;
	}

	case 0b100000: Load<Instr::LB>(instr_code); break;
	case 0b100100: Load<Instr::LBU>(instr_code); break;
	case 0b110111: Load<Instr::LD>(instr_code); break;
	case 0b011010: Load<Instr::LDL>(instr_code); break;
	case 0b011011: Load<Instr::LDR>(instr_code); break;
	case 0b100001: Load<Instr::LH>(instr_code); break;
	case 0b100101: Load<Instr::LHU>(instr_code); break;
	case 0b110000: Load<Instr::LL>(instr_code); break;
	case 0b110100: Load<Instr::LLD>(instr_code); break;
	case 0b100011: Load<Instr::LW>(instr_code); break;
	case 0b100010: Load<Instr::LWL>(instr_code); break;
	case 0b100110: Load<Instr::LWR>(instr_code); break;
	case 0b100111: Load<Instr::LWU>(instr_code); break;

	case 0b101000: Store<Instr::SB>(instr_code); break;
	case 0b111000: Store<Instr::SC>(instr_code); break;
	case 0b111100: Store<Instr::SCD>(instr_code); break;
	case 0b111111: Store<Instr::SD>(instr_code); break;
	case 0b101100: Store<Instr::SDL>(instr_code); break;
	case 0b101101: Store<Instr::SDR>(instr_code); break;
	case 0b101001: Store<Instr::SH>(instr_code); break;
	case 0b101011: Store<Instr::SW>(instr_code); break;
	case 0b101010: Store<Instr::SWL>(instr_code); break;
	case 0b101110: Store<Instr::SWR>(instr_code); break;

	case 0b001000: ALU_Immediate<Instr::ADDI>(instr_code); break;
	case 0b001001: ALU_Immediate<Instr::ADDIU>(instr_code); break;
	case 0b001100: ALU_Immediate<Instr::ANDI>(instr_code); break;
	case 0b011000: ALU_Immediate<Instr::DADDI>(instr_code); break;
	case 0b011001: ALU_Immediate<Instr::DADDIU>(instr_code); break;
	case 0b001111: ALU_Immediate<Instr::LUI>(instr_code); break;
	case 0b001101: ALU_Immediate<Instr::ORI>(instr_code); break;
	case 0b001010: ALU_Immediate<Instr::SLTI>(instr_code); break;
	case 0b001011: ALU_Immediate<Instr::SLTIU>(instr_code); break;
	case 0b001110: ALU_Immediate<Instr::XORI>(instr_code); break;

	case 0b000010: Jump<Instr::J>(instr_code); break;
	case 0b000011: Jump<Instr::JAL>(instr_code); break;

	case 0b000100: Branch<Instr::BEQ>(instr_code); break;
	case 0b010100: Branch<Instr::BEQL>(instr_code); break;
	case 0b000111: Branch<Instr::BGTZ>(instr_code); break;
	case 0b010111: Branch<Instr::BGTZL>(instr_code); break;
	case 0b000110: Branch<Instr::BLEZ>(instr_code); break;
	case 0b010110: Branch<Instr::BLEZL>(instr_code); break;
	case 0b000101: Branch<Instr::BNE>(instr_code); break;
	case 0b010101: Branch<Instr::BNEL>(instr_code); break;

	case 0b101111: CACHE(instr_code); break;
	}
}


template<MIPS4300i::Instr instr>
void MIPS4300i::Load(const u32 instr_code)
{
	const s16 offset = instr_code & 0xFFFF;
	const u8 rt = instr_code >> 16 & 0x1F;
	const u8 base = instr_code >> 21 & 0x1F;
	const u64 address = GPR[base] + offset;

	/* For all instructions:
	   Generates an address by adding a sign-extended offset to the contents of register base. */
	if constexpr (instr == Instr::LB)
	{
		/* Load Byte;
		   Sign-extends the contents of a byte specified by the address and loads the result to register rt. */
		GPR[rt] = mmu->cpu_read_mem<s8>(address);
	}
	else if constexpr (instr == Instr::LBU)
	{
		/* Load Byte Unsigned;
		   Zero-extends the contents of a byte specified by the address and loads the result to register rt. */
		GPR[rt] = mmu->cpu_read_mem<u8>(address);
	}
	else if constexpr (instr == Instr::LH)
	{
		/* Load halfword;
		   Sign-extends the contents of a halfword specified by the address and loads the result to register rt. */
		if (address & 1)
			AddressErrorException();
		else
			GPR[rt] = mmu->cpu_read_mem<s16>(address);
	}
	else if constexpr (instr == Instr::LHU)
	{
		/* Load Halfword Unsigned;
		   Zero-extends the contents of a halfword specified by the address and loads the result to register rt. */
		if (address & 1)
			AddressErrorException();
		else
			GPR[rt] = mmu->cpu_read_mem<u16>(address);
	}
	else if constexpr (instr == Instr::LW)
	{
		/* Load Word;
		   Sign-extends the contents of a word specified by the address and loads the result to register rt. */
		if (address & 3)
			AddressErrorException();
		else
			GPR[rt] = mmu->cpu_read_mem<s32>(address);
	}
	else if constexpr (instr == Instr::LWU)
	{
		/* Load Word Unsigned;
		   Zero-extends the contents of a word specified by the address and loads the result to register rt. */
		if (address & 3)
			AddressErrorException();
		else
			GPR[rt] = mmu->cpu_read_mem<u32>(address);
	}
	else if constexpr (instr == Instr::LWL)
	{
		/* Load Word Left;
		   Shifts a word specified by the address to the left, so that a byte specified by
		   the address is at the leftmost position of the word. Sign-extends (in the 64-
		   bit mode), merges the result of the shift and the contents of register rt, and
		   loads the result to register rt. */
		GPR[rt] = mmu->cpu_read_mem<u32>(address);
	}
	else if constexpr (instr == Instr::LWR)
	{
		/* Load Word Right;
		   Shifts a word specified by the address to the right, so that a byte specified by
		   the address is at the rightmost position of the word. Sign-extends (in the 64-
		   bit mode), merges the result of the shift and the contents of register rt, and
		   loads the result to register rt. */
		GPR[rt] = mmu->cpu_read_mem<u32, MMU::ReadFromNextBoundary = true>(address);
	}
	else if constexpr (instr == Instr::LD)
	{
		/* Load Doubleword;
		   Loads the contents of a word specified by the address to register rt. */
		if (address & 7)
			AddressErrorException();
		else
			GPR[rt] = mmu->cpu_read_mem<u32>(address);
	}
	else if constexpr (instr == Instr::LDL)
	{
		/* Load Doubleword Left;
		   Shifts the doubleword specified by the address to the left so that the byte
		   specified by the address is at the leftmost position of the doubleword.
		   Merges the result of the shift and the contents of register rt, and loads the
		   result to register rt. */
		GPR[rt] = mmu->cpu_read_mem<u64>(address);
	}
	else if constexpr (instr == Instr::LDR)
	{
		/* Load Doubleword Right;
		   Shifts the doubleword specified by the address to the right so that the byte
		   specified by the address is at the rightmost position of the doubleword.
		   Merges the result of the shift and the contents of register rt, and loads the
		   result to register rt. */
		GPR[rt] = mmu->cpu_read_mem<u64, MMU::ReadFromNextBoundary = true>(address);
	}
	else if constexpr (instr == Instr::LL)
	{
		/* Load Linked;
		   Loads the contents of the word specified by the address to register rt and sets the LL bit to 1. */
		GPR[rt] = mmu->cpu_read_mem<s32>(address);
		LL = 1;
		/* TODO the specified physical address of the memory is stored to the LLAddr register */
	}
	else if constexpr (instr == Instr::LLD)
	{
		/* Load Linked Doubleword;
		   Loads the contents of the doubleword specified by the address to register rt and sets the LL bit to 1. */
		GPR[rt] = mmu->cpu_read_mem<u64>(address);
		LL = 1;
	}
	else
	{
		static_assert(false, "\"Load\" template function called, but no matching load instruction was found.");
	}
}


template<MIPS4300i::Instr instr>
void MIPS4300i::Store(const u32 instr_code)
{
	const s16 offset = instr_code & 0xFFFF;
	const u8 rt = instr_code >> 16 & 0x1F;
	const u8 base = instr_code >> 21 & 0x1F;
	const u64 address = GPR[base] + offset;

	/* For all instructions:
	   Generates an address by adding a sign-extended offset to the contents of register base. */
	if constexpr (instr == Instr::SB)
	{
		/* Store Byte;
		   Stores the contents of the low-order byte of register rt to the memory specified by the address. */
		mmu->cpu_write_mem<u8>(address, u8(GPR[rt]));
	}
	else if constexpr (instr == Instr::SH)
	{
		/* Store Halfword;
		   Stores the contents of the low-order halfword of register rt to the memory specified by the address. */
		if (address & 1)
			AddressErrorException();
		else
			mmu->cpu_write_mem<u16>(address, u16(GPR[rt]));
	}
	else if constexpr (instr == Instr::SW)
	{
		/* Store Word;
		   Stores the contents of the low-order word of register rt to the memory specified by the address. */
		if (address & 3)
			AddressErrorException();
		else
			mmu->cpu_write_mem<u32>(address, u32(GPR[rt]));
	}
	else if constexpr (instr == Instr::SWL)
	{
		/* Store Word Left;
		   Shifts the contents of register rt to the right so that the leftmost byte of the
		   word is at the position of the byte specified by the address. Stores the result
		   of the shift to the lower portion of the word in memory. */
		mmu->cpu_write_mem<u32>(address, u32(GPR[rt])); /* TODO write function should handle this? */
	}
	else if constexpr (instr == Instr::SWR)
	{
		/* Store Word Right;
		   Shifts the contents of register rt to the left so that the rightmost byte of the
		   word is at the position of the byte specified by the address. Stores the result
		   of the shift to the higher portion of the word in memory. */
		mmu->cpu_write_mem<u32, MMU::WriteToNextBoundary = true>(address, u32(GPR[rt]));
	}
	else if constexpr (instr == Instr::SD)
	{
		/* Store Doublword;
		   Stores the contents of register rt to the memory specified by the address. */
		if (address & 7)
			AddressErrorException();
		else
			mmu->cpu_write_mem<u64>(address, GPR[rt]);
	}
	else if constexpr (instr == Instr::SDL)
	{
		/* Store Doubleword Left;
		   Shifts the contents of register rt to the right so that the leftmost byte of a
		   doubleword is at the position of the byte specified by the address. Stores the
		   result of the shift to the lower portion of the doubleword in memory. */
		mmu->cpu_write_mem<u64>(address, GPR[rt]);
	}
	else if constexpr (instr == Instr::SDR)
	{
		/* Store Doubleword Right;
		   Shifts the contents of register rt to the left so that the rightmost byte of a
		   doubleword is at the position of the byte specified by the address. Stores the
		   result of the shift to the higher portion of the doubleword in memory. */
		mmu->cpu_write_mem<u64, MMU::WriteToNextBoundary = true>(address, GPR[rt]);
	}
	else if constexpr (instr == Instr::SC)
	{
		/* Store Conditional;
		   If the LL bit is 1, stores the contents of the low-order word of register rt to
		   the memory specified by the address, and sets register rt to 1.
		   If the LL bit is 0, does not store the contents of the word, and clears register
		   rt to 0. */
		if (LL == 1)
		{
			mmu->cpu_write_mem<u32>(address, GPR[rt]);
			GPR[rt] = 1;
		}
		else
		{
			GPR[rt] = 0;
		}
	}
	else if constexpr (instr == Instr::SCD)
	{
		/* Store Conditional Doubleword;
		   If the LL bit is 1, stores the contents of register rt to the memory specified by
		   the address, and sets register rt to 1.
		   If the LL bit is 0, does not store the contents of the register, and clears register
		   rt to 0. */
		if (LL == 1)
		{
			mmu->cpu_write_mem<u64>(address, GPR[rt]);
			GPR[rt] = 1;
		}
		else
		{
			GPR[rt] = 0;
		}
	}
	else
	{
		static_assert(false, "\"Store\" template function called, but no matching store instruction was found.");
	}
}


template<MIPS4300i::Instr instr>
void MIPS4300i::ALU_Immediate(const u32 instr_code)
{
	const u8 rt = instr_code >> 16 & 0x1F;
	const u8 rs = instr_code >> 21 & 0x1F;

	const auto immediate = [&] {
		if constexpr (instr == Instr::ADDI || instr == Instr::ADDIU || instr == Instr::SLTI || instr == Instr::SLTIU || instr == Instr::DADDI || instr == Instr::DADDIU)
			return s16(instr_code & 0xFFFF);
		else
			return u16(instr_code & 0xFFFF);
	}();

	if constexpr (instr == Instr::ADDI)
	{
		/* Add Immediate;
		   Sign-extends the 16-bit immediate and adds it to register rs. Stores the
		   32-bit result to register rt (sign-extends the result in the 64-bit mode).
		   Generates an exception if a 2's complement integer overflow occurs. */
		const s32 sum = GPR[rs] + immediate;
		const bool overflow = (GPR[rs] ^ sum) & (immediate ^ sum) & 0x80000000;
		if (overflow)
			IntegerOverflowException();
		else
			GPR[rt] = sum;
	}
	else if constexpr (instr == Instr::ADDIU)
	{
		/* Add Immediate Unsigned;
		   Sign-extends the 16-bit immediate and adds it to register rs. Stores the 32-bit
		   result to register rt (sign-extends the result in the 64-bit mode). Does not
		   generate an exception even if an integer overflow occurs. */
		const s32 sum = GPR[rs] + immediate;
		GPR[rt] = sum;
	}
	else if constexpr (instr == Instr::SLTI)
	{
		/* Set On Less Than Immediate;
		   Sign-extends the 16-bit immediate and compares it with register rs as a
		   signed integer. If rs is less than the immediate, stores 1 to register rt;
		   otherwise, stores 0 to register rt. */
		GPR[rt] = s64(GPR[rs]) < immediate; /* TODO s32 in 32-bit mode */
	}
	else if constexpr (instr == Instr::SLTIU)
	{
		/* Set On Less Than Immediate Unsigned;
		   Sign-extends the 16-bit immediate and compares it with register rs as an
		   unsigned integer. If rs is less than the immediate, stores 1 to register rt;
		   otherwise, stores 0 to register rt. */
		/* TODO */
		GPR[rt] = GPR[rs] < immediate;
	}
	else if constexpr (instr == Instr::ANDI)
	{
		/* And Immediate;
		   Zero-extends the 16-bit immediate, ANDs it with register rs, and stores the
		   result to register rt. */
		GPR[rt] = GPR[rs] & immediate;
	}
	else if constexpr (instr == Instr::ORI)
	{
		/* Or Immediate;
		   Zero-extends the 16-bit immediate, ORs it with register rs, and stores the
		   result to register rt. */
		GPR[rt] = GPR[rs] | immediate;
	}
	else if constexpr (instr == Instr::XORI)
	{
		/* Exclusive Or Immediate;
		   Zero-extends the 16-bit immediate, exclusive-ORs it with register rs, and
		   stores the result to register rt. */
		GPR[rt] = GPR[rs] ^ immediate;
	}
	else if constexpr (instr == Instr::LUI)
	{
		/* Load Upper Immediate;
		   Shifts the 16-bit immediate 16 bits to the left, and clears the low-order 16 bits
		   of the word to 0.
		   Stores the result to register rt (by sign-extending the result in the 64-bit mode). */
		const s32 result = immediate << 16;
		GPR[rt] = result;
	}
	else if constexpr (instr == Instr::DADDI)
	{
		/* Doubleword Add Immediate;
		   Sign-extends the 16-bit immediate to 64 bits, and adds it to register rs. Stores
		   the 64-bit result to register rt. Generates an exception if an integer overflow occurs. */

		/* todo This operation is only defined for the VR4300 operating in 64-bit mode and in 32-
bit Kernel mode. Execution of this instruction in 32-bit User or Supervisor mode
causes a reserved instruction exception*/
		const s64 sum = (s64)GPR[rs] + immediate;
		const bool overflow = (GPR[rs] ^ sum) & (immediate ^ sum) & 0x80000000'00000000;
		if (overflow)
			IntegerOverflowException();
		else
			GPR[rt] = sum;
	}
	else if constexpr (instr == Instr::DADDIU)
	{
		/* Doubleword Add Immediate Unsigned;
		   Sign-extends the 16-bit immediate to 64 bits, and adds it to register rs. Stores
		   the 64-bit result to register rt. Does not generate an exception even if an
		   integer overflow occurs. */
		const s64 sum = (s64)GPR[rs] + immediate;
		GPR[rt] = sum;
	}
	else
	{
		static_assert(false, "\"ALU_Immediate\" template function called, but no matching ALU immediate instruction was found.");
	}
}


template<MIPS4300i::Instr instr>
void MIPS4300i::ALU_ThreeOperand(const u32 instr_code)
{
	const u8 rd = instr_code >> 11 & 0x1F;
	const u8 rt = instr_code >> 16 & 0x1F;
	const u8 rs = instr_code >> 21 & 0x1F;

	if constexpr (instr == Instr::ADD)
	{
		/* Add;
		   Adds the contents of register rs and rt, and stores (sign-extends in the 64-bit mode)
		   the 32-bit result to register rd. Generates an exception if an integer overflow occurs. */

		/* TODO On 64-bit processors, if either GPR rt or GPR rs do not contain sign-extended 32-bit
values (bits 63..31 equal), then the result of the operation is undefined.*/
		const s32 sum = s32(GPR[rs]) + s32(GPR[rt]);
		const bool overflow = (GPR[rs] ^ sum) & (GPR[rt] ^ sum) & 0x80000000;
		if (overflow)
			IntegerOverflowException();
		else
			GPR[rd] = sum;
	}
	else if constexpr (instr == Instr::ADDU)
	{
		/* Add Unsigned;
		   Adds the contents of register rs and rt, and stores (sign-extends in the 64-bit mode)
		   the 32-bit result to register rd. Does not generate an exception even if an integer overflow occurs. */
		const s32 sum = s32(GPR[rs]) + s32(GPR[rt]);
		GPR[rd] = sum;
	}
	else if constexpr (instr == Instr::SUB)
	{
		/* Subtract;
		   Subtracts the contents of register rs from register rt, and stores (sign-extends
		   in the 64-bit mode) the result to register rd. Generates an exception if an integer overflow occurs. */
		const s32 sum = s32(GPR[rs]) - s32(GPR[rt]);
		const bool overflow = (GPR[rs] ^ sum) & (GPR[rt] ^ sum) & 0x80000000;
		if (overflow)
			IntegerOverflowException();
		else
			GPR[rd] = sum;
	}
	else if constexpr (instr == Instr::SUBU)
	{
		/* Subtract Unsigned;
		   Subtracts the contents of register rt from register rs, and stores (sign-extends
		   in the 64-bit mode) the 32-bit result to register rd.
		   Does not generate an exception even if an integer overflow occurs.*/
		const s32 sum = s32(GPR[rs]) - s32(GPR[rt]);
		GPR[rd] = sum;
	}
	else if constexpr (instr == Instr::SLT)
	{
		/* Set On Less Than;
		   Compares the contents of registers rs and rt as signed integers.
		   If the contents of register rs are less than those of rt, stores 1 to register rd;
		   otherwise, stores 0 to rd. */
		GPR[rd] = s64(GPR[rs]) < s64(GPR[rt]); /* todo: is it 32 bit comparison or not? */
	}
	else if constexpr (instr == Instr::SLTU)
	{
		/* Set On Less Than Unsigned;
		   Compares the contents of registers rs and rt as unsigned integers.
		   If the contents of register rs are less than those of rt, stores 1 to register rd;
		   otherwise, stores 0 to rd. */
		GPR[rd] = GPR[rs] < GPR[rt];
	}
	else if constexpr (instr == Instr::AND)
	{
		/* And;
		   ANDs the contents of registers rs and rt in bit units, and stores the result to register rd. */
		GPR[rd] = GPR[rs] & GPR[rt];
	}
	else if constexpr (instr == Instr::OR)
	{
		/* Or;
		   ORs the contents of registers rs and rt in bit units, and stores the result to register rd. */
		GPR[rd] = GPR[rs] | GPR[rt];
	}
	else if constexpr (instr == Instr::XOR)
	{
		/* Exclusive Or;
		   Exclusive-ORs the contents of registers rs and rt in bit units, and stores the result to register rd. */
		GPR[rd] = GPR[rs] ^ GPR[rt];
	}
	else if constexpr (instr == Instr::NOR)
	{
		/* Nor;
		   NORs the contents of registers rs and rt in bit units, and stores the result to register rd. */
		GPR[rd] = ~(GPR[rs] | GPR[rt]);
	}
	else if constexpr (instr == Instr::DADD)
	{
		/* Doubleword Add;
		   Adds the contents of registers rs and rt, and stores the 64-bit result to register rd.
		   Generates an exception if an integer overflow occurs. */

		/* TODO Execution of this instruction in 32-bit User or Supervisor mode
causes a reserved instruction exception.*/
		const u64 sum = GPR[rs] + GPR[rt];
		const bool overflow = (GPR[rs] ^ sum) & (GPR[rt] ^ sum) & 0x80000000'00000000;
		if (overflow)
			IntegerOverflowException();
		else
			GPR[rd] = sum;
	}
	else if constexpr (instr == Instr::DADDU)
	{
		/* Doubleword Add Unsigned;
		   Adds the contents of registers rs and rt, and stores the 64-bit result to register rd.
		   Does not generate an exception even if an integer overflow occurs. */
		const u64 sum = GPR[rs] + GPR[rt];
		GPR[rd] = sum;
	}
	else if constexpr (instr == Instr::DSUB)
	{
		/* Doubleword Subtract;
		   Subtracts the contents of register rt from register rs, and stores the 64-bit
		   result to register rd. Generates an exception if an integer overflow occurs. */
		const u64 sum = GPR[rs] - GPR[rt];
		const bool overflow = (GPR[rs] ^ sum) & (GPR[rt] ^ sum) & 0x80000000'00000000;
		if (overflow)
			IntegerOverflowException();
		else
			GPR[rd] = sum;
	}
	else if constexpr (instr == Instr::DSUBU)
	{
		/* Doubleword Subtract Unsigned;
		   Subtracts the contents of register rt from register rs, and stores the 64-bit result to register rd.
		   Does not generate an exception even if an integer overflow occurs. */
		const u64 sum = GPR[rs] - GPR[rt];
		GPR[rd] = sum;
	}
	else
	{
		static_assert(false, "\"ALU_ThreeOperand\" template function called, but no matching ALU three-operand instruction was found.");
	}
}


template<MIPS4300i::Instr instr>
void MIPS4300i::ALU_Shift(const u32 instr_code)
{
	const u8 rd = instr_code >> 11 & 0x1F;
	const u8 rt = instr_code >> 16 & 0x1F;

	const u8 rs = [&] {
		if constexpr (instr == Instr::SLLV || instr == Instr::SRLV || instr == Instr::SRAV || instr == Instr::DSLLV || instr == Instr::DSRLV || instr == Instr::DSRAV)
			return instr_code >> 21 & 0x1F;
		else return 0;
	}();

	const u8 sa = [&] {
		if constexpr (instr == Instr::SLL || instr == Instr::SRL || instr == Instr::SRA || instr == Instr::DSLL || instr == Instr::DSRL || instr == Instr::DSRA || instr == Instr::DSLL32 || instr == Instr::DSRL32)
			return instr_code & 0x1F;
		else return 0;
	}();

	if constexpr (instr == Instr::SLL)
	{
		/* Shift Left Logical;
		   Shifts the contents of register rt sa bits to the left, and inserts 0 to the low-order bits.
		   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
		const s32 result = s32(GPR[rt]) << sa;
		GPR[rd] = result;
	}
	else if constexpr (instr == Instr::SRL)
	{
		/* Shift Right Logical;
		   Shifts the contents of register rt sa bits to the right, and inserts 0 to the high-order bits.
		   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
		const s32 result = GPR[rt] >> sa;
		GPR[rd] = result;
	}
	else if constexpr (instr == Instr::SRA)
	{
		/* Shift Right Arithmetic;
		   Shifts the contents of register rt sa bits to the right, and sign-extends the high-order bits.
		   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
		const s32 result = (s32)GPR[rt] >> sa;
		GPR[rd] = result;
	}
	else if constexpr (instr == Instr::SLLV)
	{
		/* Shift Left Logical Variable;
		   Shifts the contents of register rt to the left and inserts 0 to the low-order bits.
		   The number of bits by which the register contents are to be shifted is
		   specified by the low-order 5 bits of register rs.
		   Sign-extends (in the 64-bit mode) the result and stores it to register rd. */
		const s32 result = s32(GPR[rt]) << (GPR[rs] & 0x1F);
		GPR[rd] = result;
	}
	else if constexpr (instr == Instr::SRLV)
	{
		/* Shift Right Logical Variable;
		   Shifts the contents of register rt to the right, and inserts 0 to the high-order bits.
		   The number of bits by which the register contents are to be shifted is
		   specified by the low-order 5 bits of register rs.
		   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
		const s32 result = GPR[rt] >> (GPR[rs] & 0x1F);
		GPR[rd] = result;
	}
	else if constexpr (instr == Instr::SRAV)
	{
		/* Shift Right Arithmetic Variable;
		   Shifts the contents of register rt to the right and sign-extends the high-order bits.
		   The number of bits by which the register contents are to be shifted is
		   specified by the low-order 5 bits of register rs.
		   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
		GPR[rd] = (s32)GPR[rt] >> (GPR[rs] & 0x1F);
	}
	else if constexpr (instr == Instr::DSLL)
	{
		/* Doubleword Shift Left Logical;
		   Shifts the contents of register rt sa bits to the left, and inserts 0 to the low-order bits.
		   Stores the 64-bit result to register rd. */
		GPR[rd] = GPR[rt] << sa;
	}
	else if constexpr (instr == Instr::DSRL)
	{
		/* Doubleword Shift Right Logical;
		   Shifts the contents of register rt sa bits to the right, and inserts 0 to the high-order bits.
		   Stores the 64-bit result to register rd. */
		GPR[rd] = GPR[rt] >> sa;
	}
	else if constexpr (instr == Instr::DSRA)
	{
		/* Doubleword Shift Right Arithmetic;
		   Shifts the contents of register rt sa bits to the right, and sign-extends the high-order bits.
		   Stores the 64-bit result to register rd. */
		GPR[rd] = (s64)GPR[rt] >> sa;
	}
	else if constexpr (instr == Instr::DSLLV)
	{
		/* Doubleword Shift Left Logical Variable;
		   Shifts the contents of register rt to the left, and inserts 0 to the low-order bits.
		   The number of bits by which the register contents are to be shifted is
		   specified by the low-order 6 bits of register rs.
		   Stores the 64-bit result and stores it to register rd. */
		GPR[rd] = GPR[rt] << (GPR[rs] & 0x3F);
	}
	else if constexpr (instr == Instr::DSRLV)
	{
		/* Doubleword Shift Right Logical Variable;
		   Shifts the contents of register rt to the right, and inserts 0 to the higher bits.
		   The number of bits by which the register contents are to be shifted is
		   specified by the low-order 6 bits of register rs.
		   Sign-extends the 64-bit result and stores it to register rd. */
		GPR[rd] = (s64)(GPR[rt] >> (GPR[rs] & 0x3F)); /* TODO sign-extends the 64-bit result? */
	}
	else if constexpr (instr == Instr::DSRAV)
	{
		/* Doubleword Shift Right Arithmetic Variable;
		   Shifts the contents of register rt to the right, and sign-extends the high-order bits.
		   The number of bits by which the register contents are to be shifted is
		   specified by the low-order 6 bits of register rs.
		   Sign-extends the 64-bit result and stores it to register rd. */
		GPR[rd] = (s64)GPR[rt] >> (GPR[rs] & 0x3F);
	}
	else if constexpr (instr == Instr::DSLL32)
	{
		/* Doubleword Shift Left Logical + 32;
		   Shifts the contents of register rt 32+sa bits to the left, and inserts 0 to the low-order bits.
		   Stores the 64-bit result to register rd. */
		GPR[rd] = GPR[rt] << (sa + 32);
	}
	else if constexpr (instr == Instr::DSRL32)
	{
		/* Doubleword Shift Right Logical + 32;
		   Shifts the contents of register rt 32+sa bits to the right, and inserts 0 to the high-order bits.
		   Stores the 64-bit result to register rd. */
		GPR[rd] = GPR[rt] >> (sa + 32);
	}
	else if constexpr (instr == Instr::DSRA32)
	{
		/* Doubleword Shift Right Arithmetic + 32;
		   Shifts the contents of register rt 32+sa bits to the right, and sign-extends the high-order bits.
		   Stores the 64-bit result to register rd.*/
		GPR[rd] = (s64)GPR[rt] >> (sa + 32);
	}
	else
	{
		static_assert(false, "\"ALU_Shift\" template function called, but no matching ALU shift instruction was found.");
	}
}


template<MIPS4300i::Instr instr>
void MIPS4300i::ALU_MulDiv(const u32 instr_code)
{
	const u8 rt = instr_code >> 16 & 0x1F;
	const u8 rs = instr_code >> 21 & 0x1F;

	if constexpr (instr == Instr::MULT)
	{
		/* Multiply;
		   Multiplies the contents of register rs by the contents of register rt as a 32-bit
		   signed integer. Sign-extends (in the 64-bit mode) and stores the 64-bit result
		   to special registers HI and LO. */
		const s64 result = s64(GPR[rs] & 0xFFFFFFFF) * s64(GPR[rt] & 0xFFFFFFFF);
		LO = result & 0xFFFFFFFF;
		HI = result >> 32;
	}
	else if constexpr (instr == Instr::MULTU)
	{
		/* Multiply Unsigned;
		   Multiplies the contents of register rs by the contents of register rt as a 32-bit
		   unsigned integer. Sign-extends (in the 64-bit mode) and stores the 64-bit
		   result to special registers HI and LO. */
		const u64 result = (GPR[rs] & 0xFFFFFFFF) * (GPR[rt] & 0xFFFFFFFF);
		LO = s32(result & 0xFFFFFFFF);
		HI = s32(result >> 32);
	}
	else if constexpr (instr == Instr::DIV)
	{
		/* Divide;
		   Divides the contents of register rs by the contents of register rt. The operand
		   is treated as a 32-bit signed integer. Sign-extends (in the 64-bit mode) and
		   stores the 32-bit quotient to special register LO and the 32-bit remainder to
		   special register HI. */
		const s32 quotient = s32(GPR[rs]) / s32(GPR[rt]);
		const s32 remainder = s32(GPR[rs]) % s32(GPR[rt]);
		LO = quotient;
		HI = remainder;
	}
	else if constexpr (instr == Instr::DIVU)
	{
		/* Divide Unsigned;
		   Divides the contents of register rs by the contents of register rt. The operand
		   is treated as a 32-bit unsigned integer. Sign-extends (in the 64-bit mode) and
		   stores the 32-bit quotient to special register LO and the 32-bit remainder to
		   special register HI. */
		const u32 quotient = u32(GPR[rs]) / u32(GPR[rt]);
		const u32 remainder = u32(GPR[rs]) % u32(GPR[rt]);
		LO = s32(quotient);
		HI = s32(remainder);
	}
	else if constexpr (instr == Instr::DMULT)
	{
		/* Doubleword Multiply;
		   Multiplies the contents of register rs by the contents of register rt as a signed integer.
		   Stores the 128-bit result to special registers HI and LO. */
#if defined _MSC_VER
		__int64 high_product;
		__int64 low_product = _mul128(GPR[rs], GPR[rt], &high_product);
		LO = low_product;
		HI = high_product;
#elif defined __clang__ || defined __GNUC__
		__int128 product = __int128(GPR[rs]) * __int128(GPR[rt]);
		LO = product & 0xFFFFFFFF'FFFFFFFF;
		HI = product >> 64;
#else
		static_assert(false);
#endif
	}
	else if constexpr (instr == Instr::DMULTU)
	{
		/* Doubleword Multiply Unsigned;
		   Multiplies the contents of register rs by the contents of register rt as an unsigned integer.
		   Stores the 128-bit result to special registers HI and LO. */
#if defined _MSC_VER
		unsigned __int64 high_product;
		unsigned __int64 low_product = _umul128(GPR[rs], GPR[rt], &high_product);
		LO = low_product;
		HI = high_product;
#elif defined __clang__ || defined __GNUC__
		unsigned __int128 product = unsigned __int128(GPR[rs]) * unsigned __int128(GPR[rt]);
		LO = product & 0xFFFFFFFF'FFFFFFFF;
		HI = product >> 64;
#else
		static_assert(false);
#endif
	}
	else if constexpr (instr == Instr::DDIV)
	{
		/* Doubleword Divide;
		   Divides the contents of register rs by the contents of register rt.
		   The operand is treated as a signed integer.
		   Stores the 64-bit quotient to special register LO, and the 64-bit remainder to special register HI. */

		/* TODO how to handle division by zero? "the result of this operation is undefined when the divisor is zero." */
		const s64 quotient = (s64)GPR[rs] / (s64)GPR[rt];
		const s64 remainder = (s64)GPR[rs] % (s64)GPR[rt];
		LO = quotient;
		HI = remainder;
	}
	else if constexpr (instr == Instr::DDIVU)
	{
		/* Doubleword Divide Unsigned;
		   Divides the contents of register rs by the contents of register rt.
		   The operand is treated as an unsigned integer.
		   Stores the 64-bit quotient to special register LO, and the 64-bit remainder to special register HI. */
		const u64 quotient = GPR[rs] / GPR[rt];
		const u64 remainder = GPR[rs] % GPR[rt];
		LO = quotient;
		HI = remainder;
	}
	else
	{
		static_assert(false, "\"ALU_MulDiv\" template function called, but no matching ALU mul/div instruction was found.");
	}
}


template<MIPS4300i::Instr instr>
void MIPS4300i::Jump(const u32 instr_code)
{
	/* J: Jump;
	   Shifts the 26-bit target address 2 bits to the left, and jumps to the address
	   coupled with the high-order 4 bits of the PC, delayed by one instruction.

	   JAL: Jump And Link;
	   Shifts the 26-bit target address 2 bits to the left, and jumps to the address
	   coupled with the high-order 4 bits of the PC, delayed by one instruction.
	   Stores the address of the instruction following the delay slot to r31 (link register).

	   JR: Jump Register;
	   Jumps to the address of register rs, delayed by one instruction.

	   JALR: Jump And Link Register;
	   Jumps to the address of register rs, delayed by one instruction.
	   Stores the address of the instruction following the delay slot to register rd. */

	const u64 target = [&] {
		if constexpr (instr == Instr::J || instr == Instr::JAL)
		{
			return PC & 0xFFFF'FFFF'F000'0000 | u64(instr_code & 0x3FFFFFF) << 2;
		}
		else if constexpr (instr == Instr::JR || instr == Instr::JALR)
		{
			const u8 rs = instr_code >> 21 & 0x1F;
			return GPR[rs];
		}
		else
		{
			static_assert(false, "\"Jump\" template function called, but no matching jump instruction was found.");
		}
	}();

	addr_to_jump_to = target;
	jump_next_instruction = true;

	if constexpr (instr == Instr::JAL)
	{
		GPR[31] = PC + 4;
	}
	else if constexpr (instr == Instr::JALR)
	{
		const u8 rd = instr_code >> 11 & 0x1F;
		GPR[rd] = PC + 4;
	}
}


template<MIPS4300i::Instr instr>
void MIPS4300i::Branch(const u32 instr_code)
{
	const u8 rs = instr_code >> 21 & 0x1F;

	if constexpr (instr == Instr::BLTZAL || instr == Instr::BGEZAL || instr == Instr::BLTZALL || instr == Instr::BGEZALL)
	{
		GPR[31] = PC + 4;
	}

	const u8 rt = [&] {
		if constexpr (instr == Instr::BEQ || instr == Instr::BNE || instr == Instr::BEQL || instr == Instr::BNEL)
			return instr_code >> 16 & 0x1F;
		else return 0; /* TODO: feels like a hack. RT is not needed for the other instructions, but I cannot put the declaration of it in an if-constexpr. */
	}();

	const bool branch_cond = [&] {
		if constexpr (instr == Instr::BEQ || instr == Instr::BEQL) /* Branch On Equal (Likely) */
			return GPR[rs] == GPR[rt];
		else if constexpr (instr == Instr::BNE || instr == Instr::BNEL) /* Branch On Not Equal (Likely) */
			return GPR[rs] != GPR[rt];
		else if constexpr (instr == Instr::BLEZ || instr == Instr::BLEZL) /* Branch On Less Than Or Equal To Zero (Likely) */
			return s64(GPR[rs]) <= 0;
		else if constexpr (instr == Instr::BGTZ || instr == Instr::BGTZL) /* Branch On Greater Than Zero (Likely) */
			return s64(GPR[rs]) > 0;
		else if constexpr (instr == Instr::BLTZ || instr == Instr::BLTZL) /* Branch On Less Than Zero (Likely) */
			return s64(GPR[rs]) < 0;
		else if constexpr (instr == Instr::BGEZ || instr == Instr::BGEZL) /* Branch On Greater Than or Equal To Zero (Likely) */
			return s64(GPR[rs]) >= 0;
		else if constexpr (instr == Instr::BLTZAL || instr == Instr::BLTZALL) /* Branch On Less Than Zero and Link (Likely) */
			return s64(GPR[rs]) < 0;
		else if constexpr (instr == Instr::BGEZAL || instr == Instr::BGEZALL) /* Branch On Greater Than Or Equal To Zero And Link (Likely) */
			return s64(GPR[rs]) >= 0;
		else
		{
			static_assert(false, "\"Branch\" template function called, but no matching branch instruction was found.");
		}
	}();

	if (branch_cond)
	{
		const s32 offset = s32(instr_code & 0xFFFF) << 2;
		PC += offset;
	}
	else if constexpr (instr == Instr::BEQL || instr == Instr::BNEL || instr == Instr::BLEZL || instr == Instr::BGTZL ||
		instr == Instr::BEQL || instr == Instr::BLTZL || instr == Instr::BGEZL || instr == Instr::BLTZALL || instr == Instr::BGEZALL)
	{
		PC += 4; /* TODO no idea if correct or not */
	}
}


template<MIPS4300i::Instr instr>
void MIPS4300i::Trap_ThreeOperand(const u32 instr_code)
{
	const u8 rt = instr_code >> 16 & 0x1F;
	const u8 rs = instr_code >> 21 & 0x1F;

	const bool trap_cond = [&] {
		if constexpr (instr == Instr::TGE)
		{
			/* Trap If Greater Than Or Equal;
			   Compares registers rs and rt as signed integers.
			   If register rs is greater than rt, generates an exception. */
			return s64(GPR[rs]) > s64(GPR[rt]);
		}
		else if constexpr (instr == Instr::TGEU)
		{
			/* Trap If Greater Than Or Equal Unsigned;
			   Compares registers rs and rt as unsigned integers.
			   If register rs is greater than rt, generates an exception. */
			return GPR[rs] > GPR[rt];
		}
		else if constexpr (instr == Instr::TLT)
		{
			/* Trap If Less Than;
			   Compares registers rs and rt as signed integers.
			   If register rs is less than rt, generates an exception. */
			return s64(GPR[rs]) < s64(GPR[rt]);
		}
		else if constexpr (instr == Instr::TLTU)
		{
			/* Trap If Less Than Unsigned;
			   Compares registers rs and rt as unsigned integers.
			   If register rs is less than rt, generates an exception. */
			return GPR[rs] < GPR[rt];
		}
		else if constexpr (instr == Instr::TEQ)
		{
			/* Trap If Equal;
			   Generates an exception if registers rs and rt are equal. */
			return GPR[rs] == GPR[rt];
		}
		else if constexpr (instr == Instr::TNE)
		{
			/* Trap If Not Equal;
			   Generates an exception if registers rs and rt are not equal. */
			return GPR[rs] != GPR[rt];
		}
		else
		{
			static_assert(false, "\"Trap_ThreeOperand\" template function called, but no matching trap instruction was found.");
		}
	}();

	if (trap_cond)
		TrapException();
}


template<MIPS4300i::Instr instr>
void MIPS4300i::Trap_Immediate(const u32 instr_code)
{
	const s16 immedate = instr_code & 0xFFFF;
	const u8 rs = instr_code >> 21 & 0x1F;

	const bool trap_cond = [&] {
		if constexpr (instr == Instr::TGEI)
		{
			/* Trap If Greater Than Or Equal Immediate;
			   Compares the contents of register rs with 16-bit sign-extended immediate as a
			   signed integer. If rs contents are greater than the immediate, generates an exception. */
			return s64(GPR[rs]) > immedate;
		}
		else if constexpr (instr == Instr::TGEIU)
		{
			/* Trap If Greater Than Or Equal Immediate Unsigned;
			   Compares the contents of register rs with 16-bit zero-extended immediate as an
			   unsigned integer. If rs contents are greater than the immediate, generates an exception. */
			return GPR[rs] > immedate;
		}
		else if constexpr (instr == Instr::TLTI)
		{
			/* Trap If Less Than Immediate;
			   Compares the contents of register rs with 16-bit sign-extended immediate as a
			   signed integer. If rs contents are less than the immediate, generates an exception. */
			return s64(GPR[rs]) < immedate;
		}
		else if constexpr (instr == Instr::TLTIU)
		{
			/* Trap If Less Than Immediate Unsigned;
			   Compares the contents of register rs with 16-bit zero-extended immediate as an
			   unsigned integer. If rs contents are less than the immediate, generates an exception. */
			return GPR[rs] < immedate;
		}
		else if constexpr (instr == Instr::TEQI)
		{
			/* Trap If Equal Immediate;
			   Generates an exception if the contents of register rs are equal to immediate. */
			return s64(GPR[rs]) == immedate; /* TODO: should we really cast to s64? */
		}
		else if constexpr (instr == Instr::TNEI)
		{
			/* Trap If Not Equal Immediate;
			   Generates an exception if the contents of register rs are not equal to immediate. */
			return s64(GPR[rs]) != immedate;
		}
		else
		{
			static_assert(false, "\"Trap_Immediate\" template function called, but no matching trap instruction was found.");
		}
	}();

	if (trap_cond)
		TrapException();
}


void MIPS4300i::MFHI(const u32 instr_code)
{
	/* Move From HI;
	   Transfers the contents of special register HI to register rd. */
	const u8 rd = instr_code >> 11 & 0x1F;
	GPR[rd] = HI;
}


void MIPS4300i::MFLO(const u32 instr_code)
{
	/* Move From LO;
	   Transfers the contents of special register LO to register rd. */
	const u8 rd = instr_code >> 11 & 0x1F;
	GPR[rd] = LO;
}


void MIPS4300i::MTHI(const u32 instr_code)
{
	/* Move To HI;
	   Transfers the contents of register rs to special register HI. */
	const u8 rs = instr_code >> 21 & 0x1F;
	HI = GPR[rs];
}


void MIPS4300i::MTLO(const u32 instr_code)
{
	/* Move To LO;
	   Transfers the contents of register rs to special register LO. */
	const u8 rs = instr_code >> 21 & 0x1F;
	LO = GPR[rs];
}


void MIPS4300i::SYNC(const u32 instr_code)
{
	/* Synchronize;
	   Completes the load/store instruction currently in the pipeline before the new
	   load/store instruction is executed. */

	/* TODO */
}


void MIPS4300i::SYSCALL(const u32 instr_code)
{
	/* System Call;
	   Generates a system call exception and transfers control to the exception processing program. */

	/* TODO */
}


void MIPS4300i::BREAK(const u32 instr_code)
{
	/* Breakpoint;
	   Generates a breakpoint exception and transfers control to the exception processing program. */

	/* TODO */
}

void MIPS4300i::CACHE(const u32 instr_code)
{
}

void MIPS4300i::ERET(const u32 instr_code)
{
}
