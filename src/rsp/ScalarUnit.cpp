module RSP:ScalarUnit;

import :Interface;
import :Operation;

import MI;
import RDP;

namespace RSP
{
	template<ScalarInstruction instr>
	void ScalarLoad(u32 instr_code)
	{
		using enum ScalarInstruction;

		s16 offset = instr_code & 0xFFFF;
		auto rt = instr_code >> 16 & 0x1F;
		auto base = instr_code >> 21 & 0x1F;
		auto address = gpr[base] + offset;

		if constexpr (log_rsp_instructions) {
			current_instr_log_output = std::format("{} {}, ${:X}", current_instr_name, rt, static_cast<std::make_unsigned<decltype(address)>::type>(address));
		}

		/* For all ScalarInstructions:
		   Generates an address by adding a sign-extended offset to the contents of register base. */
		auto result = [&] {
			if constexpr (instr == LB) {
				/* Load Byte;
				   Sign-extends the contents of a byte specified by the address and loads the result to register rt. */
				return ReadDMEM<s8>(address);
			}
			else if constexpr (instr == LBU) {
				/* Load Byte Unsigned;
				   Zero-extends the contents of a byte specified by the address and loads the result to register rt. */
				return u8(ReadDMEM<s8>(address));
			}
			else if constexpr (instr == LH) {
				/* Load halfword;
				   Sign-extends the contents of a halfword specified by the address and loads the result to register rt. */
				return ReadDMEM<s16>(address);
			}
			else if constexpr (instr == LHU) {
				/* Load Halfword Unsigned;
				   Zero-extends the contents of a halfword specified by the address and loads the result to register rt. */
				return u16(ReadDMEM<s16>(address));
			}
			else if constexpr (instr == LW || instr == LWU) {
				/* Load Word (Unsigned);
				   Sign(Zero)-extends the contents of a word specified by the address and loads the result to register rt. */
				return ReadDMEM<s32>(address);
			}
			else if constexpr (instr == LL) {
				/* Load Linked;
				   Loads the contents of the word specified by the address to register rt and sets the LL bit to 1. */
				s32 ret = ReadDMEM<s32>(address);
				ll_bit = 1;
				return ret;
			}
			else {
				static_assert(AlwaysFalse<instr>, "\"Load\" template function called, but no matching load ScalarInstruction was found.");
			}
		}();

		gpr.Set(rt, result);

		AdvancePipeline(1);
	}


	template<ScalarInstruction instr>
	void ScalarStore(u32 instr_code)
	{
		using enum ScalarInstruction;

		s16 offset = instr_code & 0xFFFF;
		auto rt = instr_code >> 16 & 0x1F;
		auto base = instr_code >> 21 & 0x1F;
		auto address = gpr[base] + offset;

		if constexpr (log_rsp_instructions) {
			current_instr_log_output = std::format("{} {}, ${:X}", current_instr_name, rt, static_cast<std::make_unsigned<decltype(address)>::type>(address));
		}

		/* For all ScalarInstructions:
		   Generates an address by adding a sign-extended offset to the contents of register base. */
		if constexpr (instr == SB) {
			/* Store Byte;
			   Stores the contents of the low-order byte of register rt to the memory specified by the address. */
			WriteDMEM<s8>(address, s8(gpr[rt]));
		}
		else if constexpr (instr == SH) {
			/* Store Halfword;
			   Stores the contents of the low-order halfword of register rt to the memory specified by the address. */
			WriteDMEM<s16>(address, s16(gpr[rt]));
		}
		else if constexpr (instr == SW) {
			/* Store Word;
			   Stores the contents of the low-order word of register rt to the memory specified by the address. */
			WriteDMEM<s32>(address, gpr[rt]);
		}
		else if constexpr (instr == SC) {
			/* Store Conditional;
			   If the LL bit is 1, stores the contents of the low-order word of register rt to
			   the memory specified by the address, and sets register rt to 1.
			   If the LL bit is 0, does not store the contents of the word, and clears register
			   rt to 0. */
			if (ll_bit == 1) {
				WriteDMEM<s32>(address, gpr[rt]);
				gpr.Set(rt, 1);
			}
			else {
				gpr.Set(rt, 0);
			}
		}
		else {
			static_assert(AlwaysFalse<instr>, "\"Store\" template function called, but no matching store ScalarInstruction was found.");
		}

		AdvancePipeline(1);
	}


