module RSP:Operation;

import :ScalarUnit;
import :VectorUnit;

import DebugOptions;
import Logging;
import NumericalTypes;


#define EXEC_SCALAR_INSTR(INSTR) { \
	if constexpr (log_rsp_instructions) \
		current_instr_name = #INSTR; \
	ExecuteScalarInstruction<ScalarInstruction::INSTR>(); }

#define EXEC_VECTOR_INSTR(INSTR) { \
	if constexpr (log_rsp_instructions) \
		current_instr_name = #INSTR; \
	ExecuteVectorInstruction<VectorInstruction::INSTR>(); }


namespace RSP
{
	u32 instr_code;


	template<ScalarInstruction instr> void ExecuteScalarInstruction();
	template<VectorInstruction instr> void ExecuteVectorInstruction();


	void DecodeAndExecuteSpecialInstruction()
	{
		const auto sub_op_code = instr_code & 0x3F;

		switch (sub_op_code)
		{
		break; case 0b100000: EXEC_SCALAR_INSTR(ADD);
		break; case 0b100001: EXEC_SCALAR_INSTR(ADDU);
		break; case 0b100100: EXEC_SCALAR_INSTR(AND);
		break; case 0b100111: EXEC_SCALAR_INSTR(NOR);
		break; case 0b100101: EXEC_SCALAR_INSTR(OR);
		break; case 0b101010: EXEC_SCALAR_INSTR(SLT);
		break; case 0b101011: EXEC_SCALAR_INSTR(SLTU);
		break; case 0b100010: EXEC_SCALAR_INSTR(SUB);
		break; case 0b100011: EXEC_SCALAR_INSTR(SUBU);
		break; case 0b100110: EXEC_SCALAR_INSTR(XOR);

		break; case 0b000000: EXEC_SCALAR_INSTR(SLL);
		break; case 0b000100: EXEC_SCALAR_INSTR(SLLV);
		break; case 0b000011: EXEC_SCALAR_INSTR(SRA);
		break; case 0b000111: EXEC_SCALAR_INSTR(SRAV);
		break; case 0b000010: EXEC_SCALAR_INSTR(SRL);
		break; case 0b000110: EXEC_SCALAR_INSTR(SRLV);

		break; case 0b001001: EXEC_SCALAR_INSTR(JALR);
		break; case 0b001000: EXEC_SCALAR_INSTR(JR);

		break; case 0b001101: EXEC_SCALAR_INSTR(BREAK);

		break; default:
			;
		}
	}


	void DecodeAndExecuteRegimmInstruction()
	{
		const auto sub_op_code = instr_code >> 16 & 0x1F;

		switch (sub_op_code)
		{
		break; case 0b00001: EXEC_SCALAR_INSTR(BGEZ);
		break; case 0b10001: EXEC_SCALAR_INSTR(BGEZAL);
		break; case 0b00000: EXEC_SCALAR_INSTR(BLTZ);
		break; case 0b10000: EXEC_SCALAR_INSTR(BLTZAL);

		break; default:
			;
		}
	}


	void DecodeAndExecuteCOP2Instruction()
	{
		if (instr_code & 1 << 25)
		{
			const auto op_code = instr_code & 0x3F;
			switch (op_code)
			{
			case 0x00: EXEC_VECTOR_INSTR(VMULF); break;
			case 0x01: EXEC_VECTOR_INSTR(VMULU); break;
			case 0x02: EXEC_VECTOR_INSTR(VRNDP); break;
			case 0x03: EXEC_VECTOR_INSTR(VMULQ); break;
			case 0x04: EXEC_VECTOR_INSTR(VMUDL); break;
			case 0x05: EXEC_VECTOR_INSTR(VMUDM); break;
			case 0x06: EXEC_VECTOR_INSTR(VMUDN); break;
			case 0x07: EXEC_VECTOR_INSTR(VMUDH); break;
			case 0x08: EXEC_VECTOR_INSTR(VMACF); break;
			case 0x09: EXEC_VECTOR_INSTR(VMACU); break;
			case 0x0A: EXEC_VECTOR_INSTR(VRNDN); break;
			case 0x0B: EXEC_VECTOR_INSTR(VMACQ); break;
			case 0x0C: EXEC_VECTOR_INSTR(VMADL); break;
			case 0x0D: EXEC_VECTOR_INSTR(VMADM); break;
			case 0x0E: EXEC_VECTOR_INSTR(VMADN); break;
			case 0x0F: EXEC_VECTOR_INSTR(VMADH); break;
			case 0x10: EXEC_VECTOR_INSTR(VADD); break;
			case 0x11: EXEC_VECTOR_INSTR(VSUB); break;
			case 0x13: EXEC_VECTOR_INSTR(VABS); break;
			case 0x14: EXEC_VECTOR_INSTR(VADDC); break;
			case 0x15: EXEC_VECTOR_INSTR(VSUBC); break;
			case 0x1D: EXEC_VECTOR_INSTR(VSAR); break;
			case 0x20: EXEC_VECTOR_INSTR(VLT); break;
			case 0x21: EXEC_VECTOR_INSTR(VEQ); break;
			case 0x22: EXEC_VECTOR_INSTR(VNE); break;
			case 0x23: EXEC_VECTOR_INSTR(VGE); break;
			case 0x24: EXEC_VECTOR_INSTR(VCL); break;
			case 0x25: EXEC_VECTOR_INSTR(VCH); break;
			case 0x26: EXEC_VECTOR_INSTR(VCR); break;
			case 0x27: EXEC_VECTOR_INSTR(VMRG); break;
			case 0x28: EXEC_VECTOR_INSTR(VAND); break;
			case 0x29: EXEC_VECTOR_INSTR(VNAND); break;
			case 0x2A: EXEC_VECTOR_INSTR(VOR); break;
			case 0x2B: EXEC_VECTOR_INSTR(VNOR); break;
			case 0x2C: EXEC_VECTOR_INSTR(VXOR); break;
			case 0x2D: EXEC_VECTOR_INSTR(VNXOR); break;
			case 0x30: EXEC_VECTOR_INSTR(VRCP); break;
			case 0x31: EXEC_VECTOR_INSTR(VRCPL); break;
			case 0x32: EXEC_VECTOR_INSTR(VRCPH); break;
			case 0x33: EXEC_VECTOR_INSTR(VMOV); break;
			case 0x34: EXEC_VECTOR_INSTR(VRSQ); break;
			case 0x35: EXEC_VECTOR_INSTR(VRSQL); break;
			case 0x36: EXEC_VECTOR_INSTR(VRSQH); break;
			case 0x37: EXEC_VECTOR_INSTR(VNOP); break;
			default: break;
			}
		}
		else
		{
			const auto op_code = instr_code >> 21 & 0x1F;
			switch (op_code)
			{
			case 0b00000: EXEC_VECTOR_INSTR(MFC2); break;
			case 0b00100: EXEC_VECTOR_INSTR(MTC2); break;
			case 0b00010: EXEC_VECTOR_INSTR(CFC2); break;
			case 0b00110: EXEC_VECTOR_INSTR(CTC2); break;
			default: break;
			}
		}
	}


