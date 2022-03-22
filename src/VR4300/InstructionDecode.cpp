module VR4300:Operation;

import :COP0;
import :COP1;
import :CPU;
import :Exceptions;
import :MMU;
import :Registers;

import DebugOptions;
import Logging;
import NumericalTypes;


#define EXEC_CPU_INSTR(INSTR) { \
	if constexpr (log_cpu_instructions) \
		current_instr_name = #INSTR; \
	ExecuteCPUInstruction<CPUInstruction::INSTR>(); }

#define EXEC_COP0_INSTR(INSTR) { \
	if constexpr (log_cpu_instructions) \
		current_instr_name = #INSTR; \
	ExecuteCOP0Instruction<COP0Instruction::INSTR>(); }

#define EXEC_COP1_INSTR(INSTR) { \
	if constexpr (log_cpu_instructions) \
		current_instr_name = #INSTR; \
	ExecuteCOP1Instruction<COP1Instruction::INSTR>(); }


namespace VR4300
{
	u32 instr_code;


	template<CPUInstruction instr> void ExecuteCPUInstruction();
	template<COP0Instruction instr> void ExecuteCOP0Instruction();
	template<COP1Instruction instr> void ExecuteCOP1Instruction();


	void DecodeAndExecuteSpecialInstruction()
	{
		const auto sub_op_code = instr_code & 0x3F;

		switch (sub_op_code)
		{
		break; case 0b100000: EXEC_CPU_INSTR(ADD); 
		break; case 0b100001: EXEC_CPU_INSTR(ADDU);
		break; case 0b100100: EXEC_CPU_INSTR(AND);
		break; case 0b101100: EXEC_CPU_INSTR(DADD);
		break; case 0b101101: EXEC_CPU_INSTR(DADDU);
		break; case 0b101110: EXEC_CPU_INSTR(DSUB);
		break; case 0b101111: EXEC_CPU_INSTR(DSUBU);
		break; case 0b100111: EXEC_CPU_INSTR(NOR);
		break; case 0b100101: EXEC_CPU_INSTR(OR);
		break; case 0b101010: EXEC_CPU_INSTR(SLT);
		break; case 0b101011: EXEC_CPU_INSTR(SLTU);
		break; case 0b100010: EXEC_CPU_INSTR(SUB);
		break; case 0b100011: EXEC_CPU_INSTR(SUBU);
		break; case 0b100110: EXEC_CPU_INSTR(XOR);

		break; case 0b111000: EXEC_CPU_INSTR(DSLL);
		break; case 0b010100: EXEC_CPU_INSTR(DSLLV);
		break; case 0b111100: EXEC_CPU_INSTR(DSLL32);
		break; case 0b111011: EXEC_CPU_INSTR(DSRA);
		break; case 0b010111: EXEC_CPU_INSTR(DSRAV);
		break; case 0b111111: EXEC_CPU_INSTR(DSRA32);
		break; case 0b111010: EXEC_CPU_INSTR(DSRL);
		break; case 0b010110: EXEC_CPU_INSTR(DSRLV);
		break; case 0b111110: EXEC_CPU_INSTR(DSRL32);
		break; case 0b000000: EXEC_CPU_INSTR(SLL);
		break; case 0b000100: EXEC_CPU_INSTR(SLLV);
		break; case 0b000011: EXEC_CPU_INSTR(SRA);
		break; case 0b000111: EXEC_CPU_INSTR(SRAV);
		break; case 0b000010: EXEC_CPU_INSTR(SRL);
		break; case 0b000110: EXEC_CPU_INSTR(SRLV);

		break; case 0b011110: EXEC_CPU_INSTR(DDIV);
		break; case 0b011111: EXEC_CPU_INSTR(DDIVU);
		break; case 0b011010: EXEC_CPU_INSTR(DIV);
		break; case 0b011011: EXEC_CPU_INSTR(DIVU);
		break; case 0b011100: EXEC_CPU_INSTR(DMULT);
		break; case 0b011101: EXEC_CPU_INSTR(DMULTU);
		break; case 0b011000: EXEC_CPU_INSTR(MULT);
		break; case 0b011001: EXEC_CPU_INSTR(MULTU);

		break; case 0b001001: EXEC_CPU_INSTR(JALR);
		break; case 0b001000: EXEC_CPU_INSTR(JR);

		break; case 0b110100: EXEC_CPU_INSTR(TEQ);
		break; case 0b110000: EXEC_CPU_INSTR(TGE);
		break; case 0b110001: EXEC_CPU_INSTR(TGEU);
		break; case 0b110010: EXEC_CPU_INSTR(TLT);
		break; case 0b110011: EXEC_CPU_INSTR(TLTU);
		break; case 0b110110: EXEC_CPU_INSTR(TNE);

		break; case 0b010000: EXEC_CPU_INSTR(MFHI);
		break; case 0b010010: EXEC_CPU_INSTR(MFLO);
		break; case 0b010001: EXEC_CPU_INSTR(MTHI);
		break; case 0b010011: EXEC_CPU_INSTR(MTLO);

		break; case 0b001101: EXEC_CPU_INSTR(BREAK);
		break; case 0b001111: EXEC_CPU_INSTR(SYNC);
		break; case 0b001100: EXEC_CPU_INSTR(SYSCALL);

		break; default:
			SignalException<Exception::ReservedInstruction>();
		}
	}


