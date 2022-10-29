module VR4300:COP1;

import :COP0;
import :CPU;
import :Exceptions;
import :MMU;
import :Operation;

import DebugOptions;
import Util;

namespace VR4300
{
	void FCR31::Set(u32 data)
	{
		*this = std::bit_cast<FCR31>(data);
		auto new_rounding_mode = [&] {
			switch (rm) {
			case 0: return FE_TONEAREST;  /* RN */
			case 1: return FE_TOWARDZERO; /* RZ */
			case 2: return FE_UPWARD;     /* RP */
			case 3: return FE_DOWNWARD;   /* RM */
			default: std::unreachable();
			}
		}();
		std::fesetround(new_rounding_mode);
	}


	u32 FPUControl::Get(size_t index) const
	{
		static constexpr u32 fcr0 = 0; /* TODO */
		if (index == 31) {
			return std::bit_cast<u32>(fcr31);
		}
		else if (index == 0) {
			return fcr0;
		}
		else {
			return 0; /* Only #0 and #31 are "valid". */
		}
	}


	void FPUControl::Set(size_t index, u32 data)
	{
		if (index == 31) {
			fcr31.Set(data);
		}
	}


	template<FPUNumericType T>
	T FGR::Get(size_t index) const
	{
		if constexpr (std::is_same_v<T, s32>) {
			return s32(fpr[index]);
		}
		if constexpr (std::is_same_v<T, f32>) {
			return std::bit_cast<f32>(s32(fpr[index]));
		}
		if constexpr (std::is_same_v<T, s64>) {
			if (cop0_reg.status.fr) {
				return fpr[index];
			}
			else {
				/* If the index is odd, then the result is undefined.
				 The only way I can get PeterLemons's fpu tests to pass is to add 1 to the index if it is odd. */
				auto aligned_index = (index + (index & 1)) & 0x1F;
				return fpr[aligned_index] & 0xFFFF'FFFF | fpr[aligned_index + 1] << 32;
			}
		}
		if constexpr (std::is_same_v<T, f64>) {
			if (cop0_reg.status.fr) {
				return std::bit_cast<f64>(fpr[index]);
			}
			else {
				/* If the index is odd, then the result is undefined.
				 The only way I can get PeterLemons's fpu tests to pass is to add 1 to the index if it is odd. */
				auto aligned_index = (index + (index & 1)) & 0x1F;
				return std::bit_cast<f64, s64>(fpr[aligned_index] & 0xFFFF'FFFF | fpr[aligned_index + 1] << 32);
			}
		}
	}


	template<FPUNumericType T>
	void FGR::Set(size_t index, T data)
	{
		if constexpr (std::is_same_v<T, s32>) {
			fpr[index] = data;
		}
		if constexpr (std::is_same_v<T, f32>) {
			fpr[index] = std::bit_cast<s32>(data); /* TODO: no clue if sign-extending will lead to unwanted results */
		}
		if constexpr (std::is_same_v<T, s64>) {
			if (cop0_reg.status.fr) {
				fpr[index] = data;
			}
			else {
				/* If the index is odd, then the result is undefined.
				 The only way I can get PeterLemons's fpu tests to pass is to add 1 to the index if it is odd. */
				auto aligned_index = (index + (index & 1)) & 0x1F;
				fpr[aligned_index] = data & 0xFFFFFFFF;
				fpr[aligned_index + 1] = data >> 32; /* TODO: no clue if sign-extending will lead to unwanted results */
			}
		}
		if constexpr (std::is_same_v<T, f64>) {
			if (cop0_reg.status.fr) {
				fpr[index] = std::bit_cast<s64, f64>(data);
			}
			else {
				/* If the index is odd, then the result is undefined.
				 The only way I can get PeterLemons's fpu tests to pass is to add 1 to the index if it is odd. */
				auto aligned_index = (index + (index & 1)) & 0x1F;
				s64 conv = std::bit_cast<s64>(data);
				fpr[aligned_index] = conv & 0xFFFFFFFF;
				fpr[aligned_index + 1] = conv >> 32; /* TODO: no clue if sign-extending will lead to unwanted results */
			}
		}
	}


	/* Table 17.4 in VR4300 user manual by NEC; the 'fmt' instruction operand specifies in which format registers should be interpreted in.
	   The below maps formats to identifiers. */
	enum FmtTypeID
	{
		Float32 = 16,
		Float64 = 17,
		Int32   = 20,
		Int64   = 21
	};


	constexpr std::array compare_cond_strings = {
		"F", "UN", "EQ", "UEQ", "OLT", "ULT", "OLE", "ULE", "SF", "NGLE", "SEQ", "NGL", "LT", "NGE", "LE", "NGT"
	};


	bool unimplemented_operation = false;


	constexpr std::string_view FmtToString(FmtTypeID fmt)
	{
		switch (fmt) {
		case FmtTypeID::Float32: return "S";
		case FmtTypeID::Float64: return "D";
		case FmtTypeID::Int32: return "W";
		case FmtTypeID::Int64: return "L";
		default: return "INVALID";
		}
	}


	constexpr std::string_view FmtToString(int fmt)
	{
		return FmtToString(static_cast<FmtTypeID>(fmt));
	}


	void InitializeFPU()
	{
		std::fesetround(FE_TONEAREST); /* corresponds to fcr31.rm == 0b00 */
	}


	void TestAllExceptions()
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
		fcr31.cause_I = std::fetestexcept(FE_INEXACT);
		fcr31.cause_U = std::fetestexcept(FE_UNDERFLOW);
		fcr31.cause_O = std::fetestexcept(FE_OVERFLOW);
		fcr31.cause_Z = std::fetestexcept(FE_DIVBYZERO);
		fcr31.cause_V = std::fetestexcept(FE_INVALID);
		fcr31.cause_E = unimplemented_operation;

		if (fcr31.cause_E == 1) {
			SignalException<Exception::FloatingPoint>();
		}
		else {
			u32 fcr31_int = std::bit_cast<u32>(fcr31);
			u32 enables = fcr31_int >> 7;
			u32 causes = fcr31_int >> 12;
			if (causes & enables & 0x1F) {
				SignalException<Exception::FloatingPoint>();
			}
			fcr31_int |= (causes & ~enables & 0x1F) << 2;
			fcr31.Set(fcr31_int);
		}
	}


