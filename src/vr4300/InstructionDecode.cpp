module VR4300:Operation;

import :Cache;
import :COP0;
import :COP1;
import :COP2;
import :CPU;
import :Exceptions;
import :MMU;
import :Operation;
import :Recompiler;

import BuildOptions;
import Log;
import Util;


#define EXEC_CPU_INSTR(INSTR) { \
	if constexpr (log_cpu_instructions) \
		current_instr_name = #INSTR; \
	ExecuteCpuInstruction<CpuInstruction::INSTR>(); }

#define EXEC_COP0_INSTR(INSTR) { \
	if constexpr (log_cpu_instructions) \
		current_instr_name = #INSTR; \
	ExecuteCop0Instruction<Cop0Instruction::INSTR>(); }

#define EXEC_COP1_INSTR(INSTR) { \
	if constexpr (log_cpu_instructions) \
		current_instr_name = #INSTR; \
	ExecuteCop1Instruction<Cop1Instruction::INSTR>(); }

#define EXEC_COP2_INSTR(INSTR) { \
	if constexpr (log_cpu_instructions) \
		current_instr_name = #INSTR; \
	ExecuteCop2Instruction<Cop2Instruction::INSTR>(); }

#define LOG_INSTR(OUTPUT) { \
	if constexpr (log_cpu_instructions) \
		Log::CpuInstruction(last_instr_fetch_phys_addr, OUTPUT); }

#define IMM16 (instr_code & 0xFFFF)
#define IMM26 (instr_code & 0x3FF'FFFF)
#define SA (instr_code >>  6 & 0x1F)
#define RD (instr_code >> 11 & 0x1F)
#define RT (instr_code >> 16 & 0x1F)
#define RS (instr_code >> 21 & 0x1F)

namespace VR4300
{
	u32 instr_code;


	void DecodeExecuteCop0Instruction()
	{
		auto opcode = instr_code >> 21 & 0x1F;

		switch (opcode) {
		case 0b10000: {
			auto opcode = instr_code & 0x3F;
			switch (opcode) {
			case 0b011000: EXEC_COP0_INSTR(ERET); break;
			case 0b001000: EXEC_COP0_INSTR(TLBP); break;
			case 0b000001: EXEC_COP0_INSTR(TLBR); break;
			case 0b000010: EXEC_COP0_INSTR(TLBWI); break;
			case 0b000110: EXEC_COP0_INSTR(TLBWR); break;

			default:
				/* "Invalid", but does not cause a reserved instruction exception. */
				NotifyIllegalInstrCode(instr_code);
			}
			break;
		}

		 case 0b00001: EXEC_COP0_INSTR(DMFC0); break;
		 case 0b00101: EXEC_COP0_INSTR(DMTC0); break;
		 case 0b00000: EXEC_COP0_INSTR(MFC0); break;
		 case 0b00100: EXEC_COP0_INSTR(MTC0); break;

		default:
			NotifyIllegalInstrCode(instr_code);
			SignalException<Exception::ReservedInstruction>();
		}
	}


