module VR4300:CPU;

import :Exceptions;
import :MMU;
import :Operation;
import :Registers;

namespace VR4300 /* TODO check for intsructions that cause exceptions when in 32-bit mode (fig 16-1 in VR4300) */
{
	template<CPU_Instruction instr>
	void Load(const u32 instr_code)
	{
		using enum CPU_Instruction;

		const s16 offset = instr_code & 0xFFFF;
		const u8 rt = instr_code >> 16 & 0x1F;
		const u8 base = instr_code >> 21 & 0x1F;
		const u64 address = GPR[base] + offset;

		const auto result = [&] {
			/* For all instructions:
					   Generates an address by adding a sign-extended offset to the contents of register base. */
			if constexpr (instr == LB)
			{
				/* Load Byte;
				   Sign-extends the contents of a byte specified by the address and loads the result to register rt. */
				return cpu_read_mem<s8>(address);
			}
			else if constexpr (instr == LBU)
			{
				/* Load Byte Unsigned;
				   Zero-extends the contents of a byte specified by the address and loads the result to register rt. */
				return cpu_read_mem<u8>(address);
			}
			else if constexpr (instr == LH)
			{
				/* Load halfword;
				   Sign-extends the contents of a halfword specified by the address and loads the result to register rt. */
				return cpu_read_mem<s16>(address);
			}
			else if constexpr (instr == LHU)
			{
				/* Load Halfword Unsigned;
				   Zero-extends the contents of a halfword specified by the address and loads the result to register rt. */
				return cpu_read_mem<u16>(address);
			}
			else if constexpr (instr == LW)
			{
				/* Load Word;
				   Sign-extends the contents of a word specified by the address and loads the result to register rt. */
				return cpu_read_mem<s32>(address);
			}
			else if constexpr (instr == LWU)
			{
				/* Load Word Unsigned;
				   Zero-extends the contents of a word specified by the address and loads the result to register rt. */
				return cpu_read_mem<u32>(address);
			}
			else if constexpr (instr == LWL)
			{
				/* Load Word Left;
				   Shifts a word specified by the address to the left, so that a byte specified by
				   the address is at the leftmost position of the word. Sign-extends (in the 64-
				   bit mode), merges the result of the shift and the contents of register rt, and
				   loads the result to register rt. */
				return cpu_read_mem<u32, MemoryAccessAlignment::Unaligned>(address);
			}
			else if constexpr (instr == LWR)
			{
				/* Load Word Right;
				   Shifts a word specified by the address to the right, so that a byte specified by
				   the address is at the rightmost position of the word. Sign-extends (in the 64-
				   bit mode), merges the result of the shift and the contents of register rt, and
				   loads the result to register rt. */
				return cpu_read_mem<u32, MemoryAccessAlignment::Unaligned>(address);
			}
			else if constexpr (instr == LD)
			{
				/* Load Doubleword;
				   Loads the contents of a word specified by the address to register rt. */
				return cpu_read_mem<u64>(address);
			}
			else if constexpr (instr == LDL)
			{
				/* Load Doubleword Left;
				   Shifts the doubleword specified by the address to the left so that the byte
				   specified by the address is at the leftmost position of the doubleword.
				   Merges the result of the shift and the contents of register rt, and loads the
				   result to register rt. */
				return cpu_read_mem<u64, MemoryAccessAlignment::Unaligned>(address);
			}
			else if constexpr (instr == LDR)
			{
				/* Load Doubleword Right;
				   Shifts the doubleword specified by the address to the right so that the byte
				   specified by the address is at the rightmost position of the doubleword.
				   Merges the result of the shift and the contents of register rt, and loads the
				   result to register rt. */
				return cpu_read_mem<u64, MemoryAccessAlignment::Unaligned>(address);
			}
			else if constexpr (instr == LL)
			{
				/* Load Linked;
				   Loads the contents of the word specified by the address to register rt and sets the LL bit to 1. */
				return cpu_read_mem<s32>(address);
				/* TODO the specified physical address of the memory is stored to the LLAddr register */
			}
			else if constexpr (instr == LLD)
			{
				/* Load Linked Doubleword;
				   Loads the contents of the doubleword specified by the address to register rt and sets the LL bit to 1. */
				return cpu_read_mem<u64>(address);
			}
			else
			{
				static_assert(false, "\"Load\" template function called, but no matching load instruction was found.");
			}
		}();

		if (exception_has_occurred)
			return;

		GPR.Set(rt, result);

		if constexpr (instr == LL || instr == LLD)
			LL_bit = 1;
	}