	void TestUnimplementedOperationException()
	{
		fcr31.cause_E = unimplemented_operation;
		if (unimplemented_operation) {
			SignalException<Exception::FloatingPoint>();
		}
	}


	void TestInvalidException()
	{
		if (fcr31.cause_V) {
			SignalException<Exception::FloatingPoint>();
		}
	}


	void SetInvalidException(bool state)
	{
		fcr31.cause_V = state;
	}


	void ClearAllExceptions()
	{
		u32 fcr31_int = std::bit_cast<u32>(fcr31);
		fcr31_int &= ~0x3F000;
		fcr31.Set(fcr31_int);
	}


	template<COP1Instruction instr>
	void FPULoad(u32 instr_code)
	{
		using enum COP1Instruction;

		s16 offset = instr_code & 0xFFFF;
		auto ft = instr_code >> 16 & 0x1F;
		auto base = instr_code >> 21 & 0x1F;
		auto address = gpr[base] + offset;

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{} {}, ${:X}", current_instr_name, ft, static_cast<std::make_unsigned<decltype(address)>::type>(address));
		}

		auto result = [&] {
			if constexpr (instr == LWC1) {
				/* Load Word To FPU;
				   Sign-extends the 16-bit offset and adds it to the CPU register base to generate
				   an address. Loads the contents of the word specified by the address to the
				   FPU general purpose register ft. */
				return ReadVirtual<s32>(address);
			}
			else if constexpr (instr == LDC1) {
				/* Load Doubleword To FPU;
				   Sign-extends the 16-bit offset and adds it to the CPU register base to generate
				   an address. Loads the contents of the doubleword specified by the address to
				   the FPU general purpose registers ft and ft+1 when FR = 0, or to the FPU
				   general purpose register ft when FR = 1. */
				return ReadVirtual<s64>(address);
			}
			else {
				static_assert(AlwaysFalse<instr>);
			}
		}();

		AdvancePipeline(1);

		if (exception_has_occurred) {
			return;
		}

