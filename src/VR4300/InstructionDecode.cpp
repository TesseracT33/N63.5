module VR4300:Operation;

import :COP0;
import :COP1;
import :CPU;
import :Exceptions;

import NumericalTypes;

namespace VR4300
{
	void DecodeSpecialInstruction(const u32 instr_code)
	{
		const u8 sub_op_code = instr_code & 0x3F;

		switch (sub_op_code)
		{
		case 0b100000: ALU_ThreeOperand<CPU_Instruction::ADD>(instr_code); break;
		case 0b100001: ALU_ThreeOperand<CPU_Instruction::ADDU>(instr_code); break;
		case 0b100100: ALU_ThreeOperand<CPU_Instruction::AND>(instr_code); break;
		case 0b101100: ALU_ThreeOperand<CPU_Instruction::DADD>(instr_code); break;
		case 0b101101: ALU_ThreeOperand<CPU_Instruction::DADDU>(instr_code); break;
		case 0b101110: ALU_ThreeOperand<CPU_Instruction::DSUB>(instr_code); break;
		case 0b101111: ALU_ThreeOperand<CPU_Instruction::DSUBU>(instr_code); break;
		case 0b100111: ALU_ThreeOperand<CPU_Instruction::NOR>(instr_code); break;
		case 0b100101: ALU_ThreeOperand<CPU_Instruction::OR>(instr_code); break;
		case 0b101010: ALU_ThreeOperand<CPU_Instruction::SLT>(instr_code); break;
		case 0b101011: ALU_ThreeOperand<CPU_Instruction::SLTU>(instr_code); break;
		case 0b100010: ALU_ThreeOperand<CPU_Instruction::SUB>(instr_code); break;
		case 0b100011: ALU_ThreeOperand<CPU_Instruction::SUBU>(instr_code); break;
		case 0b100110: ALU_ThreeOperand<CPU_Instruction::XOR>(instr_code); break;

		case 0b111000: ALU_Shift<CPU_Instruction::DSLL>(instr_code); break;
		case 0b010100: ALU_Shift<CPU_Instruction::DSLLV>(instr_code); break;
		case 0b111100: ALU_Shift<CPU_Instruction::DSLL32>(instr_code); break;
		case 0b111011: ALU_Shift<CPU_Instruction::DSRA>(instr_code); break;
		case 0b010111: ALU_Shift<CPU_Instruction::DSRAV>(instr_code); break;
		case 0b111111: ALU_Shift<CPU_Instruction::DSRA32>(instr_code); break;
		case 0b111010: ALU_Shift<CPU_Instruction::DSRL>(instr_code); break;
		case 0b010110: ALU_Shift<CPU_Instruction::DSRLV>(instr_code); break;
		case 0b111110: ALU_Shift<CPU_Instruction::DSRL32>(instr_code); break;
		case 0b000000: ALU_Shift<CPU_Instruction::SLL>(instr_code); break;
		case 0b000100: ALU_Shift<CPU_Instruction::SLLV>(instr_code); break;
		case 0b000011: ALU_Shift<CPU_Instruction::SRA>(instr_code); break;
		case 0b000111: ALU_Shift<CPU_Instruction::SRAV>(instr_code); break;
		case 0b000010: ALU_Shift<CPU_Instruction::SRL>(instr_code); break;
		case 0b000110: ALU_Shift<CPU_Instruction::SRLV>(instr_code); break;

		case 0b011110: ALU_MulDiv<CPU_Instruction::DDIV>(instr_code); break;
		case 0b011111: ALU_MulDiv<CPU_Instruction::DDIVU>(instr_code); break;
		case 0b011010: ALU_MulDiv<CPU_Instruction::DIV>(instr_code); break;
		case 0b011011: ALU_MulDiv<CPU_Instruction::DIVU>(instr_code); break;
		case 0b011100: ALU_MulDiv<CPU_Instruction::DMULT>(instr_code); break;
		case 0b011101: ALU_MulDiv<CPU_Instruction::DMULTU>(instr_code); break;
		case 0b011000: ALU_MulDiv<CPU_Instruction::MULT>(instr_code); break;
		case 0b011001: ALU_MulDiv<CPU_Instruction::MULTU>(instr_code); break;

		case 0b001001: Jump<CPU_Instruction::JALR>(instr_code); break;
		case 0b001000: Jump<CPU_Instruction::JR>(instr_code); break;

		case 0b110100: Trap_ThreeOperand<CPU_Instruction::TEQ>(instr_code); break;
		case 0b110000: Trap_ThreeOperand<CPU_Instruction::TGE>(instr_code); break;
		case 0b110001: Trap_ThreeOperand<CPU_Instruction::TGEU>(instr_code); break;
		case 0b110010: Trap_ThreeOperand<CPU_Instruction::TLT>(instr_code); break;
		case 0b110011: Trap_ThreeOperand<CPU_Instruction::TLTU>(instr_code); break;
		case 0b110110: Trap_ThreeOperand<CPU_Instruction::TNE>(instr_code); break;

		case 0b010000: MFHI(instr_code); break;
		case 0b010010: MFLO(instr_code); break;
		case 0b010001: MTHI(instr_code); break;
		case 0b010011: MTLO(instr_code); break;

		case 0b001101: BREAK(instr_code); break;
		case 0b001111: SYNC(instr_code); break;
		case 0b001100: SYSCALL(instr_code); break;

		default:
			SignalException<Exception::ReservedInstruction>();
		}
	}