	void DecodeAndExecuteRegimmInstruction()
	{
		const auto sub_op_code = instr_code >> 16 & 0x1F;

		switch (sub_op_code)
		{
		break; case 0b00001: EXEC_CPU_INSTR(BGEZ);
		break; case 0b10001: EXEC_CPU_INSTR(BGEZAL);
		break; case 0b10011: EXEC_CPU_INSTR(BGEZALL);
		break; case 0b00011: EXEC_CPU_INSTR(BGEZL);
		break; case 0b00000: EXEC_CPU_INSTR(BLTZ);
		break; case 0b10000: EXEC_CPU_INSTR(BLTZAL);
		break; case 0b10010: EXEC_CPU_INSTR(BLTZALL);
		break; case 0b00010: EXEC_CPU_INSTR(BLTZL);

		break; case 0b01100: EXEC_CPU_INSTR(TEQI);
		break; case 0b01000: EXEC_CPU_INSTR(TGEI);
		break; case 0b01001: EXEC_CPU_INSTR(TGEIU);
		break; case 0b01010: EXEC_CPU_INSTR(TLTI);
		break; case 0b01011: EXEC_CPU_INSTR(TLTIU);
		break; case 0b01110: EXEC_CPU_INSTR(TNEI);

		break; default:
			SignalException<Exception::ReservedInstruction>();
		}
	}


	void DecodeAndExecuteCOP0Instruction()
	{
		const auto sub_op_code = instr_code >> 21 & 0x1F;

		switch (sub_op_code)
		{
		break; case 0b10000:
		{
			const auto sub_op_code = instr_code & 0x3F;
			switch (sub_op_code)
			{
			break; case 0b011000: EXEC_COP0_INSTR(ERET);
			break; case 0b001000: EXEC_COP0_INSTR(TLBP);
			break; case 0b000001: EXEC_COP0_INSTR(TLBR);
			break; case 0b000010: EXEC_COP0_INSTR(TLBWI);
			break; case 0b000110: EXEC_COP0_INSTR(TLBWR);

			break; default:
				/* "Invalid", but does not cause a reserved instruction exception. */
				assert(false); /* TODO what to do here */
			}
		}

		break; case 0b00001: EXEC_COP0_INSTR(DMFC0);
		break; case 0b00101: EXEC_COP0_INSTR(DMTC0);
		break; case 0b00000: EXEC_COP0_INSTR(MFC0);
		break; case 0b00100: EXEC_COP0_INSTR(MTC0);

		break; default:
			SignalException<Exception::ReservedInstruction>();
		}
	}