	template<CPU_Instruction instr>
	void Store(const u32 instr_code)
	{
		using enum CPU_Instruction;

		const s16 offset = instr_code & 0xFFFF;
		const u8 rt = instr_code >> 16 & 0x1F;
		const u8 base = instr_code >> 21 & 0x1F;
		const u64 address = GPR[base] + offset;

		/* For all instructions:
		   Generates an address by adding a sign-extended offset to the contents of register base. */
		if constexpr (instr == SB)
		{
			/* Store Byte;
			   Stores the contents of the low-order byte of register rt to the memory specified by the address. */
			cpu_write_mem<u8>(address, u8(GPR[rt]));
		}
		else if constexpr (instr == SH)
		{
			/* Store Halfword;
			   Stores the contents of the low-order halfword of register rt to the memory specified by the address. */
			cpu_write_mem<u16>(address, u16(GPR[rt]));
		}
		else if constexpr (instr == SW)
		{
			/* Store Word;
			   Stores the contents of the low-order word of register rt to the memory specified by the address. */
			cpu_write_mem<u32>(address, u32(GPR[rt]));
		}
		else if constexpr (instr == SWL)
		{
			/* Store Word Left;
			   Shifts the contents of register rt to the right so that the leftmost byte of the
			   word is at the position of the byte specified by the address. Stores the result
			   of the shift to the lower portion of the word in memory. */
			cpu_write_mem<u32, MemoryAccessAlignment::Unaligned>(address, u32(GPR[rt])); /* TODO write function should handle this? */
		}
		else if constexpr (instr == SWR)
		{
			/* Store Word Right;
			   Shifts the contents of register rt to the left so that the rightmost byte of the
			   word is at the position of the byte specified by the address. Stores the result
			   of the shift to the higher portion of the word in memory. */
			cpu_write_mem<u32, MemoryAccessAlignment::Unaligned>(address, u32(GPR[rt]));
		}
		else if constexpr (instr == SD)
		{
			/* Store Doublword;
			   Stores the contents of register rt to the memory specified by the address. */
			cpu_write_mem<u64>(address, GPR[rt]);
		}
		else if constexpr (instr == SDL)
		{
			/* Store Doubleword Left;
			   Shifts the contents of register rt to the right so that the leftmost byte of a
			   doubleword is at the position of the byte specified by the address. Stores the
			   result of the shift to the lower portion of the doubleword in memory. */
			cpu_write_mem<u64>(address, GPR[rt]);
		}
		else if constexpr (instr == SDR)
		{
			/* Store Doubleword Right;
			   Shifts the contents of register rt to the left so that the rightmost byte of a
			   doubleword is at the position of the byte specified by the address. Stores the
			   result of the shift to the higher portion of the doubleword in memory. */
			cpu_write_mem<u64, MemoryAccessAlignment::Unaligned>(address, GPR[rt]);
		}
		else if constexpr (instr == SC)
		{
			/* Store Conditional;
			   If the LL bit is 1, stores the contents of the low-order word of register rt to
			   the memory specified by the address, and sets register rt to 1.
			   If the LL bit is 0, does not store the contents of the word, and clears register
			   rt to 0. */
			if (LL_bit == 1)
			{
				cpu_write_mem<u32>(address, u32(GPR[rt]));
				GPR.Set(rt, 1);
			}
			else
			{
				GPR.Set(rt, 0);
			}
		}
		else if constexpr (instr == SCD)
		{
			/* Store Conditional Doubleword;
			   If the LL bit is 1, stores the contents of register rt to the memory specified by
			   the address, and sets register rt to 1.
			   If the LL bit is 0, does not store the contents of the register, and clears register
			   rt to 0. */
			if (LL_bit == 1)
			{
				cpu_write_mem<u64>(address, GPR[rt]);
				GPR.Set(rt, 1);
			}
			else
			{
				GPR.Set(rt, 0);
			}
		}
		else
		{
			static_assert(false, "\"Store\" template function called, but no matching store instruction was found.");
		}
	}


