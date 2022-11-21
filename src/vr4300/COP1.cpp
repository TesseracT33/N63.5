module VR4300:COP1;

import :COP0;
import :CPU;
import :Exceptions;
import :MMU;
import :Operation;

import DebugOptions;

/* For invoking a parameter-free lambda template */
#define INVOKE(LAMBDA, ...) LAMBDA.template operator() <__VA_ARGS__> ()

namespace VR4300
{
	u32 FPUControl::Get(size_t index) const
	{
		if (index == 31) return std::bit_cast<u32>(fcr31);
		else if (index == 0) return fcr0;
		else return 0; /* Only #0 and #31 are "valid". */
	}


	void FPUControl::Set(size_t index, u32 data)
	{
		if (index == 31) {
			static constexpr u32 mask = 0x183'FFFF;
			fcr31 = std::bit_cast<FCR31>(data & mask | std::bit_cast<u32>(fcr31) & ~mask);
			auto new_rounding_mode = [&] {
				switch (fcr31.rm) {
				case 0: return FE_TONEAREST;  /* RN */
				case 1: return FE_TOWARDZERO; /* RZ */
				case 2: return FE_UPWARD;     /* RP */
				case 3: return FE_DOWNWARD;   /* RM */
				default: std::unreachable();
				}
			}();
			std::fesetround(new_rounding_mode);
			TestAllExceptions<true /* ctc1 */>();
		}
	}


	template<FpuNumericType T>
	T FGR::Get(size_t index) const
	{
		if constexpr (sizeof(T) == 4) {
			u32 data = [&] {
				if (cop0.status.fr || !(index & 1)) return u32(fpr[index]);
				else return u32(fpr[index & ~1] >> 32);
			}();
			return std::bit_cast<T>(data);
		}
		else {
			if (!cop0.status.fr) index &= ~1;
			return std::bit_cast<T>(fpr[index]);
		}
	}


	template<FpuNumericType T>
	void FGR::Set(size_t index, T value)
	{
		if constexpr (sizeof(T) == 4) {
			if (cop0.status.fr || !(index & 1)) std::memcpy(&fpr[index], &value, 4);
			else std::memcpy((u8*)(&fpr[index & ~1]) + 4, &value, 4);
		}
		else {
			if (!cop0.status.fr) index &= ~1;
			fpr[index] = std::bit_cast<s64>(value);
		}
	}


	constexpr char FmtToChar(u32 fmt)
	{
		switch (fmt) {
		case FmtTypeID::Float32: return 'S';
		case FmtTypeID::Float64: return 'D';
		case FmtTypeID::Int32: return 'W';
		case FmtTypeID::Int64: return 'L';
		default: return '?';
		}
	}


