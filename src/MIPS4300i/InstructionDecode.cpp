module MIPS4300i;

namespace MIPS4300i
{
	static void DecodeSpecialInstruction(const u32 instr_code)
	{
		const u8 sub_op_code = instr_code & 0x3F;

		switch (sub_op_code)
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

		default:
			ReservedInstructionException();
		}
	}


	static void DecodeRegimmInstruction(const u32 instr_code)
	{
		const u8 sub_op_code = instr_code >> 16 & 0x1F;

		switch (sub_op_code)
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

		default:
			ReservedInstructionException();
		}
	}


	static void DecodeCOP0Instruction(const u32 instr_code)
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

		case 0b00001: CP0_Move<CP0_Instr::DMFC0>(instr_code); break;
		case 0b00101: CP0_Move<CP0_Instr::DMTC0>(instr_code); break;
		case 0b00000: CP0_Move<CP0_Instr::MFC0>(instr_code); break;
		case 0b00100: CP0_Move<CP0_Instr::MTC0>(instr_code); break;

		default:
			ReservedInstructionException();
		}
	}


	static void DecodeCOP1Instruction(const u32 instr_code)
	{
		const u8 sub_op_code = instr_code >> 21 & 0x1F;

		switch (sub_op_code)
		{
		case 0b01000:
		{
			const u8 sub_op_code = instr_code >> 16 & 0x1F;
			switch (sub_op_code) /* Todo possibly put into FPU_Branch instr itself to avoid so many nested switches*/
			{
			case 0b00000: FPU_Branch<FPU_Instr::BC1F>(instr_code); break;
			case 0b00010: FPU_Branch<FPU_Instr::BC1FL>(instr_code); break;
			case 0b00001: FPU_Branch<FPU_Instr::BC1T>(instr_code); break;
			case 0b01000: FPU_Branch<FPU_Instr::BC1TL>(instr_code); break;

			default:
				ReservedInstructionException();
			}
		}
		break;

		case 0b00010: FPU_Move<FPU_Instr::CFC1>(instr_code); break;
		case 0b00110: FPU_Move<FPU_Instr::CTC1>(instr_code); break;
		case 0b00001: FPU_Move<FPU_Instr::DMFC1>(instr_code); break;
		case 0b00101: FPU_Move<FPU_Instr::DMTC1>(instr_code); break;
		case 0b00000: FPU_Move<FPU_Instr::MFC1>(instr_code); break;
		case 0b00100: FPU_Move<FPU_Instr::MTC1>(instr_code); break;

		default:
		{
			if ((instr_code & 0b110000) == 0b110000)
				FPU_Compare(instr_code);
			else
			{
				const u8 sub_op_code = instr_code & 0x3F;
				switch (sub_op_code)
				{
				case 0b000101: FPU_Compute<FPU_Instr::ABS>(instr_code); break;
				case 0b000000: FPU_Compute<FPU_Instr::ADD>(instr_code); break;
				case 0b000011: FPU_Compute<FPU_Instr::DIV>(instr_code); break;
				case 0b000110: FPU_Compute<FPU_Instr::MOV>(instr_code); break;
				case 0b000010: FPU_Compute<FPU_Instr::MUL>(instr_code); break;
				case 0b000111: FPU_Compute<FPU_Instr::NEG>(instr_code); break;
				case 0b000100: FPU_Compute<FPU_Instr::SQRT>(instr_code); break;
				case 0b000001: FPU_Compute<FPU_Instr::SUB>(instr_code); break;

				case 0b001010: FPU_Convert<FPU_Instr::CEIL_L>(instr_code); break;
				case 0b001110: FPU_Convert<FPU_Instr::CEIL_W>(instr_code); break;
				case 0b100001: FPU_Convert<FPU_Instr::CVT_D>(instr_code); break;
				case 0b100101: FPU_Convert<FPU_Instr::CVT_L>(instr_code); break;
				case 0b100000: FPU_Convert<FPU_Instr::CVT_S>(instr_code); break;
				case 0b100100: FPU_Convert<FPU_Instr::CVT_W>(instr_code); break;
				case 0b001011: FPU_Convert<FPU_Instr::FLOOR_L>(instr_code); break;
				case 0b001111: FPU_Convert<FPU_Instr::FLOOR_W>(instr_code); break;
				case 0b001000: FPU_Convert<FPU_Instr::ROUND_L>(instr_code); break;
				case 0b001100: FPU_Convert<FPU_Instr::ROUND_W>(instr_code); break;
				case 0b001001: FPU_Convert<FPU_Instr::TRUNC_L>(instr_code); break;
				case 0b001101: FPU_Convert<FPU_Instr::TRUNC_W>(instr_code); break;

				default:
					UnimplementedOperationException(); // TODO: also set flags in FCR31
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

		case 0b110101: FPU_Load<FPU_Instr::LDC1>(instr_code); break;
		case 0b110001: FPU_Load<FPU_Instr::LWC1>(instr_code); break;
		case 0b111101: FPU_Store<FPU_Instr::SDC1>(instr_code); break;
		case 0b111001: FPU_Store<FPU_Instr::SWC1>(instr_code); break;

		default:
			ReservedInstructionException();
		}
	}
}