	void DecodeAndExecuteCOP1Instruction()
	{
		if (!fpu_is_enabled)
		{
			SignalException<Exception::CoprocessorUnusable>();
			return;
		}

		const auto sub_op_code = instr_code >> 21 & 0x1F;

		switch (sub_op_code)
		{
		break; case 0b01000:
		{
			const auto sub_op_code = instr_code >> 16 & 0x1F;
			switch (sub_op_code)
			{
			break; case 0b00000: EXEC_COP1_INSTR(BC1F);
			break; case 0b00010: EXEC_COP1_INSTR(BC1FL);
			break; case 0b00001: EXEC_COP1_INSTR(BC1T);
			break; case 0b00011: EXEC_COP1_INSTR(BC1TL);

			break; default:
				SignalException<Exception::ReservedInstruction>();
			}
		}

		break; case 0b00010: EXEC_COP1_INSTR(CFC1);
		break; case 0b00110: EXEC_COP1_INSTR(CTC1);
		break; case 0b00001: EXEC_COP1_INSTR(DMFC1);
		break; case 0b00101: EXEC_COP1_INSTR(DMTC1);
		break; case 0b00000: EXEC_COP1_INSTR(MFC1);
		break; case 0b00100: EXEC_COP1_INSTR(MTC1);

		break; default:
		{
			if ((instr_code & 0x30) == 0x30)
			{
				EXEC_COP1_INSTR(C);
			}
			else
			{
				const auto sub_op_code = instr_code & 0x3F;
				switch (sub_op_code)
				{
				break; case 0b000101: EXEC_COP1_INSTR(ABS);
				break; case 0b000000: EXEC_COP1_INSTR(ADD);
				break; case 0b000011: EXEC_COP1_INSTR(DIV);
				break; case 0b000110: EXEC_COP1_INSTR(MOV);
				break; case 0b000010: EXEC_COP1_INSTR(MUL);
				break; case 0b000111: EXEC_COP1_INSTR(NEG);
				break; case 0b000100: EXEC_COP1_INSTR(SQRT);
				break; case 0b000001: EXEC_COP1_INSTR(SUB);

				break; case 0b001010: EXEC_COP1_INSTR(CEIL_L);
				break; case 0b001110: EXEC_COP1_INSTR(CEIL_W);
				break; case 0b100001: EXEC_COP1_INSTR(CVT_D);
				break; case 0b100101: EXEC_COP1_INSTR(CVT_L);
				break; case 0b100000: EXEC_COP1_INSTR(CVT_S);
				break; case 0b100100: EXEC_COP1_INSTR(CVT_W);
				break; case 0b001011: EXEC_COP1_INSTR(FLOOR_L);
				break; case 0b001111: EXEC_COP1_INSTR(FLOOR_W);
				break; case 0b001000: EXEC_COP1_INSTR(ROUND_L);
				break; case 0b001100: EXEC_COP1_INSTR(ROUND_W);
				break; case 0b001001: EXEC_COP1_INSTR(TRUNC_L);
				break; case 0b001101: EXEC_COP1_INSTR(TRUNC_W);

				break; default: /* TODO: Reserved instruction exception?? */
					; // UnimplementedOperationException(); // TODO: also set flags in FCR31
				}
			}
		}
		}
	}