	void DecodeExecuteCop1Instruction()
	{
		auto opcode = instr_code >> 21 & 0x1F;

		switch (opcode) {
		case 0b01000: {
			auto opcode = instr_code >> 16 & 0x1F;
			switch (opcode) {
			 case 0b00000: EXEC_COP1_INSTR(BC1F); break;
			 case 0b00010: EXEC_COP1_INSTR(BC1FL); break;
			 case 0b00001: EXEC_COP1_INSTR(BC1T); break;
			 case 0b00011: EXEC_COP1_INSTR(BC1TL); break;

			default:
				NotifyIllegalInstrCode(instr_code);
				SignalException<Exception::ReservedInstruction>();
			}
			break;
		}

		case 0b00010: EXEC_COP1_INSTR(CFC1); break;
		case 0b00011: EXEC_COP1_INSTR(DCFC1); break;
		case 0b00110: EXEC_COP1_INSTR(CTC1); break;
		case 0b00111: EXEC_COP1_INSTR(DCTC1); break;
		case 0b00000: EXEC_COP1_INSTR(MFC1); break;
		case 0b00001: EXEC_COP1_INSTR(DMFC1); break;
		case 0b00100: EXEC_COP1_INSTR(MTC1); break;
		case 0b00101: EXEC_COP1_INSTR(DMTC1); break;

		default: {
			if ((instr_code & 0x30) == 0x30) {
				EXEC_COP1_INSTR(C);
			}
			else {
				auto opcode = instr_code & 0x3F;
				switch (opcode) {
				case 0b000101: EXEC_COP1_INSTR(ABS); break;
				case 0b000000: EXEC_COP1_INSTR(ADD); break;
				case 0b000011: EXEC_COP1_INSTR(DIV); break;
				case 0b000110: EXEC_COP1_INSTR(MOV); break;
				case 0b000010: EXEC_COP1_INSTR(MUL); break;
				case 0b000111: EXEC_COP1_INSTR(NEG); break;
				case 0b000100: EXEC_COP1_INSTR(SQRT); break;
				case 0b000001: EXEC_COP1_INSTR(SUB); break;

				case 0b001010: EXEC_COP1_INSTR(CEIL_L); break;
				case 0b001110: EXEC_COP1_INSTR(CEIL_W); break;
				case 0b100001: EXEC_COP1_INSTR(CVT_D); break;
				case 0b100101: EXEC_COP1_INSTR(CVT_L); break;
				case 0b100000: EXEC_COP1_INSTR(CVT_S); break;
				case 0b100100: EXEC_COP1_INSTR(CVT_W); break;
				case 0b001011: EXEC_COP1_INSTR(FLOOR_L); break;
				case 0b001111: EXEC_COP1_INSTR(FLOOR_W); break;
				case 0b001000: EXEC_COP1_INSTR(ROUND_L); break;
				case 0b001100: EXEC_COP1_INSTR(ROUND_W); break;
				case 0b001001: EXEC_COP1_INSTR(TRUNC_L); break;
				case 0b001101: EXEC_COP1_INSTR(TRUNC_W); break;

				default: /* TODO: Reserved instruction exception?? */
					NotifyIllegalInstrCode(instr_code);
					break; // UnimplementedOperationException(); // TODO: also set flags in FCR31
				}
			}
			break;
		}
		}
	}


	void DecodeExecuteCop2Instruction()
	{
		auto opcode = instr_code >> 21 & 0x1F;
		switch (opcode) {
		case 0: EXEC_COP2_INSTR(MFC2); break;
		case 1: EXEC_COP2_INSTR(DMFC2); break;
		case 2: EXEC_COP2_INSTR(CFC2); break;
		case 3: EXEC_COP2_INSTR(DCFC2); break;
		case 4: EXEC_COP2_INSTR(MFC2); break;
		case 5: EXEC_COP2_INSTR(DMFC2); break;
		case 6: EXEC_COP2_INSTR(CTC2); break;
		case 7: EXEC_COP2_INSTR(DCTC2); break;
		default:
			cop0.status.cu2 ? SignalException<Exception::ReservedInstructionCop2>()
				: SignalCoprocessorUnusableException(2);
			AdvancePipeline(1);
		}
	}


	void DecodeExecuteCOP3Instruction()
	{
		auto opcode = instr_code >> 21 & 0x1F;
		if (opcode == 0) { /* MFC3 */
			SignalException<Exception::ReservedInstruction>();
		}
		else {
			cop0.status.cu3 ? SignalException<Exception::ReservedInstruction>()
				: SignalCoprocessorUnusableException(3);
		}
		AdvancePipeline(1);
	}


