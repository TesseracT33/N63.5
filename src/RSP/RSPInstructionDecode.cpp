module RSP:RSPOperation;

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
			Load<instr>(instr_code);
		}

		else if constexpr (instr == SB || instr == SH || instr == SW || instr == SC)
		{
			Store<instr>(instr_code);
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

		if constexpr (log_rsp_instructions)
		{
			Logging::LogRSPInstruction(current_instr_pc, current_instr_log_output);
		}
	}
}