	template<ScalarInstruction instr>
	void ALUImmediate(u32 instr_code)
	{
		using enum ScalarInstruction;

		auto rt = instr_code >> 16 & 0x1F;
		auto rs = instr_code >> 21 & 0x1F;

		auto immediate = [&] {
			if constexpr (OneOf(instr, ANDI, LUI, ORI, XORI))
				return u16(instr_code & 0xFFFF);
			else
				return s16(instr_code & 0xFFFF);
		}();

		if constexpr (log_rsp_instructions) {
			current_instr_log_output = [&] {
				if constexpr (instr == LUI)
					return std::format("{} {}, ${:X}", current_instr_name, rt, immediate);
				else
					return std::format("{} {}, {}, ${:X}", current_instr_name, rt, rs, immediate);
			}();
		}

		gpr.Set(rt, [&] {
			if constexpr (instr == ADDI || instr == ADDIU) {
				/* Add Immediate (Unsigned);
				   Sign-extends the 16-bit immediate and adds it to register rs. Stores the
				   32-bit result to register rt. As there is no exception handling, the
				   operations of ADDI and ADDIU are identical. */
				return gpr[rs] + immediate;
			}
			else if constexpr (instr == SLTI) {
				/* Set On Less Than Immediate;
				   Sign-extends the 16-bit immediate and compares it with register rs as a
				   signed integer. If rs is less than the immediate, stores 1 to register rt;
				   otherwise, stores 0 to register rt. */
				return s32(gpr[rs] < immediate);
			}
			else if constexpr (instr == SLTIU) {
				/* Set On Less Than Immediate Unsigned;
				   Sign-extends the 16-bit immediate and compares it with register rs as an
				   unsigned integer. If rs is less than the immediate, stores 1 to register rt;
				   otherwise, stores 0 to register rt. */
				return s32(u32(gpr[rs]) < u32(immediate));
			}
			else if constexpr (instr == ANDI) {
				/* And Immediate;
				   Zero-extends the 16-bit immediate, ANDs it with register rs, and stores the
				   result to register rt. */
				return gpr[rs] & immediate;
			}
			else if constexpr (instr == ORI) {
				/* Or Immediate;
				   Zero-extends the 16-bit immediate, ORs it with register rs, and stores the
				   result to register rt. */
				return gpr[rs] | immediate;
			}
			else if constexpr (instr == XORI) {
				/* Exclusive Or Immediate;
				   Zero-extends the 16-bit immediate, exclusive-ORs it with register rs, and
				   stores the result to register rt. */
				return gpr[rs] ^ immediate;
			}
			else if constexpr (instr == LUI) {
				/* Load Upper Immediate;
				   Shifts the 16-bit immediate 16 bits to the left, and clears the low-order 16 bits
				   of the word to 0. Stores the result to register rt. */
				return immediate << 16;
			}
			else {
				static_assert(AlwaysFalse<instr>, "\"ALU_Immediate\" template function called, but no matching ALU immediate ScalarInstruction was found.");
			}
		}());

		AdvancePipeline(1);
	}

	template<ScalarInstruction instr>
	void ALUThreeOperand(u32 instr_code)
	{
		using enum ScalarInstruction;

		auto rd = instr_code >> 11 & 0x1F;
		auto rt = instr_code >> 16 & 0x1F;
		auto rs = instr_code >> 21 & 0x1F;

		if constexpr (log_rsp_instructions) {
			current_instr_log_output = std::format("{} {}, {}, {}", current_instr_name, rd, rs, rt);
		}

		gpr.Set(rd, [&] {
			if constexpr (instr == ADD || instr == ADDU) {
				/* Add (Unsigned);
				   Adds the contents of register rs and rt, and stores the 32-bit result to register rd.
				   As there is no exception handling, the operations of ADD and ADDU are identical. */
				return gpr[rs] + gpr[rt];
			}
			else if constexpr (instr == SUB || instr == SUBU) {
				/* Subtract (Unsigned);
				   Subtracts the contents of register rs from register rt, and stores the 32-bit result
				   to register rd. As there is no exception handling, the operations of SUB and SUBU are identical. */
				return gpr[rs] - gpr[rt];
			}
			else if constexpr (instr == SLT) {
				/* Set On Less Than;
				   Compares the contents of registers rs and rt as 32-bit signed integers.
				   If the contents of register rs are less than those of rt, stores 1 to register rd;
				   otherwise, stores 0 to rd. */
				return s32(gpr[rs] < gpr[rt]);
			}
			else if constexpr (instr == SLTU) {
				/* Set On Less Than Unsigned;
				   Compares the contents of registers rs and rt as 32-bit unsigned integers.
				   If the contents of register rs are less than those of rt, stores 1 to register rd;
				   otherwise, stores 0 to rd. */
				return s32(u32(gpr[rs]) < u32(gpr[rt]));
			}
			else if constexpr (instr == AND) {
				/* And;
				   ANDs the contents of registers rs and rt in bit units, and stores the result to register rd. */
				return gpr[rs] & gpr[rt];
			}
			else if constexpr (instr == OR) {
				/* Or;
				   ORs the contents of registers rs and rt in bit units, and stores the result to register rd. */
				return gpr[rs] | gpr[rt];
			}
			else if constexpr (instr == XOR) {
				/* Exclusive Or;
				   Exclusive-ORs the contents of registers rs and rt in bit units, and stores the result to register rd. */
				return gpr[rs] ^ gpr[rt];
			}
			else if constexpr (instr == NOR) {
				/* Nor;
				   NORs the contents of registers rs and rt in bit units, and stores the result to register rd. */
				return ~(gpr[rs] | gpr[rt]);
			}
			else {
				static_assert(AlwaysFalse<instr>, "\"ALU_ThreeOperand\" template function called, but no matching ALU three-operand ScalarInstruction was found.");
			}
		}());

		AdvancePipeline(1);
	}