		fpr.Set(ft, result);
	}


	template<COP1Instruction instr>
	void FPUStore(u32 instr_code)
	{
		using enum COP1Instruction;

		s16 offset = instr_code & 0xFFFF;
		auto ft = instr_code >> 16 & 0x1F;
		auto base = instr_code >> 21 & 0x1F;
		auto address = gpr[base] + offset;

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{} {}, ${:X}", current_instr_name, ft, static_cast<std::make_unsigned<decltype(address)>::type>(address));
		}

		if constexpr (instr == SWC1) {
			/* Store Word From FPU;
			   Sign-extends the 16-bit offset and adds it to the CPU register base to generate
			   an address. Stores the contents of the FPU general purpose register ft to the
			   memory position specified by the address. */
			WriteVirtual(address, fpr.Get<s32>(ft));
		}
		else if constexpr (instr == SDC1) {
			/* Store Doubleword From FPU;
			   Sign-extends the 16-bit offset and adds it to the CPU register base to generate
			   an address. Stores the contents of the FPU general purpose registers ft and
			   ft+1 to the memory position specified by the address when FR = 0, and the
			   contents of the FPU general purpose register ft when FR = 1. */
			WriteVirtual(address, fpr.Get<s64>(ft));
		}
		else {
			static_assert(AlwaysFalse<instr>);
		}

		AdvancePipeline(1);
	}


	template<COP1Instruction instr>
	void FPUMove(u32 instr_code)
	{
		using enum COP1Instruction;

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
			if (fs == 31) { // Only #31 is writeable
				fpu_control.Set(fs, u32(gpr[rt]));
				TestAllExceptions();
			}
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
		else {
			static_assert(AlwaysFalse<instr>);
		}

		AdvancePipeline(1);
	}


	template<COP1Instruction instr>
	void FPUConvert(u32 instr_code)
	{
		using enum COP1Instruction;

		/* Test for unimplemented operation exception sources for CVT/round instructions. These cannot be found out from std::fetestexcept.
		   This function should be called after the conversion has been made.
		   For all these instructions, an unimplemented exception will occur if either:
			 * If the source operand is infinity or NaN, or
			 * If overflow occurs during conversion to integer format. */
		auto TestForUnimplementedException = [&] <typename From, typename To> (From source) -> bool {
			if constexpr (std::is_integral_v<From> && std::is_same_v<To, f64>) {
				return false; /* zero-cost shortcut; integers cannot be infinity or NaN, and the operation is then always exact when converting to a double */
			}
			if constexpr (std::is_floating_point_v<From>) {
				if (std::isnan(source) || std::isinf(source)) {
					return true;
				}
			}
			if constexpr (std::is_integral_v<To>) {
				return std::fetestexcept(FE_OVERFLOW) != 0; // TODO: should this also include underflow?
			}
			return false;
		};

		auto fd = instr_code >> 6 & 0x1F;
		auto fs = instr_code >> 11 & 0x1F;
		auto fmt = instr_code >> 21 & 0x1F;

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{}.{} {}, {}", current_instr_name, FmtToString(fmt), fd, fs);
		}

		if constexpr (OneOf(instr, CVT_S, CVT_D, CVT_W, CVT_L)) {
			/* CVT.S/CVT.D: Convert To Single/Double Floating-point Format;
			   Converts the contents of floating-point register fs from the specified format (fmt)
			   to a single/double-precision floating-point format. Stores the rounded result to floating-point register fd.

			   CVT.W/CVT.L: Convert To Single/Long Fixed-point Format;
			   Converts the contents of floating-point register fs from the specified format (fmt)
			   to a 32/64-bit fixed-point format. Stores the rounded result to floating-point register fd. */
			auto Convert = [&] <FPUNumericType InputType> {
				/* Interpret a source operand as type 'From', "convert" (round according to the current rounding mode) it to a new type 'To', and store the result. */
				auto Convert = [&] <FPUNumericType From, FPUNumericType To> {
					From source = fpr.Get<From>(fs);
					std::feclearexcept(FE_ALL_EXCEPT);
					To conv = To(source); 
					fpr.Set<To>(fd, conv);

					unimplemented_operation = TestForUnimplementedException.template operator () < From, To > (source);

					/* Note: if the input and output formats are the same, the result is undefined,
					   according to table B-19 in "MIPS IV Instruction Set (Revision 3.2)" by Charles Price, 1995. */
					/* TODO: the below is assuming that conv. between W and L takes 2 cycles.
					   See footnote 2 in table 7-14, VR4300 manual */
					static constexpr int cycles = [&] {
						     if constexpr (std::is_same_v<From, To>)                             return 1;
						else if constexpr (std::is_same_v<From, f32> && std::is_same_v<To, f64>) return 1;
						else if constexpr (std::is_same_v<From, f64> && std::is_same_v<To, f32>) return 2;
						else if constexpr (std::is_integral_v<From> && std::is_integral_v<To>)   return 2;
						else                                                                     return 5;
					}();
					AdvancePipeline(cycles);
				};

				if constexpr (instr == CVT_S) Convert.template operator() < InputType, f32 > ();
				if constexpr (instr == CVT_D) Convert.template operator() < InputType, f64 > ();
				if constexpr (instr == CVT_W) Convert.template operator() < InputType, s32 > ();
				if constexpr (instr == CVT_L) Convert.template operator() < InputType, s64 > ();
			};

			switch (fmt) {
			case FmtTypeID::Float32:
				Convert.template operator() <f32> ();
				break;

			case FmtTypeID::Float64:
				Convert.template operator() <f64> ();
				break;

			case FmtTypeID::Int32:
				Convert.template operator() <s32> ();
				break;

			case FmtTypeID::Int64:
				Convert.template operator() <s64> ();
				break;

			default:
				unimplemented_operation = true;
				std::feclearexcept(FE_ALL_EXCEPT);
				AdvancePipeline(2);
				break;
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

				std::feclearexcept(FE_ALL_EXCEPT);

				OutputInt result = [&] {
					if constexpr (OneOf(instr, ROUND_W, ROUND_L)) return OutputInt(std::nearbyint(source));
					if constexpr (OneOf(instr, TRUNC_W, TRUNC_L)) return OutputInt(std::trunc(source));
					if constexpr (OneOf(instr, CEIL_W, CEIL_L))   return OutputInt(std::ceil(source));
					if constexpr (OneOf(instr, FLOOR_W, FLOOR_L)) return OutputInt(std::floor(source));
				}();

				unimplemented_operation = TestForUnimplementedException.template operator () < InputFloat, OutputInt > (source);

				/* If the invalid operation exception occurs, but the exception is not enabled, return INT_MAX */
				if (std::fetestexcept(FE_INVALID) && !fcr31.enable_I) {
					fpr.Set<OutputInt>(fd, std::numeric_limits<OutputInt>::max());
				}
				else {
					fpr.Set<OutputInt>(fd, result);
				}
			};

			constexpr static bool round_to_s32 = OneOf(instr, ROUND_W, TRUNC_W, CEIL_W, FLOOR_W);

			switch (fmt) {
			case FmtTypeID::Float32:
				if constexpr (round_to_s32) {
					Round.template operator() < f32 /* input format */, s32 /* output format */ > ();
				}
				else {
					Round.template operator() < f32, s64 > ();
				}
				AdvancePipeline(5);
				break;

			case FmtTypeID::Float64:
				if constexpr (round_to_s32) {
					Round.template operator() < f64, s32 > ();
				}
				else {
					Round.template operator() < f64, s64 > ();
				}
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
				unimplemented_operation = true;
				std::feclearexcept(FE_ALL_EXCEPT);
				AdvancePipeline(2);
				break;
			}

			TestAllExceptions();
		}
		else {
			static_assert(AlwaysFalse<instr>);
		}
	}


	template<COP1Instruction instr>
	void FPUCompute(u32 instr_code)
	{
		using enum COP1Instruction;

		auto fd = instr_code >> 6 & 0x1F;
		auto fs = instr_code >> 11 & 0x1F;
		auto ft = instr_code >> 16 & 0x1F;
		auto fmt = instr_code >> 21 & 0x1F;

		if constexpr (OneOf(instr, ADD, SUB, MUL, DIV)) {
			/* Floating-point Add/Subtract/Multiply/Divide;
			   Arithmetically adds/subtracts/multiplies/divides the contents of floating-point registers
			   fs and ft in the specified format (fmt). Stores the rounded result to floating-point register fd. */

			if constexpr (log_cpu_instructions) {
				current_instr_log_output = std::format("{}.{} {}, {}, {}", current_instr_name, FmtToString(fmt), fd, fs, ft);
			}

			auto Compute = [&] <std::floating_point Float> {
				Float op1 = fpr.Get<Float>(fs);
				Float op2 = fpr.Get<Float>(ft);
				std::feclearexcept(FE_ALL_EXCEPT);
				if constexpr (instr == ADD) {
					fpr.Set<Float>(fd, op1 + op2);
					AdvancePipeline(3);
				}
				if constexpr (instr == SUB) {
					fpr.Set<Float>(fd, op1 - op2);
					AdvancePipeline(3);
				}
				if constexpr (instr == MUL) {
					fpr.Set<Float>(fd, op1 * op2);
					AdvancePipeline(29);
				}
				if constexpr (instr == DIV) {
					fpr.Set<Float>(fd, op1 / op2);
					AdvancePipeline(58);
				}
			};

			switch (fmt) {
			case FmtTypeID::Float32:
				Compute.template operator() <f32> ();
				unimplemented_operation = false;
				break;

			case FmtTypeID::Float64:
				Compute.template operator() <f64> ();
				unimplemented_operation = false;
				break;

			default:
				AdvancePipeline(2);
				unimplemented_operation = true;
				std::feclearexcept(FE_ALL_EXCEPT);
				break;
			}

			TestAllExceptions();
		}
		else if constexpr (OneOf(instr, ABS, MOV, NEG, SQRT)) {
			/* Floating-point Absolute Value;
			   Calculates the arithmetic absolute value of the contents of floating-point
			   register fs in the specified format (fmt). Stores the result to floating-point register fd.

			   Floating-point Move;
			   Copies the contents of floating-point register fs to floating-point register fd
			   in the specified format (fmt).

			   Floating-point Negate;
			   Arithmetically negates the contents of floating-point register fs in the
			   specified format (fmt). Stores the result to floating-point register fd.

			   Floating-point Square Root;
			   Calculates arithmetic positive square root of the contents of floating-point
			   register fs in the specified format. Stores the rounded result to floating-point register fd. */

			if constexpr (log_cpu_instructions) {
				current_instr_log_output = std::format("{}.{} {}, {}", current_instr_name, FmtToString(fmt), fd, fs);
			}

			auto Compute = [&] <std::floating_point Float> {
				Float op = fpr.Get<Float>(fs);
				std::feclearexcept(FE_ALL_EXCEPT);
				if constexpr (instr == ABS) {
					fpr.Set<Float>(fd, std::abs(op)); // TODO: could this result in host exception flags changing?
					AdvancePipeline(1);
				}
				if constexpr (instr == MOV) {
					fpr.Set<Float>(fd, op);
					AdvancePipeline(1);
				}
				if constexpr (instr == NEG) {
					fpr.Set<Float>(fd, -op);
					AdvancePipeline(1);
				}
				if constexpr (instr == SQRT) {
					fpr.Set<Float>(fd, std::sqrt(op));
					if constexpr (std::is_same_v<Float, f32>) {
						AdvancePipeline(29);
					}
					else { /* f64 */
						AdvancePipeline(58);
					}
				}
			};

			switch (fmt) {
			case FmtTypeID::Float32:
				Compute.template operator() < f32 > ();
				unimplemented_operation = false;
				break;

			case FmtTypeID::Float64:
				Compute.template operator() < f64 > ();
				unimplemented_operation = false;
				break;

			default:
				AdvancePipeline(2);
				unimplemented_operation = true;
				std::feclearexcept(FE_ALL_EXCEPT);
				break;
			}

			/* MOV can cause only the unimplemented operation exception. */
			if constexpr (instr == MOV) {
				TestUnimplementedOperationException();
			}
			else {
				TestAllExceptions();
			}
		}
		else {
			static_assert(AlwaysFalse<instr>);
		}
	}


	template<COP1Instruction instr>
	void FPUBranch(u32 instr_code)
	{
		using enum COP1Instruction;

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
			current_instr_log_output = std::format("{} ${:X}", current_instr_name, s16(instr_code & 0xFFFF));
		}

		s64 offset = s64(s16(instr_code & 0xFFFF)) << 2;

		bool branch_cond = [&] {
			if constexpr (OneOf(instr, BC1T, BC1TL)) {
				return fcr31.c;
			}
			if constexpr (OneOf(instr, BC1F, BC1FL)) {
				return !fcr31.c;
			}
		}();

		if (branch_cond) {
			PrepareJump(pc + offset);
		}
		else if constexpr (OneOf(instr, BC1TL, BC1FL)) {
			pc += 4; /* The instruction in the branch delay slot is discarded. TODO: manual says "invalidated" */
		}

		AdvancePipeline(1); /* TODO: not 2? */
	}


	void FPUCompare(u32 instr_code)
	{
		/* Floating-point Compare;
		   Interprets and arithmetically compares the contents of FPU registers fs and ft
		   in the specified format (fmt). The result is identified by comparison and the
		   specified condition (cond). After a delay of one instruction, the comparison
		   result can be used by the FPU branch instruction of the CPU. */

		auto cond = instr_code & 0xF;
		auto fs = instr_code >> 11 & 0x1F;
		auto ft = instr_code >> 16 & 0x1F;
		auto fmt = instr_code >> 21 & 0x1F;

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("C.{}.{} {}, {}", compare_cond_strings[cond], FmtToString(fmt), fs, ft);
		}

		auto Compare = [&] <std::floating_point Float> {
			Float op1 = fpr.Get<Float>(fs);
			Float op2 = fpr.Get<Float>(ft);

			bool comp_result = [&] { /* See VR4300 User's Manual by NEC, p. 566 */
				if (std::isnan(op1) || std::isnan(op2)) {
					SetInvalidException(cond & 8);
					return bool(cond & 1);
				}
				else {
					SetInvalidException(false);
					return (cond & 4) && op1 < op2 || (cond & 2) && op1 == op2;
				}
			}();

			fcr31.c = comp_result; /* TODO: also set 'COC1' to result? Or is COC1 the same as fcr31.C? */
		};

		/* TODO not clear if this instruction should clear all exception flags other than invalid and unimplemented */

		switch (fmt) {
		case FmtTypeID::Float32:
			Compare.template operator() < f32 > ();
			unimplemented_operation = false;
			AdvancePipeline(1);
			break;

		case FmtTypeID::Float64:
			Compare.template operator() < f64 > ();
			unimplemented_operation = false;
			AdvancePipeline(1);
			break;

		default:
			unimplemented_operation = true;
			AdvancePipeline(2);
			break;
		}

		TestUnimplementedOperationException();
		TestInvalidException();
	}


	template void FPULoad<COP1Instruction::LWC1>(u32);
	template void FPULoad<COP1Instruction::LDC1>(u32);

	template void FPUStore<COP1Instruction::SWC1>(u32);
	template void FPUStore<COP1Instruction::SDC1>(u32);

	template void FPUMove<COP1Instruction::MTC1>(u32);
	template void FPUMove<COP1Instruction::MFC1>(u32);
	template void FPUMove<COP1Instruction::CTC1>(u32);
	template void FPUMove<COP1Instruction::CFC1>(u32);
	template void FPUMove<COP1Instruction::DMTC1>(u32);
	template void FPUMove<COP1Instruction::DMFC1>(u32);

	template void FPUConvert<COP1Instruction::CVT_S>(u32);
	template void FPUConvert<COP1Instruction::CVT_D>(u32);
	template void FPUConvert<COP1Instruction::CVT_L>(u32);
	template void FPUConvert<COP1Instruction::CVT_W>(u32);
	template void FPUConvert<COP1Instruction::ROUND_L>(u32);
	template void FPUConvert<COP1Instruction::ROUND_W>(u32);
	template void FPUConvert<COP1Instruction::TRUNC_L>(u32);
	template void FPUConvert<COP1Instruction::TRUNC_W>(u32);
	template void FPUConvert<COP1Instruction::CEIL_L>(u32);
	template void FPUConvert<COP1Instruction::CEIL_W>(u32);
	template void FPUConvert<COP1Instruction::FLOOR_L>(u32);
	template void FPUConvert<COP1Instruction::FLOOR_W>(u32);

	template void FPUCompute<COP1Instruction::ADD>(u32);
	template void FPUCompute<COP1Instruction::SUB>(u32);
	template void FPUCompute<COP1Instruction::MUL>(u32);
	template void FPUCompute<COP1Instruction::DIV>(u32);
	template void FPUCompute<COP1Instruction::ABS>(u32);
	template void FPUCompute<COP1Instruction::MOV>(u32);
	template void FPUCompute<COP1Instruction::NEG>(u32);
	template void FPUCompute<COP1Instruction::SQRT>(u32);

	template void FPUBranch<COP1Instruction::BC1T>(u32);
	template void FPUBranch<COP1Instruction::BC1F>(u32);
	template void FPUBranch<COP1Instruction::BC1TL>(u32);
	template void FPUBranch<COP1Instruction::BC1FL>(u32);

	template s32 FGR::Get<s32>(size_t) const;
	template s64 FGR::Get<s64>(size_t) const;
	template f32 FGR::Get<f32>(size_t) const;
	template f64 FGR::Get<f64>(size_t) const;

	template void FGR::Set<s32>(size_t, s32);
	template void FGR::Set<s64>(size_t, s64);
	template void FGR::Set<f32>(size_t, f32);
	template void FGR::Set<f64>(size_t, f64);
}