	void DecodeExecuteInstruction(u32 instr_code)
	{
		VR4300::instr_code = instr_code;

		auto opcode = instr_code >> 26; /* (0-63) */

		switch (opcode) {
		case 0b000000: DecodeExecuteSpecialInstruction(); break;
		case 0b000001: DecodeExecuteRegimmInstruction(); break;
		case 0b010000: DecodeExecuteCop0Instruction(); break;
		case 0b010001: DecodeExecuteCop1Instruction(); break;
		case 0b010010: DecodeExecuteCop2Instruction(); break;
		case 0b010011: DecodeExecuteCOP3Instruction(); break;

		case 0b100000: EXEC_CPU_INSTR(LB); break;
		case 0b100100: EXEC_CPU_INSTR(LBU); break;
		case 0b110111: EXEC_CPU_INSTR(LD); break;
		case 0b011010: EXEC_CPU_INSTR(LDL); break;
		case 0b011011: EXEC_CPU_INSTR(LDR); break;
		case 0b100001: EXEC_CPU_INSTR(LH); break;
		case 0b100101: EXEC_CPU_INSTR(LHU); break;
		case 0b110000: EXEC_CPU_INSTR(LL); break;
		case 0b110100: EXEC_CPU_INSTR(LLD); break;
		case 0b100011: EXEC_CPU_INSTR(LW); break;
		case 0b100010: EXEC_CPU_INSTR(LWL); break;
		case 0b100110: EXEC_CPU_INSTR(LWR); break;
		case 0b100111: EXEC_CPU_INSTR(LWU); break;

		case 0b101000: EXEC_CPU_INSTR(SB); break;
		case 0b111000: EXEC_CPU_INSTR(SC); break;
		case 0b111100: EXEC_CPU_INSTR(SCD); break;
		case 0b111111: EXEC_CPU_INSTR(SD); break;
		case 0b101100: EXEC_CPU_INSTR(SDL); break;
		case 0b101101: EXEC_CPU_INSTR(SDR); break;
		case 0b101001: EXEC_CPU_INSTR(SH); break;
		case 0b101011: EXEC_CPU_INSTR(SW); break;
		case 0b101010: EXEC_CPU_INSTR(SWL); break;
		case 0b101110: EXEC_CPU_INSTR(SWR); break;

		case 0b001000: EXEC_CPU_INSTR(ADDI); break;
		case 0b001001: EXEC_CPU_INSTR(ADDIU); break;
		case 0b001100: EXEC_CPU_INSTR(ANDI); break;
		case 0b011000: EXEC_CPU_INSTR(DADDI); break;
		case 0b011001: EXEC_CPU_INSTR(DADDIU); break;
		case 0b001111: EXEC_CPU_INSTR(LUI); break;
		case 0b001101: EXEC_CPU_INSTR(ORI); break;
		case 0b001010: EXEC_CPU_INSTR(SLTI); break;
		case 0b001011: EXEC_CPU_INSTR(SLTIU); break;
		case 0b001110: EXEC_CPU_INSTR(XORI); break;

		case 0b000010: EXEC_CPU_INSTR(J); break;
		case 0b000011: EXEC_CPU_INSTR(JAL); break;

		case 0b000100: EXEC_CPU_INSTR(BEQ); break;
		case 0b010100: EXEC_CPU_INSTR(BEQL); break;
		case 0b000111: EXEC_CPU_INSTR(BGTZ); break;
		case 0b010111: EXEC_CPU_INSTR(BGTZL); break;
		case 0b000110: EXEC_CPU_INSTR(BLEZ); break;
		case 0b010110: EXEC_CPU_INSTR(BLEZL); break;
		case 0b000101: EXEC_CPU_INSTR(BNE); break;
		case 0b010101: EXEC_CPU_INSTR(BNEL); break;

		case 0b101111: EXEC_CPU_INSTR(CACHE); break;

		case 0b110101: EXEC_COP1_INSTR(LDC1); break;
		case 0b110001: EXEC_COP1_INSTR(LWC1); break;
		case 0b111101: EXEC_COP1_INSTR(SDC1); break;
		case 0b111001: EXEC_COP1_INSTR(SWC1); break;

		default:
			NotifyIllegalInstrCode(instr_code);
			SignalException<Exception::ReservedInstruction>();
		}
	}


