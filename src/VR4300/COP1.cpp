module VR4300:COP1;

import :CPU;
import :Exceptions;
import :MMU;
import :Operation;
import :Registers;

import DebugOptions;

namespace VR4300
{
	/* Table 17.4 in VR4300 user manual by NEC; the 'fmt' instruction operand specifies in which format registers should be interpreted in.
	   The below maps formats to identifiers. */
	enum FmtTypeID
	{
		Float32 = 16,
		Float64 = 17,
		Int32   = 20,
		Int64   = 21
	};


	constexpr static std::array compare_cond_strings = {
		"F", "UN", "EQ", "UEQ", "OLT", "ULT", "OLE", "ULE", "SF", "NGLE", "SEQ", "NGL", "LT", "NGE", "LE", "NGT"
	};


	bool unimplemented_operation = false;


	constexpr std::string_view FmtToString(FmtTypeID fmt)
	{
		switch (fmt)
		{
		case FmtTypeID::Float32: return "S";
		case FmtTypeID::Float64: return "D";
		case FmtTypeID::Int32: return "W";
		case FmtTypeID::Int64: return "L";
		default: return "INVALID";
		}
	}


	constexpr std::string_view FmtToString(int fmt)
	{
		switch (static_cast<FmtTypeID>(fmt))
		{
		case FmtTypeID::Float32: return "S";
		case FmtTypeID::Float64: return "D";
		case FmtTypeID::Int32: return "W";
		case FmtTypeID::Int64: return "L";
		default: return "INVALID";
		}
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

		if (fcr31.cause_E == 1)
		{
			SignalException<Exception::FloatingPoint>();
		}
		else
		{
			u32 fcr31_int = fcr31.Get();
			u32 enables = fcr31_int >> 7;
			u32 causes = fcr31_int >> 12;
			if (causes & enables & 0x1F)
			{
				SignalException<Exception::FloatingPoint>();
			}
			fcr31_int |= (causes & ~enables & 0x1F) << 2;
			fcr31.Set(fcr31_int);
		}
	}


	void TestUnimplementedOperationException()
	{
		fcr31.cause_E = unimplemented_operation;
		if (unimplemented_operation)
		{
			SignalException<Exception::FloatingPoint>();
		}
	}


	void TestInvalidException()
	{
		if (fcr31.cause_V)
		{
			SignalException<Exception::FloatingPoint>();
		}
	}


	void SetInvalidException(bool state)
	{
		fcr31.cause_V = state;
	}


	void ClearAllExceptions()
	{
		u32 fcr31_int = fcr31.Get();
		fcr31_int &= ~0x3F000;
		fcr31.Set(fcr31_int);
	}


	template<COP1Instruction instr>
	void FPULoad(const u32 instr_code)
	{
		using enum COP1Instruction;

		const s16 offset = instr_code & 0xFFFF;
		const auto ft = instr_code >> 16 & 0x1F;
		const auto base = instr_code >> 21 & 0x1F;
		const auto address = gpr[base] + offset;

		if constexpr (log_cpu_instructions)
		{
			current_instr_log_output = std::format("{} {}, ${:X}", current_instr_name, ft, static_cast<std::make_unsigned<decltype(address)>::type>(address));
		}

		const auto result = [&] {
			if constexpr (instr == LWC1)
			{
				/* Load Word To FPU;
				   Sign-extends the 16-bit offset and adds it to the CPU register base to generate
				   an address. Loads the contents of the word specified by the address to the
				   FPU general purpose register ft. */
				return ReadVirtual<s32>(address);
			}
			else if constexpr (instr == LDC1)
			{
				/* Load Doubleword To FPU;
				   Sign-extends the 16-bit offset and adds it to the CPU register base to generate
				   an address. Loads the contents of the doubleword specified by the address to
				   the FPU general purpose registers ft and ft+1 when FR = 0, or to the FPU
				   general purpose register ft when FR = 1. */
				return ReadVirtual<s64>(address);
			}
			else
			{
				static_assert(instr != instr, "\"FPU_Load\" template function called, but no matching load instruction was found.");
			}
		}();

		AdvancePipeline<1>();

		if (exception_has_occurred)
			return;

		fpr.Set(ft, result);
	}


