module VR4300:COP1;

import :CPU;
import :Exceptions;
import :MMU;

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
			FCR31.cause_I = std::fetestexcept(FE_INEXACT);
			FCR31.cause_U = std::fetestexcept(FE_UNDERFLOW);
			FCR31.cause_O = std::fetestexcept(FE_OVERFLOW);
			FCR31.cause_Z = std::fetestexcept(FE_DIVBYZERO);
			FCR31.cause_V = std::fetestexcept(FE_INVALID);
			FCR31.cause_E = unimplemented_operation;

			if (FCR31.enable_I)
				InexactOperationException();
			else
				FCR31.flag_I = true;

			if (FCR31.enable_U)
				UnderflowException();
			else
				FCR31.flag_U = true;

			if (FCR31.enable_O)
				OverflowException();
			else
				FCR31.flag_O = true;

			if (FCR31.enable_Z)
				DivisionByZeroException();
			else
				FCR31.flag_Z = true;

			if (FCR31.enable_V)
				InvalidOperationException();
			else
				FCR31.flag_V = true;
		}

		void test_and_signal_unimplemented_exception()
		{
			FCR31.cause_E = unimplemented_operation;
		}

		void test_and_signal_invalid_exception()
		{
			FCR31.cause_V = std::fetestexcept(FE_INVALID);
		}
	} static exception_flags{};


	template<FPU_Instr instr>
	void FPU_Load(const u32 instr_code)
	{
		const s16 offset = instr_code & 0xFFFF;
		const u8 ft = instr_code >> 16 & 0x1F;
		const u8 base = instr_code >> 21 & 0x1F;
		const u64 address = GPR[base] + offset;

		if constexpr (instr == FPU_Instr::LWC1)
		{
			/* Load Word To FPU;
			   Sign-extends the 16-bit offset and adds it to the CPU register base to generate
			   an address. Loads the contents of the word specified by the address to the
			   FPU general purpose register ft. */
			if (address & 3)
				AddressErrorException();
			else
				FGR.Set<s32>(ft, cpu_read_mem<s32>(address));
		}
		else if constexpr (instr == FPU_Instr::LDC1)
		{
			/* Load Doubleword To FPU;
			   Sign-extends the 16-bit offset and adds it to the CPU register base to generate
			   an address. Loads the contents of the doubleword specified by the address to
			   the FPU general purpose registers ft and ft+1 when FR = 0, or to the FPU
			   general purpose register ft when FR = 1. */
			if (address & 7)
				AddressErrorException();
			else
				FGR.Set<s64>(ft, cpu_read_mem<s64>(address));
		}
		else
		{
			static_assert(false, "\"FPU_Load\" template function called, but no matching load instruction was found.");
		}
	}


	template<FPU_Instr instr>
	void FPU_Store(const u32 instr_code)
	{
		const s16 offset = instr_code & 0xFFFF;
		const u8 ft = instr_code >> 16 & 0x1F;
		const u8 base = instr_code >> 21 & 0x1F;
		const u64 address = GPR[base] + offset;

		if constexpr (instr == FPU_Instr::SWC1)
		{
			/* Store Word From FPU;
			   Sign-extends the 16-bit offset and adds it to the CPU register base to generate
			   an address. Stores the contents of the FPU general purpose register ft to the
			   memory position specified by the address. */
			if (address & 3)
				AddressErrorException();
			else
				cpu_write_mem<s32>(address, FGR.Get<s32>(ft));
		}
		else if constexpr (instr == FPU_Instr::SDC1)
		{
			/* Store Doubleword From FPU;
			   Sign-extends the 16-bit offset and adds it to the CPU register base to generate
			   an address. Stores the contents of the FPU general purpose registers ft and
			   ft+1 to the memory position specified by the address when FR = 0, and the
			   contents of the FPU general purpose register ft when FR = 1. */
			if (address & 7)
				AddressErrorException();
			else
				cpu_write_mem<s64>(address, FGR.Get<s64>(ft));
		}
		else
		{
			static_assert(false, "\"FPU_Store\" template function called, but no matching store instruction was found.");
		}
	}


	template<FPU_Instr instr>
	void FPU_Move(const u32 instr_code)
	{
		const u8 fs = instr_code >> 11 & 0x1F;
		const u8 rt = instr_code >> 16 & 0x1F;

		if constexpr (instr == FPU_Instr::MTC1)
		{
			/* Move Word To FPU;
			   Transfers the contents of CPU general purpose register rt to FPU general purpose register fs. */
			FGR.Set<s32>(fs, s32(GPR[rt]));
		}
		else if constexpr (instr == FPU_Instr::MFC1)
		{
			/* Move Word From FPU;
			   Transfers the contents of FPU general purpose register fs to CPU general purpose register rt. */
			GPR.Set(rt, u64(FGR.Get<s32>(fs)));
		}
		else if constexpr (instr == FPU_Instr::CTC1)
		{
			/* Move Control Word To FPU;
			   Transfers the contents of CPU general purpose register rt to FPU control register fs. */
			FPU_control.Set(fs, GPR[rt]);
			exception_flags.test_and_signal_all();
		}
		else if constexpr (instr == FPU_Instr::CFC1)
		{
			/* Move Control Word From FPU;
			   Transfers the contents of FPU control register fs to CPU general purpose register rt. */
			GPR.Set(rt, FPU_control.Get(fs));
		}
		else if constexpr (instr == FPU_Instr::DMTC1)
		{
			/* Doubleword Move To FPU;
			   Transfers the contents of CPU general purpose register rt to FPU general purpose register fs. */
			FGR.Set<s64>(fs, s64(GPR[rt]));
		}
		else if constexpr (instr == FPU_Instr::DMFC1)
		{
			/* Doubleword Move From FPU;
			   Transfers the contents of FPU general purpose register fs to CPU general purpose register rt. */
			GPR.Set(rt, FGR.Get<s64>(fs));
		}
		else
		{
			static_assert(false, "\"FPU_Move\" template function called, but no matching move instruction was found.");
		}
	}


	template<FPU_Instr instr>
	void FPU_Convert(const u32 instr_code)
	{
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

		if constexpr (instr == FPU_Instr::CVT_S || instr == FPU_Instr::CVT_D || instr == FPU_Instr::CVT_W || instr == FPU_Instr::CVT_L)
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
					const From source = FGR.Get<From>(fs);
					const To conv = To(source); // TODO this instruction may have to put exactly between when exceptions are cleared and checked via std::fetestex. Also TODO is the rounding mode taken into account here?
					FGR.Set<To>(fd, conv);

					exception_flags.unimplemented_operation =
						test_unimplemented_exception_from_conversion.template operator () < From, To > (source);

					/* Note: if the input and output formats are the same, the result is undefined,
					   according to table B-19 in "MIPS IV Instruction Set (Revision 3.2)" by Charles Price, 1995. */
				};

				if constexpr (instr == FPU_Instr::CVT_S)
					Convert2.template operator() < Input_Type, f32 > ();
				else if constexpr (instr == FPU_Instr::CVT_D)
					Convert2.template operator() < Input_Type, f64 > ();
				else if constexpr (instr == FPU_Instr::CVT_W)
					Convert2.template operator() < Input_Type, s32 > ();
				else if constexpr (instr == FPU_Instr::CVT_L)
					Convert2.template operator() < Input_Type, s64 > ();
				else
					static_assert(false, "\"FPU_Convert\" template function called, but no matching convert instruction was found.");
			};

			exception_flags.clear_all();

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
			}

			exception_flags.test_and_signal_all();
		}
		else if constexpr (instr == FPU_Instr::ROUND_W || instr == FPU_Instr::TRUNC_W || instr == FPU_Instr::CEIL_W || instr == FPU_Instr::FLOOR_W ||
			               instr == FPU_Instr::ROUND_L || instr == FPU_Instr::TRUNC_L || instr == FPU_Instr::CEIL_L || instr == FPU_Instr::FLOOR_L)
		{
			/* ROUND.L/ROUND.W/TRUNC.L/TRUNC.W/CEIL.L/CEIL.W/FLOOR.L/FLOOR.W: Round/Truncate/Ceiling/Floor To Single/Long Fixed-point Format;
			   Rounds the contents of floating-point register fs to a value closest to the 32/64-bit
			   fixed-point format and converts them from the specified format (fmt). Stores the result to floating-point register fd. */

			/* Interpret the source operand (as a float), and round it to an integer (s32 or s64). */
			auto Round = [&] <std::floating_point Input_Float, std::signed_integral Output_Int>
			{
				const Input_Float source = FGR.Get<Input_Float>(fs);

				const Output_Int result = [&] {
					if constexpr (instr == FPU_Instr::ROUND_W || instr == FPU_Instr::ROUND_L)
						return Output_Int(std::round(source));
					else if constexpr (instr == FPU_Instr::TRUNC_W || instr == FPU_Instr::TRUNC_L)
						return Output_Int(std::trunc(source));
					else if constexpr (instr == FPU_Instr::CEIL_W || instr == FPU_Instr::CEIL_L)
						return Output_Int(std::ceil(source));
					else if constexpr (instr == FPU_Instr::FLOOR_W || instr == FPU_Instr::FLOOR_L)
						return Output_Int(std::floor(source));
					else
						static_assert(false, "\"FPU_Convert\" template function called, but no matching convert instruction was found.");
				}();

				exception_flags.unimplemented_operation =
					test_unimplemented_exception_from_conversion.template operator () < Input_Float, Output_Int > (source);

				/* If the invalid operation exception occurs, but the exception is not enabled, return INT_MAX */
				FGR.Set<Output_Int>(fd, [&] {
					if (std::fetestexcept(FE_INVALID) && !FCR31.enable_I)
						return std::numeric_limits<Output_Int>::max();
					else return result;
					}());
			};

			exception_flags.clear_all();

			constexpr bool rounding_is_made_to_s32 =
				instr == FPU_Instr::ROUND_W || instr == FPU_Instr::TRUNC_W || instr == FPU_Instr::CEIL_W || instr == FPU_Instr::FLOOR_W;

			switch (fmt)
			{
			case NumericFormatID::float32:
				if constexpr (rounding_is_made_to_s32)
					Round.template operator() < f32 /* input format */, s32 /* output format */ > ();
				else
					Round.template operator() < f32, s64 > ();
				break;

			case NumericFormatID::float64:
				if constexpr (rounding_is_made_to_s32)
					Round.template operator() < f64, s32 > ();
				else
					Round.template operator() < f64, s64 > ();
				break;

			case NumericFormatID::int32:
			case NumericFormatID::int64:
				/* If the input format is integer, the result is undefined,
				   according to table B-19 in "MIPS IV Instruction Set (Revision 3.2)" by Charles Price, 1995.
				   For now, just don't do anything.
				   TODO possibly change */
				break;

			default:
				exception_flags.unimplemented_operation = true;
			}

			exception_flags.test_and_signal_all();
		}
		else
		{
			static_assert(false, "\"FPU_Convert\" template function called, but no matching convert instruction was found.");
		}
	}


	template<FPU_Instr instr>
	void FPU_Compute(const u32 instr_code)
	{
		const u8 fd = instr_code >> 6 & 0x1F;
		const u8 fs = instr_code >> 11 & 0x1F;
		const u8 fmt = instr_code >> 21 & 0x1F;

		if constexpr (instr == FPU_Instr::ADD || instr == FPU_Instr::SUB || instr == FPU_Instr::MUL || instr == FPU_Instr::DIV)
		{
			/* Floating-point Add/Subtract/Multiply/Divide;
			   Arithmetically adds/subtracts/multiplies/divides the contents of floating-point registers
			   fs and ft in the specified format (fmt). Stores the rounded result to floating-point register fd. */

			const u8 ft = instr_code >> 16 & 0x1F;

			auto Compute = [&] <std::floating_point Float>
			{
				const Float op1 = FGR.Get<Float>(fs);
				const Float op2 = FGR.Get<Float>(ft);

				const Float result = [&] {
					if constexpr (instr == FPU_Instr::ADD)
						return op1 + op2;
					else if constexpr (instr == FPU_Instr::SUB)
						return op1 - op2;
					else if constexpr (instr == FPU_Instr::MUL)
						return op1 * op2;
					else if constexpr (instr == FPU_Instr::DIV)
						return op1 / op2;
					else
						static_assert(false);
				}();

				FGR.Set<Float>(fd, result);
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
				exception_flags.unimplemented_operation = true;
			}

			exception_flags.test_and_signal_all();
		}
		else if constexpr (instr == FPU_Instr::ABS || instr == FPU_Instr::MOV || instr == FPU_Instr::NEG || instr == FPU_Instr::SQRT)
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
				const Float op = FGR.Get<Float>(fs);

				const Float result = [&] {
					if constexpr (instr == FPU_Instr::ABS)
						return std::abs(op);
					else if constexpr (instr == FPU_Instr::MOV)
						return op; /* This unnecessary copy from 'op' to 'result' should be optimized away. */
					else if constexpr (instr == FPU_Instr::NEG)
						return -op;
					else if constexpr (instr == FPU_Instr::SQRT)
						return std::sqrt(op);
					else
						static_assert(false);
				}();

				FGR.Set<Float>(fd, result);
			};

			/* The MOV instruction does not update the exception flags (except for unimplemented exception (see below)) */
			if constexpr (instr != FPU_Instr::MOV)
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
				exception_flags.unimplemented_operation = true;
			}

			if constexpr (instr != FPU_Instr::MOV)
				exception_flags.test_and_signal_all();
			else
				exception_flags.test_and_signal_unimplemented_exception();
		}
		else
		{
			static_assert(false, "\"FPU_Compute\" template function called, but no matching compute instruction was found.");
		}
	}


	template<FPU_Instr instr>
	void FPU_Branch(const u32 instr_code)
	{
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
		const bool cond = true; // TODO: = COC[1]
		const s64 target = s64(offset) << 2;

		const bool branch_cond = [&] {
			if constexpr (instr == FPU_Instr::BC1T || instr == FPU_Instr::BC1TL)
				return cond;
			else if constexpr (instr == FPU_Instr::BC1F || instr == FPU_Instr::BC1FL)
				return !cond;
			else
				static_assert(false, "\"FPU_Branch\" template function called, but no matching branch instruction was found.");
		}();

		if (branch_cond)
		{
			PC += offset;
		}
		else if constexpr (instr == FPU_Instr::BC1TL || instr == FPU_Instr::BC1FL)
		{
			// TODO invalidate current instruction
		}
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
			const Float op1 = FGR.Get<Float>(fs);
			const Float op2 = FGR.Get<Float>(ft);

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

			FCR31.C = comp_result; /* TODO: also set 'COC1' to result*/
		};

		/* TODO not clear if this instruction should clear all exception flags other than invalid and unimplemented */

		switch (fmt)
		{
		case NumericFormatID::float32:
			Compare.template operator() < f32 > ();
			exception_flags.unimplemented_operation = false;
			break;

		case NumericFormatID::float64:
			Compare.template operator() < f64 > ();
			exception_flags.unimplemented_operation = false;
			break;

		default:
			exception_flags.unimplemented_operation = true;
		}

		exception_flags.test_and_signal_invalid_exception();
	}


	template void FPU_Load<FPU_Instr::LWC1>(const u32 instr_code);
	template void FPU_Load<FPU_Instr::LDC1>(const u32 instr_code);

	template void FPU_Store<FPU_Instr::SWC1>(const u32 instr_code);
	template void FPU_Store<FPU_Instr::SDC1>(const u32 instr_code);

	template void FPU_Move<FPU_Instr::MTC1>(const u32 instr_code);
	template void FPU_Move<FPU_Instr::MFC1>(const u32 instr_code);
	template void FPU_Move<FPU_Instr::CTC1>(const u32 instr_code);
	template void FPU_Move<FPU_Instr::CFC1>(const u32 instr_code);
	template void FPU_Move<FPU_Instr::DMTC1>(const u32 instr_code);
	template void FPU_Move<FPU_Instr::DMFC1>(const u32 instr_code);

	template void FPU_Convert<FPU_Instr::CVT_S>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::CVT_D>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::CVT_L>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::CVT_W>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::ROUND_L>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::ROUND_W>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::TRUNC_L>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::TRUNC_W>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::CEIL_L>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::CEIL_W>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::FLOOR_L>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::FLOOR_W>(const u32 instr_code);

	template void FPU_Compute<FPU_Instr::ADD>(const u32 instr_code);
	template void FPU_Compute<FPU_Instr::SUB>(const u32 instr_code);
	template void FPU_Compute<FPU_Instr::MUL>(const u32 instr_code);
	template void FPU_Compute<FPU_Instr::DIV>(const u32 instr_code);
	template void FPU_Compute<FPU_Instr::ABS>(const u32 instr_code);
	template void FPU_Compute<FPU_Instr::MOV>(const u32 instr_code);
	template void FPU_Compute<FPU_Instr::NEG>(const u32 instr_code);
	template void FPU_Compute<FPU_Instr::SQRT>(const u32 instr_code);

	template void FPU_Branch<FPU_Instr::BC1T>(const u32 instr_code);
	template void FPU_Branch<FPU_Instr::BC1F>(const u32 instr_code);
	template void FPU_Branch<FPU_Instr::BC1TL>(const u32 instr_code);
	template void FPU_Branch<FPU_Instr::BC1FL>(const u32 instr_code);
}