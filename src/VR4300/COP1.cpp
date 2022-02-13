module VR4300:COP1;

import :CPU;
import :Exceptions;
import :MMU;
import :Operation;
import :Registers;

namespace VR4300
{
	/* Table 17.4 in VR4300 user manual by NEC; the 'fmt' instruction operand specifies in which format registers should be interpreted in.
	   The below maps formats to identifiers. */
	enum NumericFormatID
	{
		float32 = 16, /* 32-bit binary floating-point */
		float64 = 17, /* 64-bit binary floating-point */
		int32   = 20, /* 32-bit binary fixed-point */
		int64   = 21  /* 64-bit binary fixed-point */
	};


	struct ExceptionFlags
	{
		bool inexact_operation = false;
		bool underflow = false;
		bool overflow = false;
		bool division_by_zero = false;
		bool invalid_operation = false;
		bool unimplemented_operation = false;

		void clear_all()
		{
			*this = {};
			std::feclearexcept(FE_ALL_EXCEPT);
		}

		void test_and_signal_all()
		{
			fcr31.cause_I = std::fetestexcept(FE_INEXACT);
			fcr31.cause_U = std::fetestexcept(FE_UNDERFLOW);
			fcr31.cause_O = std::fetestexcept(FE_OVERFLOW);
			fcr31.cause_Z = std::fetestexcept(FE_DIVBYZERO);
			fcr31.cause_V = std::fetestexcept(FE_INVALID);
			fcr31.cause_E = unimplemented_operation;

			if (fcr31.enable_I)
				InexactOperationException();
			else
				fcr31.flag_I = true;

			if (fcr31.enable_U)
				UnderflowException();
			else
				fcr31.flag_U = true;

			if (fcr31.enable_O)
				OverflowException();
			else
				fcr31.flag_O = true;

			if (fcr31.enable_Z)
				DivisionByZeroException();
			else
				fcr31.flag_Z = true;

			if (fcr31.enable_V)
				InvalidOperationException();
			else
				fcr31.flag_V = true;
		}

		void test_and_signal_unimplemented_exception()
		{
			fcr31.cause_E = unimplemented_operation;
		}

		void test_and_signal_invalid_exception()
		{
			fcr31.cause_V = std::fetestexcept(FE_INVALID);
		}
	} static exception_flags{};