	template<COP1Instruction instr>
	void FPUStore(const u32 instr_code)
	{
		using enum COP1Instruction;

		const s16 offset = instr_code & 0xFFFF;
		const auto ft = instr_code >> 16 & 0x1F;
		const auto base = instr_code >> 21 & 0x1F;
		const auto address = gpr[base] + offset;

		if constexpr (log_cpu_instructions)
		{
			current_instr_log_output = std::format("{} {}, ${:X}", current_instr_name, ft, static_cast<std::make_unsigned<decltype(address)>::type>(address));
		}

		if constexpr (instr == SWC1)
		{
			/* Store Word From FPU;
			   Sign-extends the 16-bit offset and adds it to the CPU register base to generate
			   an address. Stores the contents of the FPU general purpose register ft to the
			   memory position specified by the address. */
			WriteVirtual<s32>(address, fpr.Get<s32>(ft));
		}
		else if constexpr (instr == SDC1)
		{
			/* Store Doubleword From FPU;
			   Sign-extends the 16-bit offset and adds it to the CPU register base to generate
			   an address. Stores the contents of the FPU general purpose registers ft and
			   ft+1 to the memory position specified by the address when FR = 0, and the
			   contents of the FPU general purpose register ft when FR = 1. */
			WriteVirtual<s64>(address, fpr.Get<s64>(ft));
		}
		else
		{
			static_assert(instr != instr, "\"FPU_Store\" template function called, but no matching store instruction was found.");
		}

		AdvancePipeline<1>();
	}


	template<COP1Instruction instr>
	void FPUMove(const u32 instr_code)
	{
		using enum COP1Instruction;

		const auto fs = instr_code >> 11 & 0x1F;
		const auto rt = instr_code >> 16 & 0x1F;

		if constexpr (log_cpu_instructions)
		{
			current_instr_log_output = std::format("{} {}, {}", current_instr_name, rt, fs);
		}

		if constexpr (instr == MTC1)
		{
			/* Move Word To FPU;
			   Transfers the contents of CPU general purpose register rt to FPU general purpose register fs. */
			fpr.Set<s32>(fs, s32(gpr[rt]));
		}
		else if constexpr (instr == MFC1)
		{
			/* Move Word From FPU;
			   Transfers the contents of FPU general purpose register fs to CPU general purpose register rt. */
			gpr.Set(rt, u64(fpr.Get<s32>(fs)));
		}
		else if constexpr (instr == CTC1)
		{
			/* Move Control Word To FPU;
			   Transfers the contents of CPU general purpose register rt to FPU control register fs. */
			if (fs == 31) // Only #31 is writeable
			{
				fpu_control.Set(fs, u32(gpr[rt]));
				TestAllExceptions();
			}
		}
		else if constexpr (instr == CFC1)
		{
			/* Move Control Word From FPU;
			   Transfers the contents of FPU control register fs to CPU general purpose register rt. */
			gpr.Set(rt, s32(fpu_control.Get(fs)));
		}
		else if constexpr (instr == DMTC1)
		{
			/* Doubleword Move To FPU;
			   Transfers the contents of CPU general purpose register rt to FPU general purpose register fs. */
			fpr.Set<s64>(fs, s64(gpr[rt]));
		}
		else if constexpr (instr == DMFC1)
		{
			/* Doubleword Move From FPU;
			   Transfers the contents of FPU general purpose register fs to CPU general purpose register rt. */
			gpr.Set(rt, fpr.Get<s64>(fs));
		}
		else
		{
			static_assert(instr != instr, "\"FPU_Move\" template function called, but no matching move instruction was found.");
		}

		AdvancePipeline<1>();
	}