	void DecodeExecuteRegimmInstruction()
	{
		auto opcode = instr_code >> 16 & 0x1F;

		switch (opcode) {
		case 0b00001: EXEC_CPU_INSTR(BGEZ); break;
		case 0b10001: EXEC_CPU_INSTR(BGEZAL); break;
		case 0b10011: EXEC_CPU_INSTR(BGEZALL); break;
		case 0b00011: EXEC_CPU_INSTR(BGEZL); break;
		case 0b00000: EXEC_CPU_INSTR(BLTZ); break;
		case 0b10000: EXEC_CPU_INSTR(BLTZAL); break;
		case 0b10010: EXEC_CPU_INSTR(BLTZALL); break;
		case 0b00010: EXEC_CPU_INSTR(BLTZL); break;

		case 0b01100: EXEC_CPU_INSTR(TEQI); break;
		case 0b01000: EXEC_CPU_INSTR(TGEI); break;
		case 0b01001: EXEC_CPU_INSTR(TGEIU); break;
		case 0b01010: EXEC_CPU_INSTR(TLTI); break;
		case 0b01011: EXEC_CPU_INSTR(TLTIU); break;
		case 0b01110: EXEC_CPU_INSTR(TNEI); break;

		default:
			NotifyIllegalInstrCode(instr_code);
			SignalException<Exception::ReservedInstruction>();
		}
	}


	void DecodeExecuteSpecialInstruction()
	{
		auto opcode = instr_code & 0x3F;

		switch (opcode) {
		case 0b100000: EXEC_CPU_INSTR(ADD); break;
		case 0b100001: EXEC_CPU_INSTR(ADDU); break;
		case 0b100100: EXEC_CPU_INSTR(AND); break;
		case 0b101100: EXEC_CPU_INSTR(DADD); break;
		case 0b101101: EXEC_CPU_INSTR(DADDU); break;
		case 0b101110: EXEC_CPU_INSTR(DSUB); break;
		case 0b101111: EXEC_CPU_INSTR(DSUBU); break;
		case 0b100111: EXEC_CPU_INSTR(NOR); break;
		case 0b100101: EXEC_CPU_INSTR(OR); break;
		case 0b101010: EXEC_CPU_INSTR(SLT); break;
		case 0b101011: EXEC_CPU_INSTR(SLTU); break;
		case 0b100010: EXEC_CPU_INSTR(SUB); break;
		case 0b100011: EXEC_CPU_INSTR(SUBU); break;
		case 0b100110: EXEC_CPU_INSTR(XOR); break;

		case 0b111000: EXEC_CPU_INSTR(DSLL); break;
		case 0b010100: EXEC_CPU_INSTR(DSLLV); break;
		case 0b111100: EXEC_CPU_INSTR(DSLL32); break;
		case 0b111011: EXEC_CPU_INSTR(DSRA); break;
		case 0b010111: EXEC_CPU_INSTR(DSRAV); break;
		case 0b111111: EXEC_CPU_INSTR(DSRA32); break;
		case 0b111010: EXEC_CPU_INSTR(DSRL); break;
		case 0b010110: EXEC_CPU_INSTR(DSRLV); break;
		case 0b111110: EXEC_CPU_INSTR(DSRL32); break;
		case 0b000000: EXEC_CPU_INSTR(SLL); break;
		case 0b000100: EXEC_CPU_INSTR(SLLV); break;
		case 0b000011: EXEC_CPU_INSTR(SRA); break;
		case 0b000111: EXEC_CPU_INSTR(SRAV); break;
		case 0b000010: EXEC_CPU_INSTR(SRL); break;
		case 0b000110: EXEC_CPU_INSTR(SRLV); break;

		case 0b011110: EXEC_CPU_INSTR(DDIV); break;
		case 0b011111: EXEC_CPU_INSTR(DDIVU); break;
		case 0b011010: EXEC_CPU_INSTR(DIV); break;
		case 0b011011: EXEC_CPU_INSTR(DIVU); break;
		case 0b011100: EXEC_CPU_INSTR(DMULT); break;
		case 0b011101: EXEC_CPU_INSTR(DMULTU); break;
		case 0b011000: EXEC_CPU_INSTR(MULT); break;
		case 0b011001: EXEC_CPU_INSTR(MULTU); break;

		case 0b001001: EXEC_CPU_INSTR(JALR); break;
		case 0b001000: EXEC_CPU_INSTR(JR); break;

		case 0b110100: EXEC_CPU_INSTR(TEQ); break;
		case 0b110000: EXEC_CPU_INSTR(TGE); break;
		case 0b110001: EXEC_CPU_INSTR(TGEU); break;
		case 0b110010: EXEC_CPU_INSTR(TLT); break;
		case 0b110011: EXEC_CPU_INSTR(TLTU); break;
		case 0b110110: EXEC_CPU_INSTR(TNE); break;

		case 0b010000: EXEC_CPU_INSTR(MFHI); break;
		case 0b010010: EXEC_CPU_INSTR(MFLO); break;
		case 0b010001: EXEC_CPU_INSTR(MTHI); break;
		case 0b010011: EXEC_CPU_INSTR(MTLO); break;

		case 0b001101: EXEC_CPU_INSTR(BREAK); break;
		case 0b001111: EXEC_CPU_INSTR(SYNC); break;
		case 0b001100: EXEC_CPU_INSTR(SYSCALL); break;

		default:
			NotifyIllegalInstrCode(instr_code);
			SignalException<Exception::ReservedInstruction>();
		}
	}