	void ClearAllExceptions()
	{
		fcr31 = std::bit_cast<FCR31>(std::bit_cast<u32>(fcr31) & 0xFFFC'0FFF);
		std::feclearexcept(FE_ALL_EXCEPT);
	}


	void InitializeFpu()
	{
		std::fesetround(FE_TONEAREST); /* corresponds to fcr31.rm == 0b00 */
	}


	bool IsValidInput(std::floating_point auto f)
	{
		switch (std::fpclassify(f)) {
		case FP_NAN: {
			bool signal_fpu_exc = IsQuietNan(f) ? SignalInvalidOp() : SignalUnimplementedOp();
			if (signal_fpu_exc) {
				SignalException<Exception::FloatingPoint>();
				return false;
			}
			else {
				return true;
			}
		}

		case FP_SUBNORMAL:
			SignalUnimplementedOp();
			SignalException<Exception::FloatingPoint>();
			return false;
		}
		return true;
	}

	bool IsValidOutput(std::floating_point auto& f, bool exc_raised)
	{
		switch (std::fpclassify(f)) {
		case FP_NAN:
			if constexpr (sizeof(f) == 4) f = std::bit_cast<f32>(0x7FBF'FFFF);
			else                          f = std::bit_cast<f64>(0x7FF7'FFFF'FFFF'FFFF);
			return true;

		case FP_SUBNORMAL:
			if (!fcr31.fs || fcr31.enable_underflow || fcr31.enable_inexact) {
				SignalUnimplementedOp();
				if (!exc_raised) SignalException<Exception::FloatingPoint>();
				return false;
			}
			else {
				SignalUnderflow();
				SignalInexactOp();
				f = Flush(f);
				return true;
			}
		}
		return true;
	}


	void OnInvalidFormat()
	{
		AdvancePipeline(2);
		SignalUnimplementedOp();
		SignalException<Exception::FloatingPoint>();
	}


	bool SignalDivZero()
	{ /* return true if floatingpoint exception should be raised */
		fcr31.cause_div_zero = true;
		fcr31.flag_div_zero |= !fcr31.enable_div_zero;
		return fcr31.enable_div_zero;
	}


	bool SignalInexactOp()
	{
		fcr31.cause_inexact = true;
		fcr31.flag_inexact |= !fcr31.enable_inexact;
		return fcr31.enable_inexact;
	}


	bool SignalInvalidOp()
	{
		fcr31.cause_invalid = true;
		fcr31.flag_invalid |= !fcr31.enable_invalid;
		return fcr31.enable_invalid;
	}


	bool SignalOverflow()
	{
		fcr31.cause_overflow = true;
		fcr31.flag_overflow |= !fcr31.enable_overflow;
		return fcr31.enable_overflow;
	}


	bool SignalUnderflow()
	{
		fcr31.cause_underflow = true;
		fcr31.flag_underflow |= !fcr31.enable_underflow;
		return fcr31.enable_underflow;
	}


	bool SignalUnimplementedOp()
	{
		fcr31.cause_unimplemented = true;
		return true;
	}


	template<bool ctc1>
	bool TestAllExceptions()
	{
		/* * The Cause bits are updated by the floating-point operations (except load, store,
		   and transfer instructions).

		   * A floating-point exception is generated any time a Cause bit and the
		   corresponding Enable bit are set. As soon as the Cause bit enabled through the
		   Floating-point operation, an exception occurs. There is no enable bit for unimplemented
		   operation instruction (E). An Unimplemented exception always generates a floating-point
		   exception.

		   * The Flag bits are cumulative and indicate the exceptions that were raised after
		   reset. Flag bits are set to 1 if an IEEE754 exception is raised but the occurrence
		   of the exception is prohibited. Otherwise, they remain unchanged.
		*/
		if constexpr (!ctc1) {
			fcr31.cause_inexact |= std::fetestexcept(FE_INEXACT);
			fcr31.cause_underflow |= std::fetestexcept(FE_UNDERFLOW);
			fcr31.cause_overflow |= std::fetestexcept(FE_OVERFLOW);
			fcr31.cause_div_zero |= std::fetestexcept(FE_DIVBYZERO);
			fcr31.cause_invalid |= std::fetestexcept(FE_INVALID);
			if (fcr31.cause_underflow && (!fcr31.fs || fcr31.enable_underflow || fcr31.enable_inexact)) {
				fcr31.cause_unimplemented = true;
			}
		}

		u32 fcr31_u32 = std::bit_cast<u32>(fcr31);
		u32 enables = fcr31_u32 >> 7 & 0x1F;
		u32 causes = fcr31_u32 >> 12 & 0x1F;
		if constexpr (!ctc1) {
			u32 flags = causes & ~enables;
			fcr31_u32 |= flags << 2;
			fcr31 = std::bit_cast<FCR31>(fcr31_u32);
		}
		if ((enables & causes) || fcr31.cause_unimplemented) {
			SignalException<Exception::FloatingPoint>();
			return true;
		}
		else {
			return false;
		}
	}


	template<std::floating_point Float>
	Float Flush(Float f)
	{
		switch (fcr31.rm) {
		case 0: /* FE_TONEAREST */
		case 1: /* FE_TOWARDZERO */
			return std::copysign(Float(), f);
		case 2: /* FE_UPWARD */
			return std::signbit(f) ? -Float() : std::numeric_limits<Float>::min();
		case 3: /* FE_DOWNWARD */
			return std::signbit(f) ? -std::numeric_limits<Float>::min() : Float();
		default:
			std::unreachable();
		}
	}


	template<>
	bool IsQuietNan(f32 f)
	{ /* Precondition: std::isnan(f) == true */
		return std::bit_cast<u32>(f) >> 22 & 1;
	}


	template<>
	bool IsQuietNan(f64 f)
	{ /* Precondition: std::isnan(f) == true */
		return std::bit_cast<u64>(f) >> 51 & 1;
	}


	template<Cop1Instruction instr>
	void FpuLoad(u32 instr_code)
	{
		if (!cop0.status.cu1) {
			SignalCoprocessorUnusableException(1);
			AdvancePipeline(1);
			return;
		}

		using enum Cop1Instruction;

		s16 offset = instr_code & 0xFFFF;
		auto ft = instr_code >> 16 & 0x1F;
		auto base = instr_code >> 21 & 0x1F;
		auto addr = gpr[base] + offset;

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{} {}, ${:X}", current_instr_name, ft, static_cast<std::make_unsigned<decltype(addr)>::type>(addr));
		}

		auto result = [&] {
			     if constexpr (instr == LWC1) return ReadVirtual<s32>(addr);
			else if constexpr (instr == LDC1) return ReadVirtual<s64>(addr);
			else static_assert(AlwaysFalse<instr>);
		}();

		AdvancePipeline(1);

		if (!exception_has_occurred) {
			fpr.Set(ft, result);
		}
	}


	template<Cop1Instruction instr>
	void FpuStore(u32 instr_code)
	{
		if (!cop0.status.cu1) {
			SignalCoprocessorUnusableException(1);
			AdvancePipeline(1);
			return;
		}

		using enum Cop1Instruction;

		s16 offset = instr_code & 0xFFFF;
		auto ft = instr_code >> 16 & 0x1F;
		auto base = instr_code >> 21 & 0x1F;
		auto addr = gpr[base] + offset;

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{} {}, ${:X}", current_instr_name, ft, static_cast<std::make_unsigned<decltype(addr)>::type>(addr));
		}

		     if constexpr (instr == SWC1) WriteVirtual<4>(addr, fpr.Get<s32>(ft));
		else if constexpr (instr == SDC1) WriteVirtual<4>(addr, fpr.Get<s64>(ft));
		else static_assert(AlwaysFalse<instr>);

		AdvancePipeline(1);
	}


	template<Cop1Instruction instr>
	void FpuMove(u32 instr_code)
	{
		using enum Cop1Instruction;
		if (!cop0.status.cu1) {
			SignalCoprocessorUnusableException(1);
			AdvancePipeline(1);
			return;
		}

		auto fs = instr_code >> 11 & 0x1F;
		auto rt = instr_code >> 16 & 0x1F;

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{} {}, {}", current_instr_name, rt, fs);
		}

		if constexpr (instr == MTC1) {
			/* Move Word To FPU;
			   Transfers the contents of CPU general purpose register rt to FPU general purpose register fs. */
			fpr.Set<s32>(fs, s32(gpr[rt]));
		}
		else if constexpr (instr == MFC1) {
			/* Move Word From FPU;
			   Transfers the contents of FPU general purpose register fs to CPU general purpose register rt. */
			gpr.Set(rt, u64(fpr.Get<s32>(fs)));
		}
		else if constexpr (instr == CTC1) {
			/* Move Control Word To FPU;
			   Transfers the contents of CPU general purpose register rt to FPU control register fs. */
			fpu_control.Set(fs, u32(gpr[rt]));
		}
		else if constexpr (instr == CFC1) {
			/* Move Control Word From FPU;
			   Transfers the contents of FPU control register fs to CPU general purpose register rt. */
			gpr.Set(rt, s32(fpu_control.Get(fs)));
		}
		else if constexpr (instr == DMTC1) {
			/* Doubleword Move To FPU;
			   Transfers the contents of CPU general purpose register rt to FPU general purpose register fs. */
			fpr.Set<s64>(fs, s64(gpr[rt]));
		}
		else if constexpr (instr == DMFC1) {
			/* Doubleword Move From FPU;
			   Transfers the contents of FPU general purpose register fs to CPU general purpose register rt. */
			gpr.Set(rt, fpr.Get<s64>(fs));
		}
		else if constexpr (instr == DCFC1 || instr == DCTC1) {
			ClearAllExceptions();
			SignalUnimplementedOp();
			SignalException<Exception::FloatingPoint>();
		}
		else {
			static_assert(AlwaysFalse<instr>);
		}

		AdvancePipeline(2);
	}