	template<ScalarInstruction instr>
	void ALUShift(u32 instr_code)
	{
		using enum ScalarInstruction;

		auto sa = instr_code >> 6 & 0x1F;
		auto rd = instr_code >> 11 & 0x1F;
		auto rt = instr_code >> 16 & 0x1F;
		auto rs = instr_code >> 21 & 0x1F;

		if constexpr (log_rsp_instructions) {
			current_instr_log_output = [&] {
				if (instr_code == 0)
					return std::string("NOP");
				if constexpr (OneOf(instr, SLLV, SRLV, SRAV))
					return std::format("{} {}, {}, {}", current_instr_name, rd, rt, rs);
				else
					return std::format("{} {}, {}, {}", current_instr_name, rd, rt, sa);
			}();
		}

		gpr.Set(rd, [&] {
			if constexpr (instr == SLL) {
				/* Shift Left Logical;
				   Shifts the contents of register rt sa bits to the left, and inserts 0 to the low-order bits.
				   Stores the result to register rd. */
				return gpr[rt] << sa;
			}
			else if constexpr (instr == SRL) {
				/* Shift Right Logical;
				   Shifts the contents of register rt sa bits to the right, and inserts 0 to the high-order bits.
				   Stores the result to register rd. */
				return s32(u32(gpr[rt]) >> sa);
			}
			else if constexpr (instr == SRA) {
				/* Shift Right Arithmetic;
				   Shifts the contents of register rt sa bits to the right, and sign-extends the high-order bits.
				   Stores the result to register rd. */
				return gpr[rt] >> sa;
			}
			else if constexpr (instr == SLLV) {
				/* Shift Left Logical Variable;
				   Shifts the contents of register rt to the left and inserts 0 to the low-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 5 bits of register rs. Stores the result to register rd. */
				return gpr[rt] << (gpr[rs] & 0x1F);
			}
			else if constexpr (instr == SRLV) {
				/* Shift Right Logical Variable;
				   Shifts the contents of register rt to the right, and inserts 0 to the high-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 5 bits of register rs. Stores the result to register rd. */
				return s32(u32(gpr[rt]) >> (gpr[rs] & 0x1F));
			}
			else if constexpr (instr == SRAV) {
				/* Shift Right Arithmetic Variable;
				   Shifts the contents of register rt to the right and sign-extends the high-order bits.
				   The number of bits by which the register contents are to be shifted is
				   specified by the low-order 5 bits of register rs. Stores the result to register rd. */
				return gpr[rt] >> (gpr[rs] & 0x1F);
			}
			else {
				static_assert(AlwaysFalse<instr>, "\"ALU_Shift\" template function called, but no matching ALU shift ScalarInstruction was found.");
			}
		}());

		AdvancePipeline(1);
	}