	void DecodeExecuteInstruction(const u32 instr_code)
	{
		RSP::instr_code = instr_code;

		const auto op_code = instr_code >> 26; /* (0-63) */

		switch (op_code)
		{
		break; case 0b000000: DecodeAndExecuteSpecialInstruction();
		break; case 0b000001: DecodeAndExecuteRegimmInstruction();
		break; case 0b010010: DecodeAndExecuteCOP2Instruction();

		break; case 0b100000: EXEC_SCALAR_INSTR(LB);
		break; case 0b100100: EXEC_SCALAR_INSTR(LBU);
		break; case 0b100001: EXEC_SCALAR_INSTR(LH);
		break; case 0b100101: EXEC_SCALAR_INSTR(LHU);
		break; case 0b110000: EXEC_SCALAR_INSTR(LL);
		break; case 0b100011: EXEC_SCALAR_INSTR(LW);
		break; case 0b100111: EXEC_SCALAR_INSTR(LWU);

		break; case 0b101000: EXEC_SCALAR_INSTR(SB);
		break; case 0b111000: EXEC_SCALAR_INSTR(SC);
		break; case 0b101001: EXEC_SCALAR_INSTR(SH);
		break; case 0b101011: EXEC_SCALAR_INSTR(SW);

		break; case 0b001000: EXEC_SCALAR_INSTR(ADDI);
		break; case 0b001001: EXEC_SCALAR_INSTR(ADDIU);
		break; case 0b001100: EXEC_SCALAR_INSTR(ANDI);
		break; case 0b001111: EXEC_SCALAR_INSTR(LUI);
		break; case 0b001101: EXEC_SCALAR_INSTR(ORI);
		break; case 0b001010: EXEC_SCALAR_INSTR(SLTI);
		break; case 0b001011: EXEC_SCALAR_INSTR(SLTIU);
		break; case 0b001110: EXEC_SCALAR_INSTR(XORI);

		break; case 0b000010: EXEC_SCALAR_INSTR(J);
		break; case 0b000011: EXEC_SCALAR_INSTR(JAL);

		break; case 0b000100: EXEC_SCALAR_INSTR(BEQ);
		break; case 0b000111: EXEC_SCALAR_INSTR(BGTZ);
		break; case 0b000110: EXEC_SCALAR_INSTR(BLEZ);
		break; case 0b000101: EXEC_SCALAR_INSTR(BNE);

		break; case 0b110010:
		{
			const auto op_code = instr_code >> 11 & 0x1F;
			switch (op_code)
			{
			case 0x00: EXEC_VECTOR_INSTR(LBV); break;
			case 0x01: EXEC_VECTOR_INSTR(LSV); break;
			case 0x02: EXEC_VECTOR_INSTR(LLV); break;
			case 0x03: EXEC_VECTOR_INSTR(LDV); break;
			case 0x04: EXEC_VECTOR_INSTR(LQV); break;
			case 0x05: EXEC_VECTOR_INSTR(LRV); break;
			case 0x06: EXEC_VECTOR_INSTR(LPV); break;
			case 0x07: EXEC_VECTOR_INSTR(LUV); break;
			case 0x08: EXEC_VECTOR_INSTR(LTV); break;
			default: break;
			}
		}

		break; case 0b111010:
		{
			const auto op_code = instr_code >> 11 & 0x1F;
			switch (op_code)
			{
			case 0x00: EXEC_VECTOR_INSTR(SBV); break;
			case 0x01: EXEC_VECTOR_INSTR(SSV); break;
			case 0x02: EXEC_VECTOR_INSTR(SLV); break;
			case 0x03: EXEC_VECTOR_INSTR(SDV); break;
			case 0x04: EXEC_VECTOR_INSTR(SQV); break;
			case 0x05: EXEC_VECTOR_INSTR(SRV); break;
			case 0x06: EXEC_VECTOR_INSTR(SPV); break;
			case 0x07: EXEC_VECTOR_INSTR(SUV); break;
			case 0x08: EXEC_VECTOR_INSTR(STV); break;
			default: break;
			}
		}

		break; default:
			;
		}
	}


