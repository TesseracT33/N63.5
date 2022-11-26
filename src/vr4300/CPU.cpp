module VR4300:CPU;

import :COP0;
import :Exceptions;
import :MMU;
import :Operation;

import Memory;

namespace VR4300
{
	s64 GPR::Get(size_t index) const
	{
		return gpr[index];
	}


	s64 GPR::Get(Reg reg) const
	{
		return gpr[std::to_underlying(reg)];
	}


	void GPR::Set(size_t index, s64 data)
	{ /* gpr[0] is hardwired to 0. Prefer setting it to zero every time over a branch checking if 'index' is zero. */
		gpr[index] = data;
		gpr[0] = 0;
	}


	void GPR::Set(Reg reg, s64 data)
	{
		gpr[std::to_underlying(reg)] = data;
		gpr[0] = 0;
	}


	s64 GPR::operator[](size_t index)
	{ /* returns by value so that assignments have to made through function "Set". */
		return gpr[index];
	}


	template<CpuInstruction instr>
	void Load(u32 rs, u32 rt, s16 imm16)
	{
		using enum CpuInstruction;

		auto addr = gpr[rs] + imm16;

		/* For all instructions:
		   Generates an address by adding a sign-extended offset to the contents of register base.
		   In the 32-bit Kernel mode, the high-order 32 bits are ignored during
		   virtual address creation. */
		if constexpr (OneOf(instr, LD, LDL, LDR, LLD)) {
			if (operating_mode == OperatingMode::Kernel && addressing_mode == AddressingMode::_32bit) {
				addr = s32(addr);
			}
		}

		auto result = [&] {
			if constexpr (instr == LB) {
				/* Load Byte;
				   Sign-extends the contents of a byte specified by the address and loads the result to register rt. */
				return ReadVirtual<s8>(addr);
			}
			else if constexpr (instr == LBU) {
				/* Load Byte Unsigned;
				   Zero-extends the contents of a byte specified by the address and loads the result to register rt. */
				return u8(ReadVirtual<s8>(addr));
			}
			else if constexpr (instr == LH) {
				/* Load halfword;
				   Sign-extends the contents of a halfword specified by the address and loads the result to register rt. */
				return ReadVirtual<s16>(addr);
			}
			else if constexpr (instr == LHU) {
				/* Load Halfword Unsigned;
				   Zero-extends the contents of a halfword specified by the address and loads the result to register rt. */
				return u16(ReadVirtual<s16>(addr));
			}
			else if constexpr (instr == LW) {
				/* Load Word;
				   Sign-extends the contents of a word specified by the address and loads the result to register rt. */
				return ReadVirtual<s32>(addr);
			}
			else if constexpr (instr == LWU) {
				/* Load Word Unsigned;
				   Zero-extends the contents of a word specified by the address and loads the result to register rt. */
				return u32(ReadVirtual<s32>(addr));
			}
			else if constexpr (instr == LWL) {
				/* Load Word Left;
				   Shifts a word specified by the address to the left, so that a byte specified by
				   the address is at the leftmost position of the word. Sign-extends (in the 64-
				   bit mode), merges the result of the shift and the contents of register rt, and
				   loads the result to register rt. */
				return ReadVirtual<s32, Alignment::UnalignedLeft>(addr);
			}
			else if constexpr (instr == LWR) {
				/* Load Word Right;
				   Shifts a word specified by the address to the right, so that a byte specified by
				   the address is at the rightmost position of the word. Sign-extends (in the 64-
				   bit mode), merges the result of the shift and the contents of register rt, and
				   loads the result to register rt. */
				return ReadVirtual<s32, Alignment::UnalignedRight>(addr);
			}
			else if constexpr (instr == LD) {
				/* Load Doubleword;
				   Loads the contents of a word specified by the address to register rt. */
				return ReadVirtual<s64>(addr);
			}
			else if constexpr (instr == LDL) {
				/* Load Doubleword Left;
				   Shifts the doubleword specified by the address to the left so that the byte
				   specified by the address is at the leftmost position of the doubleword.
				   Merges the result of the shift and the contents of register rt, and loads the
				   result to register rt. */
				return ReadVirtual<s64, Alignment::UnalignedLeft>(addr);
			}
			else if constexpr (instr == LDR) {
				/* Load Doubleword Right;
				   Shifts the doubleword specified by the address to the right so that the byte
				   specified by the address is at the rightmost position of the doubleword.
				   Merges the result of the shift and the contents of register rt, and loads the
				   result to register rt. */
				return ReadVirtual<s64, Alignment::UnalignedRight>(addr);
			}
			else if constexpr (instr == LL) {
				/* Load Linked;
				   Loads the contents of the word specified by the address to register rt and sets the LL bit to 1.
				   Additionally, the specified physical address of the memory is stored to the LLAddr register. */
				s32 ret = ReadVirtual<s32>(addr);
				cop0.ll_addr = last_physical_address_on_load >> 4;
				ll_bit = 1;
				return ret;
			}
			else if constexpr (instr == LLD) {
				/* Load Linked Doubleword;
				   Loads the contents of the doubleword specified by the address to register rt and sets the LL bit to 1.
				   Additionally, the specified physical address of the memory is stored to the LLAddr register. */
				s64 ret = ReadVirtual<s64>(addr);
				cop0.ll_addr = last_physical_address_on_load >> 4;
				ll_bit = 1;
				return ret;
			}
			else {
				static_assert(AlwaysFalse<instr>);
			}
		}();

		if (exception_has_occurred) {
			return;
		}

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
		if constexpr (OneOf(instr, LWL, LDL, LWR, LDR)) {
			static constexpr std::array<u64, 8> right_load_mask = {
				0xFFFF'FFFF'FFFF'FF00, 0xFFFF'FFFF'FFFF'0000, 0xFFFF'FFFF'FF00'0000, 0xFFFF'FFFF'0000'0000,
				0xFFFF'FF00'0000'0000, 0xFFFF'0000'0000'0000, 0xFF00'0000'0000'0000, 0
			};
			if constexpr (instr == LWL) {
				u32 bits_from_last_boundary = (addr & 3) << 3;
				result <<= bits_from_last_boundary;
				s32 untouched_gpr = s32(gpr[rt] & ((1 << bits_from_last_boundary) - 1));
				gpr.Set(rt, result | untouched_gpr);
			}
			else if constexpr (instr == LDL) {
				u32 bits_from_last_boundary = (addr & 7) << 3;
				result <<= bits_from_last_boundary;
				s64 untouched_gpr = gpr[rt] & ((1ll << bits_from_last_boundary) - 1);
				gpr.Set(rt, result | untouched_gpr);
			}
			else if constexpr (instr == LWR) {
				u32 bytes_from_last_boundary = addr & 3;
				result >>= 8 * (3 - bytes_from_last_boundary);
				s32 untouched_gpr = s32(gpr[rt] & right_load_mask[bytes_from_last_boundary]);
				gpr.Set(rt, result | untouched_gpr);
			}
			else if constexpr (instr == LDR) {
				u32 bytes_from_last_boundary = addr & 7;
				result >>= 8 * (7 - bytes_from_last_boundary);
				s64 untouched_gpr = gpr[rt] & right_load_mask[bytes_from_last_boundary];
				gpr.Set(rt, result | untouched_gpr);
			}
			AdvancePipeline(2);
		}
		else { /* Aligned read */
			gpr.Set(rt, result);
			AdvancePipeline(1);
		}
	}