	template<CPU_Instruction instr>
	void ALU_Immediate(const u32 instr_code)
	{
		using enum CPU_Instruction;

		const u8 rt = instr_code >> 16 & 0x1F;
		const u8 rs = instr_code >> 21 & 0x1F;

		const auto immediate = [&] {
			if constexpr (instr == ADDI || instr == ADDIU || instr == SLTI || instr == SLTIU || instr == DADDI || instr == DADDIU)
				return s16(instr_code & 0xFFFF);
			else
				return u16(instr_code & 0xFFFF);
		}();

		if constexpr (instr == ADDI)
		{
			/* Add Immediate;
			   Sign-extends the 16-bit immediate and adds it to register rs. Stores the
			   32-bit result to register rt (sign-extends the result in the 64-bit mode).
			   Generates an exception if a 2's complement integer overflow occurs. */
			const s32 sum = s32(GPR[rs] + immediate);
			const bool overflow = (GPR[rs] ^ sum) & (immediate ^ sum) & 0x80000000;
			if (overflow)
				IntegerOverflowException();
			else
				GPR.Set(rt, sum);
		}
		else if constexpr (instr == ADDIU)
		{
			/* Add Immediate Unsigned;
			   Sign-extends the 16-bit immediate and adds it to register rs. Stores the 32-bit
			   result to register rt (sign-extends the result in the 64-bit mode). Does not
			   generate an exception even if an integer overflow occurs. */
			const s32 sum = s32(GPR[rs] + immediate);
			GPR.Set(rt, sum);
		}
		else if constexpr (instr == SLTI)
		{
			/* Set On Less Than Immediate;
			   Sign-extends the 16-bit immediate and compares it with register rs as a
			   signed integer. If rs is less than the immediate, stores 1 to register rt;
			   otherwise, stores 0 to register rt. */
			GPR.Set(rt, s64(GPR[rs]) < immediate); /* TODO s32 in 32-bit mode */
		}
		else if constexpr (instr == SLTIU)
		{
			/* Set On Less Than Immediate Unsigned;
			   Sign-extends the 16-bit immediate and compares it with register rs as an
			   unsigned integer. If rs is less than the immediate, stores 1 to register rt;
			   otherwise, stores 0 to register rt. */
			   /* TODO */
			GPR.Set(rt, GPR[rs] < immediate);
		}
		else if constexpr (instr == ANDI)
		{
			/* And Immediate;
			   Zero-extends the 16-bit immediate, ANDs it with register rs, and stores the
			   result to register rt. */
			GPR.Set(rt, GPR[rs] & immediate);
		}
		else if constexpr (instr == ORI)
		{
			/* Or Immediate;
			   Zero-extends the 16-bit immediate, ORs it with register rs, and stores the
			   result to register rt. */
			GPR.Set(rt, GPR[rs] | immediate);
		}
		else if constexpr (instr == XORI)
		{
			/* Exclusive Or Immediate;
			   Zero-extends the 16-bit immediate, exclusive-ORs it with register rs, and
			   stores the result to register rt. */
			GPR.Set(rt, GPR[rs] ^ immediate);
		}
		else if constexpr (instr == LUI)
		{
			/* Load Upper Immediate;
			   Shifts the 16-bit immediate 16 bits to the left, and clears the low-order 16 bits
			   of the word to 0.
			   Stores the result to register rt (by sign-extending the result in the 64-bit mode). */
			const s32 result = immediate << 16;
			GPR.Set(rt, result);
		}
		else if constexpr (instr == DADDI)
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
				GPR.Set(rt, sum);
		}
		else if constexpr (instr == DADDIU)
		{
			/* Doubleword Add Immediate Unsigned;
			   Sign-extends the 16-bit immediate to 64 bits, and adds it to register rs. Stores
			   the 64-bit result to register rt. Does not generate an exception even if an
			   integer overflow occurs. */
			const s64 sum = (s64)GPR[rs] + immediate;
			GPR.Set(rt, sum);
		}
		else
		{
			static_assert(false, "\"ALU_Immediate\" template function called, but no matching ALU immediate instruction was found.");
		}
	}

	template<CPU_Instruction instr>
	void ALU_ThreeOperand(const u32 instr_code)
	{
		using enum CPU_Instruction;

		const u8 rd = instr_code >> 11 & 0x1F;
		const u8 rt = instr_code >> 16 & 0x1F;
		const u8 rs = instr_code >> 21 & 0x1F;

		if constexpr (instr == ADD)
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
				GPR.Set(rd, sum);
		}
		else if constexpr (instr == ADDU)
		{
			/* Add Unsigned;
			   Adds the contents of register rs and rt, and stores (sign-extends in the 64-bit mode)
			   the 32-bit result to register rd. Does not generate an exception even if an integer overflow occurs. */
			const s32 sum = s32(GPR[rs]) + s32(GPR[rt]);
			GPR.Set(rd, sum);
		}
		else if constexpr (instr == SUB)
		{
			/* Subtract;
			   Subtracts the contents of register rs from register rt, and stores (sign-extends
			   in the 64-bit mode) the result to register rd. Generates an exception if an integer overflow occurs. */
			const s32 sum = s32(GPR[rs]) - s32(GPR[rt]);
			const bool overflow = (GPR[rs] ^ sum) & (GPR[rt] ^ sum) & 0x80000000;
			if (overflow)
				IntegerOverflowException();
			else
				GPR.Set(rd, sum);
		}
		else if constexpr (instr == SUBU)
		{
			/* Subtract Unsigned;
			   Subtracts the contents of register rt from register rs, and stores (sign-extends
			   in the 64-bit mode) the 32-bit result to register rd.
			   Does not generate an exception even if an integer overflow occurs.*/
			const s32 sum = s32(GPR[rs]) - s32(GPR[rt]);
			GPR.Set(rd, sum);
		}
		else if constexpr (instr == SLT)
		{
			/* Set On Less Than;
			   Compares the contents of registers rs and rt as signed integers.
			   If the contents of register rs are less than those of rt, stores 1 to register rd;
			   otherwise, stores 0 to rd. */
			GPR.Set(rd, s64(GPR[rs]) < s64(GPR[rt])); /* todo: is it 32 bit comparison or not? */
		}
		else if constexpr (instr == SLTU)
		{
			/* Set On Less Than Unsigned;
			   Compares the contents of registers rs and rt as unsigned integers.
			   If the contents of register rs are less than those of rt, stores 1 to register rd;
			   otherwise, stores 0 to rd. */
			GPR.Set(rd, GPR[rs] < GPR[rt]);
		}
		else if constexpr (instr == AND)
		{
			/* And;
			   ANDs the contents of registers rs and rt in bit units, and stores the result to register rd. */
			GPR.Set(rd, GPR[rs] & GPR[rt]);
		}
		else if constexpr (instr == OR)
		{
			/* Or;
			   ORs the contents of registers rs and rt in bit units, and stores the result to register rd. */
			GPR.Set(rd, GPR[rs] | GPR[rt]);
		}
		else if constexpr (instr == XOR)
		{
			/* Exclusive Or;
			   Exclusive-ORs the contents of registers rs and rt in bit units, and stores the result to register rd. */
			GPR.Set(rd, GPR[rs] ^ GPR[rt]);
		}
		else if constexpr (instr == NOR)
		{
			/* Nor;
			   NORs the contents of registers rs and rt in bit units, and stores the result to register rd. */
			GPR.Set(rd, ~(GPR[rs] | GPR[rt]));
		}
		else if constexpr (instr == DADD)
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
				GPR.Set(rd, sum);
		}
		else if constexpr (instr == DADDU)
		{
			/* Doubleword Add Unsigned;
			   Adds the contents of registers rs and rt, and stores the 64-bit result to register rd.
			   Does not generate an exception even if an integer overflow occurs. */
			const u64 sum = GPR[rs] + GPR[rt];
			GPR.Set(rd, sum);
		}
		else if constexpr (instr == DSUB)
		{
			/* Doubleword Subtract;
			   Subtracts the contents of register rt from register rs, and stores the 64-bit
			   result to register rd. Generates an exception if an integer overflow occurs. */
			const u64 sum = GPR[rs] - GPR[rt];
			const bool overflow = (GPR[rs] ^ sum) & (GPR[rt] ^ sum) & 0x80000000'00000000;
			if (overflow)
				IntegerOverflowException();
			else
				GPR.Set(rd, sum);
		}
		else if constexpr (instr == DSUBU)
		{
			/* Doubleword Subtract Unsigned;
			   Subtracts the contents of register rt from register rs, and stores the 64-bit result to register rd.
			   Does not generate an exception even if an integer overflow occurs. */
			const u64 sum = GPR[rs] - GPR[rt];
			GPR.Set(rd, sum);
		}
		else
		{
			static_assert(false, "\"ALU_ThreeOperand\" template function called, but no matching ALU three-operand instruction was found.");
		}
	}


	template<CPU_Instruction instr>
	void ALU_Shift(const u32 instr_code)
	{
		using enum CPU_Instruction;

		const u8 rd = instr_code >> 11 & 0x1F;
		const u8 rt = instr_code >> 16 & 0x1F;

		const u8 rs = [&] {
			if constexpr (instr == SLLV || instr == SRLV || instr == SRAV || instr == DSLLV || instr == DSRLV || instr == DSRAV)
				return instr_code >> 21 & 0x1F;
			else return 0;
		}();

		const u8 sa = [&] {
			if constexpr (instr == SLL || instr == SRL || instr == SRA || instr == DSLL || instr == DSRL || instr == DSRA || instr == DSLL32 || instr == DSRL32)
				return instr_code & 0x1F;
			else return 0;
		}();

		const u64 result = static_cast<u64>( [&] {
			if constexpr (instr == SLL)
			{
				/* Shift Left Logical;
				   Shifts the contents of register rt sa bits to the left, and inserts 0 to the low-order bits.
				   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
				return s32(GPR[rt] << sa);
			}
			else if constexpr (instr == SRL)
			{
				/* Shift Right Logical;
				   Shifts the contents of register rt sa bits to the right, and inserts 0 to the high-order bits.
				   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
				return s32(GPR[rt] >> sa);
			}
			else if constexpr (instr == SRA)
			{
				/* Shift Right Arithmetic;
				   Shifts the contents of register rt sa bits to the right, and sign-extends the high-order bits.
				   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
				return s32(GPR[rt]) >> sa;
			}
			else if constexpr (instr == SLLV)
			{
				/* Shift Left Logical Variable;
				   Shifts the contents of register rt to the left and inserts 0 to the low-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 5 bits of register rs.
				   Sign-extends (in the 64-bit mode) the result and stores it to register rd. */
				return s32(GPR[rt]) << (GPR[rs] & 0x1F);
			}
			else if constexpr (instr == SRLV)
			{
				/* Shift Right Logical Variable;
				   Shifts the contents of register rt to the right, and inserts 0 to the high-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 5 bits of register rs.
				   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
				return s32(GPR[rt] >> (GPR[rs] & 0x1F));
			}
			else if constexpr (instr == SRAV)
			{
				/* Shift Right Arithmetic Variable;
				   Shifts the contents of register rt to the right and sign-extends the high-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 5 bits of register rs.
				   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
				return s32(GPR[rt]) >> (GPR[rs] & 0x1F);
			}
			else if constexpr (instr == DSLL)
			{
				/* Doubleword Shift Left Logical;
				   Shifts the contents of register rt sa bits to the left, and inserts 0 to the low-order bits.
				   Stores the 64-bit result to register rd. */
				return GPR[rt] << sa;
			}
			else if constexpr (instr == DSRL)
			{
				/* Doubleword Shift Right Logical;
				   Shifts the contents of register rt sa bits to the right, and inserts 0 to the high-order bits.
				   Stores the 64-bit result to register rd. */
				return GPR[rt] >> sa;
			}
			else if constexpr (instr == DSRA)
			{
				/* Doubleword Shift Right Arithmetic;
				   Shifts the contents of register rt sa bits to the right, and sign-extends the high-order bits.
				   Stores the 64-bit result to register rd. */
				return s64(GPR[rt]) >> sa;
			}
			else if constexpr (instr == DSLLV)
			{
				/* Doubleword Shift Left Logical Variable;
				   Shifts the contents of register rt to the left, and inserts 0 to the low-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 6 bits of register rs.
				   Stores the 64-bit result and stores it to register rd. */
				return GPR[rt] << (GPR[rs] & 0x3F);
			}
			else if constexpr (instr == DSRLV)
			{
				/* Doubleword Shift Right Logical Variable;
				   Shifts the contents of register rt to the right, and inserts 0 to the higher bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 6 bits of register rs.
				   Sign-extends the 64-bit result and stores it to register rd. */
				return GPR[rt] >> (GPR[rs] & 0x3F); /* TODO sign-extends the 64-bit result? */
			}
			else if constexpr (instr == DSRAV)
			{
				/* Doubleword Shift Right Arithmetic Variable;
				   Shifts the contents of register rt to the right, and sign-extends the high-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 6 bits of register rs.
				   Sign-extends the 64-bit result and stores it to register rd. */
				return s64(GPR[rt]) >> (GPR[rs] & 0x3F);
			}
			else if constexpr (instr == DSLL32)
			{
				/* Doubleword Shift Left Logical + 32;
				   Shifts the contents of register rt 32+sa bits to the left, and inserts 0 to the low-order bits.
				   Stores the 64-bit result to register rd. */
				return GPR[rt] << (sa + 32);
			}
			else if constexpr (instr == DSRL32)
			{
				/* Doubleword Shift Right Logical + 32;
				   Shifts the contents of register rt 32+sa bits to the right, and inserts 0 to the high-order bits.
				   Stores the 64-bit result to register rd. */
				return GPR[rt] >> (sa + 32);
			}
			else if constexpr (instr == DSRA32)
			{
				/* Doubleword Shift Right Arithmetic + 32;
				   Shifts the contents of register rt 32+sa bits to the right, and sign-extends the high-order bits.
				   Stores the 64-bit result to register rd.*/
				return s64(GPR[rt]) >> (sa + 32);
			}
			else
			{
				static_assert(false, "\"ALU_Shift\" template function called, but no matching ALU shift instruction was found.");
			}
		}());

		GPR.Set(rd, result);
	}


	template<CPU_Instruction instr>
	void ALU_MulDiv(const u32 instr_code)
	{
		using enum CPU_Instruction;

		const u8 rt = instr_code >> 16 & 0x1F;
		const u8 rs = instr_code >> 21 & 0x1F;

		if constexpr (instr == MULT)
		{
			/* Multiply;
			   Multiplies the contents of register rs by the contents of register rt as a 32-bit
			   signed integer. Sign-extends (in the 64-bit mode) and stores the 64-bit result
			   to special registers HI and LO. */
			const s64 result = s64(GPR[rs] & 0xFFFFFFFF) * s64(GPR[rt] & 0xFFFFFFFF);
			LO = s32(result & 0xFFFFFFFF);
			HI = result >> 32;
		}
		else if constexpr (instr == MULTU)
		{
			/* Multiply Unsigned;
			   Multiplies the contents of register rs by the contents of register rt as a 32-bit
			   unsigned integer. Sign-extends (in the 64-bit mode) and stores the 64-bit
			   result to special registers HI and LO. */
			const u64 result = (GPR[rs] & 0xFFFFFFFF) * (GPR[rt] & 0xFFFFFFFF);
			LO = s32(result & 0xFFFFFFFF);
			HI = s32(result >> 32);
		}
		else if constexpr (instr == DIV)
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
		else if constexpr (instr == DIVU)
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
		else if constexpr (instr == DMULT)
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
			const __int128 product = __int128(GPR[rs]) * __int128(GPR[rt]);
			LO = product & 0xFFFFFFFF'FFFFFFFF;
			HI = product >> 64;
#else
			static_assert(false);
#endif
		}
		else if constexpr (instr == DMULTU)
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
			const unsigned __int128 product = unsigned __int128(GPR[rs]) * unsigned __int128(GPR[rt]);
			LO = product & 0xFFFFFFFF'FFFFFFFF;
			HI = product >> 64;
