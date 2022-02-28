module VR4300:CPU;

import :Exceptions;
import :MMU;
import :Operation;
import :Registers;

import MemoryAccess;

namespace VR4300 /* TODO check for intsructions that cause exceptions when in 32-bit mode (fig 16-1 in VR4300) */
{
	template<CPU_Instruction instr>
	void Load(const u32 instr_code)
	{
		using enum CPU_Instruction;

		const s16 offset = instr_code & 0xFFFF;
		const u8 rt = instr_code >> 16 & 0x1F;
		const u8 base = instr_code >> 21 & 0x1F;
		const u64 address = gpr[base] + offset;

		auto result = [&] {
			/* For all instructions:
			   Generates an address by adding a sign-extended offset to the contents of register base. */
			if constexpr (instr == LB)
			{
				/* Load Byte;
				   Sign-extends the contents of a byte specified by the address and loads the result to register rt. */
				return ReadVirtual<s8>(address);
			}
			else if constexpr (instr == LBU)
			{
				/* Load Byte Unsigned;
				   Zero-extends the contents of a byte specified by the address and loads the result to register rt. */
				return ReadVirtual<u8>(address);
			}
			else if constexpr (instr == LH)
			{
				/* Load halfword;
				   Sign-extends the contents of a halfword specified by the address and loads the result to register rt. */
				return ReadVirtual<s16>(address);
			}
			else if constexpr (instr == LHU)
			{
				/* Load Halfword Unsigned;
				   Zero-extends the contents of a halfword specified by the address and loads the result to register rt. */
				return ReadVirtual<u16>(address);
			}
			else if constexpr (instr == LW)
			{
				/* Load Word;
				   Sign-extends the contents of a word specified by the address and loads the result to register rt. */
				return ReadVirtual<s32>(address);
			}
			else if constexpr (instr == LWU)
			{
				/* Load Word Unsigned;
				   Zero-extends the contents of a word specified by the address and loads the result to register rt. */
				return ReadVirtual<u32>(address);
			}
			else if constexpr (instr == LWL)
			{
				/* Load Word Left;
				   Shifts a word specified by the address to the left, so that a byte specified by
				   the address is at the leftmost position of the word. Sign-extends (in the 64-
				   bit mode), merges the result of the shift and the contents of register rt, and
				   loads the result to register rt. */
				return ReadVirtual<s32, MemoryAccess::Alignment::UnalignedLeft>(address);
			}
			else if constexpr (instr == LWR)
			{
				/* Load Word Right;
				   Shifts a word specified by the address to the right, so that a byte specified by
				   the address is at the rightmost position of the word. Sign-extends (in the 64-
				   bit mode), merges the result of the shift and the contents of register rt, and
				   loads the result to register rt. */
				return ReadVirtual<s32, MemoryAccess::Alignment::UnalignedRight>(address);
			}
			else if constexpr (instr == LD)
			{
				/* Load Doubleword;
				   Loads the contents of a word specified by the address to register rt. */
				return ReadVirtual<u64>(address);
			}
			else if constexpr (instr == LDL)
			{
				/* Load Doubleword Left;
				   Shifts the doubleword specified by the address to the left so that the byte
				   specified by the address is at the leftmost position of the doubleword.
				   Merges the result of the shift and the contents of register rt, and loads the
				   result to register rt. */
				return ReadVirtual<u64, MemoryAccess::Alignment::UnalignedLeft>(address);
			}
			else if constexpr (instr == LDR)
			{
				/* Load Doubleword Right;
				   Shifts the doubleword specified by the address to the right so that the byte
				   specified by the address is at the rightmost position of the doubleword.
				   Merges the result of the shift and the contents of register rt, and loads the
				   result to register rt. */
				return ReadVirtual<u64, MemoryAccess::Alignment::UnalignedRight>(address);
			}
			else if constexpr (instr == LL)
			{
				/* Load Linked;
				   Loads the contents of the word specified by the address to register rt and sets the LL bit to 1.
				   Additionally, the specified physical address of the memory is stored to the LLAddr register. */
				LL_bit = 1;
				store_physical_address_on_load = true;
				const s32 ret = ReadVirtual<s32>(address);
				store_physical_address_on_load = false;
				return ret;
			}
			else if constexpr (instr == LLD)
			{
				/* Load Linked Doubleword;
				   Loads the contents of the doubleword specified by the address to register rt and sets the LL bit to 1.
				   Additionally, the specified physical address of the memory is stored to the LLAddr register. */
				LL_bit = 1;
				store_physical_address_on_load = true;
				const s64 ret = ReadVirtual<s64>(address);
				store_physical_address_on_load = false;
				return ret;
			}
			else
			{
				static_assert(instr != instr, "\"Load\" template function called, but no matching load instruction was found.");
			}
		}();

		if (exception_has_occurred)
			return;

		/* Unaligned memory access example: demonstrating that the below works.
		   Memory in big endian: 01234567 (bytes), we perform LWL at $1 and then LWR at $4.
		   The end result should be: reg = 1234
			LWL, little endian host
			N64 mem		host value
			0123		3210		after read
						0123		after swap (done in ReadVirtual)
						123X		after shift (<< 8); 8 == 8 * (addr & (sizeof word - 1)) == 8 * (1 & 3)

			LWR, little endian host
			N64 mem		host value
			4567		7654		after read
						4567		after swap
						XXX4		after shift (>> 24); 24 == 8 * (sizeof word - 1 - (addr & (sizeof word - 1))) == 8 * (3 - (4 & 3))

			LWL, big endian host
			N64 mem		host value
			0123		0123		after read
						123X		after shift (<< 8)

			LWR, big endian host
			N64 mem		host value
			4567		4567		after read
						XXX5		after shift (>> 24)
		*/
		/* For load right instructions, this represents the 'X's above, that is to be ANDed with GPR when combining it with the read value. */
		static constexpr std::array<u64, 8> right_load_mask = {
			0xFFFF'FFFF'FFFF'FF00,
			0xFFFF'FFFF'FFFF'0000,
			0xFFFF'FFFF'FF00'0000,
			0xFFFF'FFFF'0000'0000,
			0xFFFF'FF00'0000'0000,
			0xFFFF'0000'0000'0000,
			0xFF00'0000'0000'0000,
			0
		};
		if constexpr (instr == LWL)
		{
			const std::size_t bits_from_last_boundary = 8 * (address & 3);
			result <<= bits_from_last_boundary;
			const s32 untouched_gpr = s32(gpr.Get(rt) & ((1 << bits_from_last_boundary) - 1));
			gpr.Set(rt, result | untouched_gpr);
			/* To access data not aligned at a boundary, an additional 1P cycle is necessary as compared when accessing data aligned at a boundary. */
			if (bits_from_last_boundary > 0)
				AdvancePipeline<1>();
		}
		else if constexpr (instr == LDL)
		{
			const std::size_t bits_from_last_boundary = 8 * (address & 7);
			result <<= bits_from_last_boundary;
			const u64 untouched_gpr = gpr.Get(rt) & ((1ll << bits_from_last_boundary) - 1);
			gpr.Set(rt, result | untouched_gpr);

			if (bits_from_last_boundary > 0)
				AdvancePipeline<1>();
		}
		else if constexpr (instr == LWR)
		{
			const std::size_t bytes_from_last_boundary = address & 3;
			result >>= 8 * (3 - bytes_from_last_boundary);
			const s32 untouched_gpr = s32(gpr.Get(rt) & right_load_mask[bytes_from_last_boundary]);
			gpr.Set(rt, result | untouched_gpr);

			if (bytes_from_last_boundary > 0)
				AdvancePipeline<1>();
		}
		else if constexpr (instr == LDR)
		{
			const std::size_t bytes_from_last_boundary = address & 7;
			result >>= 8 * (7 - bytes_from_last_boundary);
			const u64 untouched_gpr = gpr.Get(rt) & right_load_mask[bytes_from_last_boundary];
			gpr.Set(rt, result | untouched_gpr);

			if (bytes_from_last_boundary > 0)
				AdvancePipeline<1>();
		}
		else /* Aligned read */
		{
			gpr.Set(rt, result);
		}

		AdvancePipeline<1>();
	}