	template<COP1Instruction instr>
	void FPUConvert(const u32 instr_code)
	{
		using enum COP1Instruction;

		/* Test for unimplemented operation exception sources for CVT/round instructions. These cannot be found out from std::fetestexcept.
		   This function should be called after the conversion has been made.
		   For all these instructions, an unimplemented exception will occur if either:
			 * If the source operand is infinity or NaN, or
			 * If overflow occurs during conversion to integer format. */
		auto TestForUnimplementedException = [&] <typename From, typename To> (const From source) -> bool
		{
			if constexpr (std::is_integral_v<From> && std::is_same_v<To, f64>)
			{
				return false; /* zero-cost shortcut; integers cannot be infinity or NaN, and the operation is then always exact when converting to a double */
			}
			if constexpr (std::is_floating_point_v<From>)
			{
				if (std::isnan(source) || std::isinf(source))
					return true;
			}
			if constexpr (std::is_integral_v<To>)
			{
				return std::fetestexcept(FE_OVERFLOW) != 0; // TODO: should this also include underflow?
			}
			return false;
		};

		const auto fd = instr_code >> 6 & 0x1F;
		const auto fs = instr_code >> 11 & 0x1F;
		const auto fmt = instr_code >> 21 & 0x1F;

		if constexpr (log_cpu_instructions)
		{
			current_instr_log_output = std::format("{}.{} {}, {}", current_instr_name, FmtToString(fmt), fd, fs);
		}

		if constexpr (instr == CVT_S || instr == CVT_D || instr == CVT_W || instr == CVT_L)
		{
			/* CVT.S/CVT.D: Convert To Single/Double Floating-point Format;
			   Converts the contents of floating-point register fs from the specified format (fmt)
			   to a single/double-precision floating-point format. Stores the rounded result to floating-point register fd.

			   CVT.W/CVT.L: Convert To Single/Long Fixed-point Format;
			   Converts the contents of floating-point register fs from the specified format (fmt)
			   to a 32/64-bit fixed-point format. Stores the rounded result to floating-point register fd. */
			auto Convert = [&] <FPUNumericType InputType>
			{
				/* Interpret a source operand as type 'From', "convert" (round according to the current rounding mode) it to a new type 'To', and store the result. */
				auto Convert2 = [&] <FPUNumericType From, FPUNumericType To> // TODO think of a new lambda name ;)
				{
					const From source = fpr.Get<From>(fs);
					std::feclearexcept(FE_ALL_EXCEPT);
					const To conv = To(source); 
					fpr.Set<To>(fd, conv);

					unimplemented_operation = TestForUnimplementedException.template operator () < From, To > (source);

					/* Note: if the input and output formats are the same, the result is undefined,
					   according to table B-19 in "MIPS IV Instruction Set (Revision 3.2)" by Charles Price, 1995. */
					/* TODO: the below is assuming that conv. between W and L takes 2 cycles.
					   See footnote 2 in table 7-14, VR4300 manual */
					AdvancePipeline<[&] {
						     if constexpr (std::is_same_v<From, To>)                             return 1;
						else if constexpr (std::is_same_v<From, f32> && std::is_same_v<To, f64>) return 1;
						else if constexpr (std::is_same_v<From, f64> && std::is_same_v<To, f32>) return 2;
						else if constexpr (std::is_integral_v<From> && std::is_integral_v<To>)   return 2;
						else                                                                     return 5;
					}()>();
				};

				if constexpr (instr == CVT_S)
					Convert2.template operator() < InputType, f32 > ();
				else if constexpr (instr == CVT_D)
					Convert2.template operator() < InputType, f64 > ();
				else if constexpr (instr == CVT_W)
					Convert2.template operator() < InputType, s32 > ();
				else if constexpr (instr == CVT_L)
					Convert2.template operator() < InputType, s64 > ();
				else
					static_assert(instr != instr, "\"FPU_Convert\" template function called, but no matching convert instruction was found.");
			};

			switch (fmt)
			{
			case FmtTypeID::Float32:
				Convert.template operator() <f32 /* input format */> ();
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
				AdvancePipeline<2>();
				break;
			}

			TestAllExceptions();
		}
		else if constexpr (instr == ROUND_W || instr == TRUNC_W || instr == CEIL_W || instr == FLOOR_W ||
			               instr == ROUND_L || instr == TRUNC_L || instr == CEIL_L || instr == FLOOR_L)
		{
			/* ROUND.L/ROUND.W/TRUNC.L/TRUNC.W/CEIL.L/CEIL.W/FLOOR.L/FLOOR.W: Round/Truncate/Ceiling/Floor To Single/Long Fixed-point Format;
			   Rounds the contents of floating-point register fs to a value closest to the 32/64-bit
			   fixed-point format and converts them from the specified format (fmt). Stores the result to floating-point register fd. */

			/* Interpret the source operand (as a float), and round it to an integer (s32 or s64). */
			auto Round = [&] <std::floating_point InputFloat, std::signed_integral OutputInt>
			{
				const InputFloat source = fpr.Get<InputFloat>(fs);

				std::feclearexcept(FE_ALL_EXCEPT);

				const OutputInt result = [&] {
					     if constexpr (instr == ROUND_W || instr == ROUND_L) return OutputInt(std::nearbyint(source));
					else if constexpr (instr == TRUNC_W || instr == TRUNC_L) return OutputInt(std::trunc(source));
					else if constexpr (instr == CEIL_W  || instr == CEIL_L)  return OutputInt(std::ceil(source));
					else if constexpr (instr == FLOOR_W || instr == FLOOR_L) return OutputInt(std::floor(source));
					else static_assert(instr != instr, "\"FPU_Convert\" template function called, but no matching convert instruction was found.");
				}();

				unimplemented_operation = TestForUnimplementedException.template operator () < InputFloat, OutputInt > (source);

				/* If the invalid operation exception occurs, but the exception is not enabled, return INT_MAX */
				fpr.Set<OutputInt>(fd, [&] {
					if (std::fetestexcept(FE_INVALID) && !fcr31.enable_I)
						return std::numeric_limits<OutputInt>::max();
					else return result;
				}());
			};

			constexpr static bool rounding_is_made_to_s32 =
				instr == ROUND_W || instr == TRUNC_W || instr == CEIL_W || instr == FLOOR_W;

			switch (fmt)
			{
			case FmtTypeID::Float32:
				if constexpr (rounding_is_made_to_s32)
					Round.template operator() < f32 /* input format */, s32 /* output format */ > ();
				else
					Round.template operator() < f32, s64 > ();
				AdvancePipeline<5>();
				break;

			case FmtTypeID::Float64:
				if constexpr (rounding_is_made_to_s32)
					Round.template operator() < f64, s32 > ();
				else
					Round.template operator() < f64, s64 > ();
				AdvancePipeline<5>();
				break;

			case FmtTypeID::Int32:
			case FmtTypeID::Int64:
				/* If the input format is integer, the result is undefined,
				   according to table B-19 in "MIPS IV Instruction Set (Revision 3.2)" by Charles Price, 1995.
				   For now, just don't do anything.
				   TODO possibly change */
				AdvancePipeline<1>();
				break;

			default:
				unimplemented_operation = true;
				std::feclearexcept(FE_ALL_EXCEPT);
				AdvancePipeline<2>();
				break;
			}

			TestAllExceptions();
		}
		else
		{
			static_assert(instr != instr, "\"FPU_Convert\" template function called, but no matching convert instruction was found.");
		}
	}