#else
			static_assert(false);
#endif
		}
		else if constexpr (instr == DDIV)
		{
			/* Doubleword Divide;
			   Divides the contents of register rs by the contents of register rt.
			   The operand is treated as a signed integer.
			   Stores the 64-bit quotient to special register LO, and the 64-bit remainder to special register HI. */
			const s64 quotient = (s64)GPR[rs] / (s64)GPR[rt];
			const s64 remainder = (s64)GPR[rs] % (s64)GPR[rt];
			LO = quotient;
			HI = remainder;
		}
		else if constexpr (instr == DDIVU)
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

		p_cycle_counter += [&] {
			     if constexpr (instr == MULT  || instr == MULTU)  return 5;
			else if constexpr (instr == DMULT || instr == DMULTU) return 8;
			else if constexpr (instr == DIV   || instr == DIVU)   return 37;
			else if constexpr (instr == DDIV  || instr == DDIVU)  return 69;
			else static_assert(false, "\"ALU_MulDiv\" template function called, but no matching ALU mul/div instruction was found.");
		}();
	}


	template<CPU_Instruction instr>
	void Jump(const u32 instr_code)
	{
		using enum CPU_Instruction;

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
			if constexpr (instr == J || instr == JAL)
			{
				return PC & 0xFFFF'FFFF'F000'0000 | u64(instr_code & 0x3FFFFFF) << 2;
			}
			else if constexpr (instr == JR || instr == JALR)
			{
				const u8 rs = instr_code >> 21 & 0x1F;
				return GPR[rs];
			}
			else
			{
				static_assert(false, "\"Jump\" template function called, but no matching jump instruction was found.");
			}
		}();

		PrepareJump(target);

		if constexpr (instr == JAL)
		{
			GPR.Set(31, PC + 4);
		}
		else if constexpr (instr == JALR)
		{
			const u8 rd = instr_code >> 11 & 0x1F;
			GPR.Set(rd, PC + 4);
		}
	}


	template<CPU_Instruction instr>
	void Branch(const u32 instr_code)
	{
		using enum CPU_Instruction;

		const u8 rs = instr_code >> 21 & 0x1F;

		const u8 rt = [&] {
			if constexpr (instr == BEQ || instr == BNE || instr == BEQL || instr == BNEL)
				return instr_code >> 16 & 0x1F;
			else return 0; /* 'rt' is not needed for the other instructions; this will be optimized away. */
		}();

		/* For all instructions: branch to the branch address if the condition is met, with a delay of one instruction.
		   For "likely" instructions: if the branch condition is not satisfied, the instruction in the branch delay slot is discarded.
		   For "link" instructions: stores the address of the instruction following the delay slot to register r31 (link register). */

		if constexpr (instr == BLTZAL || instr == BGEZAL || instr == BLTZALL || instr == BGEZALL)
		{
			GPR.Set(31, PC + 4);
		}

		const bool branch_cond = [&] {
			if constexpr (instr == BEQ || instr == BEQL) /* Branch On Equal (Likely) */
				return GPR[rs] == GPR[rt];
			else if constexpr (instr == BNE || instr == BNEL) /* Branch On Not Equal (Likely) */
				return GPR[rs] != GPR[rt];
			else if constexpr (instr == BLEZ || instr == BLEZL) /* Branch On Less Than Or Equal To Zero (Likely) */
				return s64(GPR[rs]) <= 0;
			else if constexpr (instr == BGTZ || instr == BGTZL) /* Branch On Greater Than Zero (Likely) */
				return s64(GPR[rs]) > 0;
			else if constexpr (instr == BLTZ || instr == BLTZL) /* Branch On Less Than Zero (Likely) */
				return s64(GPR[rs]) < 0;
			else if constexpr (instr == BGEZ || instr == BGEZL) /* Branch On Greater Than or Equal To Zero (Likely) */
				return s64(GPR[rs]) >= 0;
			else if constexpr (instr == BLTZAL || instr == BLTZALL) /* Branch On Less Than Zero and Link (Likely) */
				return s64(GPR[rs]) < 0;
			else if constexpr (instr == BGEZAL || instr == BGEZALL) /* Branch On Greater Than Or Equal To Zero And Link (Likely) */
				return s64(GPR[rs]) >= 0;
			else
				static_assert(false, "\"Branch\" template function called, but no matching branch instruction was found.");
		}();

		if (branch_cond)
		{
			const s32 offset = s32(instr_code & 0xFFFF) << 2;
			PrepareJump(PC + offset);
		}
		else if constexpr (instr == BEQL || instr == BNEL || instr == BLEZL || instr == BGTZL ||
			instr == BEQL || instr == BLTZL || instr == BGEZL || instr == BLTZALL || instr == BGEZALL)
		{
			PC += 4; /* The instruction in the branch delay slot is discarded. */
		}
	}


	template<CPU_Instruction instr>
	void Trap_ThreeOperand(const u32 instr_code)
	{
		using enum CPU_Instruction;

		const u8 rt = instr_code >> 16 & 0x1F;
		const u8 rs = instr_code >> 21 & 0x1F;

		const bool trap_cond = [&] {
			if constexpr (instr == TGE)
			{
				/* Trap If Greater Than Or Equal;
				   Compares registers rs and rt as signed integers.
				   If register rs is greater than rt, generates an exception. */
				return s64(GPR[rs]) > s64(GPR[rt]);
			}
			else if constexpr (instr == TGEU)
			{
				/* Trap If Greater Than Or Equal Unsigned;
				   Compares registers rs and rt as unsigned integers.
				   If register rs is greater than rt, generates an exception. */
				return GPR[rs] > GPR[rt];
			}
			else if constexpr (instr == TLT)
			{
				/* Trap If Less Than;
				   Compares registers rs and rt as signed integers.
				   If register rs is less than rt, generates an exception. */
				return s64(GPR[rs]) < s64(GPR[rt]);
			}
			else if constexpr (instr == TLTU)
			{
				/* Trap If Less Than Unsigned;
				   Compares registers rs and rt as unsigned integers.
				   If register rs is less than rt, generates an exception. */
				return GPR[rs] < GPR[rt];
			}
			else if constexpr (instr == TEQ)
			{
				/* Trap If Equal;
				   Generates an exception if registers rs and rt are equal. */
				return GPR[rs] == GPR[rt];
			}
			else if constexpr (instr == TNE)
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


	template<CPU_Instruction instr>
	void Trap_Immediate(const u32 instr_code)
	{
		using enum CPU_Instruction;

		const s16 immedate = instr_code & 0xFFFF;
		const u8 rs = instr_code >> 21 & 0x1F;

		const bool trap_cond = [&] {
			if constexpr (instr == TGEI)
			{
				/* Trap If Greater Than Or Equal Immediate;
				   Compares the contents of register rs with 16-bit sign-extended immediate as a
				   signed integer. If rs contents are greater than the immediate, generates an exception. */
				return s64(GPR[rs]) > immedate;
			}
			else if constexpr (instr == TGEIU)
			{
				/* Trap If Greater Than Or Equal Immediate Unsigned;
				   Compares the contents of register rs with 16-bit zero-extended immediate as an
				   unsigned integer. If rs contents are greater than the immediate, generates an exception. */
				return GPR[rs] > immedate;
			}
			else if constexpr (instr == TLTI)
			{
				/* Trap If Less Than Immediate;
				   Compares the contents of register rs with 16-bit sign-extended immediate as a
				   signed integer. If rs contents are less than the immediate, generates an exception. */
				return s64(GPR[rs]) < immedate;
			}
			else if constexpr (instr == TLTIU)
			{
				/* Trap If Less Than Immediate Unsigned;
				   Compares the contents of register rs with 16-bit zero-extended immediate as an
				   unsigned integer. If rs contents are less than the immediate, generates an exception. */
				return GPR[rs] < immedate;
			}
			else if constexpr (instr == TEQI)
			{
				/* Trap If Equal Immediate;
				   Generates an exception if the contents of register rs are equal to immediate. */
				return s64(GPR[rs]) == immedate; /* TODO: should we really cast to s64? */
			}
			else if constexpr (instr == TNEI)
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

	void MFHI(const u32 instr_code)
	{
		/* Move From HI;
		   Transfers the contents of special register HI to register rd. */
		const u8 rd = instr_code >> 11 & 0x1F;
		GPR.Set(rd, HI);
	}


	void MFLO(const u32 instr_code)
	{
		/* Move From LO;
		   Transfers the contents of special register LO to register rd. */
		const u8 rd = instr_code >> 11 & 0x1F;
		GPR.Set(rd, LO);
	}


	void MTHI(const u32 instr_code)
	{
		/* Move To HI;
		   Transfers the contents of register rs to special register HI. */
		const u8 rs = instr_code >> 21 & 0x1F;
		HI = GPR[rs];
	}


	void MTLO(const u32 instr_code)
	{
		/* Move To LO;
		   Transfers the contents of register rs to special register LO. */
		const u8 rs = instr_code >> 21 & 0x1F;
		LO = GPR[rs];
	}


	void SYNC(const u32 instr_code)
	{
		/* Synchronize;
		   Completes the Load/store instruction currently in the pipeline before the new
		   Load/store instruction is executed. */

		   /* TODO */
		assert(false);
	}


	void SYSCALL(const u32 instr_code)
	{
		/* System Call;
		   Generates a system call exception and transfers control to the exception processing program. */

		   /* TODO */
		assert(false);
	}


	void BREAK(const u32 instr_code)
	{
		/* Breakpoint;
		   Generates a breakpoint exception and transfers control to the exception processing program. */

		   /* TODO */
		assert(false);
	}

	template void Load<CPU_Instruction::LB>(const u32 instr_code);
	template void Load<CPU_Instruction::LBU>(const u32 instr_code);
	template void Load<CPU_Instruction::LH>(const u32 instr_code);
	template void Load<CPU_Instruction::LHU>(const u32 instr_code);
	template void Load<CPU_Instruction::LW>(const u32 instr_code);
	template void Load<CPU_Instruction::LWU>(const u32 instr_code);
	template void Load<CPU_Instruction::LWL>(const u32 instr_code);
	template void Load<CPU_Instruction::LWR>(const u32 instr_code);
	template void Load<CPU_Instruction::LD>(const u32 instr_code);
	template void Load<CPU_Instruction::LDL>(const u32 instr_code);
	template void Load<CPU_Instruction::LDR>(const u32 instr_code);
	template void Load<CPU_Instruction::LL>(const u32 instr_code);
	template void Load<CPU_Instruction::LLD>(const u32 instr_code);

	template void Store<CPU_Instruction::SB>(const u32 instr_code);
	template void Store<CPU_Instruction::SH>(const u32 instr_code);
	template void Store<CPU_Instruction::SW>(const u32 instr_code);
	template void Store<CPU_Instruction::SWL>(const u32 instr_code);
	template void Store<CPU_Instruction::SWR>(const u32 instr_code);
	template void Store<CPU_Instruction::SC>(const u32 instr_code);
	template void Store<CPU_Instruction::SCD>(const u32 instr_code);
	template void Store<CPU_Instruction::SD>(const u32 instr_code);
	template void Store<CPU_Instruction::SDL>(const u32 instr_code);
	template void Store<CPU_Instruction::SDR>(const u32 instr_code);

	template void ALU_Immediate<CPU_Instruction::ADDI>(const u32 instr_code);
	template void ALU_Immediate<CPU_Instruction::ADDIU>(const u32 instr_code);
	template void ALU_Immediate<CPU_Instruction::SLTI>(const u32 instr_code);
	template void ALU_Immediate<CPU_Instruction::SLTIU>(const u32 instr_code);
	template void ALU_Immediate<CPU_Instruction::ANDI>(const u32 instr_code);
	template void ALU_Immediate<CPU_Instruction::ORI>(const u32 instr_code);
	template void ALU_Immediate<CPU_Instruction::XORI>(const u32 instr_code);
	template void ALU_Immediate<CPU_Instruction::LUI>(const u32 instr_code);
	template void ALU_Immediate<CPU_Instruction::DADDI>(const u32 instr_code);
	template void ALU_Immediate<CPU_Instruction::DADDIU>(const u32 instr_code);

	template void ALU_ThreeOperand<CPU_Instruction::ADD>(const u32 instr_code);
	template void ALU_ThreeOperand<CPU_Instruction::ADDU>(const u32 instr_code);
	template void ALU_ThreeOperand<CPU_Instruction::SUB>(const u32 instr_code);
	template void ALU_ThreeOperand<CPU_Instruction::SUBU>(const u32 instr_code);
	template void ALU_ThreeOperand<CPU_Instruction::SLT>(const u32 instr_code);
	template void ALU_ThreeOperand<CPU_Instruction::SLTU>(const u32 instr_code);
	template void ALU_ThreeOperand<CPU_Instruction::AND>(const u32 instr_code);
	template void ALU_ThreeOperand<CPU_Instruction::OR>(const u32 instr_code);
	template void ALU_ThreeOperand<CPU_Instruction::XOR>(const u32 instr_code);
	template void ALU_ThreeOperand<CPU_Instruction::NOR>(const u32 instr_code);
	template void ALU_ThreeOperand<CPU_Instruction::DADD>(const u32 instr_code);
	template void ALU_ThreeOperand<CPU_Instruction::DADDU>(const u32 instr_code);
	template void ALU_ThreeOperand<CPU_Instruction::DSUB>(const u32 instr_code);
	template void ALU_ThreeOperand<CPU_Instruction::DSUBU>(const u32 instr_code);

	template void ALU_Shift<CPU_Instruction::SLL>(const u32 instr_code);
	template void ALU_Shift<CPU_Instruction::SRL>(const u32 instr_code);
	template void ALU_Shift<CPU_Instruction::SRA>(const u32 instr_code);
	template void ALU_Shift<CPU_Instruction::SLLV>(const u32 instr_code);
	template void ALU_Shift<CPU_Instruction::SRLV>(const u32 instr_code);
	template void ALU_Shift<CPU_Instruction::SRAV>(const u32 instr_code);
	template void ALU_Shift<CPU_Instruction::DSLL>(const u32 instr_code);
	template void ALU_Shift<CPU_Instruction::DSRL>(const u32 instr_code);
	template void ALU_Shift<CPU_Instruction::DSRA>(const u32 instr_code);
	template void ALU_Shift<CPU_Instruction::DSLLV>(const u32 instr_code);
	template void ALU_Shift<CPU_Instruction::DSRLV>(const u32 instr_code);
	template void ALU_Shift<CPU_Instruction::DSRAV>(const u32 instr_code);
	template void ALU_Shift<CPU_Instruction::DSLL32>(const u32 instr_code);
	template void ALU_Shift<CPU_Instruction::DSRL32>(const u32 instr_code);
	template void ALU_Shift<CPU_Instruction::DSRA32>(const u32 instr_code);

	template void ALU_MulDiv<CPU_Instruction::MULT>(const u32 instr_code);
	template void ALU_MulDiv<CPU_Instruction::MULTU>(const u32 instr_code);
	template void ALU_MulDiv<CPU_Instruction::DIV>(const u32 instr_code);
	template void ALU_MulDiv<CPU_Instruction::DIVU>(const u32 instr_code);
	template void ALU_MulDiv<CPU_Instruction::DMULT>(const u32 instr_code);
	template void ALU_MulDiv<CPU_Instruction::DMULTU>(const u32 instr_code);
	template void ALU_MulDiv<CPU_Instruction::DDIV>(const u32 instr_code);
	template void ALU_MulDiv<CPU_Instruction::DDIVU>(const u32 instr_code);

	template void Jump<CPU_Instruction::J>(const u32 instr_code);
	template void Jump<CPU_Instruction::JAL>(const u32 instr_code);
	template void Jump<CPU_Instruction::JR>(const u32 instr_code);
	template void Jump<CPU_Instruction::JALR>(const u32 instr_code);

	template void Branch<CPU_Instruction::BEQ>(const u32 instr_code);
	template void Branch<CPU_Instruction::BNE>(const u32 instr_code);
	template void Branch<CPU_Instruction::BLEZ>(const u32 instr_code);
	template void Branch<CPU_Instruction::BGTZ>(const u32 instr_code);
	template void Branch<CPU_Instruction::BLTZ>(const u32 instr_code);
	template void Branch<CPU_Instruction::BGEZ>(const u32 instr_code);
	template void Branch<CPU_Instruction::BLTZAL>(const u32 instr_code);
	template void Branch<CPU_Instruction::BGEZAL>(const u32 instr_code);
	template void Branch<CPU_Instruction::BEQL>(const u32 instr_code);
	template void Branch<CPU_Instruction::BNEL>(const u32 instr_code);
	template void Branch<CPU_Instruction::BLEZL>(const u32 instr_code);
	template void Branch<CPU_Instruction::BGTZL>(const u32 instr_code);
	template void Branch<CPU_Instruction::BLTZL>(const u32 instr_code);
	template void Branch<CPU_Instruction::BGEZL>(const u32 instr_code);
	template void Branch<CPU_Instruction::BLTZALL>(const u32 instr_code);
	template void Branch<CPU_Instruction::BGEZALL>(const u32 instr_code);

	template void Trap_ThreeOperand<CPU_Instruction::TGE>(const u32 instr_code);
	template void Trap_ThreeOperand<CPU_Instruction::TGEU>(const u32 instr_code);
	template void Trap_ThreeOperand<CPU_Instruction::TLT>(const u32 instr_code);
	template void Trap_ThreeOperand<CPU_Instruction::TLTU>(const u32 instr_code);
	template void Trap_ThreeOperand<CPU_Instruction::TEQ>(const u32 instr_code);
	template void Trap_ThreeOperand<CPU_Instruction::TNE>(const u32 instr_code);

	template void Trap_Immediate<CPU_Instruction::TGEI>(const u32 instr_code);
	template void Trap_Immediate<CPU_Instruction::TGEIU>(const u32 instr_code);
	template void Trap_Immediate<CPU_Instruction::TLTI>(const u32 instr_code);
	template void Trap_Immediate<CPU_Instruction::TLTIU>(const u32 instr_code);
	template void Trap_Immediate<CPU_Instruction::TEQI>(const u32 instr_code);
	template void Trap_Immediate<CPU_Instruction::TNEI>(const u32 instr_code);
}