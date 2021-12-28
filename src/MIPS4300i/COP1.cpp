module MIPS4300i;

namespace MIPS4300i
{
	/* Table 17.4 in VR4300 user manual by NEC; the 'fmt' instruction operand specifies in which format registers should be interpreted in.
	   The below maps formats to identifiers. */
	enum NumericFormat
	{
		S = 16, /* 32-bit binary floating-point */
		D = 17, /* 64-bit binary floating-point */
		W = 20, /* 32-bit binary fixed-point */
		L = 21  /* 64-bit binary fixed-point */
	};

	template<FPU_Instr instr>
	void FPU_Load(const u32 instr_code)
	{
		const s16 offset = instr_code & 0xFFFF;
		const u8 ft = instr_code >> 16 & 0x1F;
		const u8 base = instr_code >> 21 & 0x1F;
		const u64 address = GPR[base] + offset;

		if constexpr (instr == FPU_Instr::LWC1)
		{
			FGR[ft] = MMU::cpu_read_mem<u32>(address);
		}
		else if constexpr (instr == FPU_Instr::LDC1)
		{
			FGR[ft] = MMU::cpu_read_mem<u64>(address);
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
			MMU::cpu_write_mem<u32>(address, FGR[ft]);
		}
		else if constexpr (instr == FPU_Instr::SDC1)
		{
			MMU::cpu_write_mem<u64>(address, FGR[ft]);
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
			FGR[fs] = GPR[rt];
		}
		else if constexpr (instr == FPU_Instr::MFC1)
		{
			GPR[rt] = FGR[fs];
		}
		else if constexpr (instr == FPU_Instr::CTC1)
		{
			control[fs] = GPR[rt];
		}
		else if constexpr (instr == FPU_Instr::CFC1)
		{
			GPR[rt] = control[fs];
		}
		else if constexpr (instr == FPU_Instr::DMTC1)
		{
			FGR[fs] = GPR[rt];
		}
		else if constexpr (instr == FPU_Instr::DMFC1)
		{
			GPR[rt] = FGR[fs];
		}
		else
		{
			static_assert(false, "\"FPU_Move\" template function called, but no matching move instruction was found.");
		}
	}