	template<CpuInstruction instr>
	void Store(u32 rs, u32 rt, s16 imm16)
	{
		using enum CpuInstruction;

		auto addr = gpr[rs] + imm16;

		/* For all instructions:
		   Generates an address by adding a sign-extended offset to the contents of register base.
		   For "doubleword instructions", in the 32-bit Kernel mode, the high-order 32 bits are ignored during
		   virtual address creation. */
		if constexpr (OneOf(instr, SD, SDL, SDR, SCD)) {
			if (operating_mode == OperatingMode::Kernel && addressing_mode == AddressingMode::_32bit) {
				addr = s32(addr);
			}
		}

		if constexpr (instr == SB) {
			/* Store Byte;
			   Stores the contents of the low-order byte of register rt to the memory specified by the address. */
			WriteVirtual<1>(addr, gpr[rt]);
		}
		else if constexpr (instr == SH) {
			/* Store Halfword;
			   Stores the contents of the low-order halfword of register rt to the memory specified by the address. */
			WriteVirtual<2>(addr, gpr[rt]);
		}
		else if constexpr (instr == SW) {
			/* Store Word;
			   Stores the contents of the low-order word of register rt to the memory specified by the address. */
			WriteVirtual<4>(addr, gpr[rt]);
		}
		else if constexpr (instr == SWL) {
			/* Store Word Left;
			   Shifts the contents of register rt to the right so that the leftmost byte of the
			   word is at the position of the byte specified by the address. Stores the result
			   of the shift to the lower portion of the word in memory. */
			WriteVirtual<4, Alignment::UnalignedLeft>(addr, u32(gpr[rt]) >> (8 * (addr & 3)));
		}
		else if constexpr (instr == SWR) {
			/* Store Word Right;
			   Shifts the contents of register rt to the left so that the rightmost byte of the
			   word is at the position of the byte specified by the address. Stores the result
			   of the shift to the higher portion of the word in memory. */
			WriteVirtual<4, Alignment::UnalignedRight>(addr, gpr[rt] << (8 * (3 - (addr & 3))));
		}
		else if constexpr (instr == SD) {
			/* Store Doublword;
			   Stores the contents of register rt to the memory specified by the address. */
			WriteVirtual<8>(addr, gpr[rt]);
		}
		else if constexpr (instr == SDL) {
			/* Store Doubleword Left;
			   Shifts the contents of register rt to the right so that the leftmost byte of a
			   doubleword is at the position of the byte specified by the address. Stores the
			   result of the shift to the lower portion of the doubleword in memory. */
			WriteVirtual<8, Alignment::UnalignedLeft>(addr, gpr[rt] >> (8 * (addr & 7)));
		}
		else if constexpr (instr == SDR) {
			/* Store Doubleword Right;
			   Shifts the contents of register rt to the left so that the rightmost byte of a
			   doubleword is at the position of the byte specified by the address. Stores the
			   result of the shift to the higher portion of the doubleword in memory. */
			WriteVirtual<8, Alignment::UnalignedRight>(addr, gpr[rt] << (8 * (7 - (addr & 7))));
		}
		else if constexpr (instr == SC) {
			/* Store Conditional;
			   If the LL bit is 1, stores the contents of the low-order word of register rt to
			   the memory specified by the address, and sets register rt to 1.
			   If the LL bit is 0, does not store the contents of the word, and clears register
			   rt to 0. */
			if (ll_bit == 1) {
				WriteVirtual<4>(addr, gpr[rt]);
				gpr.Set(rt, 1);
			}
			else {
				gpr.Set(rt, 0);
			}
		}
		else if constexpr (instr == SCD) {
			/* Store Conditional Doubleword;
			   If the LL bit is 1, stores the contents of register rt to the memory specified by
			   the address, and sets register rt to 1.
			   If the LL bit is 0, does not store the contents of the register, and clears register
			   rt to 0. */
			if (ll_bit == 1) {
				WriteVirtual<8>(addr, gpr[rt]);
				gpr.Set(rt, 1);
			}
			else {
				gpr.Set(rt, 0);
			}
		}
		else {
			static_assert(AlwaysFalse<instr>);
		}

		AdvancePipeline(1);
	}