	void DecodeRegimmInstruction(const u32 instr_code)
	{
		const u8 sub_op_code = instr_code >> 16 & 0x1F;

		switch (sub_op_code)
		{
		case 0b00001: Branch<CPU_Instruction::BGEZ>(instr_code); break;
		case 0b10001: Branch<CPU_Instruction::BGEZAL>(instr_code); break;
		case 0b10011: Branch<CPU_Instruction::BGEZALL>(instr_code); break;
		case 0b00011: Branch<CPU_Instruction::BGEZL>(instr_code); break;
		case 0b00000: Branch<CPU_Instruction::BLTZ>(instr_code); break;
		case 0b10000: Branch<CPU_Instruction::BLTZAL>(instr_code); break;
		case 0b10010: Branch<CPU_Instruction::BLTZALL>(instr_code); break;
		case 0b00010: Branch<CPU_Instruction::BLTZL>(instr_code); break;

		case 0b01100: Trap_Immediate<CPU_Instruction::TEQI>(instr_code); break;
		case 0b01000: Trap_Immediate<CPU_Instruction::TGEI>(instr_code); break;
		case 0b01001: Trap_Immediate<CPU_Instruction::TGEIU>(instr_code); break;
		case 0b01010: Trap_Immediate<CPU_Instruction::TLTI>(instr_code); break;
		case 0b01011: Trap_Immediate<CPU_Instruction::TLTIU>(instr_code); break;
		case 0b01110: Trap_Immediate<CPU_Instruction::TNEI>(instr_code); break;

		default:
			SignalException<Exception::ReservedInstruction>();
		}
	}


	void DecodeCOP0Instruction(const u32 instr_code)
	{
		const u8 sub_op_code = instr_code >> 21 & 0x1F;

		switch (sub_op_code)
		{
		case 0b10000:
		{
			const u8 sub_op_code = instr_code & 0x3F;
			switch (sub_op_code)
			{
			case 0b011000: ERET(); break;
			case 0b001000: TLBP(); break;
			case 0b000001: TLBR(); break;
			case 0b000010: TLBWI(); break;
			case 0b000110: TLBWR(); break;

			default:
				/* "Invalid", but does not cause a reserved instruction exception. */
				assert(false); /* TODO what to do here */
			}
			break;
		}

		case 0b00001: CP0_Move<CP0_Instruction::DMFC0>(instr_code); break;
		case 0b00101: CP0_Move<CP0_Instruction::DMTC0>(instr_code); break;
		case 0b00000: CP0_Move<CP0_Instruction::MFC0>(instr_code); break;
		case 0b00100: CP0_Move<CP0_Instruction::MTC0>(instr_code); break;

		default:
			SignalException<Exception::ReservedInstruction>();
		}
	}