	template<ScalarInstruction instr>
	void ExecuteScalarInstruction()
	{
		using enum ScalarInstruction;

		if constexpr (instr == LB || instr == LBU || instr == LH || instr == LHU || instr == LW || instr == LWU || instr == LL)
		{
			ScalarLoad<instr>(instr_code);
		}

		else if constexpr (instr == SB || instr == SH || instr == SW || instr == SC)
		{
			ScalarStore<instr>(instr_code);
		}

		else if constexpr (instr == ADDI || instr == ADDIU || instr == SLTI || instr == SLTIU || instr == ANDI ||
			instr == ORI || instr == XORI || instr == LUI)
		{
			ALUImmediate<instr>(instr_code);
		}

		else if constexpr (instr == ADD || instr == ADDU || instr == AND || instr == NOR || instr == OR ||
			instr == SLT || instr == SLTU || instr == SUB || instr == SUBU || instr == XOR)
		{
			ALUThreeOperand<instr>(instr_code);
		}

		else if constexpr (instr == SLL || instr == SRL || instr == SRA || instr == SLLV || instr == SRLV || instr == SRAV)
		{
			ALUShift<instr>(instr_code);
		}

		else if constexpr (instr == J || instr == JAL || instr == JR || instr == JALR)
		{
			Jump<instr>(instr_code);
		}

		else if constexpr (instr == BEQ || instr == BNE || instr == BLEZ || instr == BGTZ || instr == BLTZ || instr == BGEZ || instr == BLTZAL || instr == BGEZAL)
		{
			Branch<instr>(instr_code);
		}

		else if constexpr (instr == BREAK)
		{
			Break();
		}

		else
		{
			static_assert(instr != instr);
		}

		if constexpr (log_rsp_instructions)
		{
			Logging::LogRSPInstruction(current_instr_pc, current_instr_log_output);
		}
	}


	template<VectorInstruction instr>
	void ExecuteVectorInstruction()
	{
		using enum VectorInstruction;
		
		if constexpr (instr == LBV || instr == LSV || instr == LLV || instr == LDV ||
			instr == LQV || instr == LRV || instr == LPV || instr == LUV || instr == LTV)
		{
			VectorLoadStore<instr>(instr_code);
		}

		else if constexpr (instr == SBV || instr == SSV || instr == SLV || instr == SDV ||
			instr == SQV || instr == SRV || instr == SPV || instr == SUV || instr == STV)
		{
			VectorLoadStore<instr>(instr_code);
		}

		else if constexpr (instr == MTC2 || instr == MFC2 || instr == CTC2 || instr == CFC2)
		{
			Move<instr>(instr_code);
		}

		else if constexpr (instr == VMOV || instr == VRCP || instr == VRSQ || instr == VRCPH || instr == VRSQH ||
			instr == VRCPL || instr == VRSQL || instr == VRNDN || instr == VRNDP || instr == VNOP || instr == VNULL)
		{
			SingleLaneInstr<instr>(instr_code);
		}

		else if constexpr (instr == VMULF || instr == VMULU || instr == VMULQ || instr == VMUDL ||
			instr == VMUDM || instr == VMUDN || instr == VMUDH || instr == VMACF || instr == VMACU ||
			instr == VMADL || instr == VMADM || instr == VADMN || instr == VADMH || instr == VADD ||
			instr == VMACQ || instr == VABS || instr == VADDC || instr == VSUB || instr == VSUBC ||
			instr == VMADN || instr == VMADH || instr == VSAR || instr == VAND || instr == VNAND ||
			instr == VOR || instr == VNOR || instr == VXOR || instr == VNXOR)
		{
			ComputeInstr<instr>(instr_code);
		}

		else if constexpr (instr == VLT || instr == VEQ || instr == VNE ||instr == VGE ||
			instr == VCH || instr == VCR || instr == VCL || instr == VMRG)
		{
			SelectInstr<instr>(instr_code);
		}

		else
		{
			static_assert(instr != instr);
		}

		if constexpr (log_rsp_instructions)
		{
			Logging::LogRSPInstruction(current_instr_pc, current_instr_log_output);
		}
	}
}