	template<CpuInstruction instr>
	void AluImmediate(u32 rs, u32 rt, s16 imm16)
	{
		using enum CpuInstruction;

		auto imm = [&] {
			if constexpr (OneOf(instr, ANDI, LUI, ORI, XORI)) return u16(imm16);
			else                                              return s16(imm16);
		}();

		if constexpr (instr == ADDI) {
			/* Add Immediate;
			   Sign-extends the 16-bit immediate and adds it to register rs. Stores the
			   32-bit result to register rt (sign-extends the result in the 64-bit mode).
			   Generates an exception if a 2's complement integer overflow occurs.
			   In that case, the contents of destination register rt are not modified */
			s32 sum = s32(gpr[rs] + imm);
			if ((gpr[rs] ^ sum) & (imm ^ sum) & 0x8000'0000) {
				SignalException<Exception::IntegerOverflow>();
			}
			else {
				gpr.Set(rt, sum);
			}
		}
		else if constexpr (instr == ADDIU) {
			/* Add Immediate Unsigned;
			   Sign-extends the 16-bit immediate and adds it to register rs. Stores the 32-bit
			   result to register rt (sign-extends the result in the 64-bit mode). Does not
			   generate an exception even if an integer overflow occurs. */
			gpr.Set(rt, s32(gpr[rs] + imm));
		}
		else if constexpr (instr == SLTI) {
			/* Set On Less Than Immediate;
			   Sign-extends the 16-bit immediate and compares it with register rs as a
			   signed integer. If rs is less than the immediate, stores 1 to register rt;
			   otherwise, stores 0 to register rt. */
			gpr.Set(rt, gpr[rs] < imm);
		}
		else if constexpr (instr == SLTIU) {
			/* Set On Less Than Immediate Unsigned;
			   Sign-extends the 16-bit immediate and compares it with register rs as an
			   unsigned integer. If rs is less than the immediate, stores 1 to register rt;
			   otherwise, stores 0 to register rt. */
			gpr.Set(rt, u64(gpr[rs]) < imm);
		}
		else if constexpr (instr == ANDI) {
			/* And Immediate;
			   Zero-extends the 16-bit immediate, ANDs it with register rs, and stores the
			   result to register rt. */
			gpr.Set(rt, gpr[rs] & imm);
		}
		else if constexpr (instr == ORI) {
			/* Or Immediate;
			   Zero-extends the 16-bit immediate, ORs it with register rs, and stores the
			   result to register rt. */
			gpr.Set(rt, gpr[rs] | imm);
		}
		else if constexpr (instr == XORI) {
			/* Exclusive Or Immediate;
			   Zero-extends the 16-bit immediate, exclusive-ORs it with register rs, and
			   stores the result to register rt. */
			gpr.Set(rt, gpr[rs] ^ imm);
		}
		else if constexpr (instr == LUI) {
			/* Load Upper Immediate;
			   Shifts the 16-bit immediate 16 bits to the left, and clears the low-order 16 bits
			   of the word to 0.
			   Stores the result to register rt (by sign-extending the result in the 64-bit mode). */
			gpr.Set(rt, s32(imm << 16));
		}
		else if constexpr (instr == DADDI) {
			/* Doubleword Add Immediate;
			   Sign-extends the 16-bit immediate to 64 bits, and adds it to register rs. Stores
			   the 64-bit result to register rt. Generates an exception if an integer overflow occurs. */
			s64 sum = gpr[rs] + imm;
			if ((gpr[rs] ^ sum) & (imm ^ sum) & 0x8000'0000'0000'0000) {
				SignalException<Exception::IntegerOverflow>();
			}
			else {
				gpr.Set(rt, sum);
			}
		}
		else if constexpr (instr == DADDIU) {
			/* Doubleword Add Immediate Unsigned;
			   Sign-extends the 16-bit immediate to 64 bits, and adds it to register rs. Stores
			   the 64-bit result to register rt. Does not generate an exception even if an
			   integer overflow occurs. */
			s64 sum = gpr[rs] + imm;
			gpr.Set(rt, sum);
		}
		else {
			static_assert(AlwaysFalse<instr>);
		}

		AdvancePipeline(1);
	}