	template<ScalarInstruction instr>
	void Jump(u32 instr_code)
	{
		using enum ScalarInstruction;

		/* J: Jump;
		   Shifts the 26-bit target address 2 bits to the left, and jumps to the address
		   coupled with the high-order 4 bits of the PC, delayed by one ScalarInstruction.

		   JAL: Jump And Link;
		   Shifts the 26-bit target address 2 bits to the left, and jumps to the address
		   coupled with the high-order 4 bits of the PC, delayed by one ScalarInstruction.
		   Stores the address of the ScalarInstruction following the delay slot to r31 (link register).

		   JR: Jump Register;
		   Jumps to the address of register rs, delayed by one ScalarInstruction.

		   JALR: Jump And Link Register;
		   Jumps to the address of register rs, delayed by one ScalarInstruction.
		   Stores the address of the ScalarInstruction following the delay slot to register rd. */
		if (!in_branch_delay_slot) {
			u32 target = [&] {
				if constexpr (instr == J || instr == JAL) {
					u32 target = (instr_code & 0x3FF) << 2;
					if constexpr (log_rsp_instructions) {
						current_instr_log_output = std::format("{} ${:X}", current_instr_name, target);
					}
					return target;
				}
				else if constexpr (instr == JR || instr == JALR) {
					auto rs = instr_code >> 21 & 0x1F;
					if constexpr (log_rsp_instructions) {
						current_instr_log_output = std::format("{} {}", current_instr_name, rs);
					}
					return gpr[rs] & 0xFFC;
				}
				else {
					static_assert(AlwaysFalse<instr>, "\"Jump\" template function called, but no matching jump ScalarInstruction was found.");
				}
			}();
			PrepareJump(target);
		}

		if constexpr (instr == JAL) {
			gpr.Set(31, 4 + (in_branch_delay_slot ? addr_to_jump_to : pc)); /* TODO: mask with $FFF? */
		}
		else if constexpr (instr == JALR) {
			auto rd = instr_code >> 11 & 0x1F;
			gpr.Set(rd, 4 + (in_branch_delay_slot ? addr_to_jump_to : pc)); /* TODO: mask with $FFF? */
		}

		AdvancePipeline(1);
	}


	template<ScalarInstruction instr>
	void Branch(u32 instr_code)
	{
		using enum ScalarInstruction;

		auto rt = instr_code >> 16 & 0x1F;
		auto rs = instr_code >> 21 & 0x1F;

		if constexpr (log_rsp_instructions) {
			current_instr_log_output = [&] {
				s16 offset = instr_code & 0xFFFF;
				if constexpr (instr == BEQ || instr == BNE)
					return std::format("{} {}, {}, ${:X}", current_instr_name, rs, rt, offset);
				else
					return std::format("{} {}, ${:X}", current_instr_name, rs, offset);
			}();
		}

		/* For all ScalarInstructions: branch to the branch address if the condition is met, with a delay of one ScalarInstruction.
		   For "link" ScalarInstructions: stores the address of the ScalarInstruction following the delay slot to register r31 (link register). */

		bool branch_cond = [&] {
			if constexpr (instr == BEQ)         return gpr[rs] == gpr[rt];
			else if constexpr (instr == BNE)    return gpr[rs] != gpr[rt];
			else if constexpr (instr == BLEZ)   return gpr[rs] <= 0;
			else if constexpr (instr == BGTZ)   return gpr[rs] > 0;
			else if constexpr (instr == BLTZ)   return gpr[rs] < 0;
			else if constexpr (instr == BGEZ)   return gpr[rs] >= 0;
			else if constexpr (instr == BLTZAL) return gpr[rs] < 0;
			else if constexpr (instr == BGEZAL) return gpr[rs] >= 0;
			else static_assert(AlwaysFalse<instr>);
		}();

		if constexpr (instr == BLTZAL || instr == BGEZAL) {
			gpr.Set(31, 0xFFF & (4 + (in_branch_delay_slot ? addr_to_jump_to : pc)));
		}
		if (branch_cond) {
			auto offset = s16(instr_code) << 2;
			PrepareJump(pc + offset);
		}

		AdvancePipeline(1);
	}


	template<ScalarInstruction instr>
	void Move(u32 instr_code)
	{
		using enum ScalarInstruction;

		/* Registers are c0-c7 beginning from SP_DMA_SP_ADDR to SP_SEMAPHORE if bit 3 is clear,
			else DP_START_REG to DP_TMEM_REG. */
		auto rd = instr_code >> 11 & 0x1F;
		auto rt = instr_code >> 16 & 0x1F;
		auto reg_addr = (rd & 7) << 2;
		bool rdp_reg = rd & 8;

		if constexpr (log_rsp_instructions) {
			current_instr_log_output = std::format("{} {}, {}", current_instr_name, rt, rd);
		}

		if constexpr (instr == MFC0) {
			/* Move From System Control Coprocessor */
			if (rdp_reg) gpr.Set(rt, ::RDP::ReadReg(reg_addr));
			else         gpr.Set(rt, ::RSP::ReadReg(reg_addr));
		}
		else if constexpr (instr == MTC0) {
			/* Move To System Control Coprocessor */
			if (rdp_reg) ::RDP::WriteReg(reg_addr, gpr[rt]);
			else         ::RSP::WriteReg(reg_addr, gpr[rt]);
		}
		else {
			static_assert(AlwaysFalse<instr>, "\"RSP Move\" template function called, but no matching move instruction was found.");
		}

		AdvancePipeline(1);
	}