	template<CPU_Instruction instr>
	void Store(const u32 instr_code)
	{
		using enum CPU_Instruction;

		const s16 offset = instr_code & 0xFFFF;
		const u8 rt = instr_code >> 16 & 0x1F;
		const u8 base = instr_code >> 21 & 0x1F;
		const u64 address = gpr[base] + offset;

		/* For all instructions:
		   Generates an address by adding a sign-extended offset to the contents of register base. */
		if constexpr (instr == SB)
		{
			/* Store Byte;
			   Stores the contents of the low-order byte of register rt to the memory specified by the address. */
			WriteVirtual<s8>(address, s8(gpr[rt]));
		}
		else if constexpr (instr == SH)
		{
			/* Store Halfword;
			   Stores the contents of the low-order halfword of register rt to the memory specified by the address. */
			WriteVirtual<s16>(address, s16(gpr[rt]));
		}
		else if constexpr (instr == SW)
		{
			/* Store Word;
			   Stores the contents of the low-order word of register rt to the memory specified by the address. */
			WriteVirtual<s32>(address, s32(gpr[rt]));
		}
		else if constexpr (instr == SWL)
		{
			/* Store Word Left;
			   Shifts the contents of register rt to the right so that the leftmost byte of the
			   word is at the position of the byte specified by the address. Stores the result
			   of the shift to the lower portion of the word in memory. */
			const s32 data_to_write = s32(gpr[rt] & ~((1 << (8 * (address & 3))) - 1));
			WriteVirtual<s32, MemoryAccess::Alignment::UnalignedLeft>(address, data_to_write);
		}
		else if constexpr (instr == SWR)
		{
			/* Store Word Right;
			   Shifts the contents of register rt to the left so that the rightmost byte of the
			   word is at the position of the byte specified by the address. Stores the result
			   of the shift to the higher portion of the word in memory. */
			const s32 data_to_write = s32(gpr[rt] << (8 * (3 - (address & 3))));
			WriteVirtual<s32, MemoryAccess::Alignment::UnalignedRight>(address, data_to_write);
		}
		else if constexpr (instr == SD)
		{
			/* Store Doublword;
			   Stores the contents of register rt to the memory specified by the address. */
			WriteVirtual<s64>(address, gpr[rt]);
		}
		else if constexpr (instr == SDL)
		{
			/* Store Doubleword Left;
			   Shifts the contents of register rt to the right so that the leftmost byte of a
			   doubleword is at the position of the byte specified by the address. Stores the
			   result of the shift to the lower portion of the doubleword in memory. */
			const s64 data_to_write = gpr[rt] & ~((1ll << (8 * (address & 7))) - 1);
			WriteVirtual<s64, MemoryAccess::Alignment::UnalignedLeft>(address, data_to_write);
		}
		else if constexpr (instr == SDR)
		{
			/* Store Doubleword Right;
			   Shifts the contents of register rt to the left so that the rightmost byte of a
			   doubleword is at the position of the byte specified by the address. Stores the
			   result of the shift to the higher portion of the doubleword in memory. */
			const s64 data_to_write = gpr[rt] << (8 * (7 - (address & 7)));
			WriteVirtual<s64, MemoryAccess::Alignment::UnalignedRight>(address, data_to_write);
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
				WriteVirtual<s32>(address, s32(gpr[rt]));
				gpr.Set(rt, 1);
			}
			else
			{
				gpr.Set(rt, 0);
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
				WriteVirtual<s64>(address, gpr[rt]);
				gpr.Set(rt, 1);
			}
			else
			{
				gpr.Set(rt, 0);
			}
		}
		else
		{
			static_assert(instr != instr, "\"Store\" template function called, but no matching store instruction was found.");
		}