	void DecodeAndExecuteInstruction(const u32 instr_code)
	{
		VR4300::instr_code = instr_code;

		const auto op_code = instr_code >> 26; /* (0-63) */

		switch (op_code)
		{
		break; case 0b000000: DecodeAndExecuteSpecialInstruction();
		break; case 0b000001: DecodeAndExecuteRegimmInstruction();
		break; case 0b010000: DecodeAndExecuteCOP0Instruction();
		break; case 0b010001: DecodeAndExecuteCOP1Instruction();

		break; case 0b100000: EXEC_CPU_INSTR(LB);
		break; case 0b100100: EXEC_CPU_INSTR(LBU);
		break; case 0b110111: EXEC_CPU_INSTR(LD);
		break; case 0b011010: EXEC_CPU_INSTR(LDL);
		break; case 0b011011: EXEC_CPU_INSTR(LDR);
		break; case 0b100001: EXEC_CPU_INSTR(LH);
		break; case 0b100101: EXEC_CPU_INSTR(LHU);
		break; case 0b110000: EXEC_CPU_INSTR(LL);
		break; case 0b110100: EXEC_CPU_INSTR(LLD);
		break; case 0b100011: EXEC_CPU_INSTR(LW);
		break; case 0b100010: EXEC_CPU_INSTR(LWL);
		break; case 0b100110: EXEC_CPU_INSTR(LWR);
		break; case 0b100111: EXEC_CPU_INSTR(LWU);

		break; case 0b101000: EXEC_CPU_INSTR(SB);
		break; case 0b111000: EXEC_CPU_INSTR(SC);
		break; case 0b111100: EXEC_CPU_INSTR(SCD);
		break; case 0b111111: EXEC_CPU_INSTR(SD);
		break; case 0b101100: EXEC_CPU_INSTR(SDL);
		break; case 0b101101: EXEC_CPU_INSTR(SDR);
		break; case 0b101001: EXEC_CPU_INSTR(SH);
		break; case 0b101011: EXEC_CPU_INSTR(SW);
		break; case 0b101010: EXEC_CPU_INSTR(SWL);
		break; case 0b101110: EXEC_CPU_INSTR(SWR);

		break; case 0b001000: EXEC_CPU_INSTR(ADDI);
		break; case 0b001001: EXEC_CPU_INSTR(ADDIU);
		break; case 0b001100: EXEC_CPU_INSTR(ANDI);
		break; case 0b011000: EXEC_CPU_INSTR(DADDI);
		break; case 0b011001: EXEC_CPU_INSTR(DADDIU);
		break; case 0b001111: EXEC_CPU_INSTR(LUI);
		break; case 0b001101: EXEC_CPU_INSTR(ORI);
		break; case 0b001010: EXEC_CPU_INSTR(SLTI);
		break; case 0b001011: EXEC_CPU_INSTR(SLTIU);
		break; case 0b001110: EXEC_CPU_INSTR(XORI);

		break; case 0b000010: EXEC_CPU_INSTR(J);
		break; case 0b000011: EXEC_CPU_INSTR(JAL);

		break; case 0b000100: EXEC_CPU_INSTR(BEQ);
		break; case 0b010100: EXEC_CPU_INSTR(BEQL);
		break; case 0b000111: EXEC_CPU_INSTR(BGTZ);
		break; case 0b010111: EXEC_CPU_INSTR(BGTZL);
		break; case 0b000110: EXEC_CPU_INSTR(BLEZ);
		break; case 0b010110: EXEC_CPU_INSTR(BLEZL);
		break; case 0b000101: EXEC_CPU_INSTR(BNE);
		break; case 0b010101: EXEC_CPU_INSTR(BNEL);

		break; case 0b101111: EXEC_COP0_INSTR(CACHE);

		break; case 0b110101:
			if (!fpu_is_enabled)
				SignalException<Exception::CoprocessorUnusable>();
			else
				EXEC_COP1_INSTR(LDC1);

		break; case 0b110001:
			if (!fpu_is_enabled)
				SignalException<Exception::CoprocessorUnusable>();
			else
				EXEC_COP1_INSTR(LWC1);

		break; case 0b111101:
			if (!fpu_is_enabled)
				SignalException<Exception::CoprocessorUnusable>();
			else
				EXEC_COP1_INSTR(SDC1);

		break; case 0b111001:
			if (!fpu_is_enabled)
				SignalException<Exception::CoprocessorUnusable>();
			else
				EXEC_COP1_INSTR(SWC1);

		break; default:
			SignalException<Exception::ReservedInstruction>();
		}
	}


	template<CPUInstruction instr>
	void ExecuteCPUInstruction()
	{
		using enum CPUInstruction;

		if constexpr (instr == LB || instr == LBU || instr == LH || instr == LHU || instr == LW || instr == LWU || instr == LWL ||
			instr == LWR || instr == LD || instr == LDL || instr == LDR || instr == LL || instr == LLD)
		{
			CPULoad<instr>(instr_code);
		}

		else if constexpr (instr == SB || instr == SH || instr == SW || instr == SWL || instr == SWR ||
			instr == SC || instr == SCD || instr == SD || instr == SDL || instr == SDR)
		{
			CPUStore<instr>(instr_code);
		}

		else if constexpr (instr == ADDI || instr == ADDIU || instr == SLTI || instr == SLTIU || instr == ANDI ||
			instr == ORI || instr == XORI || instr == LUI || instr == DADDI || instr == DADDIU)
		{
			ALUImmediate<instr>(instr_code);
		}

		else if constexpr (instr == ADD || instr == ADDU || instr == AND || instr == DADD || instr == DADDU || instr == DSUB || instr == DSUBU ||
			instr == NOR || instr == OR || instr == SLT || instr == SLTU || instr == SUB || instr == SUBU || instr == XOR)
		{
			ALUThreeOperand<instr>(instr_code);
		}

		else if constexpr (instr == SLL || instr == SRL || instr == SRA || instr == SLLV || instr == SRLV || instr == SRAV || instr == DSLL ||
			instr == DSRL || instr == DSRA || instr == DSLLV || instr == DSRLV || instr == DSRAV || instr == DSLL32 || instr == DSRL32 || instr == DSRA32)
		{
			ALUShift<instr>(instr_code);
		}

		else if constexpr (instr == MULT || instr == MULTU || instr == DIV || instr == DIVU ||
			instr == DMULT || instr == DMULTU || instr == DDIV || instr == DDIVU)
		{
			ALUMulDiv<instr>(instr_code);
		}

		else if constexpr (instr == MFHI || instr == MFLO || instr == MTHI || instr == MTLO)
		{
			CPUMove<instr>(instr_code);
		}

		else if constexpr (instr == J || instr == JAL || instr == JR || instr == JALR)
		{
			Jump<instr>(instr_code);
		}

		else if constexpr (instr == BEQ || instr == BNE || instr == BLEZ || instr == BGTZ || instr == BLTZ || instr == BGEZ || instr == BLTZAL ||
			instr == BGEZAL || instr == BEQL || instr == BNEL || instr == BLEZL || instr == BGTZL || instr == BLTZL || instr == BGEZL || instr == BLTZALL || instr == BGEZALL)
		{
			CPUBranch<instr>(instr_code);
		}

		else if constexpr (instr == TGE || instr == TGEU || instr == TLT || instr == TLTU || instr == TEQ || instr == TNE)
		{
			TrapThreeOperand<instr>(instr_code);
		}

		else if constexpr (instr == TGEI || instr == TGEIU || instr == TLTI || instr == TLTIU || instr == TEQI || instr == TNEI)
		{
			TrapImmediate<instr>(instr_code);
		}

		else if constexpr (instr == BREAK)
		{
			Break();
		}
		else if constexpr (instr == SYNC)
		{
			Sync();
		}
		else if constexpr (instr == SYSCALL)
		{
			Syscall();
		}
		else
		{
			static_assert(instr != instr);
		}

		if constexpr (log_cpu_instructions)
		{
			Logging::LogVR4300Instruction(current_instr_pc, current_instr_log_output, last_instr_fetch_phys_addr);
		}
	}