	void DecodeCOP1Instruction(const u32 instr_code)
	{
		if (!fpu_is_enabled)
		{
			SignalException<Exception::CoprocessorUnusable>();
			return;
		}

		const u8 sub_op_code = instr_code >> 21 & 0x1F;

		switch (sub_op_code)
		{
		case 0b01000:
		{
			const u8 sub_op_code = instr_code >> 16 & 0x1F;
			switch (sub_op_code)
			{
			case 0b00000: FPU_Branch<FPU_Instruction::BC1F>(instr_code); break;
			case 0b00010: FPU_Branch<FPU_Instruction::BC1FL>(instr_code); break;
			case 0b00001: FPU_Branch<FPU_Instruction::BC1T>(instr_code); break;
			case 0b00011: FPU_Branch<FPU_Instruction::BC1TL>(instr_code); break;

			default:
				SignalException<Exception::ReservedInstruction>();
			}
		}
		break;

		case 0b00010: FPU_Move<FPU_Instruction::CFC1>(instr_code); break;
		case 0b00110: FPU_Move<FPU_Instruction::CTC1>(instr_code); break;
		case 0b00001: FPU_Move<FPU_Instruction::DMFC1>(instr_code); break;
		case 0b00101: FPU_Move<FPU_Instruction::DMTC1>(instr_code); break;
		case 0b00000: FPU_Move<FPU_Instruction::MFC1>(instr_code); break;
		case 0b00100: FPU_Move<FPU_Instruction::MTC1>(instr_code); break;

		default:
		{
			if ((instr_code & 0x30) == 0x30)
				FPU_Compare(instr_code);
			else
			{
				const u8 sub_op_code = instr_code & 0x3F;
				switch (sub_op_code)
				{
				case 0b000101: FPU_Compute<FPU_Instruction::ABS>(instr_code); break;
				case 0b000000: FPU_Compute<FPU_Instruction::ADD>(instr_code); break;
				case 0b000011: FPU_Compute<FPU_Instruction::DIV>(instr_code); break;
				case 0b000110: FPU_Compute<FPU_Instruction::MOV>(instr_code); break;
				case 0b000010: FPU_Compute<FPU_Instruction::MUL>(instr_code); break;
				case 0b000111: FPU_Compute<FPU_Instruction::NEG>(instr_code); break;
				case 0b000100: FPU_Compute<FPU_Instruction::SQRT>(instr_code); break;
				case 0b000001: FPU_Compute<FPU_Instruction::SUB>(instr_code); break;

				case 0b001010: FPU_Convert<FPU_Instruction::CEIL_L>(instr_code); break;
				case 0b001110: FPU_Convert<FPU_Instruction::CEIL_W>(instr_code); break;
				case 0b100001: FPU_Convert<FPU_Instruction::CVT_D>(instr_code); break;
				case 0b100101: FPU_Convert<FPU_Instruction::CVT_L>(instr_code); break;
				case 0b100000: FPU_Convert<FPU_Instruction::CVT_S>(instr_code); break;
				case 0b100100: FPU_Convert<FPU_Instruction::CVT_W>(instr_code); break;
				case 0b001011: FPU_Convert<FPU_Instruction::FLOOR_L>(instr_code); break;
				case 0b001111: FPU_Convert<FPU_Instruction::FLOOR_W>(instr_code); break;
				case 0b001000: FPU_Convert<FPU_Instruction::ROUND_L>(instr_code); break;
				case 0b001100: FPU_Convert<FPU_Instruction::ROUND_W>(instr_code); break;
				case 0b001001: FPU_Convert<FPU_Instruction::TRUNC_L>(instr_code); break;
				case 0b001101: FPU_Convert<FPU_Instruction::TRUNC_W>(instr_code); break;

				default: /* TODO: Reserved instruction exception?? */
					; // UnimplementedOperationException(); // TODO: also set flags in FCR31
				}
			}
		}
		}
	}