	template<CpuInstruction instr>
	void AluThreeOperand(u32 rs, u32 rt, u32 rd)
	{
		using enum CpuInstruction;

		if constexpr (instr == ADD) {
			/* Add;
			   Adds the contents of register rs and rt, and stores (sign-extends in the 64-bit mode)
			   the 32-bit result to register rd. Generates an exception if an integer overflow occurs.
			   In that case, the contents of the destination register rd are not modified.
			   In 64-bit mode, the operands must be sign-extended, 32-bit values. */
			s32 sum = s32(gpr[rs] + gpr[rt]);
			if ((gpr[rs] ^ sum) & (gpr[rt] ^ sum) & 0x8000'0000) {
				SignalException<Exception::IntegerOverflow>();
			}
			else {
				gpr.Set(rd, sum);
			}
		}
		else if constexpr (instr == ADDU) {
			/* Add Unsigned;
			   Adds the contents of register rs and rt, and stores (sign-extends in the 64-bit mode)
			   the 32-bit result to register rd. Does not generate an exception even if an integer overflow occurs. */
			gpr.Set(rd, s32(gpr[rs] + gpr[rt]));
		}
		else if constexpr (instr == SUB) {
			/* Subtract;
			   Subtracts the contents of register rs from register rt, and stores (sign-extends
			   in the 64-bit mode) the result to register rd. Generates an exception if an integer overflow occurs. */
			s32 sum = s32(gpr[rs] - gpr[rt]);
			if ((gpr[rs] ^ gpr[rt]) & ~(gpr[rt] ^ sum) & 0x8000'0000) {
				SignalException<Exception::IntegerOverflow>();
			}
			else {
				gpr.Set(rd, sum);
			}
		}
		else if constexpr (instr == SUBU) {
			/* Subtract Unsigned;
			   Subtracts the contents of register rt from register rs, and stores (sign-extends
			   in the 64-bit mode) the 32-bit result to register rd.
			   Does not generate an exception even if an integer overflow occurs.*/
			gpr.Set(rd, s32(gpr[rs] - gpr[rt]));
		}
		else if constexpr (instr == SLT) {
			/* Set On Less Than;
			   Compares the contents of registers rs and rt as 64-bit signed integers.
			   If the contents of register rs are less than those of rt, stores 1 to register rd;
			   otherwise, stores 0 to rd. */
			gpr.Set(rd, gpr[rs] < gpr[rt]);
		}
		else if constexpr (instr == SLTU) {
			/* Set On Less Than Unsigned;
			   Compares the contents of registers rs and rt as 64-bit unsigned integers.
			   If the contents of register rs are less than those of rt, stores 1 to register rd;
			   otherwise, stores 0 to rd. */
			gpr.Set(rd, u64(gpr[rs]) < u64(gpr[rt]));
		}
		else if constexpr (instr == AND) {
			/* And;
			   ANDs the contents of registers rs and rt in bit units, and stores the result to register rd. */
			gpr.Set(rd, gpr[rs] & gpr[rt]);
		}
		else if constexpr (instr == OR) {
			/* Or;
			   ORs the contents of registers rs and rt in bit units, and stores the result to register rd. */
			gpr.Set(rd, gpr[rs] | gpr[rt]);
		}
		else if constexpr (instr == XOR) {
			/* Exclusive Or;
			   Exclusive-ORs the contents of registers rs and rt in bit units, and stores the result to register rd. */
			gpr.Set(rd, gpr[rs] ^ gpr[rt]);
		}
		else if constexpr (instr == NOR) {
			/* Nor;
			   NORs the contents of registers rs and rt in bit units, and stores the result to register rd. */
			gpr.Set(rd, ~(gpr[rs] | gpr[rt]));
		}
		else if constexpr (instr == DADD) {
			/* Doubleword Add;
			   Adds the contents of registers rs and rt, and stores the 64-bit result to register rd.
			   Generates an exception if an integer overflow occurs. */
			s64 sum = gpr[rs] + gpr[rt];
			if ((gpr[rs] ^ sum) & (gpr[rt] ^ sum) & 0x8000'0000'0000'0000) {
				SignalException<Exception::IntegerOverflow>();
			}
			else {
				gpr.Set(rd, sum);
			}
		}
		else if constexpr (instr == DADDU) {
			/* Doubleword Add Unsigned;
			   Adds the contents of registers rs and rt, and stores the 64-bit result to register rd.
			   Does not generate an exception even if an integer overflow occurs. */
			gpr.Set(rd, gpr[rs] + gpr[rt]);
		}
		else if constexpr (instr == DSUB) {
			/* Doubleword Subtract;
			   Subtracts the contents of register rt from register rs, and stores the 64-bit
			   result to register rd. Generates an exception if an integer overflow occurs. */
			s64 sum = gpr[rs] - gpr[rt];
			if ((gpr[rs] ^ gpr[rt]) & ~(gpr[rt] ^ sum) & 0x8000'0000'0000'0000) {
				SignalException<Exception::IntegerOverflow>();
			}
			else {
				gpr.Set(rd, sum);
			}
		}
		else if constexpr (instr == DSUBU) {
			/* Doubleword Subtract Unsigned;
			   Subtracts the contents of register rt from register rs, and stores the 64-bit result to register rd.
			   Does not generate an exception even if an integer overflow occurs. */
			gpr.Set(rd, gpr[rs] - gpr[rt]);
		}
		else {
			static_assert(AlwaysFalse<instr>);
		}

		AdvancePipeline(1);
	}


	template<CpuInstruction instr>
	void Shift(u32 rt, u32 rd, u32 sa)
	{
		using enum CpuInstruction;

		s64 result = static_cast<s64>( [&] {
			if constexpr (instr == SLL) {
				/* Shift Left Logical;
				   Shifts the contents of register rt sa bits to the left, and inserts 0 to the low-order bits.
				   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
				return s32(gpr[rt] << sa);
			}
			else if constexpr (instr == SRL) {
				/* Shift Right Logical;
				   Shifts the contents of register rt sa bits to the right, and inserts 0 to the high-order bits.
				   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
				return s32(u32(gpr[rt]) >> sa);
			}
			else if constexpr (instr == SRA) {
				/* Shift Right Arithmetic;
				   Shifts the contents of register rt sa bits to the right, and sign-extends the high-order bits.
				   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
				return s32(gpr[rt] >> sa);
			}
			else if constexpr (instr == DSLL) {
				/* Doubleword Shift Left Logical;
				   Shifts the contents of register rt sa bits to the left, and inserts 0 to the low-order bits.
				   Stores the 64-bit result to register rd. */
				return gpr[rt] << sa;
			}
			else if constexpr (instr == DSRL) {
				/* Doubleword Shift Right Logical;
				   Shifts the contents of register rt sa bits to the right, and inserts 0 to the high-order bits.
				   Stores the 64-bit result to register rd. */
				return u64(gpr[rt]) >> sa;
			}
			else if constexpr (instr == DSRA) {
				/* Doubleword Shift Right Arithmetic;
				   Shifts the contents of register rt sa bits to the right, and sign-extends the high-order bits.
				   Stores the 64-bit result to register rd. */
				return gpr[rt] >> sa;
			}
			else if constexpr (instr == DSLL32) {
				/* Doubleword Shift Left Logical + 32;
				   Shifts the contents of register rt 32+sa bits to the left, and inserts 0 to the low-order bits.
				   Stores the 64-bit result to register rd. */
				return gpr[rt] << (sa + 32);
			}
			else if constexpr (instr == DSRL32) {
				/* Doubleword Shift Right Logical + 32;
				   Shifts the contents of register rt 32+sa bits to the right, and inserts 0 to the high-order bits.
				   Stores the 64-bit result to register rd. */
				return u64(gpr[rt]) >> (sa + 32);
			}
			else if constexpr (instr == DSRA32) {
				/* Doubleword Shift Right Arithmetic + 32;
				   Shifts the contents of register rt 32+sa bits to the right, and sign-extends the high-order bits.
				   Stores the 64-bit result to register rd.*/
				return gpr[rt] >> (sa + 32);
			}
			else {
				static_assert(AlwaysFalse<instr>);
			}
		}());

		gpr.Set(rd, result);
		AdvancePipeline(1);
	}