	template<COP0Instruction instr>
	void ExecuteCOP0Instruction()
	{
		if constexpr (instr == COP0Instruction::MTC0 || instr == COP0Instruction::MFC0 || instr == COP0Instruction::DMTC0 || instr == COP0Instruction::DMFC0)
		{
			COP0Move<instr>(instr_code);
		}
		else if constexpr (instr == COP0Instruction::TLBP) TLBP();
		else if constexpr (instr == COP0Instruction::TLBR) TLBR();
		else if constexpr (instr == COP0Instruction::TLBWI) TLBWI();
		else if constexpr (instr == COP0Instruction::TLBWR) TLBWR();
		else if constexpr (instr == COP0Instruction::ERET) ERET();
		else if constexpr (instr == COP0Instruction::CACHE) CACHE(instr_code);
		else static_assert(instr != instr);

		if constexpr (log_cpu_instructions)
		{
			Logging::LogVR4300Instruction(current_instr_pc, current_instr_log_output, last_instr_fetch_phys_addr);
		}
	}


	template<COP1Instruction instr>
	void ExecuteCOP1Instruction()
	{
		using enum COP1Instruction;

		if constexpr (instr == LWC1 || instr == LDC1)
		{
			FPULoad<instr>(instr_code);
		}
		else if constexpr (instr == SWC1 || instr == SDC1)
		{
			FPUStore<instr>(instr_code);
		}
		else if constexpr (instr == MTC1 || instr == MFC1 || instr == CTC1 || instr == CFC1 || instr == DMTC1 || instr == DMFC1)
		{
			FPUMove<instr>(instr_code);
		}
		else if constexpr (instr == CVT_S || instr == CVT_D || instr == CVT_L || instr == CVT_W || instr == ROUND_L || instr == ROUND_W ||
			instr == TRUNC_L || instr == TRUNC_W || instr == CEIL_L || instr == CEIL_W || instr == FLOOR_L || instr == FLOOR_W)
		{
			FPUConvert<instr>(instr_code);
		}
		else if constexpr (instr == ADD || instr == SUB || instr == MUL || instr == DIV ||
			instr == ABS || instr == MOV || instr == NEG || instr == SQRT)
		{
			FPUCompute<instr>(instr_code);
		}
		else if constexpr (instr == BC1T || instr == BC1F || instr == BC1TL || instr == BC1FL)
		{
			FPUBranch<instr>(instr_code);
		}
		else if constexpr (instr == C)
		{
			FPUCompare(instr_code);
		}
		else
		{
			static_assert(instr != instr);
		}

		if constexpr (log_cpu_instructions)
		{
			Logging::LogVR4300Instruction(current_instr_pc, current_instr_log_output, last_instr_fetch_phys_addr);
		}
	}
}