	template<Cop1Instruction instr>
	void FpuConvert(u32 instr_code)
	{
		if (!cop0.status.cu1) {
			SignalCoprocessorUnusableException(1);
			AdvancePipeline(1);
			return;
		}
		ClearAllExceptions();

		using enum Cop1Instruction;

		/* Test for unimplemented operation exception sources for CVT/round instructions. These cannot be found out from std::fetestexcept.
		   This function should be called after the conversion has been made.
		   For all these instructions, an unimplemented exception will occur if either:
			 * If the source operand is infinity or NaN, or
			 * If overflow occurs during conversion to integer format. */
		auto TestForUnimplementedException = [&] <typename From, typename To> (From source) -> bool {
			if constexpr (std::integral<From> && std::same_as<To, f64>) {
				return false; /* zero-cost shortcut; integers cannot be infinity or NaN, and the operation is then always exact when converting to a double */
			}
			if constexpr (std::floating_point<From>) {
				if (std::isnan(source) || std::isinf(source)) {
					return true;
				}
			}
			if constexpr (std::integral<To>) {
				return std::fetestexcept(FE_OVERFLOW) != 0; // TODO: should this also include underflow?
			}
			return false;
		};

		auto fd = instr_code >> 6 & 0x1F;
		auto fs = instr_code >> 11 & 0x1F;
		auto fmt = instr_code >> 21 & 0x1F;

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{}.{} {}, {}", current_instr_name, FmtToChar(fmt), fd, fs);
		}