	template<CpuInstruction instr>
	void ShiftVariable(u32 rs, u32 rt, u32 rd)
	{
		using enum CpuInstruction;

		s64 result = static_cast<s64>([&] {
			if constexpr (instr == SLLV) {
				/* Shift Left Logical Variable;
				   Shifts the contents of register rt to the left and inserts 0 to the low-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 5 bits of register rs.
				   Sign-extends (in the 64-bit mode) the result and stores it to register rd. */
				return s32(gpr[rt]) << (gpr[rs] & 0x1F);
			}
			else if constexpr (instr == SRLV) {
				/* Shift Right Logical Variable;
				   Shifts the contents of register rt to the right, and inserts 0 to the high-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 5 bits of register rs.
				   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
				return s32(u32(gpr[rt]) >> (gpr[rs] & 0x1F));
			}
			else if constexpr (instr == SRAV) {
				/* Shift Right Arithmetic Variable;
				   Shifts the contents of register rt to the right and sign-extends the high-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 5 bits of register rs.
				   Sign-extends (in the 64-bit mode) the 32-bit result and stores it to register rd. */
				return s32(gpr[rt] >> (gpr[rs] & 0x1F));
			}
			else if constexpr (instr == DSLLV) {
				/* Doubleword Shift Left Logical Variable;
				   Shifts the contents of register rt to the left, and inserts 0 to the low-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 6 bits of register rs.
				   Stores the 64-bit result and stores it to register rd. */
				return gpr[rt] << (gpr[rs] & 0x3F);
			}
			else if constexpr (instr == DSRLV) {
				/* Doubleword Shift Right Logical Variable;
				   Shifts the contents of register rt to the right, and inserts 0 to the higher bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 6 bits of register rs. */
				return u64(gpr[rt]) >> (gpr[rs] & 0x3F);
			}
			else if constexpr (instr == DSRAV) {
				/* Doubleword Shift Right Arithmetic Variable;
				   Shifts the contents of register rt to the right, and sign-extends the high-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 6 bits of register rs.  */
				return gpr[rt] >> (gpr[rs] & 0x3F);
			}
			else {
				static_assert(AlwaysFalse<instr>);
			}
			}());

		gpr.Set(rd, result);
		AdvancePipeline(1);
	}