	template<FPU_Instr instr>
	void FPU_Convert(const u32 instr_code)
	{
		const u8 fd = instr_code >> 6 & 0x1F;
		const u8 fs = instr_code >> 11 & 0x1F;
		const u8 fmt = instr_code >> 21 & 0x1F;

		/* For CVT instructions; interpret a source operand, "convert" it to a new format, then store the result in another register. */
		auto Convert = [&] <typename From, typename To>
		{
			const From source = FGR.InterpretAs<From>(fs);
			const To conv = To(source);
			FGR.Set<To>(fd, conv);
		};

		/* For ROUND/TRUNC/CEIL/FLOOR instructions; interpret a source operand (as a float), and round it to an integer (s32 or s64).
		   Also, check for the "validity" of the rounding (check for NaN etc.). */
		auto Round = [&] <typename Input_Float, typename Output_Int>
		{
			const Input_Float source = FGR.InterpretAs<Input_Float>(fs);

			if constexpr (std::is_same<Output_Int, s32>::value)
			{
				if (source < std::numeric_limits<s32>::min() ||
					source > std::numeric_limits<s32>::max() ||
					std::isnan(source) || std::isinf(source))
				{
					// TODO: if invalid operation exception is disabled, return 2^32 - 1
					bool exc_enabled = true; /* placeholder */
					if (exc_enabled)
						InvalidOperationException();
					else
						FGR.Set<s32>(fd, std::numeric_limits<s32>::max());
					return;
				}
			}
			else if constexpr (std::is_same<Output_Int, s64>::value)
			{
				if (source < std::numeric_limits<s64>::min() ||
					source > std::numeric_limits<s64>::max() ||
					std::isnan(source) || std::isinf(source))
				{
					// TODO: if invalid operation exception is disabled, return 2^32 - 1
					bool exc_enabled = true; /* placeholder */
					if (exc_enabled)
						InvalidOperationException();
					else
						FGR.Set<s64>(fd, std::numeric_limits<s64>::max());
					return;
				}
			}
			else
			{
				static_assert(false, "Incorrectly sized integer type given as an argument to the \"FPU_Convert\" function template.");
			}

			const Output_Int result = [&] {
				if constexpr (instr == FPU_Instr::ROUND_W || instr == FPU_Instr::ROUND_L)
					return std::round(source);
				else if constexpr (instr == FPU_Instr::TRUNC_W || instr == FPU_Instr::TRUNC_L)
					return std::trunc(source);
				else if constexpr (instr == FPU_Instr::CEIL_W || instr == FPU_Instr::CEIL_L)
					return std::ceil(source);
				else if constexpr (instr == FPU_Instr::FLOOR_W || instr == FPU_Instr::FLOOR_L)
					return std::floor(source);
				else
					static_assert(false, "\"FPU_Convert\" template function called, but no matching convert instruction was found.");
			}();

			FGR.Set<Output_Int>(fd, result);
		};

		if constexpr (instr == FPU_Instr::CVT_S)
		{
			/* TODO remove code duplication by having switch in lambda 'Convert'? */
			switch (fmt)
			{
			case NumericFormat::S:
				InvalidOperationException();
				break;

			case NumericFormat::D:
				Convert.template operator() <f64 /* old format */, f32 /* new format */> ();
				break;

			case NumericFormat::W:
				Convert.template operator() <s32, f32> ();
				break;

			case NumericFormat::L:
				Convert.template operator() <s64, f32> ();
				break;

			default:
				assert(false);
			}
		}
		else if constexpr (instr == FPU_Instr::CVT_D)
		{
			switch (fmt)
			{
			case NumericFormat::S:
				Convert.template operator() <f32, f64> ();
				break;

			case NumericFormat::D:
				InvalidOperationException();
				break;

			case NumericFormat::W:
				Convert.template operator() <s32, f64> ();
				break;

			case NumericFormat::L:
				Convert.template operator() <s64, f64> ();
				break;

			default:
				assert(false);
			}
		}
		else if constexpr (instr == FPU_Instr::CVT_W)
		{
			switch (fmt)
			{
			case NumericFormat::S:
				Convert.template operator() <f32, s32> ();
				break;

			case NumericFormat::D:
				Convert.template operator() <f64, s32> ();
				break;

			case NumericFormat::W:
				InvalidOperationException();
				break;

			case NumericFormat::L:
				Convert.template operator() <s64, s32> ();
				break;

			default:
				assert(false);
			}
		}
		else if constexpr (instr == FPU_Instr::CVT_L)
		{
			switch (fmt)
			{
			case NumericFormat::S:
				Convert.template operator() <f32, s64> ();
				break;

			case NumericFormat::D:
				Convert.template operator() <f64, s64> ();
				break;

			case NumericFormat::W:
				Convert.template operator() <s32, s64> ();
				break;

			case NumericFormat::L:
				InvalidOperationException();
				break;

			default:
				assert(false);
			}
		}

		else if constexpr (instr == FPU_Instr::ROUND_W || instr == FPU_Instr::TRUNC_W ||
			instr == FPU_Instr::CEIL_W  || instr == FPU_Instr::FLOOR_W)
		{
			switch (fmt)
			{
			case NumericFormat::S:
				Round.template operator() <f32 /* input format */, s32 /* output format */> ();
				break;

			case NumericFormat::D:
				Round.template operator() <f64, s32> ();
				break;

			case NumericFormat::W:
			case NumericFormat::L:
				InvalidOperationException();
				break;

			default:
				assert(false);
			}
		}
		else if constexpr (instr == FPU_Instr::ROUND_L || instr == FPU_Instr::TRUNC_L ||
			instr == FPU_Instr::CEIL_L || instr == FPU_Instr::FLOOR_L)
		{
			switch (fmt)
			{
			case NumericFormat::S:
				Round.template operator() <f32, s64> ();
				break;

			case NumericFormat::D:
				Round.template operator() <f64, s64> ();
				break;

			case NumericFormat::W:
			case NumericFormat::L:
				InvalidOperationException();
				break;

			default:
				assert(false);
			}
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
			const u8 ft = instr_code >> 16 & 0x1F;

			auto Compute = [&] <typename Float>
			{
				const Float op1 = FGR.InterpretAs<Float>(fs);
				const Float op2 = FGR.InterpretAs<Float>(ft);

				if constexpr (instr == FPU_Instr::DIV)
				{
					if (op2 == 0.0)
					{
						DivisionByZeroException();
						return;
					}
				}

				std::feclearexcept(FE_ALL_EXCEPT);

				const Float result = [&] {
					if constexpr (instr == FPU_Instr::ADD)
						return Float(op1 + op2);
					else if constexpr (instr == FPU_Instr::SUB)
						return op1 - op2;
					else if constexpr (instr == FPU_Instr::MUL)
						return op1 * op2;
					else if constexpr (instr == FPU_Instr::DIV)
						return op1 / op2;
					else
						static_assert(false);
				}();

				if (std::fetestexcept(FE_UNDERFLOW))
					UnderflowException();
				else if (std::fetestexcept(FE_OVERFLOW))
					OverflowException();
				// TODO: check for "inexact operation exception" (flag is FE_INEXACT)?

				FGR.Set<Float>(fd, result);
			};

			switch (fmt)
			{
			case NumericFormat::S:
				Compute.template operator() <f32> ();
				break;

			case NumericFormat::D:
				Compute.template operator() <f64> ();
				break;

			case NumericFormat::W:
			case NumericFormat::L:
				InvalidOperationException();
				break;

			default:
				assert(false);
			}
		}
		else if constexpr (instr == FPU_Instr::ABS || instr == FPU_Instr::MOV || instr == FPU_Instr::NEG || instr == FPU_Instr::SQRT)
		{
			auto Compute = [&] <typename Float>
			{
				const Float op = FGR.InterpretAs<Float>(fs);
				if constexpr (instr != FPU_Instr::MOV)
				{
					if (std::isnan(op))
					{
						InvalidOperationException();
						return;
					}
				}
				if constexpr (instr == FPU_Instr::SQRT)
				{
					if (op < 0.0)
					{
						InvalidOperationException();
						return;
					}
				}

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

			switch (fmt)
			{
			case NumericFormat::S:
				Compute.template operator() < f32 > ();
				break;

			case NumericFormat::D:
				Compute.template operator() < f64 > ();
				break;

			case NumericFormat::W:
			case NumericFormat::L:
				InvalidOperationException();
				break;

			default:
				assert(false);
			}
		}
		else
		{
			static_assert(false, "\"FPU_Compute\" template function called, but no matching compute instruction was found.");
		}
	}


	template<FPU_Instr instr>
	void FPU_Branch(const u32 instr_code)
	{

	}


	void FPU_Compare(const u32 instr_code)
	{
		const u8 cond = instr_code & 0xF;
		const u8 fs = instr_code >> 11 & 0x1F;
		const u8 ft = instr_code >> 16 & 0x1F;
		const u8 fmt = instr_code >> 21 & 0x1F;

		auto Compare = [&] <typename Float>
		{
			const Float op1 = FGR.InterpretAs<Float>(fs);
			const Float op2 = FGR.InterpretAs<Float>(ft);

			if ((cond & 0x80) && (std::isnan(op1) || std::isnan(op2)))
			{
				InvalidOperationException();
				return;
			}

			const bool comp_result = [&] {
				switch (cond)
				{
					/* TODO */
				}
			}();
		};
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