		if constexpr (OneOf(instr, CVT_S, CVT_D, CVT_W, CVT_L)) {
			/* CVT.S/CVT.D: Convert To Single/Double Floating-point Format;
			   Converts the contents of floating-point register fs from the specified format (fmt)
			   to a single/double-precision floating-point format. Stores the rounded result to floating-point register fd.
			   CVT.W/CVT.L: Convert To Single/Long Fixed-point Format;
			   Converts the contents of floating-point register fs from the specified format (fmt)
			   to a 32/64-bit fixed-point format. Stores the rounded result to floating-point register fd. */
			auto Convert = [&] <FpuNumericType InputType> {
				/* Interpret a source operand as type 'From', "convert" (round according to the current rounding mode) it to a new type 'To', and store the result. */
				auto Convert = [&] <FpuNumericType From, FpuNumericType To> {
					/* TODO: the below is assuming that conv. between W and L takes 2 cycles.
					See footnote 2 in table 7-14, VR4300 manual */
					static constexpr int cycles = [&] {
						     if constexpr (std::same_as<From, To>)                           return 1;
						else if constexpr (std::same_as<From, f32> && std::same_as<To, f64>) return 1;
						else if constexpr (std::same_as<From, f64> && std::same_as<To, f32>) return 2;
						else if constexpr (std::integral<From> && std::integral<To>)         return 2;
						else                                                                 return 5;
					}();
					AdvancePipeline(cycles);
					if constexpr (std::same_as<From, To> || std::integral<From> && std::integral<To>) {
						SignalUnimplementedOp();
						SignalException<Exception::FloatingPoint>();
						return;
					}
					From source = fpr.Get<From>(fs);
					if constexpr (std::floating_point<From>) {
						if (!IsValidInput(source)) return;
					}
					if constexpr (std::same_as<From, s64> && std::floating_point<To>) {
						if (source >= s64(0x0080'0000'0000'0000) || source < s64(0xFF80'0000'0000'0000)) {
							SignalUnimplementedOp();
							SignalException<Exception::FloatingPoint>();
							return;
						}
					}
					To conv = To(source);
					bool exc_raised = TestAllExceptions();
					if constexpr (std::floating_point<From> && std::same_as<To, s64>) {
						if (conv >= s64(0x0020'0000'0000'0000) || conv < s64(0xFFE0'0000'0000'0000)) {
							SignalUnimplementedOp();
							SignalException<Exception::FloatingPoint>();
							return;
						}
					}
					if constexpr (std::floating_point<To>) {
						if (IsValidOutput(conv, exc_raised)) {
							fpr.Set<To>(fd, conv);
						}
					}
					else if (exc_raised) {
						SignalException<Exception::FloatingPoint>();
					}
					else {
						fpr.Set<To>(fd, conv);
					}
				};
				if constexpr (instr == CVT_S) INVOKE(Convert, InputType, f32);
				if constexpr (instr == CVT_D) INVOKE(Convert, InputType, f64);
				if constexpr (instr == CVT_W) INVOKE(Convert, InputType, s32);
				if constexpr (instr == CVT_L) INVOKE(Convert, InputType, s64);
			};

			switch (fmt) {
			case FmtTypeID::Float32: INVOKE(Convert, f32); break;
			case FmtTypeID::Float64: INVOKE(Convert, f64); break;
			case FmtTypeID::Int32:   INVOKE(Convert, s32); break;
			case FmtTypeID::Int64:   INVOKE(Convert, s64); break;
			default: OnInvalidFormat(); break;
			}

			TestAllExceptions();
		}
		else if constexpr (OneOf(instr, ROUND_W, TRUNC_W, CEIL_W, FLOOR_W, ROUND_L, TRUNC_L, CEIL_L, FLOOR_L)) {
			/* ROUND.L/ROUND.W/TRUNC.L/TRUNC.W/CEIL.L/CEIL.W/FLOOR.L/FLOOR.W: Round/Truncate/Ceiling/Floor To Single/Long Fixed-point Format;
			   Rounds the contents of floating-point register fs to a value closest to the 32/64-bit
			   fixed-point format and converts them from the specified format (fmt). Stores the result to floating-point register fd. */

			   /* Interpret the source operand (as a float), and round it to an integer (s32 or s64). */
			auto Round = [&] <std::floating_point InputFloat, std::signed_integral OutputInt> {
				InputFloat source = fpr.Get<InputFloat>(fs);

				OutputInt result = [&] {
					if constexpr (OneOf(instr, ROUND_W, ROUND_L)) return OutputInt(std::nearbyint(source));
					if constexpr (OneOf(instr, TRUNC_W, TRUNC_L)) return OutputInt(std::trunc(source));
					if constexpr (OneOf(instr, CEIL_W, CEIL_L))   return OutputInt(std::ceil(source));
					if constexpr (OneOf(instr, FLOOR_W, FLOOR_L)) return OutputInt(std::floor(source));
				}();

				fcr31.cause_unimplemented = TestForUnimplementedException.template operator () < InputFloat, OutputInt > (source);

				/* If the invalid operation exception occurs, but the exception is not enabled, return INT_MAX */
				if (std::fetestexcept(FE_INVALID) && !fcr31.enable_inexact) {
					fpr.Set<OutputInt>(fd, std::numeric_limits<OutputInt>::max());
				}
				else {
					fpr.Set<OutputInt>(fd, result);
				}
			};

			constexpr static bool round_to_s32 = OneOf(instr, ROUND_W, TRUNC_W, CEIL_W, FLOOR_W);

			switch (fmt) {
			case FmtTypeID::Float32:
				if constexpr (round_to_s32) INVOKE(Round, f32, s32);
				else                        INVOKE(Round, f32, s64);
				AdvancePipeline(5);
				break;

			case FmtTypeID::Float64:
				if constexpr (round_to_s32) INVOKE(Round, f64, s32);
				else                        INVOKE(Round, f64, s64);
				AdvancePipeline(5);
				break;

			case FmtTypeID::Int32:
			case FmtTypeID::Int64:
				/* If the input format is integer, the result is undefined,
				   according to table B-19 in "MIPS IV Instruction Set (Revision 3.2)" by Charles Price, 1995.
				   For now, just don't do anything.
				   TODO possibly change */
				AdvancePipeline(1);
				break;

			default:
				OnInvalidFormat();
			}
		}
		else {
			static_assert(AlwaysFalse<instr>);
		}
	}


	template<Cop1Instruction instr>
	void FpuCompute(u32 instr_code)
	{
		if (!cop0.status.cu1) {
			SignalCoprocessorUnusableException(1);
			AdvancePipeline(1);
			return;
		}
		ClearAllExceptions();

		using enum Cop1Instruction;

		auto fd = instr_code >> 6 & 0x1F;
		auto fs = instr_code >> 11 & 0x1F;
		auto ft = instr_code >> 16 & 0x1F;
		auto fmt = instr_code >> 21 & 0x1F;

		if constexpr (OneOf(instr, ADD, SUB, MUL, DIV)) {
			if constexpr (log_cpu_instructions) {
				current_instr_log_output = std::format("{}.{} {}, {}, {}", current_instr_name, FmtToChar(fmt), fd, fs, ft);
			}

			auto Compute = [&] <std::floating_point Float> {
				Float op1 = fpr.Get<Float>(fs);
				if (!IsValidInput(op1)) {
					AdvancePipeline(2);
					return;
				}
				Float op2 = fpr.Get<Float>(ft);
				if (!IsValidInput(op2)) {
					AdvancePipeline(2);
					return;
				}
				Float result = [&] {
					if constexpr (instr == ADD) {
						AdvancePipeline(3);
						return op1 + op2;
					}
					if constexpr (instr == SUB) {
						AdvancePipeline(3);
						return op1 - op2;
					}
					if constexpr (instr == MUL) {
						if constexpr (sizeof(Float) == 4) AdvancePipeline(5);
						else                              AdvancePipeline(8);
						return op1 * op2;
					}
					if constexpr (instr == DIV) {
						if constexpr (sizeof(Float) == 4) AdvancePipeline(29);
						else                              AdvancePipeline(58);
						return op1 / op2;
					}
				}();
				bool exc_raised = TestAllExceptions();
				if (IsValidOutput(result, exc_raised)) {
					fpr.Set<Float>(fd, result);
				}
			};

			switch (fmt) {
			case FmtTypeID::Float32: INVOKE(Compute, f32); break;
			case FmtTypeID::Float64: INVOKE(Compute, f64); break;
			default: OnInvalidFormat(); break;
			}
		}
		else if constexpr (OneOf(instr, ABS, MOV, NEG, SQRT)) {
			if constexpr (log_cpu_instructions) {
				current_instr_log_output = std::format("{}.{} {}, {}", current_instr_name, FmtToChar(fmt), fd, fs);
			}

			auto Compute = [&] <std::floating_point Float> {
				Float op = fpr.Get<Float>(fs);
				if constexpr (instr != MOV) {
					if (!IsValidInput(op)) {
						AdvancePipeline(2);
						return;
					}
				}
				Float result = [&] {
					if constexpr (instr == ABS) {
						AdvancePipeline(1);
						return std::abs(op);
					}
					if constexpr (instr == MOV) {
						AdvancePipeline(1);
						return op;
					}
					if constexpr (instr == NEG) {
						AdvancePipeline(1);
						return -op;
					}
					if constexpr (instr == SQRT) {
						if constexpr (sizeof(op) == 4) AdvancePipeline(29);
						else                           AdvancePipeline(58);
						return std::sqrt(op);
					}
				}();
				if constexpr (instr == MOV) {
					fpr.Set<Float>(fd, result);
				}
				else {
					bool exc_raised = TestAllExceptions();
					if (IsValidOutput(result, exc_raised)) {
						fpr.Set<Float>(fd, result);
					}
				}
			};

			switch (fmt) {
			case FmtTypeID::Float32: INVOKE(Compute, f32); break;
			case FmtTypeID::Float64: INVOKE(Compute, f64); break;
			default: OnInvalidFormat(); break;
			}
		}
		else {
			static_assert(AlwaysFalse<instr>);
		}
	}


	template<Cop1Instruction instr>
	void FpuBranch(u32 instr_code)
	{
		if (!cop0.status.cu1) {
			SignalCoprocessorUnusableException(1);
			AdvancePipeline(1);
			return;
		}
		ClearAllExceptions();

		using enum Cop1Instruction;

		/* For all instructions: Adds the instruction address in the delay slot and a 16-bit offset (shifted 2 bits
		   to the left and sign-extended) to calculate the branch target address.

		   BC1T: Branch On FPU True;
		   If the FPU condition line is true, branches to the target address (delay of one instruction).

		   BC1F: Branch On FPU False;
		   If the FPU condition line is false, branches to the target address (delay of one instruction).

		   BC1TL: Branch On FPU True Likely;
		   If the FPU condition line is true, branches to the target address (delay of one instruction).
		   If conditional branch does not take place, the instruction in the delay slot is invalidated.

		   BC1FL: Branch On FPU False Likely;
		   If the FPU condition line is false, branches to the target address (delay of one instruction).
		   If conditional branch does not take place, the instruction in the delay slot is invalidated. */

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{} ${:X}", current_instr_name, s16(instr_code));
		}

		s64 offset = s64(s16(instr_code)) << 2;

		bool branch_cond = [&] {
			if constexpr (OneOf(instr, BC1T, BC1TL)) return  fcr31.c;
			if constexpr (OneOf(instr, BC1F, BC1FL)) return !fcr31.c;
		}();

		if (branch_cond) {
			PrepareJump(pc + offset);
		}
		else if constexpr (OneOf(instr, BC1TL, BC1FL)) {
			pc += 4; /* The instruction in the branch delay slot is discarded. TODO: manual says "invalidated" */
		}

		AdvancePipeline(1); /* TODO: not 2? */
	}


	void FpuCompare(u32 instr_code)
	{
		/* Floating-point Compare;
		   Interprets and arithmetically compares the contents of FPU registers fs and ft
		   in the specified format (fmt). The result is identified by comparison and the
		   specified condition (cond). After a delay of one instruction, the comparison
		   result can be used by the FPU branch instruction of the CPU. */
		if (!cop0.status.cu1) {
			SignalCoprocessorUnusableException(1);
			AdvancePipeline(1);
			return;
		}
		ClearAllExceptions();

		auto cond = instr_code & 0xF;
		auto fs = instr_code >> 11 & 0x1F;
		auto ft = instr_code >> 16 & 0x1F;
		auto fmt = instr_code >> 21 & 0x1F;

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("C.{}.{} {}, {}", compare_cond_strings[cond], FmtToChar(fmt), fs, ft);
		}

		auto Compare = [&] <std::floating_point Float> {
			/* See VR4300 User's Manual by NEC, p. 566
			Ordered instructions are: LE, LT, NGE, NGL, NGLE, NGT, SEQ, SF (cond.3 set)
			Unordered: EQ, F, OLE, OLT, UEQ, ULE, ULT, UN (cond.3 clear) */
			auto IsValidInput = [&](std::floating_point auto f) {
				if (std::isnan(f) && ((cond & 8) || IsQuietNan(f))) {
					bool signal_fpu_exc = SignalInvalidOp();
					if (signal_fpu_exc) SignalException<Exception::FloatingPoint>();
					return !signal_fpu_exc;
				}
				else return true;
			};
			Float op1 = fpr.Get<Float>(fs);
			Float op2 = fpr.Get<Float>(ft);
			if (std::isnan(op1) || std::isnan(op2)) {
				if (!IsValidInput(op1)) {
					AdvancePipeline(2);
					return;
				}
				if (!IsValidInput(op2)) {
					AdvancePipeline(2);
					return;
				}
				fcr31.c = cond & 1;
			}
			else {
				fcr31.c = (cond >> 2 & 1) & (op1 < op2) | (cond >> 1 & 1) & (op1 == op2);
			}
			AdvancePipeline(1);
		};

		switch (fmt) {
		case FmtTypeID::Float32: INVOKE(Compare, f32); break;
		case FmtTypeID::Float64: INVOKE(Compare, f64); break;
		default: OnInvalidFormat(); break;
		}
	}

	template bool IsQuietNan<f32>(f32);
	template bool IsQuietNan<f64>(f64);

	template void FpuLoad<Cop1Instruction::LWC1>(u32);
	template void FpuLoad<Cop1Instruction::LDC1>(u32);

	template void FpuStore<Cop1Instruction::SWC1>(u32);
	template void FpuStore<Cop1Instruction::SDC1>(u32);

	template void FpuMove<Cop1Instruction::MTC1>(u32);
	template void FpuMove<Cop1Instruction::MFC1>(u32);
	template void FpuMove<Cop1Instruction::CTC1>(u32);
	template void FpuMove<Cop1Instruction::CFC1>(u32);
	template void FpuMove<Cop1Instruction::DMTC1>(u32);
	template void FpuMove<Cop1Instruction::DMFC1>(u32);
	template void FpuMove<Cop1Instruction::DCFC1>(u32);
	template void FpuMove<Cop1Instruction::DCTC1>(u32);

	template void FpuConvert<Cop1Instruction::CVT_S>(u32);
	template void FpuConvert<Cop1Instruction::CVT_D>(u32);
	template void FpuConvert<Cop1Instruction::CVT_L>(u32);
	template void FpuConvert<Cop1Instruction::CVT_W>(u32);
	template void FpuConvert<Cop1Instruction::ROUND_L>(u32);
	template void FpuConvert<Cop1Instruction::ROUND_W>(u32);
	template void FpuConvert<Cop1Instruction::TRUNC_L>(u32);
	template void FpuConvert<Cop1Instruction::TRUNC_W>(u32);
	template void FpuConvert<Cop1Instruction::CEIL_L>(u32);
	template void FpuConvert<Cop1Instruction::CEIL_W>(u32);
	template void FpuConvert<Cop1Instruction::FLOOR_L>(u32);
	template void FpuConvert<Cop1Instruction::FLOOR_W>(u32);

	template void FpuCompute<Cop1Instruction::ADD>(u32);
	template void FpuCompute<Cop1Instruction::SUB>(u32);
	template void FpuCompute<Cop1Instruction::MUL>(u32);
	template void FpuCompute<Cop1Instruction::DIV>(u32);
	template void FpuCompute<Cop1Instruction::ABS>(u32);
	template void FpuCompute<Cop1Instruction::MOV>(u32);
	template void FpuCompute<Cop1Instruction::NEG>(u32);
	template void FpuCompute<Cop1Instruction::SQRT>(u32);

	template void FpuBranch<Cop1Instruction::BC1T>(u32);
	template void FpuBranch<Cop1Instruction::BC1F>(u32);
	template void FpuBranch<Cop1Instruction::BC1TL>(u32);
	template void FpuBranch<Cop1Instruction::BC1FL>(u32);

	template s32 FGR::Get<s32>(size_t) const;
	template s64 FGR::Get<s64>(size_t) const;
	template f32 FGR::Get<f32>(size_t) const;
	template f64 FGR::Get<f64>(size_t) const;

	template void FGR::Set<s32>(size_t, s32);
	template void FGR::Set<s64>(size_t, s64);
	template void FGR::Set<f32>(size_t, f32);
	template void FGR::Set<f64>(size_t, f64);
}