	template<CpuInstruction instr>
	void MulDiv(u32 rs, u32 rt)
	{
		using enum CpuInstruction;

		if constexpr (instr == MULT) {
			/* Multiply;
			   Multiplies the contents of register rs by the contents of register rt as a 32-bit
			   signed integer. Sign-extends (in the 64-bit mode) and stores the 64-bit result
			   to special registers HI and LO. */
			s64 result = (gpr[rs] & 0xFFFF'FFFF) * (gpr[rt] & 0xFFFF'FFFF);
			lo_reg = s32(result);
			hi_reg = result >> 32;
		}
		else if constexpr (instr == MULTU) {
			/* Multiply Unsigned;
			   Multiplies the contents of register rs by the contents of register rt as a 32-bit
			   unsigned integer. Sign-extends (in the 64-bit mode) and stores the 64-bit
			   result to special registers HI and LO. */
			u64 result = u64(gpr[rs] & 0xFFFF'FFFF) * u64(gpr[rt] & 0xFFFF'FFFF);
			lo_reg = s32(result);
			hi_reg = s32(result >> 32);
		}
		else if constexpr (instr == DIV) {
			/* Divide;
			   Divides the contents of register rs by the contents of register rt. The operand
			   is treated as a 32-bit signed integer. Sign-extends (in the 64-bit mode) and
			   stores the 32-bit quotient to special register LO and the 32-bit remainder to
			   special register HI. */
			s32 op1 = s32(gpr[rs]);
			s32 op2 = s32(gpr[rt]);
			if (op2 == 0) { /* Peter Lemon N64 CPUTest>CPU>DIV */
				lo_reg = op1 >= 0 ? -1 : 1;
				hi_reg = op1;
			}
			else if (op1 == std::numeric_limits<s32>::min() && op2 == -1) {
				lo_reg = op1;
				hi_reg = 0;
			}
			else {
				lo_reg = op1 / op2;
				hi_reg = op1 % op2;
			}
		}
		else if constexpr (instr == DIVU) {
			/* Divide Unsigned;
			   Divides the contents of register rs by the contents of register rt. The operand
			   is treated as a 32-bit unsigned integer. Sign-extends (in the 64-bit mode) and
			   stores the 32-bit quotient to special register LO and the 32-bit remainder to
			   special register HI. */
			u32 op1 = u32(gpr[rs]);
			u32 op2 = u32(gpr[rt]);
			if (op2 == 0) {
				lo_reg = -1;
				hi_reg = s32(op1);
			}
			else {
				lo_reg = s32(op1 / op2);
				hi_reg = s32(op1 % op2);
			}
		}
		else if constexpr (instr == DMULT) {
			/* Doubleword Multiply;
			   Multiplies the contents of register rs by the contents of register rt as a signed integer.
			   Stores the 128-bit result to special registers HI and LO. */
#if defined _MSC_VER
			   __int64 high_product;
			   __int64 low_product = _mul128(gpr[rs], gpr[rt], &high_product);
			   lo_reg = low_product;
			   hi_reg = high_product;
#elif defined __clang__ || defined __GNUC__
			__int128 product = __int128(gpr[rs]) * __int128(gpr[rt]);
			lo_reg = product & 0xFFFF'FFFF'FFFF'FFFF;
			hi_reg = product >> 64;
#else
			/* TODO */
			static_assert(AlwaysFalse<instr>);
#endif
		}
		else if constexpr (instr == DMULTU) {
			/* Doubleword Multiply Unsigned;
			   Multiplies the contents of register rs by the contents of register rt as an unsigned integer.
			   Stores the 128-bit result to special registers HI and LO. */
#if defined _MSC_VER
			   unsigned __int64 high_product;
			   unsigned __int64 low_product = _umul128(gpr[rs], gpr[rt], &high_product);
			   lo_reg = low_product;
			   hi_reg = high_product;
#elif defined __clang__ || defined __GNUC__
			unsigned __int128 product = unsigned __int128(gpr[rs]) * unsigned __int128(gpr[rt]);
			lo_reg = product & 0xFFFF'FFFF'FFFF'FFFF;
			hi_reg = product >> 64;
#else
			/* TODO */
			static_assert(AlwaysFalse<instr>);
#endif
		}
		else if constexpr (instr == DDIV) {
			/* Doubleword Divide;
			   Divides the contents of register rs by the contents of register rt.
			   The operand is treated as a signed integer.
			   Stores the 64-bit quotient to special register LO, and the 64-bit remainder to special register HI. */
			s64 op1 = gpr[rs];
			s64 op2 = gpr[rt];
			if (op2 == 0) { /* Peter Lemon N64 CPUTest>CPU>DDIV */
				lo_reg = op1 >= 0 ? -1 : 1;
				hi_reg = op1;
			}
			else if (op1 == std::numeric_limits<s64>::min() && op2 == -1) {
				lo_reg = op1;
				hi_reg = 0;
			}
			else {
				lo_reg = op1 / op2;
				hi_reg = op1 % op2;
			}
		}
		else if constexpr (instr == DDIVU) {
			/* Doubleword Divide Unsigned;
			   Divides the contents of register rs by the contents of register rt.
			   The operand is treated as an unsigned integer.
			   Stores the 64-bit quotient to special register LO, and the 64-bit remainder to special register HI. */
			u64 op1 = u64(gpr[rs]);
			u64 op2 = u64(gpr[rt]);
			if (op2 == 0) {
				lo_reg = -1;
				hi_reg = op1;
			}
			else {
				lo_reg = op1 / op2;
				hi_reg = op1 % op2;
			}
		}
		else {
			static_assert(AlwaysFalse<instr>);
		}
		static constexpr int cycles = [&] {
			     if constexpr (OneOf(instr, MULT, MULTU))   return 5;
			else if constexpr (OneOf(instr, DMULT, DMULTU)) return 8;
			else if constexpr (OneOf(instr, DIV, DIVU))     return 37;
			else if constexpr (OneOf(instr, DDIV, DDIVU))   return 69;
			else static_assert(AlwaysFalse<instr>);
		}();
		AdvancePipeline(cycles);
	}


	template<CpuInstruction instr>
	void Branch(u32 rs, s16 imm16)
	{
		using enum CpuInstruction;

		bool branch_cond = [&] {
			if constexpr (OneOf(instr, BLEZ, BLEZL))          return gpr[rs] <= 0;
			else if constexpr (OneOf(instr, BGTZ, BGTZL))     return gpr[rs] > 0;
			else if constexpr (OneOf(instr, BLTZ, BLTZL))     return gpr[rs] < 0;
			else if constexpr (OneOf(instr, BGEZ, BGEZL))     return gpr[rs] >= 0;
			else if constexpr (OneOf(instr, BLTZAL, BLTZALL)) return gpr[rs] < 0;
			else if constexpr (OneOf(instr, BGEZAL, BGEZALL)) return gpr[rs] >= 0;
			else static_assert(AlwaysFalse<instr>);
		}();
		if constexpr (OneOf(instr, BLTZAL, BGEZAL, BLTZALL, BGEZALL)) {
			gpr.Set(31, 4 + (in_branch_delay_slot ? addr_to_jump_to : pc));
		}
		if (branch_cond) {
			auto offset = imm16 << 2;
			PrepareJump(pc + offset);
		}
		else if constexpr (OneOf(instr, BLEZL, BGTZL, BEQL, BLTZL, BGEZL, BLTZALL, BGEZALL)) {
			pc += 4; /* The instruction in the branch delay slot is discarded. */
		}

		AdvancePipeline(1);
	}


	template<CpuInstruction instr>
	void Branch(u32 rs, u32 rt, s16 imm16)
	{
		using enum CpuInstruction;

		bool branch_cond = [&] {
			if constexpr (OneOf(instr, BEQ, BEQL))      return gpr[rs] == gpr[rt];
			else if constexpr (OneOf(instr, BNE, BNEL)) return gpr[rs] != gpr[rt];
			else static_assert(AlwaysFalse<instr>);
		}();
		if (branch_cond) {
			auto offset = imm16 << 2;
			PrepareJump(pc + offset);
		}
		else if constexpr (OneOf(instr, BEQL, BNEL)) {
			pc += 4; /* The instruction in the branch delay slot is discarded. */
		}

		AdvancePipeline(1);
	}