	void DecodeAndExecuteInstruction(const u32 instr_code)
	{
		const u8 op_code = instr_code >> 26; /* (0-63) */

		switch (op_code)
		{
		case 0b000000: /* "SPECIAL" instructions */
			DecodeSpecialInstruction(instr_code);
			break;

		case 0b000001: /* "REGIMM" instructions */
			DecodeRegimmInstruction(instr_code);
			break;

		case 0b010000: /* "COP0" instructions */
			DecodeCOP0Instruction(instr_code);
			break;

		case 0b010001: /* "COP1" instructions */
			DecodeCOP1Instruction(instr_code);
			break;

		case 0b100000: Load<CPU_Instruction::LB>(instr_code); break;
		case 0b100100: Load<CPU_Instruction::LBU>(instr_code); break;
		case 0b110111: Load<CPU_Instruction::LD>(instr_code); break;
		case 0b011010: Load<CPU_Instruction::LDL>(instr_code); break;
		case 0b011011: Load<CPU_Instruction::LDR>(instr_code); break;
		case 0b100001: Load<CPU_Instruction::LH>(instr_code); break;
		case 0b100101: Load<CPU_Instruction::LHU>(instr_code); break;
		case 0b110000: Load<CPU_Instruction::LL>(instr_code); break;
		case 0b110100: Load<CPU_Instruction::LLD>(instr_code); break;
		case 0b100011: Load<CPU_Instruction::LW>(instr_code); break;
		case 0b100010: Load<CPU_Instruction::LWL>(instr_code); break;
		case 0b100110: Load<CPU_Instruction::LWR>(instr_code); break;
		case 0b100111: Load<CPU_Instruction::LWU>(instr_code); break;

		case 0b101000: Store<CPU_Instruction::SB>(instr_code); break;
		case 0b111000: Store<CPU_Instruction::SC>(instr_code); break;
		case 0b111100: Store<CPU_Instruction::SCD>(instr_code); break;
		case 0b111111: Store<CPU_Instruction::SD>(instr_code); break;
		case 0b101100: Store<CPU_Instruction::SDL>(instr_code); break;
		case 0b101101: Store<CPU_Instruction::SDR>(instr_code); break;
		case 0b101001: Store<CPU_Instruction::SH>(instr_code); break;
		case 0b101011: Store<CPU_Instruction::SW>(instr_code); break;
		case 0b101010: Store<CPU_Instruction::SWL>(instr_code); break;
		case 0b101110: Store<CPU_Instruction::SWR>(instr_code); break;

		case 0b001000: ALU_Immediate<CPU_Instruction::ADDI>(instr_code); break;
		case 0b001001: ALU_Immediate<CPU_Instruction::ADDIU>(instr_code); break;
		case 0b001100: ALU_Immediate<CPU_Instruction::ANDI>(instr_code); break;
		case 0b011000: ALU_Immediate<CPU_Instruction::DADDI>(instr_code); break;
		case 0b011001: ALU_Immediate<CPU_Instruction::DADDIU>(instr_code); break;
		case 0b001111: ALU_Immediate<CPU_Instruction::LUI>(instr_code); break;
		case 0b001101: ALU_Immediate<CPU_Instruction::ORI>(instr_code); break;
		case 0b001010: ALU_Immediate<CPU_Instruction::SLTI>(instr_code); break;
		case 0b001011: ALU_Immediate<CPU_Instruction::SLTIU>(instr_code); break;
		case 0b001110: ALU_Immediate<CPU_Instruction::XORI>(instr_code); break;

		case 0b000010: Jump<CPU_Instruction::J>(instr_code); break;
		case 0b000011: Jump<CPU_Instruction::JAL>(instr_code); break;

		case 0b000100: Branch<CPU_Instruction::BEQ>(instr_code); break;
		case 0b010100: Branch<CPU_Instruction::BEQL>(instr_code); break;
		case 0b000111: Branch<CPU_Instruction::BGTZ>(instr_code); break;
		case 0b010111: Branch<CPU_Instruction::BGTZL>(instr_code); break;
		case 0b000110: Branch<CPU_Instruction::BLEZ>(instr_code); break;
		case 0b010110: Branch<CPU_Instruction::BLEZL>(instr_code); break;
		case 0b000101: Branch<CPU_Instruction::BNE>(instr_code); break;
		case 0b010101: Branch<CPU_Instruction::BNEL>(instr_code); break;

		case 0b101111: CACHE(instr_code); break;

		case 0b110101:
			if (!fpu_is_enabled)
				SignalException<Exception::CoprocessorUnusable>();
			else
				FPU_Load<FPU_Instruction::LDC1>(instr_code);
			break;

		case 0b110001:
			if (!fpu_is_enabled)
				SignalException<Exception::CoprocessorUnusable>();
			else
				FPU_Load<FPU_Instruction::LWC1>(instr_code);
			break;

		case 0b111101:
			if (!fpu_is_enabled)
				SignalException<Exception::CoprocessorUnusable>();
			else
				FPU_Store<FPU_Instruction::SDC1>(instr_code);
			break;

		case 0b111001:
			if (!fpu_is_enabled)
				SignalException<Exception::CoprocessorUnusable>();
			else
				FPU_Store<FPU_Instruction::SWC1>(instr_code);
			break;

		default:
			SignalException<Exception::ReservedInstruction>();
		}
	}
}