module VR4300:CPU;

import :COP0;
import :Exceptions;
import :MMU;
import :Operation;

import DebugOptions;
import Memory;
import Util;

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


	template<CPUInstruction instr>
	void CPULoad(const u32 instr_code)
	{
		using enum CPUInstruction;

		s16 offset = instr_code & 0xFFFF;
		auto rt = instr_code >> 16 & 0x1F;
		auto base = instr_code >> 21 & 0x1F;
		auto addr = gpr[base] + offset;

		/* For all instructions:
		   Generates an address by adding a sign-extended offset to the contents of register base.
		   In the 32-bit Kernel mode, the high-order 32 bits are ignored during
		   virtual address creation. */
		if constexpr (OneOf(instr, LD, LDL, LDR, LLD)) {
			if (operating_mode == OperatingMode::Kernel && addressing_mode == AddressingMode::_32bit) {
				addr = s32(addr & 0xFFFF'FFFF);
			}
		}
		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{} {}, ${:X}",
				current_instr_name, rt, MakeUnsigned(addr));
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
				return ReadVirtual<u8>(addr);
			}
			else if constexpr (instr == LH) {
				/* Load halfword;
				   Sign-extends the contents of a halfword specified by the address and loads the result to register rt. */
				return ReadVirtual<s16>(addr);
			}
			else if constexpr (instr == LHU) {
				/* Load Halfword Unsigned;
				   Zero-extends the contents of a halfword specified by the address and loads the result to register rt. */
				return ReadVirtual<u16>(addr);
			}
			else if constexpr (instr == LW) {
				/* Load Word;
				   Sign-extends the contents of a word specified by the address and loads the result to register rt. */
				return ReadVirtual<s32>(addr);
			}
			else if constexpr (instr == LWU) {
				/* Load Word Unsigned;
				   Zero-extends the contents of a word specified by the address and loads the result to register rt. */
				return ReadVirtual<u32>(addr);
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
				return ReadVirtual<u64>(addr);
			}
			else if constexpr (instr == LDL) {
				/* Load Doubleword Left;
				   Shifts the doubleword specified by the address to the left so that the byte
				   specified by the address is at the leftmost position of the doubleword.
				   Merges the result of the shift and the contents of register rt, and loads the
				   result to register rt. */
				return ReadVirtual<u64, Alignment::UnalignedLeft>(addr);
			}
			else if constexpr (instr == LDR) {
				/* Load Doubleword Right;
				   Shifts the doubleword specified by the address to the right so that the byte
				   specified by the address is at the rightmost position of the doubleword.
				   Merges the result of the shift and the contents of register rt, and loads the
				   result to register rt. */
				return ReadVirtual<u64, Alignment::UnalignedRight>(addr);
			}
			else if constexpr (instr == LL) {
				/* Load Linked;
				   Loads the contents of the word specified by the address to register rt and sets the LL bit to 1.
				   Additionally, the specified physical address of the memory is stored to the LLAddr register. */
				s32 ret = ReadVirtual<s32>(addr);
				cop0_reg.ll_addr.p_addr = last_physical_address_on_load;
				ll_bit = 1;
				return ret;
			}
			else if constexpr (instr == LLD) {
				/* Load Linked Doubleword;
				   Loads the contents of the doubleword specified by the address to register rt and sets the LL bit to 1.
				   Additionally, the specified physical address of the memory is stored to the LLAddr register. */
				s64 ret = ReadVirtual<s64>(addr);
				cop0_reg.ll_addr.p_addr = last_physical_address_on_load;
				ll_bit = 1;
				return ret;
			}
			else {
				static_assert(AlwaysFalse<instr>, "\"Load\" template function called, but no matching load instruction was found.");
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
		if constexpr (instr == LWL) {
			u32 bits_from_last_boundary = (addr & 3) << 3;
			result <<= bits_from_last_boundary;
			s32 untouched_gpr = s32(gpr.Get(rt) & ((1 << bits_from_last_boundary) - 1));
			gpr.Set(rt, result | untouched_gpr);
			/* To access data not aligned at a boundary, an additional 1P cycle is necessary as compared when accessing data aligned at a boundary. */
			AdvancePipeline(bits_from_last_boundary > 0 ? 2 : 1);
		}
		else if constexpr (instr == LDL) {
			u32 bits_from_last_boundary = (addr & 7) << 3;
			result <<= bits_from_last_boundary;
			u64 untouched_gpr = gpr.Get(rt) & ((1ll << bits_from_last_boundary) - 1);
			gpr.Set(rt, result | untouched_gpr);
			AdvancePipeline(bits_from_last_boundary > 0 ? 2 : 1);
		}
		else if constexpr (instr == LWR)
		{
			u32 bytes_from_last_boundary = addr & 3;
			result >>= 8 * (3 - bytes_from_last_boundary);
			s32 untouched_gpr = s32(gpr.Get(rt) & right_load_mask[bytes_from_last_boundary]);
			gpr.Set(rt, result | untouched_gpr);
			AdvancePipeline(bytes_from_last_boundary > 0 ? 2 : 1);
		}
		else if constexpr (instr == LDR)
		{
			u32 bytes_from_last_boundary = addr & 7;
			result >>= 8 * (7 - bytes_from_last_boundary);
			u64 untouched_gpr = gpr.Get(rt) & right_load_mask[bytes_from_last_boundary];
			gpr.Set(rt, result | untouched_gpr);
			AdvancePipeline(bytes_from_last_boundary > 0 ? 2 : 1);
		}
		else { /* Aligned read */
			gpr.Set(rt, result);
			AdvancePipeline(1);
		}
	}


	template<CPUInstruction instr>
	void CPUStore(const u32 instr_code)
	{
		using enum CPUInstruction;

		s16 offset = instr_code & 0xFFFF;
		auto rt = instr_code >> 16 & 0x1F;
		auto base = instr_code >> 21 & 0x1F;
		auto addr = gpr[base] + offset;

		/* For all instructions:
		   Generates an address by adding a sign-extended offset to the contents of register base.
		   For "doubleword instructions", in the 32-bit Kernel mode, the high-order 32 bits are ignored during
		   virtual address creation. */
		if constexpr (OneOf(instr, SD, SDL, SDR, SCD)) {
			if (operating_mode == OperatingMode::Kernel && addressing_mode == AddressingMode::_32bit) {
				addr = s32(addr & 0xFFFF'FFFF);
			}
		}

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{} {}, ${:X}",
				current_instr_name, rt, MakeUnsigned(addr));
		}

		if constexpr (instr == SB) {
			/* Store Byte;
			   Stores the contents of the low-order byte of register rt to the memory specified by the address. */
			WriteVirtual<s8>(addr, s8(gpr[rt]));
		}
		else if constexpr (instr == SH) {
			/* Store Halfword;
			   Stores the contents of the low-order halfword of register rt to the memory specified by the address. */
			WriteVirtual<s16>(addr, s16(gpr[rt]));
		}
		else if constexpr (instr == SW) {
			/* Store Word;
			   Stores the contents of the low-order word of register rt to the memory specified by the address. */
			WriteVirtual<s32>(addr, s32(gpr[rt]));
		}
		else if constexpr (instr == SWL)
		{
			/* Store Word Left;
			   Shifts the contents of register rt to the right so that the leftmost byte of the
			   word is at the position of the byte specified by the address. Stores the result
			   of the shift to the lower portion of the word in memory. */
			s32 data_to_write = s32(gpr[rt] & ~((1 << (8 * (addr & 3))) - 1));
			WriteVirtual<s32, Alignment::UnalignedLeft>(addr, data_to_write);
		}
		else if constexpr (instr == SWR) {
			/* Store Word Right;
			   Shifts the contents of register rt to the left so that the rightmost byte of the
			   word is at the position of the byte specified by the address. Stores the result
			   of the shift to the higher portion of the word in memory. */
			s32 data_to_write = s32(gpr[rt] << (8 * (3 - (addr & 3))));
			WriteVirtual<s32, Alignment::UnalignedRight>(addr, data_to_write);
		}
		else if constexpr (instr == SD) {
			/* Store Doublword;
			   Stores the contents of register rt to the memory specified by the address. */
			WriteVirtual<s64>(addr, gpr[rt]);
		}
		else if constexpr (instr == SDL) {
			/* Store Doubleword Left;
			   Shifts the contents of register rt to the right so that the leftmost byte of a
			   doubleword is at the position of the byte specified by the address. Stores the
			   result of the shift to the lower portion of the doubleword in memory. */
			s64 data_to_write = gpr[rt] & ~((1ll << (8 * (addr & 7))) - 1);
			WriteVirtual<s64, Alignment::UnalignedLeft>(addr, data_to_write);
		}
		else if constexpr (instr == SDR) {
			/* Store Doubleword Right;
			   Shifts the contents of register rt to the left so that the rightmost byte of a
			   doubleword is at the position of the byte specified by the address. Stores the
			   result of the shift to the higher portion of the doubleword in memory. */
			s64 data_to_write = gpr[rt] << (8 * (7 - (addr & 7)));
			WriteVirtual<s64, Alignment::UnalignedRight>(addr, data_to_write);
		}
		else if constexpr (instr == SC) {
			/* Store Conditional;
			   If the LL bit is 1, stores the contents of the low-order word of register rt to
			   the memory specified by the address, and sets register rt to 1.
			   If the LL bit is 0, does not store the contents of the word, and clears register
			   rt to 0. */
			if (ll_bit == 1) {
				WriteVirtual<s32>(addr, s32(gpr[rt]));
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
				WriteVirtual<s64>(addr, gpr[rt]);
				gpr.Set(rt, 1);
			}
			else {
				gpr.Set(rt, 0);
			}
		}
		else {
			static_assert(AlwaysFalse<instr>, "\"Store\" template function called, but no matching store instruction was found.");
		}

		AdvancePipeline(1);
	}


	template<CPUInstruction instr>
	void ALUImmediate(const u32 instr_code)
	{
		using enum CPUInstruction;

		auto rt = instr_code >> 16 & 0x1F;
		auto rs = instr_code >> 21 & 0x1F;

		auto immediate = [&] {
			if constexpr (OneOf(instr, ANDI, LUI, ORI, XORI))
				return u16(instr_code & 0xFFFF);
			else
				return s16(instr_code & 0xFFFF);
		}();

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = [&] {
				if constexpr (instr == LUI)
					return std::format("{} {}, ${:X}", current_instr_name, rt, immediate);
				else
					return std::format("{} {}, {}, ${:X}", current_instr_name, rt, rs, immediate);
			}();
		}

		if constexpr (instr == ADDI) {
			/* Add Immediate;
			   Sign-extends the 16-bit immediate and adds it to register rs. Stores the
			   32-bit result to register rt (sign-extends the result in the 64-bit mode).
			   Generates an exception if a 2's complement integer overflow occurs.
			   In that case, the contents of destination register rt are not modified */
			s32 sum = s32(gpr[rs] + immediate);
			bool overflow = (gpr[rs] ^ sum) & (immediate ^ sum) & 0x8000'0000;
			if (overflow) {
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
			s32 sum = s32(gpr[rs] + immediate);
			gpr.Set(rt, sum);
		}
		else if constexpr (instr == SLTI) {
			/* Set On Less Than Immediate;
			   Sign-extends the 16-bit immediate and compares it with register rs as a
			   signed integer. If rs is less than the immediate, stores 1 to register rt;
			   otherwise, stores 0 to register rt. */
			gpr.Set(rt, gpr[rs] < immediate);
		}
		else if constexpr (instr == SLTIU) {
			/* Set On Less Than Immediate Unsigned;
			   Sign-extends the 16-bit immediate and compares it with register rs as an
			   unsigned integer. If rs is less than the immediate, stores 1 to register rt;
			   otherwise, stores 0 to register rt. */
			gpr.Set(rt, u64(gpr[rs]) < immediate);
		}
		else if constexpr (instr == ANDI) {
			/* And Immediate;
			   Zero-extends the 16-bit immediate, ANDs it with register rs, and stores the
			   result to register rt. */
			gpr.Set(rt, gpr[rs] & immediate);
		}
		else if constexpr (instr == ORI) {
			/* Or Immediate;
			   Zero-extends the 16-bit immediate, ORs it with register rs, and stores the
			   result to register rt. */
			gpr.Set(rt, gpr[rs] | immediate);
		}
		else if constexpr (instr == XORI) {
			/* Exclusive Or Immediate;
			   Zero-extends the 16-bit immediate, exclusive-ORs it with register rs, and
			   stores the result to register rt. */
			gpr.Set(rt, gpr[rs] ^ immediate);
		}
		else if constexpr (instr == LUI) {
			/* Load Upper Immediate;
			   Shifts the 16-bit immediate 16 bits to the left, and clears the low-order 16 bits
			   of the word to 0.
			   Stores the result to register rt (by sign-extending the result in the 64-bit mode). */
			s32 result = immediate << 16;
			gpr.Set(rt, result);
		}
		else if constexpr (instr == DADDI) {
			/* Doubleword Add Immediate;
			   Sign-extends the 16-bit immediate to 64 bits, and adds it to register rs. Stores
			   the 64-bit result to register rt. Generates an exception if an integer overflow occurs. */
			s64 sum = gpr[rs] + immediate;
			bool overflow = (gpr[rs] ^ sum) & (immediate ^ sum) & 0x8000'0000'0000'0000;
			if (overflow) {
				SignalException<Exception::IntegerOverflow>();
			}
			else {
				gpr.Set(rt, sum);
			}
		}
		else if constexpr (instr == DADDIU){
			/* Doubleword Add Immediate Unsigned;
			   Sign-extends the 16-bit immediate to 64 bits, and adds it to register rs. Stores
			   the 64-bit result to register rt. Does not generate an exception even if an
			   integer overflow occurs. */
			s64 sum = gpr[rs] + immediate;
			gpr.Set(rt, sum);
		}
		else {
			static_assert(AlwaysFalse<instr>, "\"ALU_Immediate\" template function called, but no matching ALU immediate instruction was found.");
		}

		AdvancePipeline(1);
	}

	template<CPUInstruction instr>
	void ALUThreeOperand(const u32 instr_code)
	{
		using enum CPUInstruction;

		auto rd = instr_code >> 11 & 0x1F;
		auto rt = instr_code >> 16 & 0x1F;
		auto rs = instr_code >> 21 & 0x1F;

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{} {}, {}, {}", current_instr_name, rd, rs, rt);
		}

		if constexpr (instr == ADD) {
			/* Add;
			   Adds the contents of register rs and rt, and stores (sign-extends in the 64-bit mode)
			   the 32-bit result to register rd. Generates an exception if an integer overflow occurs.
			   In that case, the contents of the destination register rd are not modified.
			   In 64-bit mode, the operands must be sign-extended, 32-bit values. */
			s32 sum = s32(gpr[rs] + gpr[rt]);
			bool overflow = (gpr[rs] ^ sum) & (gpr[rt] ^ sum) & 0x8000'0000;
			if (overflow) {
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
			s32 sum = s32(gpr[rs] + gpr[rt]);
			gpr.Set(rd, sum);
		}
		else if constexpr (instr == SUB) {
			/* Subtract;
			   Subtracts the contents of register rs from register rt, and stores (sign-extends
			   in the 64-bit mode) the result to register rd. Generates an exception if an integer overflow occurs. */
			s32 sum = s32(gpr[rs] - gpr[rt]);
			bool overflow = (gpr[rs] ^ gpr[rt]) & ~(gpr[rt] ^ sum) & 0x8000'0000;
			if (overflow) {
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
			s32 sum = s32(gpr[rs] - gpr[rt]);
			gpr.Set(rd, sum);
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
			bool overflow = (gpr[rs] ^ sum) & (gpr[rt] ^ sum) & 0x8000'0000'0000'0000;
			if (overflow) {
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
			s64 sum = gpr[rs] + gpr[rt];
			gpr.Set(rd, sum);
		}
		else if constexpr (instr == DSUB) {
			/* Doubleword Subtract;
			   Subtracts the contents of register rt from register rs, and stores the 64-bit
			   result to register rd. Generates an exception if an integer overflow occurs. */
			s64 sum = gpr[rs] - gpr[rt];
			bool overflow = (gpr[rs] ^ gpr[rt]) & ~(gpr[rt] ^ sum) & 0x8000'0000'0000'0000;
			if (overflow) {
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
			s64 sum = gpr[rs] - gpr[rt];
			gpr.Set(rd, sum);
		}
		else {
			static_assert(AlwaysFalse<instr>, "\"ALU_ThreeOperand\" template function called, but no matching ALU three-operand instruction was found.");
		}

		AdvancePipeline(1);
	}


	template<CPUInstruction instr>
	void ALUShift(const u32 instr_code)
	{
		using enum CPUInstruction;

		auto sa = instr_code >>  6 & 0x1F;
		auto rd = instr_code >> 11 & 0x1F;
		auto rt = instr_code >> 16 & 0x1F;
		auto rs = instr_code >> 21 & 0x1F;

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = [&] {
				if (instr_code == 0)
					return std::string("NOP");
				if constexpr (OneOf(instr, SLLV, SRLV, SRAV, DSLLV, DSRLV, DSRAV))
					return std::format("{} {}, {}, {}", current_instr_name, rd, rt, rs);
				else
					return std::format("{} {}, {}, {}", current_instr_name, rd, rt, sa);
			}();
		}

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
			else if constexpr (instr == SLLV) {
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
				static_assert(AlwaysFalse<instr>, "\"ALU_Shift\" template function called, but no matching ALU shift instruction was found.");
			}
		}());

		gpr.Set(rd, result);
		AdvancePipeline(1);
	}


	template<CPUInstruction instr>
	void ALUMulDiv(const u32 instr_code)
	{
		using enum CPUInstruction;

		auto rt = instr_code >> 16 & 0x1F;
		auto rs = instr_code >> 21 & 0x1F;

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{} {}, {}", current_instr_name, rs, rt);
		}

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
				lo_reg = op1 > 0 ? -1 : 1;
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
				hi_reg = op1;
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
			assert(false);
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
			assert(false);
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
				lo_reg = op1 > 0 ? -1 : 1;
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
			static_assert(AlwaysFalse<instr>, "\"ALU_MulDiv\" template function called, but no matching ALU mul/div instruction was found.");
		}
		static constexpr int cycles = [&] {
			     if constexpr (OneOf(instr, MULT, MULTU))   return 5;
			else if constexpr (OneOf(instr, DMULT, DMULTU)) return 8;
			else if constexpr (OneOf(instr, DIV, DIVU))     return 37;
			else if constexpr (OneOf(instr, DDIV, DDIVU))   return 69;
			else static_assert(AlwaysFalse<instr>, "\"ALU_MulDiv\" template function called, but no matching ALU mul/div instruction was found.");
		}();
		AdvancePipeline(cycles);
	}


	template<CPUInstruction instr>
	void Jump(const u32 instr_code)
	{
		using enum CPUInstruction;

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
		u64 target = [&] {
			if constexpr (OneOf(instr, J, JAL)) {
				u64 target = u64((instr_code & 0x03FF'FFFF) << 2);
				if constexpr (log_cpu_instructions) {
					current_instr_log_output = std::format("{} ${:X}", current_instr_name, target);
				}
				return pc & 0xFFFF'FFFF'F000'0000 | target;
			}
			else if constexpr (OneOf(instr, JR, JALR)) {
				auto rs = instr_code >> 21 & 0x1F;
				if constexpr (log_cpu_instructions) {
					current_instr_log_output = std::format("{} {}", current_instr_name, rs);
				}
				if (gpr[rs] & 3) {
					SignalAddressErrorException<Memory::Operation::InstrFetch>(pc);
				}
				return gpr[rs];
			}
			else {
				static_assert(AlwaysFalse<instr>, "\"Jump\" template function called, but no matching jump instruction was found.");
			}
		}();

		PrepareJump(target);

		if constexpr (instr == JAL) {
			gpr.Set(31, pc + 4);
		}
		else if constexpr (instr == JALR) {
			auto rd = instr_code >> 11 & 0x1F;
			gpr.Set(rd, pc + 4);
		}

		AdvancePipeline(1);
	}


	template<CPUInstruction instr>
	void CPUBranch(const u32 instr_code)
	{
		using enum CPUInstruction;

		auto rt = instr_code >> 16 & 0x1F;
		auto rs = instr_code >> 21 & 0x1F;

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = [&] {
				s16 offset = instr_code & 0xFFFF;
				if constexpr (OneOf(instr, BEQ, BNE, BEQL, BNEL))
					return std::format("{} {}, {}, ${:X}", current_instr_name, rs, rt, offset);
				else
					return std::format("{} {}, ${:X}", current_instr_name, rs, offset);
			}();
		}

		/* For all instructions: branch to the branch address if the condition is met, with a delay of one instruction.
		   For "likely" instructions: if the branch condition is not satisfied, the instruction in the branch delay slot is discarded.
		   For "link" instructions: stores the address of the instruction following the delay slot to register r31 (link register). */

		if constexpr (OneOf(instr, BLTZAL, BGEZAL, BLTZALL, BGEZALL)) {
			gpr.Set(31, pc + 4);
		}

		bool branch_cond = [&] {
			if constexpr (OneOf(instr, BEQ, BEQL)) /* Branch On Equal (Likely) */
				return gpr[rs] == gpr[rt];
			else if constexpr (OneOf(instr, BNE, BNEL)) /* Branch On Not Equal (Likely) */
				return gpr[rs] != gpr[rt];
			else if constexpr (OneOf(instr, BLEZ, BLEZL)) /* Branch On Less Than Or Equal To Zero (Likely) */
				return gpr[rs] <= 0;
			else if constexpr (OneOf(instr, BGTZ, BGTZL)) /* Branch On Greater Than Zero (Likely) */
				return gpr[rs] > 0;
			else if constexpr (OneOf(instr, BLTZ, BLTZL)) /* Branch On Less Than Zero (Likely) */
				return gpr[rs] < 0;
			else if constexpr (OneOf(instr, BGEZ, BGEZL)) /* Branch On Greater Than or Equal To Zero (Likely) */
				return gpr[rs] >= 0;
			else if constexpr (OneOf(instr, BLTZAL, BLTZALL)) /* Branch On Less Than Zero and Link (Likely) */
				return gpr[rs] < 0;
			else if constexpr (OneOf(instr, BGEZAL, BGEZALL)) /* Branch On Greater Than Or Equal To Zero And Link (Likely) */
				return gpr[rs] >= 0;
			else
				static_assert(AlwaysFalse<instr>, "\"Branch\" template function called, but no matching branch instruction was found.");
		}();

		if (branch_cond) {
			s64 offset = s64(s16(instr_code & 0xFFFF)) << 2;
			PrepareJump(pc + offset);
			pc_is_inside_branch_delay_slot = true;
		}
		else if constexpr (OneOf(instr, BEQL, BNEL, BLEZL, BGTZL, BEQL, BLTZL, BGEZL, BLTZALL, BGEZALL)) {
			pc += 4; /* The instruction in the branch delay slot is discarded. */
		}

		AdvancePipeline(1);
	}


	template<CPUInstruction instr>
	void TrapThreeOperand(const u32 instr_code)
	{
		using enum CPUInstruction;

		auto rt = instr_code >> 16 & 0x1F;
		auto rs = instr_code >> 21 & 0x1F;

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{} {}, {}", current_instr_name, rs, rt);
		}

		bool trap_cond = [&] {
			if constexpr (instr == TGE) {
				/* Trap If Greater Than Or Equal;
				   Compares registers rs and rt as signed integers.
				   If register rs is greater than rt, generates an exception. */
				return gpr[rs] > gpr[rt];
			}
			else if constexpr (instr == TGEU) {
				/* Trap If Greater Than Or Equal Unsigned;
				   Compares registers rs and rt as unsigned integers.
				   If register rs is greater than rt, generates an exception. */
				return u64(gpr[rs]) > u64(gpr[rt]);
			}
			else if constexpr (instr == TLT) {
				/* Trap If Less Than;
				   Compares registers rs and rt as signed integers.
				   If register rs is less than rt, generates an exception. */
				return gpr[rs] < gpr[rt];
			}
			else if constexpr (instr == TLTU) {
				/* Trap If Less Than Unsigned;
				   Compares registers rs and rt as unsigned integers.
				   If register rs is less than rt, generates an exception. */
				return u64(gpr[rs]) < u64(gpr[rt]);
			}
			else if constexpr (instr == TEQ) {
				/* Trap If Equal;
				   Generates an exception if registers rs and rt are equal. */
				return gpr[rs] == gpr[rt];
			}
			else if constexpr (instr == TNE) {
				/* Trap If Not Equal;
				   Generates an exception if registers rs and rt are not equal. */
				return gpr[rs] != gpr[rt];
			}
			else {
				static_assert(AlwaysFalse<instr>, "\"Trap_ThreeOperand\" template function called, but no matching trap instruction was found.");
			}
		}();

		if (trap_cond) {
			SignalException<Exception::Trap>();
		}

		AdvancePipeline(1);
	}


	template<CPUInstruction instr>
	void TrapImmediate(const u32 instr_code)
	{
		using enum CPUInstruction;

		auto rs = instr_code >> 21 & 0x1F;
		auto immediate = [&] {
			if constexpr (OneOf(instr, TGEI, TLTI))
				return s16(instr_code & 0xFFFF);
			else
				return u16(instr_code & 0xFFFF);
		}();

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{} {}, ${:X}", current_instr_name, rs, immediate);
		}

		bool trap_cond = [&] {
			if constexpr (instr == TGEI) {
				/* Trap If Greater Than Or Equal Immediate;
				   Compares the contents of register rs with 16-bit sign-extended immediate as a
				   signed integer. If rs contents are greater than the immediate, generates an exception. */
				return gpr[rs] > immediate;
			}
			else if constexpr (instr == TGEIU) {
				/* Trap If Greater Than Or Equal Immediate Unsigned;
				   Compares the contents of register rs with 16-bit zero-extended immediate as an
				   unsigned integer. If rs contents are greater than the immediate, generates an exception. */
				return u64(gpr[rs]) > immediate;
			}
			else if constexpr (instr == TLTI) {
				/* Trap If Less Than Immediate;
				   Compares the contents of register rs with 16-bit sign-extended immediate as a
				   signed integer. If rs contents are less than the immediate, generates an exception. */
				return gpr[rs] < immediate;
			}
			else if constexpr (instr == TLTIU) {
				/* Trap If Less Than Immediate Unsigned;
				   Compares the contents of register rs with 16-bit zero-extended immediate as an
				   unsigned integer. If rs contents are less than the immediate, generates an exception. */
				return u64(gpr[rs]) < immediate;
			}
			else if constexpr (instr == TEQI) {
				/* Trap If Equal Immediate;
				   Generates an exception if the contents of register rs are equal to immediate. */
				return s64(gpr[rs]) == immediate; /* TODO: should we really cast to s64? */
			}
			else if constexpr (instr == TNEI) {
				/* Trap If Not Equal Immediate;
				   Generates an exception if the contents of register rs are not equal to immediate. */
				return s64(gpr[rs]) != immediate;
			}
			else {
				static_assert(AlwaysFalse<instr>, "\"Trap_Immediate\" template function called, but no matching trap instruction was found.");
			}
		}();

		if (trap_cond) {
			SignalException<Exception::Trap>();
		}

		AdvancePipeline(1);
	}


	template<CPUInstruction instr>
	void CPUMove(const u32 instr_code)
	{
		using enum CPUInstruction;

		if constexpr (OneOf(instr, MFLO, MFHI)) {
			/* Move From LO/HI;
			   Transfers the contents of special register LO/HI to register rd. */
			auto rd = instr_code >> 11 & 0x1F;
			gpr.Set(rd, [] {
				if constexpr (instr == MFLO)
					return lo_reg;
				else
					return hi_reg;
			}());
			if constexpr (log_cpu_instructions) {
				current_instr_log_output = std::format("{} {}", current_instr_name, rd);
			}
		}
		else if constexpr (OneOf(instr, MTLO, MTHI)) {
			/* Move To LO/HI;
			   Transfers the contents of register rs to special register LO/HI. */
			auto rs = instr_code >> 21 & 0x1F;
			if constexpr (instr == MTLO) lo_reg = gpr[rs];
			else                         hi_reg = gpr[rs];

			if constexpr (log_cpu_instructions) {
				current_instr_log_output = std::format("{} {}", current_instr_name, rs);
			}
		}
		AdvancePipeline(1);
	}


	void Sync()
	{
		/* Synchronize;
		   Completes the Load/store instruction currently in the pipeline before the new
		   Load/store instruction is executed. Is executed as a NOP on the VR4300. */
		if constexpr (log_cpu_instructions) {
			current_instr_log_output = current_instr_name;
		}
		AdvancePipeline(1);
	}


	void Syscall()
	{
		/* System Call;
		   Generates a system call exception and transfers control to the exception processing program. */
		if constexpr (log_cpu_instructions) {
			current_instr_log_output = current_instr_name;
		}
		SignalException<Exception::Syscall>();
		AdvancePipeline(1);
	}


	void Break()
	{
		/* Breakpoint;
		   Generates a breakpoint exception and transfers control to the exception processing program. */
		if constexpr (log_cpu_instructions) {
			current_instr_log_output = current_instr_name;
		}
		SignalException<Exception::Breakpoint>();
		AdvancePipeline(1);
	}


	template void CPULoad<CPUInstruction::LB>(u32);
	template void CPULoad<CPUInstruction::LBU>(u32);
	template void CPULoad<CPUInstruction::LH>(u32);
	template void CPULoad<CPUInstruction::LHU>(u32);
	template void CPULoad<CPUInstruction::LW>(u32);
	template void CPULoad<CPUInstruction::LWU>(u32);
	template void CPULoad<CPUInstruction::LWL>(u32);
	template void CPULoad<CPUInstruction::LWR>(u32);
	template void CPULoad<CPUInstruction::LD>(u32);
	template void CPULoad<CPUInstruction::LDL>(u32);
	template void CPULoad<CPUInstruction::LDR>(u32);
	template void CPULoad<CPUInstruction::LL>(u32);
	template void CPULoad<CPUInstruction::LLD>(u32);

	template void CPUStore<CPUInstruction::SB>(u32);
	template void CPUStore<CPUInstruction::SH>(u32);
	template void CPUStore<CPUInstruction::SW>(u32);
	template void CPUStore<CPUInstruction::SWL>(u32);
	template void CPUStore<CPUInstruction::SWR>(u32);
	template void CPUStore<CPUInstruction::SC>(u32);
	template void CPUStore<CPUInstruction::SCD>(u32);
	template void CPUStore<CPUInstruction::SD>(u32);
	template void CPUStore<CPUInstruction::SDL>(u32);
	template void CPUStore<CPUInstruction::SDR>(u32);

	template void ALUImmediate<CPUInstruction::ADDI>(u32);
	template void ALUImmediate<CPUInstruction::ADDIU>(u32);
	template void ALUImmediate<CPUInstruction::SLTI>(u32);
	template void ALUImmediate<CPUInstruction::SLTIU>(u32);
	template void ALUImmediate<CPUInstruction::ANDI>(u32);
	template void ALUImmediate<CPUInstruction::ORI>(u32);
	template void ALUImmediate<CPUInstruction::XORI>(u32);
	template void ALUImmediate<CPUInstruction::LUI>(u32);
	template void ALUImmediate<CPUInstruction::DADDI>(u32);
	template void ALUImmediate<CPUInstruction::DADDIU>(u32);

	template void ALUThreeOperand<CPUInstruction::ADD>(u32);
	template void ALUThreeOperand<CPUInstruction::ADDU>(u32);
	template void ALUThreeOperand<CPUInstruction::SUB>(u32);
	template void ALUThreeOperand<CPUInstruction::SUBU>(u32);
	template void ALUThreeOperand<CPUInstruction::SLT>(u32);
	template void ALUThreeOperand<CPUInstruction::SLTU>(u32);
	template void ALUThreeOperand<CPUInstruction::AND>(u32);
	template void ALUThreeOperand<CPUInstruction::OR>(u32);
	template void ALUThreeOperand<CPUInstruction::XOR>(u32);
	template void ALUThreeOperand<CPUInstruction::NOR>(u32);
	template void ALUThreeOperand<CPUInstruction::DADD>(u32);
	template void ALUThreeOperand<CPUInstruction::DADDU>(u32);
	template void ALUThreeOperand<CPUInstruction::DSUB>(u32);
	template void ALUThreeOperand<CPUInstruction::DSUBU>(u32);

	template void ALUShift<CPUInstruction::SLL>(u32);
	template void ALUShift<CPUInstruction::SRL>(u32);
	template void ALUShift<CPUInstruction::SRA>(u32);
	template void ALUShift<CPUInstruction::SLLV>(u32);
	template void ALUShift<CPUInstruction::SRLV>(u32);
	template void ALUShift<CPUInstruction::SRAV>(u32);
	template void ALUShift<CPUInstruction::DSLL>(u32);
	template void ALUShift<CPUInstruction::DSRL>(u32);
	template void ALUShift<CPUInstruction::DSRA>(u32);
	template void ALUShift<CPUInstruction::DSLLV>(u32);
	template void ALUShift<CPUInstruction::DSRLV>(u32);
	template void ALUShift<CPUInstruction::DSRAV>(u32);
	template void ALUShift<CPUInstruction::DSLL32>(u32);
	template void ALUShift<CPUInstruction::DSRL32>(u32);
	template void ALUShift<CPUInstruction::DSRA32>(u32);

	template void ALUMulDiv<CPUInstruction::MULT>(u32);
	template void ALUMulDiv<CPUInstruction::MULTU>(u32);
	template void ALUMulDiv<CPUInstruction::DIV>(u32);
	template void ALUMulDiv<CPUInstruction::DIVU>(u32);
	template void ALUMulDiv<CPUInstruction::DMULT>(u32);
	template void ALUMulDiv<CPUInstruction::DMULTU>(u32);
	template void ALUMulDiv<CPUInstruction::DDIV>(u32);
	template void ALUMulDiv<CPUInstruction::DDIVU>(u32);

	template void Jump<CPUInstruction::J>(u32);
	template void Jump<CPUInstruction::JAL>(u32);
	template void Jump<CPUInstruction::JR>(u32);
	template void Jump<CPUInstruction::JALR>(u32);

	template void CPUBranch<CPUInstruction::BEQ>(u32);
	template void CPUBranch<CPUInstruction::BNE>(u32);
	template void CPUBranch<CPUInstruction::BLEZ>(u32);
	template void CPUBranch<CPUInstruction::BGTZ>(u32);
	template void CPUBranch<CPUInstruction::BLTZ>(u32);
	template void CPUBranch<CPUInstruction::BGEZ>(u32);
	template void CPUBranch<CPUInstruction::BLTZAL>(u32);
	template void CPUBranch<CPUInstruction::BGEZAL>(u32);
	template void CPUBranch<CPUInstruction::BEQL>(u32);
	template void CPUBranch<CPUInstruction::BNEL>(u32);
	template void CPUBranch<CPUInstruction::BLEZL>(u32);
	template void CPUBranch<CPUInstruction::BGTZL>(u32);
	template void CPUBranch<CPUInstruction::BLTZL>(u32);
	template void CPUBranch<CPUInstruction::BGEZL>(u32);
	template void CPUBranch<CPUInstruction::BLTZALL>(u32);
	template void CPUBranch<CPUInstruction::BGEZALL>(u32);

	template void TrapThreeOperand<CPUInstruction::TGE>(u32);
	template void TrapThreeOperand<CPUInstruction::TGEU>(u32);
	template void TrapThreeOperand<CPUInstruction::TLT>(u32);
	template void TrapThreeOperand<CPUInstruction::TLTU>(u32);
	template void TrapThreeOperand<CPUInstruction::TEQ>(u32);
	template void TrapThreeOperand<CPUInstruction::TNE>(u32);

	template void TrapImmediate<CPUInstruction::TGEI>(u32);
	template void TrapImmediate<CPUInstruction::TGEIU>(u32);
	template void TrapImmediate<CPUInstruction::TLTI>(u32);
	template void TrapImmediate<CPUInstruction::TLTIU>(u32);
	template void TrapImmediate<CPUInstruction::TEQI>(u32);
	template void TrapImmediate<CPUInstruction::TNEI>(u32);

	template void CPUMove<CPUInstruction::MFLO>(u32);
	template void CPUMove<CPUInstruction::MFHI>(u32);
	template void CPUMove<CPUInstruction::MTLO>(u32);
	template void CPUMove<CPUInstruction::MTHI>(u32);
}