	template<CpuInstruction instr>
	void TrapThreeOperand(u32 rs, u32 rt)
	{
		using enum CpuInstruction;

		/* Generates a trap exception if the given condition is true. */
		bool cond = [&] {
			     if constexpr (instr == TGE)  return s64(gpr[rs]) >= s64(gpr[rt]);
			else if constexpr (instr == TGEU) return u64(gpr[rs]) >= u64(gpr[rt]);
			else if constexpr (instr == TLT)  return s64(gpr[rs]) <  s64(gpr[rt]);
			else if constexpr (instr == TLTU) return u64(gpr[rs]) <  u64(gpr[rt]);
			else if constexpr (instr == TEQ)  return gpr[rs] == gpr[rt];
			else if constexpr (instr == TNE)  return gpr[rs] != gpr[rt];
			else static_assert(AlwaysFalse<instr>);
		}();

		if (cond) {
			SignalException<Exception::Trap>();
		}

		AdvancePipeline(1);
	}


	template<CpuInstruction instr>
	void TrapImmediate(u32 rs, s16 imm16)
	{
		using enum CpuInstruction;

		/* Generates a trap exception if the given condition is true. */
		bool trap_cond = [&] {
			     if constexpr (instr == TGEI)  return s64(gpr[rs]) >= imm16;
			else if constexpr (instr == TGEIU) return u64(gpr[rs]) >= imm16;
			else if constexpr (instr == TLTI)  return s64(gpr[rs]) <  imm16;
			else if constexpr (instr == TLTIU) return u64(gpr[rs]) <  imm16;
			else if constexpr (instr == TEQI)  return s64(gpr[rs]) == imm16;
			else if constexpr (instr == TNEI)  return s64(gpr[rs]) != imm16;
			else static_assert(AlwaysFalse<instr>);
		}();

		if (trap_cond) {
			SignalException<Exception::Trap>();
		}

		AdvancePipeline(1);
	}


	void J(u32 imm26)
	{
		if (!in_branch_delay_slot) {
			u64 target = pc & 0xFFFF'FFFF'F000'0000 | imm26 << 2;
			PrepareJump(target);
		}
		AdvancePipeline(1);
	}


	void JAL(u32 imm26)
	{
		if (!in_branch_delay_slot) {
			u64 target = pc & 0xFFFF'FFFF'F000'0000 | imm26 << 2;
			PrepareJump(target);
		}
		gpr.Set(31, 4 + (in_branch_delay_slot ? addr_to_jump_to : pc));
		AdvancePipeline(1);
	}


	void JR(u32 rs)
	{
		if (!in_branch_delay_slot) {
			u64 target = gpr[rs];
			if (target & 3) {
				SignalAddressErrorException<MemOp::InstrFetch>(target);
			}
			else {
				PrepareJump(target);
			}
		}
		AdvancePipeline(1);
	}


	void JALR(u32 rs, u32 rd)
	{
		if (!in_branch_delay_slot) {
			u64 target = gpr[rs];
			if (target & 3) {
				SignalAddressErrorException<MemOp::InstrFetch>(target);
			}
			else {
				PrepareJump(target);
			}
		}
		gpr.Set(rd, 4 + (in_branch_delay_slot ? addr_to_jump_to : pc));
		AdvancePipeline(1);
	}


	void MFLO(u32 rd)
	{
		gpr.Set(rd, lo_reg);
		AdvancePipeline(1);
	}


	void MFHI(u32 rd)
	{
		gpr.Set(rd, hi_reg);
		AdvancePipeline(1);
	}


	void MTLO(u32 rs)
	{
		lo_reg = gpr[rs];
		AdvancePipeline(1);
	}


	void MTHI(u32 rs)
	{
		hi_reg = gpr[rs];
		AdvancePipeline(1);
	}


	void SYNC()
	{
		/* Synchronize;
		   Completes the Load/store instruction currently in the pipeline before the new
		   Load/store instruction is executed. Is executed as a NOP on the VR4300. */
		AdvancePipeline(1);
	}


	void SYSCALL()
	{
		/* System Call;
		   Generates a system call exception and transfers control to the exception processing program. */
		SignalException<Exception::Syscall>();
		AdvancePipeline(1);
	}


	void BREAK()
	{
		/* Breakpoint;
		   Generates a breakpoint exception and transfers control to the exception processing program. */
		SignalException<Exception::Breakpoint>();
		AdvancePipeline(1);
	}


	template void Load<CpuInstruction::LB>(u32, u32, s16);
	template void Load<CpuInstruction::LBU>(u32, u32, s16);
	template void Load<CpuInstruction::LH>(u32, u32, s16);
	template void Load<CpuInstruction::LHU>(u32, u32, s16);
	template void Load<CpuInstruction::LW>(u32, u32, s16);
	template void Load<CpuInstruction::LWU>(u32, u32, s16);
	template void Load<CpuInstruction::LWL>(u32, u32, s16);
	template void Load<CpuInstruction::LWR>(u32, u32, s16);
	template void Load<CpuInstruction::LD>(u32, u32, s16);
	template void Load<CpuInstruction::LDL>(u32, u32, s16);
	template void Load<CpuInstruction::LDR>(u32, u32, s16);
	template void Load<CpuInstruction::LL>(u32, u32, s16);
	template void Load<CpuInstruction::LLD>(u32, u32, s16);

	template void Store<CpuInstruction::SB>(u32, u32, s16);
	template void Store<CpuInstruction::SH>(u32, u32, s16);
	template void Store<CpuInstruction::SW>(u32, u32, s16);
	template void Store<CpuInstruction::SWL>(u32, u32, s16);
	template void Store<CpuInstruction::SWR>(u32, u32, s16);
	template void Store<CpuInstruction::SC>(u32, u32, s16);
	template void Store<CpuInstruction::SCD>(u32, u32, s16);
	template void Store<CpuInstruction::SD>(u32, u32, s16);
	template void Store<CpuInstruction::SDL>(u32, u32, s16);
	template void Store<CpuInstruction::SDR>(u32, u32, s16);