	void Break()
	{
		if constexpr (log_rsp_instructions) {
			current_instr_log_output = "BREAK";
		}
		sp.status.halted = sp.status.broke = true;
		if (sp.status.intbreak) {
			MI::SetInterruptFlag(MI::InterruptType::SP);
		}
		AdvancePipeline(1);
	}


	template void ScalarLoad<ScalarInstruction::LB>(u32);
	template void ScalarLoad<ScalarInstruction::LBU>(u32);
	template void ScalarLoad<ScalarInstruction::LH>(u32);
	template void ScalarLoad<ScalarInstruction::LHU>(u32);
	template void ScalarLoad<ScalarInstruction::LW>(u32);
	template void ScalarLoad<ScalarInstruction::LWU>(u32);
	template void ScalarLoad<ScalarInstruction::LL>(u32);

	template void ScalarStore<ScalarInstruction::SB>(u32);
	template void ScalarStore<ScalarInstruction::SH>(u32);
	template void ScalarStore<ScalarInstruction::SW>(u32);
	template void ScalarStore<ScalarInstruction::SC>(u32);

	template void ALUImmediate<ScalarInstruction::ADDI>(u32);
	template void ALUImmediate<ScalarInstruction::ADDIU>(u32);
	template void ALUImmediate<ScalarInstruction::SLTI>(u32);
	template void ALUImmediate<ScalarInstruction::SLTIU>(u32);
	template void ALUImmediate<ScalarInstruction::ANDI>(u32);
	template void ALUImmediate<ScalarInstruction::ORI>(u32);
	template void ALUImmediate<ScalarInstruction::XORI>(u32);
	template void ALUImmediate<ScalarInstruction::LUI>(u32);

	template void ALUThreeOperand<ScalarInstruction::ADD>(u32);
	template void ALUThreeOperand<ScalarInstruction::ADDU>(u32);
	template void ALUThreeOperand<ScalarInstruction::SUB>(u32);
	template void ALUThreeOperand<ScalarInstruction::SUBU>(u32);
	template void ALUThreeOperand<ScalarInstruction::SLT>(u32);
	template void ALUThreeOperand<ScalarInstruction::SLTU>(u32);
	template void ALUThreeOperand<ScalarInstruction::AND>(u32);
	template void ALUThreeOperand<ScalarInstruction::OR>(u32);
	template void ALUThreeOperand<ScalarInstruction::XOR>(u32);
	template void ALUThreeOperand<ScalarInstruction::NOR>(u32);

	template void ALUShift<ScalarInstruction::SLL>(u32);
	template void ALUShift<ScalarInstruction::SRL>(u32);
	template void ALUShift<ScalarInstruction::SRA>(u32);
	template void ALUShift<ScalarInstruction::SLLV>(u32);
	template void ALUShift<ScalarInstruction::SRLV>(u32);
	template void ALUShift<ScalarInstruction::SRAV>(u32);

	template void Jump<ScalarInstruction::J>(u32);
	template void Jump<ScalarInstruction::JAL>(u32);
	template void Jump<ScalarInstruction::JR>(u32);
	template void Jump<ScalarInstruction::JALR>(u32);

	template void Branch<ScalarInstruction::BEQ>(u32);
	template void Branch<ScalarInstruction::BNE>(u32);
	template void Branch<ScalarInstruction::BLEZ>(u32);
	template void Branch<ScalarInstruction::BGTZ>(u32);
	template void Branch<ScalarInstruction::BLTZ>(u32);
	template void Branch<ScalarInstruction::BGEZ>(u32);
	template void Branch<ScalarInstruction::BLTZAL>(u32);
	template void Branch<ScalarInstruction::BGEZAL>(u32);

	template void Move<ScalarInstruction::MTC0>(u32);
	template void Move<ScalarInstruction::MFC0>(u32);
}