		AdvancePipeline<1>();
	}


	template<CPU_Instruction instr>
	void ALU_Immediate(const u32 instr_code)
	{
		using enum CPU_Instruction;

		const u8 rt = instr_code >> 16 & 0x1F;
		const u8 rs = instr_code >> 21 & 0x1F;

		const auto immediate = [&] {
			if constexpr (instr == ANDI || instr == ORI || instr == XORI)
				return u16(instr_code & 0xFFFF);
			else
				return s16(instr_code & 0xFFFF);
		}();

		if constexpr (instr == ADDI)
		{
			/* Add Immediate;
			   Sign-extends the 16-bit immediate and adds it to register rs. Stores the
			   32-bit result to register rt (sign-extends the result in the 64-bit mode).
			   Generates an exception if a 2's complement integer overflow occurs.
			   In that case, the contents of destination register rt are not modified */
			const s32 sum = s32(gpr[rs] + immediate);
			const bool overflow = (gpr[rs] ^ sum) & (immediate ^ sum) & 0x8000'0000;
			if (overflow)
				IntegerOverflowException();
			else
				gpr.Set(rt, sum);
		}
		else if constexpr (instr == ADDIU)
		{
			/* Add Immediate Unsigned;
			   Sign-extends the 16-bit immediate and adds it to register rs. Stores the 32-bit
			   result to register rt (sign-extends the result in the 64-bit mode). Does not
			   generate an exception even if an integer overflow occurs. */
			const s32 sum = s32(gpr[rs] + immediate);
			gpr.Set(rt, sum);
		}
		else if constexpr (instr == SLTI)
		{
			/* Set On Less Than Immediate;
			   Sign-extends the 16-bit immediate and compares it with register rs as a
			   signed integer. If rs is less than the immediate, stores 1 to register rt;
			   otherwise, stores 0 to register rt. */
			gpr.Set(rt, gpr[rs] < immediate);
		}
		else if constexpr (instr == SLTIU)
		{
			/* Set On Less Than Immediate Unsigned;
			   Sign-extends the 16-bit immediate and compares it with register rs as an
			   unsigned integer. If rs is less than the immediate, stores 1 to register rt;
			   otherwise, stores 0 to register rt. */
			gpr.Set(rt, u64(gpr[rs]) < immediate);
		}
		else if constexpr (instr == ANDI)
		{
			/* And Immediate;
			   Zero-extends the 16-bit immediate, ANDs it with register rs, and stores the
			   result to register rt. */
			gpr.Set(rt, gpr[rs] & immediate);
		}
		else if constexpr (instr == ORI)
		{
			/* Or Immediate;
			   Zero-extends the 16-bit immediate, ORs it with register rs, and stores the
			   result to register rt. */
			gpr.Set(rt, gpr[rs] | immediate);
		}
		else if constexpr (instr == XORI)
		{
			/* Exclusive Or Immediate;
			   Zero-extends the 16-bit immediate, exclusive-ORs it with register rs, and
			   stores the result to register rt. */
			gpr.Set(rt, gpr[rs] ^ immediate);
		}
		else if constexpr (instr == LUI)
		{
			/* Load Upper Immediate;
			   Shifts the 16-bit immediate 16 bits to the left, and clears the low-order 16 bits
			   of the word to 0.
			   Stores the result to register rt (by sign-extending the result in the 64-bit mode). */
			const s32 result = immediate << 16;
			gpr.Set(rt, result);
		}
		else if constexpr (instr == DADDI)
		{
			/* Doubleword Add Immediate;
			   Sign-extends the 16-bit immediate to 64 bits, and adds it to register rs. Stores
			   the 64-bit result to register rt. Generates an exception if an integer overflow occurs. */

			   /* TODO This operation is only defined for the VR4300 operating in 64-bit mode and in 32-
	   bit Kernel mode. Execution of this instruction in 32-bit User or Supervisor mode
	   causes a reserved instruction exception*/
			const s64 sum = gpr[rs] + immediate;
			const bool overflow = (gpr[rs] ^ sum) & (immediate ^ sum) & 0x8000'0000'0000'0000;
			if (overflow)
				IntegerOverflowException();
			else
				gpr.Set(rt, sum);
		}
		else if constexpr (instr == DADDIU)
		{
			/* Doubleword Add Immediate Unsigned;
			   Sign-extends the 16-bit immediate to 64 bits, and adds it to register rs. Stores
			   the 64-bit result to register rt. Does not generate an exception even if an
			   integer overflow occurs. */
			const s64 sum = gpr[rs] + immediate;
			gpr.Set(rt, sum);
		}
		else
		{
			static_assert(instr != instr, "\"ALU_Immediate\" template function called, but no matching ALU immediate instruction was found.");
		}

		AdvancePipeline<1>();
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
			   the 32-bit result to register rd. Generates an exception if an integer overflow occurs.
			   In that case, the contents of the destination register rd are not modified.
			   In 64-bit mode, the operands must be sign-extended, 32-bit values. */
			const s32 sum = s32(gpr[rs] + gpr[rt]);
			const bool overflow = (gpr[rs] ^ sum) & (gpr[rt] ^ sum) & 0x8000'0000;
			if (overflow)
				IntegerOverflowException();
			else
				gpr.Set(rd, sum);
		}
		else if constexpr (instr == ADDU)
		{
			/* Add Unsigned;
			   Adds the contents of register rs and rt, and stores (sign-extends in the 64-bit mode)
			   the 32-bit result to register rd. Does not generate an exception even if an integer overflow occurs. */
			const s32 sum = s32(gpr[rs] + gpr[rt]);
			gpr.Set(rd, sum);
		}
		else if constexpr (instr == SUB)
		{
			/* Subtract;
			   Subtracts the contents of register rs from register rt, and stores (sign-extends
			   in the 64-bit mode) the result to register rd. Generates an exception if an integer overflow occurs. */
			const s32 sum = s32(gpr[rs] - gpr[rt]);
			const bool overflow = (gpr[rs] ^ sum) & (gpr[rt] ^ sum) & 0x8000'0000;
			if (overflow)
				IntegerOverflowException();
			else
				gpr.Set(rd, sum);
		}
		else if constexpr (instr == SUBU)
		{
			/* Subtract Unsigned;
			   Subtracts the contents of register rt from register rs, and stores (sign-extends
			   in the 64-bit mode) the 32-bit result to register rd.
			   Does not generate an exception even if an integer overflow occurs.*/
			const s32 sum = s32(gpr[rs] - gpr[rt]);
			gpr.Set(rd, sum);
		}
		else if constexpr (instr == SLT)
		{
			/* Set On Less Than;
			   Compares the contents of registers rs and rt as 64-bit signed integers.
			   If the contents of register rs are less than those of rt, stores 1 to register rd;
			   otherwise, stores 0 to rd. */
			gpr.Set(rd, gpr[rs] < gpr[rt]);
		}
		else if constexpr (instr == SLTU)
		{
			/* Set On Less Than Unsigned;
			   Compares the contents of registers rs and rt as 64-bit unsigned integers.
			   If the contents of register rs are less than those of rt, stores 1 to register rd;
			   otherwise, stores 0 to rd. */
			gpr.Set(rd, u64(gpr[rs]) < u64(gpr[rt]));
		}
		else if constexpr (instr == AND)
		{
			/* And;
			   ANDs the contents of registers rs and rt in bit units, and stores the result to register rd. */
			gpr.Set(rd, gpr[rs] & gpr[rt]);
		}
		else if constexpr (instr == OR)
		{
			/* Or;
			   ORs the contents of registers rs and rt in bit units, and stores the result to register rd. */
			gpr.Set(rd, gpr[rs] | gpr[rt]);
		}
		else if constexpr (instr == XOR)
		{
			/* Exclusive Or;
			   Exclusive-ORs the contents of registers rs and rt in bit units, and stores the result to register rd. */
			gpr.Set(rd, gpr[rs] ^ gpr[rt]);
		}
		else if constexpr (instr == NOR)
		{
			/* Nor;
			   NORs the contents of registers rs and rt in bit units, and stores the result to register rd. */
			gpr.Set(rd, ~(gpr[rs] | gpr[rt]));
		}
		else if constexpr (instr == DADD)
		{
			/* Doubleword Add;
			   Adds the contents of registers rs and rt, and stores the 64-bit result to register rd.
			   Generates an exception if an integer overflow occurs. */

			   /* TODO Execution of this instruction in 32-bit User or Supervisor mode
	   causes a reserved instruction exception.*/
			const s64 sum = gpr[rs] + gpr[rt];
			const bool overflow = (gpr[rs] ^ sum) & (gpr[rt] ^ sum) & 0x8000'0000'0000'0000;
			if (overflow)
				IntegerOverflowException();
			else
				gpr.Set(rd, sum);
		}
		else if constexpr (instr == DADDU)
		{
			/* Doubleword Add Unsigned;
			   Adds the contents of registers rs and rt, and stores the 64-bit result to register rd.
			   Does not generate an exception even if an integer overflow occurs. */
			const s64 sum = gpr[rs] + gpr[rt];
			gpr.Set(rd, sum);
		}
		else if constexpr (instr == DSUB)
		{
			/* Doubleword Subtract;
			   Subtracts the contents of register rt from register rs, and stores the 64-bit
			   result to register rd. Generates an exception if an integer overflow occurs. */
			const s64 sum = gpr[rs] - gpr[rt];
			const bool overflow = (gpr[rs] ^ sum) & (gpr[rt] ^ sum) & 0x8000'0000'0000'0000;
			if (overflow)
				IntegerOverflowException();
			else
				gpr.Set(rd, sum);
		}
		else if constexpr (instr == DSUBU)
		{
			/* Doubleword Subtract Unsigned;
			   Subtracts the contents of register rt from register rs, and stores the 64-bit result to register rd.
			   Does not generate an exception even if an integer overflow occurs. */
			const s64 sum = gpr[rs] - gpr[rt];
			gpr.Set(rd, sum);
		}
		else
		{
			static_assert(instr != instr, "\"ALU_ThreeOperand\" template function called, but no matching ALU three-operand instruction was found.");
		}

		AdvancePipeline<1>();
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
			if constexpr (instr == SLL || instr == SRL || instr == SRA || instr == DSLL ||
				          instr == DSRL || instr == DSRA || instr == DSRA32 || instr == DSLL32 || instr == DSRL32)
				return instr_code >> 6 & 0x1F;
			else return 0;
		}();

		const u64 result = static_cast<u64>( [&] {
			if constexpr (instr == SLL)
			{
				/* Shift Left Logical;
				   Shifts the contents of register rt sa bits to the left, and inserts 0 to the low-order bits.
				   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
				return s32(gpr[rt] << sa);
			}
			else if constexpr (instr == SRL)
			{
				/* Shift Right Logical;
				   Shifts the contents of register rt sa bits to the right, and inserts 0 to the high-order bits.
				   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
				return s32(u32(gpr[rt]) >> sa);
			}
			else if constexpr (instr == SRA)
			{
				/* Shift Right Arithmetic;
				   Shifts the contents of register rt sa bits to the right, and sign-extends the high-order bits.
				   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
				return s32(gpr[rt]) >> sa;
			}
			else if constexpr (instr == SLLV)
			{
				/* Shift Left Logical Variable;
				   Shifts the contents of register rt to the left and inserts 0 to the low-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 5 bits of register rs.
				   Sign-extends (in the 64-bit mode) the result and stores it to register rd. */
				return s32(gpr[rt]) << (gpr[rs] & 0x1F);
			}
			else if constexpr (instr == SRLV)
			{
				/* Shift Right Logical Variable;
				   Shifts the contents of register rt to the right, and inserts 0 to the high-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 5 bits of register rs.
				   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
				return s32(u32(gpr[rt]) >> (gpr[rs] & 0x1F));
			}
			else if constexpr (instr == SRAV)
			{
				/* Shift Right Arithmetic Variable;
				   Shifts the contents of register rt to the right and sign-extends the high-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 5 bits of register rs.
				   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
				return s32(gpr[rt]) >> (gpr[rs] & 0x1F);
			}
			else if constexpr (instr == DSLL)
			{
				/* Doubleword Shift Left Logical;
				   Shifts the contents of register rt sa bits to the left, and inserts 0 to the low-order bits.
				   Stores the 64-bit result to register rd. */
				return gpr[rt] << sa;
			}
			else if constexpr (instr == DSRL)
			{
				/* Doubleword Shift Right Logical;
				   Shifts the contents of register rt sa bits to the right, and inserts 0 to the high-order bits.
				   Stores the 64-bit result to register rd. */
				return u64(gpr[rt]) >> sa;
			}
			else if constexpr (instr == DSRA)
			{
				/* Doubleword Shift Right Arithmetic;
				   Shifts the contents of register rt sa bits to the right, and sign-extends the high-order bits.
				   Stores the 64-bit result to register rd. */
				return gpr[rt] >> sa;
			}
			else if constexpr (instr == DSLLV)
			{
				/* Doubleword Shift Left Logical Variable;
				   Shifts the contents of register rt to the left, and inserts 0 to the low-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 6 bits of register rs.
				   Stores the 64-bit result and stores it to register rd. */
				return gpr[rt] << (gpr[rs] & 0x3F);
			}
			else if constexpr (instr == DSRLV)
			{
				/* Doubleword Shift Right Logical Variable;
				   Shifts the contents of register rt to the right, and inserts 0 to the higher bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 6 bits of register rs. */
				return u64(gpr[rt]) >> (gpr[rs] & 0x3F);
			}
			else if constexpr (instr == DSRAV)
			{
				/* Doubleword Shift Right Arithmetic Variable;
				   Shifts the contents of register rt to the right, and sign-extends the high-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 6 bits of register rs.  */
				return gpr[rt] >> (gpr[rs] & 0x3F);
			}
			else if constexpr (instr == DSLL32)
			{
				/* Doubleword Shift Left Logical + 32;
				   Shifts the contents of register rt 32+sa bits to the left, and inserts 0 to the low-order bits.
				   Stores the 64-bit result to register rd. */
				return gpr[rt] << (sa + 32);
			}
			else if constexpr (instr == DSRL32)
			{
				/* Doubleword Shift Right Logical + 32;
				   Shifts the contents of register rt 32+sa bits to the right, and inserts 0 to the high-order bits.
				   Stores the 64-bit result to register rd. */
				return u64(gpr[rt]) >> (sa + 32);
			}
			else if constexpr (instr == DSRA32)
			{
				/* Doubleword Shift Right Arithmetic + 32;
				   Shifts the contents of register rt 32+sa bits to the right, and sign-extends the high-order bits.
				   Stores the 64-bit result to register rd.*/
				return gpr[rt] >> (sa + 32);
			}
			else
			{
				static_assert(instr != instr, "\"ALU_Shift\" template function called, but no matching ALU shift instruction was found.");
			}
		}());

		gpr.Set(rd, result);
		AdvancePipeline<1>();
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
			const s64 result = (gpr[rs] & 0xFFFF'FFFF) * (gpr[rt] & 0xFFFF'FFFF);
			lo_reg = s32(result);
			hi_reg = result >> 32;
		}
		else if constexpr (instr == MULTU)
		{
			/* Multiply Unsigned;
			   Multiplies the contents of register rs by the contents of register rt as a 32-bit
			   unsigned integer. Sign-extends (in the 64-bit mode) and stores the 64-bit
			   result to special registers HI and LO. */
			const u64 result = u64(gpr[rs] & 0xFFFF'FFFF) * u64(gpr[rt] & 0xFFFF'FFFF);
			lo_reg = s32(result);
			hi_reg = s32(result >> 32);
		}
		else if constexpr (instr == DIV)
		{
			/* Divide;
			   Divides the contents of register rs by the contents of register rt. The operand
			   is treated as a 32-bit signed integer. Sign-extends (in the 64-bit mode) and
			   stores the 32-bit quotient to special register LO and the 32-bit remainder to
			   special register HI. */
			const s32 quotient = s32(gpr[rs]) / s32(gpr[rt]);
			const s32 remainder = s32(gpr[rs]) % s32(gpr[rt]);
			lo_reg = quotient;
			hi_reg = remainder;
		}
		else if constexpr (instr == DIVU)
		{
			/* Divide Unsigned;
			   Divides the contents of register rs by the contents of register rt. The operand
			   is treated as a 32-bit unsigned integer. Sign-extends (in the 64-bit mode) and
			   stores the 32-bit quotient to special register LO and the 32-bit remainder to
			   special register HI. */
			const u32 quotient = u32(gpr[rs]) / u32(gpr[rt]);
			const u32 remainder = u32(gpr[rs]) % u32(gpr[rt]);
			lo_reg = s32(quotient);
			hi_reg = s32(remainder);
		}
		else if constexpr (instr == DMULT)
		{
			/* Doubleword Multiply;
			   Multiplies the contents of register rs by the contents of register rt as a signed integer.
			   Stores the 128-bit result to special registers HI and LO. */
#if defined _MSC_VER
			   __int64 high_product;
			   __int64 low_product = _mul128(gpr[rs], gpr[rt], &high_product);
			   lo_reg = low_product;
			   hi_reg = high_product;
#elif defined __clang__ || defined __GNUC__
			const __int128 product = __int128(gpr[rs]) * __int128(gpr[rt]);
			lo_reg = product & 0xFFFF'FFFF'FFFF'FFFF;
			hi_reg = product >> 64;
#else
			/* TODO */
			assert(false);
#endif
		}
		else if constexpr (instr == DMULTU)
		{
			/* Doubleword Multiply Unsigned;
			   Multiplies the contents of register rs by the contents of register rt as an unsigned integer.
			   Stores the 128-bit result to special registers HI and LO. */
#if defined _MSC_VER
			   unsigned __int64 high_product;
			   unsigned __int64 low_product = _umul128(gpr[rs], gpr[rt], &high_product);
			   lo_reg = low_product;
			   hi_reg = high_product;
#elif defined __clang__ || defined __GNUC__
			const unsigned __int128 product = unsigned __int128(gpr[rs]) * unsigned __int128(gpr[rt]);
			lo_reg = product & 0xFFFF'FFFF'FFFF'FFFF;
			hi_reg = product >> 64;
#else
			/* TODO */
			assert(false);
#endif
		}
		else if constexpr (instr == DDIV)
		{
			/* Doubleword Divide;
			   Divides the contents of register rs by the contents of register rt.
			   The operand is treated as a signed integer.
			   Stores the 64-bit quotient to special register LO, and the 64-bit remainder to special register HI. */
			const s64 quotient = gpr[rs] / gpr[rt];
			const s64 remainder = gpr[rs] % gpr[rt];
			lo_reg = quotient;
			hi_reg = remainder;
		}
		else if constexpr (instr == DDIVU)
		{
			/* Doubleword Divide Unsigned;
			   Divides the contents of register rs by the contents of register rt.
			   The operand is treated as an unsigned integer.
			   Stores the 64-bit quotient to special register LO, and the 64-bit remainder to special register HI. */
			const u64 quotient = u64(gpr[rs]) / u64(gpr[rt]);
			const u64 remainder = u64(gpr[rs]) % u64(gpr[rt]);
			lo_reg = quotient;
			hi_reg = remainder;
		}
		else
		{
			static_assert(instr != instr, "\"ALU_MulDiv\" template function called, but no matching ALU mul/div instruction was found.");
		}

		AdvancePipeline< [&] {
			     if constexpr (instr == MULT  || instr == MULTU)  return 5;
			else if constexpr (instr == DMULT || instr == DMULTU) return 8;
			else if constexpr (instr == DIV   || instr == DIVU)   return 37;
			else if constexpr (instr == DDIV  || instr == DDIVU)  return 69;
			else static_assert(instr != instr, "\"ALU_MulDiv\" template function called, but no matching ALU mul/div instruction was found.");
		}()>();
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
				const u64 target = u64((instr_code & 0x03FF'FFFF) << 2);
				return pc & 0xFFFF'FFFF'F000'0000 | target;
			}
			else if constexpr (instr == JR || instr == JALR)
			{
				const u8 rs = instr_code >> 21 & 0x1F;
				if (gpr[rs] & 3)
				{
					SignalException<Exception::AddressError>();
				}
				return gpr[rs];
			}
			else
			{
				static_assert(instr != instr, "\"Jump\" template function called, but no matching jump instruction was found.");
			}
		}();

		PrepareJump(target);

		if constexpr (instr == JAL)
		{
			gpr.Set(31, pc + 4);
		}
		else if constexpr (instr == JALR)
		{
			const u8 rd = instr_code >> 11 & 0x1F;
			gpr.Set(rd, pc + 4);
		}

		AdvancePipeline<1>();
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
			gpr.Set(31, pc + 4);
		}

		const bool branch_cond = [&] {
			if constexpr (instr == BEQ || instr == BEQL) /* Branch On Equal (Likely) */
				return gpr[rs] == gpr[rt];
			else if constexpr (instr == BNE || instr == BNEL) /* Branch On Not Equal (Likely) */
				return gpr[rs] != gpr[rt];
			else if constexpr (instr == BLEZ || instr == BLEZL) /* Branch On Less Than Or Equal To Zero (Likely) */
				return gpr[rs] <= 0;
			else if constexpr (instr == BGTZ || instr == BGTZL) /* Branch On Greater Than Zero (Likely) */
				return gpr[rs] > 0;
			else if constexpr (instr == BLTZ || instr == BLTZL) /* Branch On Less Than Zero (Likely) */
				return gpr[rs] < 0;
			else if constexpr (instr == BGEZ || instr == BGEZL) /* Branch On Greater Than or Equal To Zero (Likely) */
				return gpr[rs] >= 0;
			else if constexpr (instr == BLTZAL || instr == BLTZALL) /* Branch On Less Than Zero and Link (Likely) */
				return gpr[rs] < 0;
			else if constexpr (instr == BGEZAL || instr == BGEZALL) /* Branch On Greater Than Or Equal To Zero And Link (Likely) */
				return gpr[rs] >= 0;
			else
				static_assert(instr != instr, "\"Branch\" template function called, but no matching branch instruction was found.");
		}();

		if (branch_cond)
		{
			const s64 offset = s64(s16(instr_code & 0xFFFF)) << 2;
			PrepareJump(pc + offset);
			pc_is_inside_branch_delay_slot = true;
		}
		else if constexpr (instr == BEQL || instr == BNEL || instr == BLEZL || instr == BGTZL ||
			instr == BEQL || instr == BLTZL || instr == BGEZL || instr == BLTZALL || instr == BGEZALL)
		{
			pc += 4; /* The instruction in the branch delay slot is discarded. */
		}

		AdvancePipeline<1>();
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
				return gpr[rs] > gpr[rt];
			}
			else if constexpr (instr == TGEU)
			{
				/* Trap If Greater Than Or Equal Unsigned;
				   Compares registers rs and rt as unsigned integers.
				   If register rs is greater than rt, generates an exception. */
				return u64(gpr[rs]) > u64(gpr[rt]);
			}
			else if constexpr (instr == TLT)
			{
				/* Trap If Less Than;
				   Compares registers rs and rt as signed integers.
				   If register rs is less than rt, generates an exception. */
				return gpr[rs] < gpr[rt];
			}
			else if constexpr (instr == TLTU)
			{
				/* Trap If Less Than Unsigned;
				   Compares registers rs and rt as unsigned integers.
				   If register rs is less than rt, generates an exception. */
				return u64(gpr[rs]) < u64(gpr[rt]);
			}
			else if constexpr (instr == TEQ)
			{
				/* Trap If Equal;
				   Generates an exception if registers rs and rt are equal. */
				return gpr[rs] == gpr[rt];
			}
			else if constexpr (instr == TNE)
			{
				/* Trap If Not Equal;
				   Generates an exception if registers rs and rt are not equal. */
				return gpr[rs] != gpr[rt];
			}
			else
			{
				static_assert(instr != instr, "\"Trap_ThreeOperand\" template function called, but no matching trap instruction was found.");
			}
		}();

		if (trap_cond)
			TrapException();

		AdvancePipeline<1>();
	}


	template<CPU_Instruction instr>
	void Trap_Immediate(const u32 instr_code)
	{
		using enum CPU_Instruction;

		const u8 rs = instr_code >> 21 & 0x1F;
		const auto immediate = [&] {
			if constexpr (instr == TGEI || instr == TLTI)
				return s16(instr_code & 0xFFFF);
			else
				return u16(instr_code & 0xFFFF);
		}();

		const bool trap_cond = [&] {
			if constexpr (instr == TGEI)
			{
				/* Trap If Greater Than Or Equal Immediate;
				   Compares the contents of register rs with 16-bit sign-extended immediate as a
				   signed integer. If rs contents are greater than the immediate, generates an exception. */
				return gpr[rs] > immediate;
			}
			else if constexpr (instr == TGEIU)
			{
				/* Trap If Greater Than Or Equal Immediate Unsigned;
				   Compares the contents of register rs with 16-bit zero-extended immediate as an
				   unsigned integer. If rs contents are greater than the immediate, generates an exception. */
				return u64(gpr[rs]) > immediate;
			}
			else if constexpr (instr == TLTI)
			{
				/* Trap If Less Than Immediate;
				   Compares the contents of register rs with 16-bit sign-extended immediate as a
				   signed integer. If rs contents are less than the immediate, generates an exception. */
				return gpr[rs] < immediate;
			}
			else if constexpr (instr == TLTIU)
			{
				/* Trap If Less Than Immediate Unsigned;
				   Compares the contents of register rs with 16-bit zero-extended immediate as an
				   unsigned integer. If rs contents are less than the immediate, generates an exception. */
				return u64(gpr[rs]) < immediate;
			}
			else if constexpr (instr == TEQI)
			{
				/* Trap If Equal Immediate;
				   Generates an exception if the contents of register rs are equal to immediate. */
				return s64(gpr[rs]) == immediate; /* TODO: should we really cast to s64? */
			}
			else if constexpr (instr == TNEI)
			{
				/* Trap If Not Equal Immediate;
				   Generates an exception if the contents of register rs are not equal to immediate. */
				return s64(gpr[rs]) != immediate;
			}
			else
			{
				static_assert(instr != instr, "\"Trap_Immediate\" template function called, but no matching trap instruction was found.");
			}
		}();

		if (trap_cond)
			TrapException();

		AdvancePipeline<1>();
	}

	void MFHI(const u32 instr_code)
	{
		/* Move From HI;
		   Transfers the contents of special register HI to register rd. */
		const u8 rd = instr_code >> 11 & 0x1F;
		gpr.Set(rd, hi_reg);
		AdvancePipeline<1>();
	}


	void MFLO(const u32 instr_code)
	{
		/* Move From LO;
		   Transfers the contents of special register LO to register rd. */
		const u8 rd = instr_code >> 11 & 0x1F;
		gpr.Set(rd, lo_reg);
		AdvancePipeline<1>();
	}


	void MTHI(const u32 instr_code)
	{
		/* Move To HI;
		   Transfers the contents of register rs to special register HI. */
		const u8 rs = instr_code >> 21 & 0x1F;
		hi_reg = gpr[rs];
		AdvancePipeline<1>();
	}


	void MTLO(const u32 instr_code)
	{
		/* Move To LO;
		   Transfers the contents of register rs to special register LO. */
		const u8 rs = instr_code >> 21 & 0x1F;
		lo_reg = gpr[rs];
		AdvancePipeline<1>();
	}


	void SYNC(const u32 instr_code)
	{
		/* Synchronize;
		   Completes the Load/store instruction currently in the pipeline before the new
		   Load/store instruction is executed. Is executed as a NOP on the VR4300. */
		AdvancePipeline<1>();
	}


	void SYSCALL(const u32 instr_code)
	{
		/* System Call;
		   Generates a system call exception and transfers control to the exception processing program.
		   If a SYSCALL instruction is in a branch delay slot, the branch instruction is decoded to
		   branch and re-execute. */
		if (pc_is_inside_branch_delay_slot)
		{
			pc -= 8;
		}
		else
		{
			SignalException<Exception::Syscall>();
		}
		AdvancePipeline<1>();
	}


	void BREAK(const u32 instr_code)
	{
		/* Breakpoint;
		   Generates a breakpoint exception and transfers control to the exception processing program. */
		SignalException<Exception::Breakpoint>();
		AdvancePipeline<1>();
	}


	void PrepareJump(const u64 target_address)
	{
		jump_is_pending = true;
		instructions_until_jump = 1;
		addr_to_jump_to = target_address;
		pc_is_inside_branch_delay_slot = true;
	}


	template void Load<CPU_Instruction::LB>(const u32);
	template void Load<CPU_Instruction::LBU>(const u32);
	template void Load<CPU_Instruction::LH>(const u32);
	template void Load<CPU_Instruction::LHU>(const u32);
	template void Load<CPU_Instruction::LW>(const u32);
	template void Load<CPU_Instruction::LWU>(const u32);
	template void Load<CPU_Instruction::LWL>(const u32);
	template void Load<CPU_Instruction::LWR>(const u32);
	template void Load<CPU_Instruction::LD>(const u32);
	template void Load<CPU_Instruction::LDL>(const u32);
	template void Load<CPU_Instruction::LDR>(const u32);
	template void Load<CPU_Instruction::LL>(const u32);
	template void Load<CPU_Instruction::LLD>(const u32);

	template void Store<CPU_Instruction::SB>(const u32);
	template void Store<CPU_Instruction::SH>(const u32);
	template void Store<CPU_Instruction::SW>(const u32);
	template void Store<CPU_Instruction::SWL>(const u32);
	template void Store<CPU_Instruction::SWR>(const u32);
	template void Store<CPU_Instruction::SC>(const u32);
	template void Store<CPU_Instruction::SCD>(const u32);
	template void Store<CPU_Instruction::SD>(const u32);
	template void Store<CPU_Instruction::SDL>(const u32);
	template void Store<CPU_Instruction::SDR>(const u32);

	template void ALU_Immediate<CPU_Instruction::ADDI>(const u32);
	template void ALU_Immediate<CPU_Instruction::ADDIU>(const u32);
	template void ALU_Immediate<CPU_Instruction::SLTI>(const u32);
	template void ALU_Immediate<CPU_Instruction::SLTIU>(const u32);
	template void ALU_Immediate<CPU_Instruction::ANDI>(const u32);
	template void ALU_Immediate<CPU_Instruction::ORI>(const u32);
	template void ALU_Immediate<CPU_Instruction::XORI>(const u32);
	template void ALU_Immediate<CPU_Instruction::LUI>(const u32);
	template void ALU_Immediate<CPU_Instruction::DADDI>(const u32);
	template void ALU_Immediate<CPU_Instruction::DADDIU>(const u32);

	template void ALU_ThreeOperand<CPU_Instruction::ADD>(const u32);
	template void ALU_ThreeOperand<CPU_Instruction::ADDU>(const u32);
	template void ALU_ThreeOperand<CPU_Instruction::SUB>(const u32);
	template void ALU_ThreeOperand<CPU_Instruction::SUBU>(const u32);
	template void ALU_ThreeOperand<CPU_Instruction::SLT>(const u32);
	template void ALU_ThreeOperand<CPU_Instruction::SLTU>(const u32);
	template void ALU_ThreeOperand<CPU_Instruction::AND>(const u32);
	template void ALU_ThreeOperand<CPU_Instruction::OR>(const u32);
	template void ALU_ThreeOperand<CPU_Instruction::XOR>(const u32);
	template void ALU_ThreeOperand<CPU_Instruction::NOR>(const u32);
	template void ALU_ThreeOperand<CPU_Instruction::DADD>(const u32);
	template void ALU_ThreeOperand<CPU_Instruction::DADDU>(const u32);
	template void ALU_ThreeOperand<CPU_Instruction::DSUB>(const u32);
	template void ALU_ThreeOperand<CPU_Instruction::DSUBU>(const u32);

	template void ALU_Shift<CPU_Instruction::SLL>(const u32);
	template void ALU_Shift<CPU_Instruction::SRL>(const u32);
	template void ALU_Shift<CPU_Instruction::SRA>(const u32);
	template void ALU_Shift<CPU_Instruction::SLLV>(const u32);
	template void ALU_Shift<CPU_Instruction::SRLV>(const u32);
	template void ALU_Shift<CPU_Instruction::SRAV>(const u32);
	template void ALU_Shift<CPU_Instruction::DSLL>(const u32);
	template void ALU_Shift<CPU_Instruction::DSRL>(const u32);
	template void ALU_Shift<CPU_Instruction::DSRA>(const u32);
	template void ALU_Shift<CPU_Instruction::DSLLV>(const u32);
	template void ALU_Shift<CPU_Instruction::DSRLV>(const u32);
	template void ALU_Shift<CPU_Instruction::DSRAV>(const u32);
	template void ALU_Shift<CPU_Instruction::DSLL32>(const u32);
	template void ALU_Shift<CPU_Instruction::DSRL32>(const u32);
	template void ALU_Shift<CPU_Instruction::DSRA32>(const u32);

	template void ALU_MulDiv<CPU_Instruction::MULT>(const u32);
	template void ALU_MulDiv<CPU_Instruction::MULTU>(const u32);
	template void ALU_MulDiv<CPU_Instruction::DIV>(const u32);
	template void ALU_MulDiv<CPU_Instruction::DIVU>(const u32);
	template void ALU_MulDiv<CPU_Instruction::DMULT>(const u32);
	template void ALU_MulDiv<CPU_Instruction::DMULTU>(const u32);
	template void ALU_MulDiv<CPU_Instruction::DDIV>(const u32);
	template void ALU_MulDiv<CPU_Instruction::DDIVU>(const u32);

	template void Jump<CPU_Instruction::J>(const u32);
	template void Jump<CPU_Instruction::JAL>(const u32);
	template void Jump<CPU_Instruction::JR>(const u32);
	template void Jump<CPU_Instruction::JALR>(const u32);

	template void Branch<CPU_Instruction::BEQ>(const u32);
	template void Branch<CPU_Instruction::BNE>(const u32);
	template void Branch<CPU_Instruction::BLEZ>(const u32);
	template void Branch<CPU_Instruction::BGTZ>(const u32);
	template void Branch<CPU_Instruction::BLTZ>(const u32);
	template void Branch<CPU_Instruction::BGEZ>(const u32);
	template void Branch<CPU_Instruction::BLTZAL>(const u32);
	template void Branch<CPU_Instruction::BGEZAL>(const u32);
	template void Branch<CPU_Instruction::BEQL>(const u32);
	template void Branch<CPU_Instruction::BNEL>(const u32);
	template void Branch<CPU_Instruction::BLEZL>(const u32);
	template void Branch<CPU_Instruction::BGTZL>(const u32);
	template void Branch<CPU_Instruction::BLTZL>(const u32);
	template void Branch<CPU_Instruction::BGEZL>(const u32);
	template void Branch<CPU_Instruction::BLTZALL>(const u32);
	template void Branch<CPU_Instruction::BGEZALL>(const u32);

	template void Trap_ThreeOperand<CPU_Instruction::TGE>(const u32);
	template void Trap_ThreeOperand<CPU_Instruction::TGEU>(const u32);
	template void Trap_ThreeOperand<CPU_Instruction::TLT>(const u32);
	template void Trap_ThreeOperand<CPU_Instruction::TLTU>(const u32);
	template void Trap_ThreeOperand<CPU_Instruction::TEQ>(const u32);
	template void Trap_ThreeOperand<CPU_Instruction::TNE>(const u32);

	template void Trap_Immediate<CPU_Instruction::TGEI>(const u32);
	template void Trap_Immediate<CPU_Instruction::TGEIU>(const u32);
	template void Trap_Immediate<CPU_Instruction::TLTI>(const u32);
	template void Trap_Immediate<CPU_Instruction::TLTIU>(const u32);
	template void Trap_Immediate<CPU_Instruction::TEQI>(const u32);
	template void Trap_Immediate<CPU_Instruction::TNEI>(const u32);
}