	template<COP1Instruction instr>
	void FPUCompute(const u32 instr_code)
	{
		using enum COP1Instruction;

		const auto fd = instr_code >> 6 & 0x1F;
		const auto fs = instr_code >> 11 & 0x1F;
		const auto fmt = instr_code >> 21 & 0x1F;

		if constexpr (instr == ADD || instr == SUB || instr == MUL || instr == DIV)
		{
			/* Floating-point Add/Subtract/Multiply/Divide;
			   Arithmetically adds/subtracts/multiplies/divides the contents of floating-point registers
			   fs and ft in the specified format (fmt). Stores the rounded result to floating-point register fd. */

			const auto ft = instr_code >> 16 & 0x1F;

			if constexpr (log_cpu_instructions)
			{
				current_instr_log_output = std::format("{}.{} {}, {}, {}", current_instr_name, FmtToString(fmt), fd, fs, ft);
			}

			auto Compute = [&] <std::floating_point Float>
			{
				const Float op1 = fpr.Get<Float>(fs);
				const Float op2 = fpr.Get<Float>(ft);

				std::feclearexcept(FE_ALL_EXCEPT);

				const Float result = [&] {
					     if constexpr (instr == ADD) return op1 + op2;
					else if constexpr (instr == SUB) return op1 - op2;
					else if constexpr (instr == MUL) return op1 * op2;
					else if constexpr (instr == DIV) return op1 / op2;
					else                             static_assert(instr != instr);
				}();

				AdvancePipeline<[&] {
					     if constexpr (instr == ADD || instr == SUB) return 3;
					else if constexpr (std::is_same_v<Float, f32>)   return 29;
					else if constexpr (std::is_same_v<Float, f64>)   return 58;
					else                                             static_assert(instr != instr);
				}()>();

				fpr.Set<Float>(fd, result);
			};

			switch (fmt)
			{
			case FmtTypeID::Float32:
				Compute.template operator() <f32> ();
				unimplemented_operation = false;
				break;

			case FmtTypeID::Float64:
				Compute.template operator() <f64> ();
				unimplemented_operation = false;
				break;

			default:
				AdvancePipeline<2>();
				unimplemented_operation = true;
				std::feclearexcept(FE_ALL_EXCEPT);
				break;
			}

			TestAllExceptions();
		}
		else if constexpr (instr == ABS || instr == MOV || instr == NEG || instr == SQRT)
		{
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

			if constexpr (log_cpu_instructions)
			{
				current_instr_log_output = std::format("{}.{} {}, {}", current_instr_name, FmtToString(fmt), fd, fs);
			}

			auto Compute = [&] <std::floating_point Float>
			{
				const Float op = fpr.Get<Float>(fs);

				std::feclearexcept(FE_ALL_EXCEPT);

				const Float result = [&] {
					     if constexpr (instr == ABS)  return std::abs(op);
					else if constexpr (instr == MOV)  return op; 
					else if constexpr (instr == NEG)  return -op;
					else if constexpr (instr == SQRT) return std::sqrt(op);
					else                              static_assert(instr != instr);
				}();

				AdvancePipeline<[&] {
					if constexpr (instr == SQRT)
					{
						     if constexpr (std::is_same_v<Float, f32>) return 29;
						else if constexpr (std::is_same_v<Float, f64>) return 58;
						else                                           static_assert(instr != instr);
					}
					else return 1;
				}()>();

				fpr.Set<Float>(fd, result); // TODO: could this result in host exception flags changing?
			};

			switch (fmt)
			{
			case FmtTypeID::Float32:
				Compute.template operator() < f32 > ();
				unimplemented_operation = false;
				break;

			case FmtTypeID::Float64:
				Compute.template operator() < f64 > ();
				unimplemented_operation = false;
				break;

			default:
				AdvancePipeline<2>();
				unimplemented_operation = true;
				std::feclearexcept(FE_ALL_EXCEPT);
				break;
			}

			/* MOV can cause only the unimplemented operation exception. */
			if constexpr (instr == MOV)
				TestUnimplementedOperationException();
			else
				TestAllExceptions();
		}
		else
		{
			static_assert(instr != instr, "\"FPU_Compute\" template function called, but no matching compute instruction was found.");
		}
	}