	template void AluImmediate<CpuInstruction::ADDI>(u32, u32, s16);
	template void AluImmediate<CpuInstruction::ADDIU>(u32, u32, s16);
	template void AluImmediate<CpuInstruction::SLTI>(u32, u32, s16);
	template void AluImmediate<CpuInstruction::SLTIU>(u32, u32, s16);
	template void AluImmediate<CpuInstruction::ANDI>(u32, u32, s16);
	template void AluImmediate<CpuInstruction::ORI>(u32, u32, s16);
	template void AluImmediate<CpuInstruction::XORI>(u32, u32, s16);
	template void AluImmediate<CpuInstruction::LUI>(u32, u32, s16);
	template void AluImmediate<CpuInstruction::DADDI>(u32, u32, s16);
	template void AluImmediate<CpuInstruction::DADDIU>(u32, u32, s16);

	template void AluThreeOperand<CpuInstruction::ADD>(u32, u32, u32);
	template void AluThreeOperand<CpuInstruction::ADDU>(u32, u32, u32);
	template void AluThreeOperand<CpuInstruction::SUB>(u32, u32, u32);
	template void AluThreeOperand<CpuInstruction::SUBU>(u32, u32, u32);
	template void AluThreeOperand<CpuInstruction::SLT>(u32, u32, u32);
	template void AluThreeOperand<CpuInstruction::SLTU>(u32, u32, u32);
	template void AluThreeOperand<CpuInstruction::AND>(u32, u32, u32);
	template void AluThreeOperand<CpuInstruction::OR>(u32, u32, u32);
	template void AluThreeOperand<CpuInstruction::XOR>(u32, u32, u32);
	template void AluThreeOperand<CpuInstruction::NOR>(u32, u32, u32);
	template void AluThreeOperand<CpuInstruction::DADD>(u32, u32, u32);
	template void AluThreeOperand<CpuInstruction::DADDU>(u32, u32, u32);
	template void AluThreeOperand<CpuInstruction::DSUB>(u32, u32, u32);
	template void AluThreeOperand<CpuInstruction::DSUBU>(u32, u32, u32);

	template void Shift<CpuInstruction::SLL>(u32, u32, u32);
	template void Shift<CpuInstruction::SRL>(u32, u32, u32);
	template void Shift<CpuInstruction::SRA>(u32, u32, u32);
	template void Shift<CpuInstruction::DSLL>(u32, u32, u32);
	template void Shift<CpuInstruction::DSRL>(u32, u32, u32);
	template void Shift<CpuInstruction::DSRA>(u32, u32, u32);
	template void Shift<CpuInstruction::DSLL32>(u32, u32, u32);
	template void Shift<CpuInstruction::DSRL32>(u32, u32, u32);
	template void Shift<CpuInstruction::DSRA32>(u32, u32, u32);

	template void ShiftVariable<CpuInstruction::SLLV>(u32, u32, u32);
	template void ShiftVariable<CpuInstruction::SRLV>(u32, u32, u32);
	template void ShiftVariable<CpuInstruction::SRAV>(u32, u32, u32);
	template void ShiftVariable<CpuInstruction::DSLLV>(u32, u32, u32);
	template void ShiftVariable<CpuInstruction::DSRLV>(u32, u32, u32);
	template void ShiftVariable<CpuInstruction::DSRAV>(u32, u32, u32);

	template void MulDiv<CpuInstruction::MULT>(u32, u32);
	template void MulDiv<CpuInstruction::MULTU>(u32, u32);
	template void MulDiv<CpuInstruction::DIV>(u32, u32);
	template void MulDiv<CpuInstruction::DIVU>(u32, u32);
	template void MulDiv<CpuInstruction::DMULT>(u32, u32);
	template void MulDiv<CpuInstruction::DMULTU>(u32, u32);
	template void MulDiv<CpuInstruction::DDIV>(u32, u32);
	template void MulDiv<CpuInstruction::DDIVU>(u32, u32);

	template void Branch<CpuInstruction::BLEZ>(u32, s16);
	template void Branch<CpuInstruction::BGTZ>(u32, s16);
	template void Branch<CpuInstruction::BLTZ>(u32, s16);
	template void Branch<CpuInstruction::BGEZ>(u32, s16);
	template void Branch<CpuInstruction::BLTZAL>(u32, s16);
	template void Branch<CpuInstruction::BGEZAL>(u32, s16);
	template void Branch<CpuInstruction::BLEZL>(u32, s16);
	template void Branch<CpuInstruction::BGTZL>(u32, s16);
	template void Branch<CpuInstruction::BLTZL>(u32, s16);
	template void Branch<CpuInstruction::BGEZL>(u32, s16);
	template void Branch<CpuInstruction::BLTZALL>(u32, s16);
	template void Branch<CpuInstruction::BGEZALL>(u32, s16);

	template void Branch<CpuInstruction::BEQ>(u32, u32, s16);
	template void Branch<CpuInstruction::BNE>(u32, u32, s16);
	template void Branch<CpuInstruction::BEQL>(u32, u32, s16);
	template void Branch<CpuInstruction::BNEL>(u32, u32, s16);

	template void TrapThreeOperand<CpuInstruction::TGE>(u32, u32);
	template void TrapThreeOperand<CpuInstruction::TGEU>(u32, u32);
	template void TrapThreeOperand<CpuInstruction::TLT>(u32, u32);
	template void TrapThreeOperand<CpuInstruction::TLTU>(u32, u32);
	template void TrapThreeOperand<CpuInstruction::TEQ>(u32, u32);
	template void TrapThreeOperand<CpuInstruction::TNE>(u32, u32);

	template void TrapImmediate<CpuInstruction::TGEI>(u32, s16);
	template void TrapImmediate<CpuInstruction::TGEIU>(u32, s16);
	template void TrapImmediate<CpuInstruction::TLTI>(u32, s16);
	template void TrapImmediate<CpuInstruction::TLTIU>(u32, s16);
	template void TrapImmediate<CpuInstruction::TEQI>(u32, s16);
	template void TrapImmediate<CpuInstruction::TNEI>(u32, s16);
}