	template<FPU_Instruction instr>
	void FPU_Load(const u32 instr_code)
	{
		using enum FPU_Instruction;

		const s16 offset = instr_code & 0xFFFF;
		const u8 ft = instr_code >> 16 & 0x1F;
		const u8 base = instr_code >> 21 & 0x1F;
		const u64 address = gpr[base] + offset;

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
				static_assert(false, "\"FPU_Load\" template function called, but no matching load instruction was found.");
			}
		}();

		AdvancePipeline(1);

		if (exception_has_occurred)
			return;

		fgr.Set(ft, result);
	}


	template<FPU_Instruction instr>
	void FPU_Store(const u32 instr_code)
	{
		using enum FPU_Instruction;

		const s16 offset = instr_code & 0xFFFF;
		const u8 ft = instr_code >> 16 & 0x1F;
		const u8 base = instr_code >> 21 & 0x1F;
		const u64 address = gpr[base] + offset;

		if constexpr (instr == SWC1)
		{
			/* Store Word From FPU;
			   Sign-extends the 16-bit offset and adds it to the CPU register base to generate
			   an address. Stores the contents of the FPU general purpose register ft to the
			   memory position specified by the address. */
			WriteVirtual<s32>(address, fgr.Get<s32>(ft));
		}
		else if constexpr (instr == SDC1)
		{
			/* Store Doubleword From FPU;
			   Sign-extends the 16-bit offset and adds it to the CPU register base to generate
			   an address. Stores the contents of the FPU general purpose registers ft and
			   ft+1 to the memory position specified by the address when FR = 0, and the
			   contents of the FPU general purpose register ft when FR = 1. */
			WriteVirtual<s64>(address, fgr.Get<s64>(ft));
		}
		else
		{
			static_assert(false, "\"FPU_Store\" template function called, but no matching store instruction was found.");
		}

		AdvancePipeline(1);
	}


	template<FPU_Instruction instr>
	void FPU_Move(const u32 instr_code)
	{
		using enum FPU_Instruction;

		const u8 fs = instr_code >> 11 & 0x1F;
		const u8 rt = instr_code >> 16 & 0x1F;

		if constexpr (instr == MTC1)
		{
			/* Move Word To FPU;
			   Transfers the contents of CPU general purpose register rt to FPU general purpose register fs. */
			fgr.Set<s32>(fs, s32(gpr[rt]));
		}
		else if constexpr (instr == MFC1)
		{
			/* Move Word From FPU;
			   Transfers the contents of FPU general purpose register fs to CPU general purpose register rt. */
			gpr.Set(rt, u64(fgr.Get<s32>(fs)));
		}
		else if constexpr (instr == CTC1)
		{
			/* Move Control Word To FPU;
			   Transfers the contents of CPU general purpose register rt to FPU control register fs. */
			fpu_control.Set(fs, u32(gpr[rt]));
			exception_flags.test_and_signal_all();
		}
		else if constexpr (instr == CFC1)
		{
			/* Move Control Word From FPU;
			   Transfers the contents of FPU control register fs to CPU general purpose register rt. */
			gpr.Set(rt, fpu_control.Get(fs));
		}
		else if constexpr (instr == DMTC1)
		{
			/* Doubleword Move To FPU;
			   Transfers the contents of CPU general purpose register rt to FPU general purpose register fs. */
			fgr.Set<s64>(fs, s64(gpr[rt]));
		}
		else if constexpr (instr == DMFC1)
		{
			/* Doubleword Move From FPU;
			   Transfers the contents of FPU general purpose register fs to CPU general purpose register rt. */
			gpr.Set(rt, fgr.Get<s64>(fs));
		}
		else
		{
			static_assert(false, "\"FPU_Move\" template function called, but no matching move instruction was found.");
		}

		AdvancePipeline(1);
	}


	template<FPU_Instruction instr>
	void FPU_Convert(const u32 instr_code)
	{
		using enum FPU_Instruction;

		/* Test for unimplemented operation exception sources for CVT/round instructions. These cannot be found out from std::fetestexcept.
		   This function should be called after the conversion has been made.
		   For all these instructions, an unimplemented exception will occur if either:
			 * If the source operand is infinity or NaN, or
			 * If overflow occurs during conversion to integer format. */
		auto test_unimplemented_exception_from_conversion = [&] <typename From, typename To> (const From source) -> bool
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
				return std::fetestexcept(FE_OVERFLOW) != 0; // todo should this also include underflow?
			}
			return false;
		};

		const u8 fd = instr_code >> 6 & 0x1F;
		const u8 fs = instr_code >> 11 & 0x1F;
		const u8 fmt = instr_code >> 21 & 0x1F;

		if constexpr (instr == CVT_S || instr == CVT_D || instr == CVT_W || instr == CVT_L)
		{
			/* CVT.S/CVT.D: Convert To Single/Double Floating-point Format;
			   Converts the contents of floating-point register fs from the specified format (fmt)
			   to a single/double-precision floating-point format. Stores the rounded result to floating-point register fd.

			   CVT.W/CVT.L: Convert To Single/Long Fixed-point Format;
			   Converts the contents of floating-point register fs from the specified format (fmt)
			   to a 32/64-bit fixed-point format. Stores the rounded result to floating-point register fd. */
			auto Convert = [&] <FPU_NumericType Input_Type>
			{
				/* Interpret a source operand as type 'From', "convert" (round according to the current rounding mode) it to a new type 'To', and store the result. */
				auto Convert2 = [&] <FPU_NumericType From, FPU_NumericType To> // TODO think of a new lambda name ;)
				{
					const From source = fgr.Get<From>(fs);
					exception_flags.clear_all();
					const To conv = To(source); // TODO is the rounding mode taken into account here?
					fgr.Set<To>(fd, conv);

					exception_flags.unimplemented_operation =
						test_unimplemented_exception_from_conversion.template operator () < From, To > (source);

					/* Note: if the input and output formats are the same, the result is undefined,
					   according to table B-19 in "MIPS IV Instruction Set (Revision 3.2)" by Charles Price, 1995. */
					/* TODO: the below is assuming that conv. between W and L takes 2 cycles.
					   See footnote 2 in table 7-14, VR4300 manual */
					AdvancePipeline([&] {
						     if constexpr (std::is_same_v<From, To>)                             return 1;
						else if constexpr (std::is_same_v<From, f32> && std::is_same_v<To, f64>) return 1;
						else if constexpr (std::is_same_v<From, f64> && std::is_same_v<To, f32>) return 2;
						else if constexpr (std::is_integral_v<From> && std::is_integral_v<To>)   return 2;
						else                                                                     return 5;
					}());
				};

				if constexpr (instr == CVT_S)
					Convert2.template operator() < Input_Type, f32 > ();
				else if constexpr (instr == CVT_D)
					Convert2.template operator() < Input_Type, f64 > ();
				else if constexpr (instr == CVT_W)
					Convert2.template operator() < Input_Type, s32 > ();
				else if constexpr (instr == CVT_L)
					Convert2.template operator() < Input_Type, s64 > ();
				else
					static_assert(false, "\"FPU_Convert\" template function called, but no matching convert instruction was found.");
			};

			switch (fmt)
			{
			case NumericFormatID::float32:
				Convert.template operator() <f32 /* input format */> ();
				break;

			case NumericFormatID::float64:
				Convert.template operator() <f64> ();
				break;

			case NumericFormatID::int32:
				Convert.template operator() <s32> ();
				break;

			case NumericFormatID::int64:
				Convert.template operator() <s64> ();
				break;

			default:
				exception_flags.unimplemented_operation = true; /* TODO other exception flags are cleared in this case, should they be? */
				AdvancePipeline(2);
				break;
			}

			exception_flags.test_and_signal_all();
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
				const InputFloat source = fgr.Get<InputFloat>(fs);

				const OutputInt result = [&] {
					     if constexpr (instr == ROUND_W || instr == ROUND_L) return OutputInt(std::round(source));
					else if constexpr (instr == TRUNC_W || instr == TRUNC_L) return OutputInt(std::trunc(source));
					else if constexpr (instr == CEIL_W  || instr == CEIL_L)  return OutputInt(std::ceil(source));
					else if constexpr (instr == FLOOR_W || instr == FLOOR_L) return OutputInt(std::floor(source));
					else static_assert(false, "\"FPU_Convert\" template function called, but no matching convert instruction was found.");
				}();

				exception_flags.unimplemented_operation =
					test_unimplemented_exception_from_conversion.template operator () < InputFloat, OutputInt > (source);

				/* If the invalid operation exception occurs, but the exception is not enabled, return INT_MAX */
				fgr.Set<OutputInt>(fd, [&] {
					if (std::fetestexcept(FE_INVALID) && !fcr31.enable_I)
						return std::numeric_limits<OutputInt>::max();
					else return result;
				}());
			};

			exception_flags.clear_all();

			constexpr bool rounding_is_made_to_s32 =
				instr == ROUND_W || instr == TRUNC_W || instr == CEIL_W || instr == FLOOR_W;

			switch (fmt)
			{
			case NumericFormatID::float32:
				if constexpr (rounding_is_made_to_s32)
					Round.template operator() < f32 /* input format */, s32 /* output format */ > ();
				else
					Round.template operator() < f32, s64 > ();
				AdvancePipeline(5);
				break;

			case NumericFormatID::float64:
				if constexpr (rounding_is_made_to_s32)
					Round.template operator() < f64, s32 > ();
				else
					Round.template operator() < f64, s64 > ();
				AdvancePipeline(5);
				break;

			case NumericFormatID::int32:
			case NumericFormatID::int64:
				/* If the input format is integer, the result is undefined,
				   according to table B-19 in "MIPS IV Instruction Set (Revision 3.2)" by Charles Price, 1995.
				   For now, just don't do anything.
				   TODO possibly change */
				AdvancePipeline(1);
				break;

			default:
				exception_flags.unimplemented_operation = true;
				AdvancePipeline(2);
				break;
			}

			exception_flags.test_and_signal_all();
		}
		else
		{
			static_assert(false, "\"FPU_Convert\" template function called, but no matching convert instruction was found.");
		}
	}


	template<FPU_Instruction instr>
	void FPU_Compute(const u32 instr_code)
	{
		using enum FPU_Instruction;

		const u8 fd = instr_code >> 6 & 0x1F;
		const u8 fs = instr_code >> 11 & 0x1F;
		const u8 fmt = instr_code >> 21 & 0x1F;

		if constexpr (instr == ADD || instr == SUB || instr == MUL || instr == DIV)
		{
			/* Floating-point Add/Subtract/Multiply/Divide;
			   Arithmetically adds/subtracts/multiplies/divides the contents of floating-point registers
			   fs and ft in the specified format (fmt). Stores the rounded result to floating-point register fd. */

			const u8 ft = instr_code >> 16 & 0x1F;

			auto Compute = [&] <std::floating_point Float>
			{
				const Float op1 = fgr.Get<Float>(fs);
				const Float op2 = fgr.Get<Float>(ft);

				const Float result = [&] {
					     if constexpr (instr == ADD) return op1 + op2;
					else if constexpr (instr == SUB) return op1 - op2;
					else if constexpr (instr == MUL) return op1 * op2;
					else if constexpr (instr == DIV) return op1 / op2;
					else                             static_assert(false);
				}();

				AdvancePipeline([&] {
					     if constexpr (instr == ADD || instr == SUB) return 3;
					else if constexpr (std::is_same_v<Float, f32>)   return 29;
					else if constexpr (std::is_same_v<Float, f64>)   return 58;
					else                                             static_assert(false);
				}());

				fgr.Set<Float>(fd, result);
			};

			exception_flags.clear_all();

			switch (fmt)
			{
			case NumericFormatID::float32:
				Compute.template operator() <f32> ();
				exception_flags.unimplemented_operation = false;
				break;

			case NumericFormatID::float64:
				Compute.template operator() <f64> ();
				exception_flags.unimplemented_operation = false;
				break;

			default:
				AdvancePipeline(2);
				exception_flags.unimplemented_operation = true;
				break;
			}

			exception_flags.test_and_signal_all();
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

			auto Compute = [&] <std::floating_point Float>
			{
				const Float op = fgr.Get<Float>(fs);

				const Float result = [&] {
					     if constexpr (instr == ABS)  return std::abs(op);
					else if constexpr (instr == MOV)  return op; 
					else if constexpr (instr == NEG)  return -op;
					else if constexpr (instr == SQRT) return std::sqrt(op);
					else                              static_assert(false);
				}();

				AdvancePipeline([&] {
					if constexpr (instr == SQRT)
					{
						     if constexpr (std::is_same_v<Float, f32>) return 29;
						else if constexpr (std::is_same_v<Float, f64>) return 58;
						else                                           static_assert(false);
					}
					else return 1;
				}());

				fgr.Set<Float>(fd, result);
			};

			/* The MOV instruction does not update the exception flags (except for unimplemented exception (see below)) */
			if constexpr (instr != MOV)
				exception_flags.clear_all();

			switch (fmt)
			{
			case NumericFormatID::float32:
				Compute.template operator() < f32 > ();
				exception_flags.unimplemented_operation = false;
				break;

			case NumericFormatID::float64:
				Compute.template operator() < f64 > ();
				exception_flags.unimplemented_operation = false;
				break;

			default:
				AdvancePipeline(2);
				exception_flags.unimplemented_operation = true;
				break;
			}

			if constexpr (instr != MOV)
				exception_flags.test_and_signal_all();
			else
				exception_flags.test_and_signal_unimplemented_exception();
		}
		else
		{
			static_assert(false, "\"FPU_Compute\" template function called, but no matching compute instruction was found.");
		}
	}


	template<FPU_Instruction instr>
	void FPU_Branch(const u32 instr_code)
	{
		using enum FPU_Instruction;

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

		const s16 offset = instr_code & 0xFFFF;
		const bool cond = fcr31.C;
		const s64 target = s64(offset << 2);

		const bool branch_cond = [&] {
			     if constexpr (instr == BC1T || instr == BC1TL) return cond;
			else if constexpr (instr == BC1F || instr == BC1FL) return !cond;
			else static_assert(false, "\"FPU_Branch\" template function called, but no matching branch instruction was found.");
		}();

		if (branch_cond)
		{
			PrepareJump(pc + offset);
		}
		else if constexpr (instr == BC1TL || instr == BC1FL)
		{
			pc += 4; /* The instruction in the branch delay slot is discarded. TODO: manual says "invalidated" */
		}

		AdvancePipeline(1); /* TODO: not if 2? */
	}


	void FPU_Compare(const u32 instr_code)
	{
		/* Floating-point Compare;
		   Interprets and arithmetically compares the contents of FPU registers fs and ft
		   in the specified format (fmt). The result is identified by comparison and the
		   specified condition (cond). After a delay of one instruction, the comparison
		   result can be used by the FPU branch instruction of the CPU. */

		const u8 cond = instr_code & 0xF;
		const u8 fs = instr_code >> 11 & 0x1F;
		const u8 ft = instr_code >> 16 & 0x1F;
		const u8 fmt = instr_code >> 21 & 0x1F;

		auto Compare = [&] <std::floating_point Float>
		{
			const Float op1 = fgr.Get<Float>(fs);
			const Float op2 = fgr.Get<Float>(ft);

			const bool comp_result = [&] { /* See VR4300 User's Manual by NEC, p. 566 */
				if (std::isnan(op1) || std::isnan(op2))
				{
					exception_flags.invalid_operation = cond & 8;
					return bool(cond & 1);
				}
				else
				{
					exception_flags.invalid_operation = false;
					return (cond & 4) && op1 < op2 || (cond & 2) && op1 == op2;
				}
			}();

			fcr31.C = comp_result; /* TODO: also set 'COC1' to result? Or is COC1 the same as fcr31.C? */
		};

		/* TODO not clear if this instruction should clear all exception flags other than invalid and unimplemented */

		switch (fmt)
		{
		case NumericFormatID::float32:
			Compare.template operator() < f32 > ();
			exception_flags.unimplemented_operation = false;
			AdvancePipeline(1);
			break;

		case NumericFormatID::float64:
			Compare.template operator() < f64 > ();
			exception_flags.unimplemented_operation = false;
			AdvancePipeline(1);
			break;

		default:
			exception_flags.unimplemented_operation = true;
			AdvancePipeline(2);
			break;
		}

		exception_flags.test_and_signal_invalid_exception();
	}


	template void FPU_Load<FPU_Instruction::LWC1>(const u32);
	template void FPU_Load<FPU_Instruction::LDC1>(const u32);

	template void FPU_Store<FPU_Instruction::SWC1>(const u32);
	template void FPU_Store<FPU_Instruction::SDC1>(const u32);

	template void FPU_Move<FPU_Instruction::MTC1>(const u32);
	template void FPU_Move<FPU_Instruction::MFC1>(const u32);
	template void FPU_Move<FPU_Instruction::CTC1>(const u32);
	template void FPU_Move<FPU_Instruction::CFC1>(const u32);
	template void FPU_Move<FPU_Instruction::DMTC1>(const u32);
	template void FPU_Move<FPU_Instruction::DMFC1>(const u32);

	template void FPU_Convert<FPU_Instruction::CVT_S>(const u32);
	template void FPU_Convert<FPU_Instruction::CVT_D>(const u32);
	template void FPU_Convert<FPU_Instruction::CVT_L>(const u32);
	template void FPU_Convert<FPU_Instruction::CVT_W>(const u32);
	template void FPU_Convert<FPU_Instruction::ROUND_L>(const u32);
	template void FPU_Convert<FPU_Instruction::ROUND_W>(const u32);
	template void FPU_Convert<FPU_Instruction::TRUNC_L>(const u32);
	template void FPU_Convert<FPU_Instruction::TRUNC_W>(const u32);
	template void FPU_Convert<FPU_Instruction::CEIL_L>(const u32);
	template void FPU_Convert<FPU_Instruction::CEIL_W>(const u32);
	template void FPU_Convert<FPU_Instruction::FLOOR_L>(const u32);
	template void FPU_Convert<FPU_Instruction::FLOOR_W>(const u32);

	template void FPU_Compute<FPU_Instruction::ADD>(const u32);
	template void FPU_Compute<FPU_Instruction::SUB>(const u32);
	template void FPU_Compute<FPU_Instruction::MUL>(const u32);
	template void FPU_Compute<FPU_Instruction::DIV>(const u32);
	template void FPU_Compute<FPU_Instruction::ABS>(const u32);
	template void FPU_Compute<FPU_Instruction::MOV>(const u32);
	template void FPU_Compute<FPU_Instruction::NEG>(const u32);
	template void FPU_Compute<FPU_Instruction::SQRT>(const u32);

	template void FPU_Branch<FPU_Instruction::BC1T>(const u32);
	template void FPU_Branch<FPU_Instruction::BC1F>(const u32);
	template void FPU_Branch<FPU_Instruction::BC1TL>(const u32);
	template void FPU_Branch<FPU_Instruction::BC1FL>(const u32);
}