	template<CpuInstruction instr>
	void ExecuteCpuInstruction()
	{
		static constexpr bool recompiler_breakup_block = [] {
			using enum CpuInstruction;
			return OneOf(instr, BEQ, BNE, BLEZ, BGTZ, BLTZ, BGEZ, BLTZAL, BGEZAL, BEQL, BNEL, BLEZL,
				BGTZL, BLTZL, BGEZL, BLTZALL, BGEZALL, TGE, TGEU, TLT, TLTU, TEQ, TNE, TGEI, TGEIU,
				TLTI, TLTIU, TEQI, TNEI, BREAK, SYSCALL);
		}();
		if constexpr (recompile_cpu && recompiler_breakup_block) {
			Recompiler::BreakupBlock();
		}

		if constexpr (instr == CpuInstruction::J) {
			LOG_INSTR(std::format("J ${:X}", pc & 0xFFFF'FFFF'F000'0000 | IMM26 << 2));
			J(IMM26);
		}
		else if constexpr (instr == CpuInstruction::JAL) {
			LOG_INSTR(std::format("JAL ${:X}", pc & 0xFFFF'FFFF'F000'0000 | IMM26 << 2));
			JAL(IMM26);
		}
		else if constexpr (instr == CpuInstruction::JR) {
			LOG_INSTR(std::format("JR {}", RS));
			JR(RS);
		}
		else if constexpr (instr == CpuInstruction::JALR) {
			LOG_INSTR(std::format("JALR {}, {}", RS, RD));
			JALR(RS, RD);
		}
		else if constexpr (instr == CpuInstruction::MFLO) {
			LOG_INSTR(std::format("MFLO {}", RD));
			MFLO(RD);
		}
		else if constexpr (instr == CpuInstruction::MFHI) {
			LOG_INSTR(std::format("MFHI {}", RD));
			MFHI(RD);
		}
		else if constexpr (instr == CpuInstruction::MTLO) {
			LOG_INSTR(std::format("MTLO {}", RS));
			MTLO(RS);
		}
		else if constexpr (instr == CpuInstruction::MTHI) {
			LOG_INSTR(std::format("MTHI {}", RS));
			MTHI(RS);
		}
		else if constexpr (instr == CpuInstruction::BREAK) {
			LOG_INSTR("BREAK");
			BREAK();
		}
		else if constexpr (instr == CpuInstruction::SYNC) {
			LOG_INSTR("SYNC");
			SYNC();
		}
		else if constexpr (instr == CpuInstruction::SYSCALL) {
			LOG_INSTR("SYSCALL");
			SYSCALL();
		}
		else if constexpr (instr == CpuInstruction::CACHE) {
			LOG_INSTR("CACHE");
			CACHE(RS, RT, IMM16);
		}
		else {
			using enum CpuInstruction;
			if constexpr (OneOf(instr, LB, LBU, LH, LHU, LW, LWU, LWL, LWR, LD, LDL, LDR, LL, LLD)) {
				LOG_INSTR(std::format("{} {}, ${:X}", current_instr_name, RT, MakeUnsigned(gpr[RS] + IMM16)));
				Load<instr>(RS, RT, IMM16);
			}
			else if constexpr (OneOf(instr, SB, SH, SW, SWL, SWR, SC, SCD, SD, SDL, SDR)) {
				LOG_INSTR(std::format("{} {}, ${:X}", current_instr_name, RT, MakeUnsigned(gpr[RS] + IMM16)));
				Store<instr>(RS, RT, IMM16);
			}
			else if constexpr (OneOf(instr, ADDI, ADDIU, SLTI, SLTIU, ANDI, ORI, XORI, LUI, DADDI, DADDIU)) {
				if constexpr (instr == LUI) {
					LOG_INSTR(std::format("{} {}, ${:X}", current_instr_name, RT, IMM16));
				}
				else {
					LOG_INSTR(std::format("{} {}, {}, ${:X}", current_instr_name, RT, RS, IMM16));
				}
				AluImmediate<instr>(RS, RT, IMM16);
			}
			else if constexpr (OneOf(instr, ADD, ADDU, AND, DADD, DADDU, DSUB, DSUBU, NOR, OR, SLT, SLTU, SUB, SUBU, XOR)) {
				LOG_INSTR(std::format("{} {}, {}, {}", current_instr_name, RD, RS, RT));
				AluThreeOperand<instr>(RS, RT, RD);
			}
			else if constexpr (OneOf(instr, SLL, SRL, SRA, DSLL, DSRL, DSRA, DSLL32, DSRL32, DSRA32)) {
				if (instr_code == 0) {
					LOG_INSTR("NOP");
				}
				else {
					LOG_INSTR(std::format("{} {}, {}, {}", current_instr_name, RD, RT, SA));
				}
				Shift<instr>(RT, RD, SA);
			}
			else if constexpr (OneOf(instr, SLLV, SRLV, SRAV, DSLLV, DSRLV, DSRAV)) {
				LOG_INSTR(std::format("{} {}, {}, {}", current_instr_name, RD, RT, RS));
				ShiftVariable<instr>(RS, RT, RD);
			}
			else if constexpr (OneOf(instr, MULT, MULTU, DIV, DIVU, DMULT, DMULTU, DDIV, DDIVU)) {
				LOG_INSTR(std::format("{} {}, {}", current_instr_name, RS, RT));
				MulDiv<instr>(RS, RT);
			}
			else if constexpr (OneOf(instr, BLEZ, BGTZ, BLTZ, BGEZ, BLTZAL, BGEZAL, BLEZL, BGTZL, BLTZL, BGEZL, BLTZALL, BGEZALL)) {
				LOG_INSTR(std::format("{} {}, ${:X}", current_instr_name, RS, IMM16));
				Branch<instr>(RS, IMM16);
			}
			else if constexpr (OneOf(instr, BEQ, BEQL, BNE, BNEL)) {
				LOG_INSTR(std::format("{} {}, {}, ${:X}", current_instr_name, RS, RT, IMM16));
				Branch<instr>(RS, RT, IMM16);
			}
			else if constexpr (OneOf(instr, TGE, TGEU, TLT, TLTU, TEQ, TNE)) {
				LOG_INSTR(std::format("{} {}, {}", current_instr_name, RS, RT));
				TrapThreeOperand<instr>(RS, RT);
			}
			else if constexpr (OneOf(instr, TGEI, TGEIU, TLTI, TLTIU, TEQI, TNEI)) {
				LOG_INSTR(std::format("{} {}, ${:X}", current_instr_name, RS, IMM16));
				TrapImmediate<instr>(RS, IMM16);
			}
			else {
				static_assert(AlwaysFalse<instr>);
			}
		}
	}


	template<Cop0Instruction instr>
	void ExecuteCop0Instruction()
	{
		if constexpr (OneOf(instr, Cop0Instruction::MTC0, Cop0Instruction::MFC0, Cop0Instruction::DMTC0, Cop0Instruction::DMFC0)) {
			LOG_INSTR(std::format("{} {}, {}", current_instr_name, RT, cop0_reg_str_repr[RD]))
			Cop0Move<instr>(RT, RD);
		}
		else {
			LOG_INSTR(current_instr_name.data());
			if constexpr (instr == Cop0Instruction::TLBP)       TLBP();
			else if constexpr (instr == Cop0Instruction::TLBR)  TLBR();
			else if constexpr (instr == Cop0Instruction::TLBWI) TLBWI();
			else if constexpr (instr == Cop0Instruction::TLBWR) TLBWR();
			else if constexpr (instr == Cop0Instruction::ERET)  ERET();
			else static_assert(AlwaysFalse<instr>);
		}
	}


	template<Cop1Instruction instr>
	void ExecuteCop1Instruction()
	{
		using enum Cop1Instruction;
		if constexpr (OneOf(instr, LWC1, LDC1)) {
			FpuLoad<instr>(instr_code);
		}
		else if constexpr (OneOf(instr, SWC1, SDC1)) {
			FpuStore<instr>(instr_code);
		}
		else if constexpr (OneOf(instr, MTC1, MFC1, CTC1, CFC1, DMTC1, DMFC1, DCFC1, DCTC1)) {
			FpuMove<instr>(instr_code);
		}
		else if constexpr (OneOf(instr, CVT_S, CVT_D, CVT_L, CVT_W, ROUND_L, ROUND_W, TRUNC_L, TRUNC_W, CEIL_L, CEIL_W, FLOOR_L, FLOOR_W)) {
			FpuConvert<instr>(instr_code);
		}
		else if constexpr (OneOf(instr, ADD, SUB, MUL, DIV, ABS, MOV, NEG, SQRT)) {
			FpuCompute<instr>(instr_code);
		}
		else if constexpr (OneOf(instr, BC1T, BC1F, BC1TL, BC1FL)) {
			FpuBranch<instr>(instr_code);
		}
		else if constexpr (instr == C) {
			FpuCompare(instr_code);
		}
		else {
			static_assert(AlwaysFalse<instr>);
		}
		if constexpr (log_cpu_instructions) {
			Log::CpuInstruction(last_instr_fetch_phys_addr, current_instr_log_output);
		}
	}


	template<Cop2Instruction instr>
	void ExecuteCop2Instruction()
	{
		using enum Cop2Instruction;
		if constexpr (OneOf(instr, CFC2, CTC2, MFC2, MTC2, DCFC2, DCTC2, DMFC2, DMTC2)) {
			Cop2Move<instr>(RT);
		}
		else {
			static_assert(AlwaysFalse<instr>);
		}
		if constexpr (log_cpu_instructions) {
			Log::CpuInstruction(last_instr_fetch_phys_addr, current_instr_log_output);
		}
	}
}