	template<COP1Instruction instr>
	void FPUBranch(const u32 instr_code)
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

		if constexpr (log_cpu_instructions)
		{
			current_instr_log_output = std::format("{} ${:X}", current_instr_name, s16(instr_code & 0xFFFF));
		}

		const s64 offset = s64(s16(instr_code & 0xFFFF)) << 2;

		const bool branch_cond = [&] {
			     if constexpr (instr == BC1T || instr == BC1TL) return fcr31.c;
			else if constexpr (instr == BC1F || instr == BC1FL) return !fcr31.c;
			else static_assert(instr != instr, "\"FPU_Branch\" template function called, but no matching branch instruction was found.");
		}();

		if (branch_cond)
		{
			PrepareJump(pc + offset);
		}
		else if constexpr (instr == BC1TL || instr == BC1FL)
		{
			pc += 4; /* The instruction in the branch delay slot is discarded. TODO: manual says "invalidated" */
		}

		AdvancePipeline<1>(); /* TODO: not 2? */
	}


	void FPUCompare(const u32 instr_code)
	{
		/* Floating-point Compare;
		   Interprets and arithmetically compares the contents of FPU registers fs and ft
		   in the specified format (fmt). The result is identified by comparison and the
		   specified condition (cond). After a delay of one instruction, the comparison
		   result can be used by the FPU branch instruction of the CPU. */

		const auto cond = instr_code & 0xF;
		const auto fs = instr_code >> 11 & 0x1F;
		const auto ft = instr_code >> 16 & 0x1F;
		const auto fmt = instr_code >> 21 & 0x1F;

		if constexpr (log_cpu_instructions)
		{
			current_instr_log_output = std::format("C.{}.{} {}, {}", compare_cond_strings[cond], FmtToString(fmt), fs, ft);
		}

		auto Compare = [&] <std::floating_point Float>
		{
			const Float op1 = fpr.Get<Float>(fs);
			const Float op2 = fpr.Get<Float>(ft);

			const bool comp_result = [&] { /* See VR4300 User's Manual by NEC, p. 566 */
				if (std::isnan(op1) || std::isnan(op2))
				{
					SetInvalidException(cond & 8);
					return bool(cond & 1);
				}
				else
				{
					SetInvalidException(false);
					return (cond & 4) && op1 < op2 || (cond & 2) && op1 == op2;
				}
			}();

			fcr31.c = comp_result; /* TODO: also set 'COC1' to result? Or is COC1 the same as fcr31.C? */
		};

		/* TODO not clear if this instruction should clear all exception flags other than invalid and unimplemented */

		switch (fmt)
		{
		case FmtTypeID::Float32:
			Compare.template operator() < f32 > ();
			unimplemented_operation = false;
			AdvancePipeline<1>();
			break;

		case FmtTypeID::Float64:
			Compare.template operator() < f64 > ();
			unimplemented_operation = false;
			AdvancePipeline<1>();
			break;

		default:
			unimplemented_operation = true;
			AdvancePipeline<2>();
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
}