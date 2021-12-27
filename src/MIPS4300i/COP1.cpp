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
			From source = FGR.InterpretAs<From>(fs);
			To conv = To(source);
			FGR.Set<To>(fd, conv);
		};

		/* For ROUND/TRUNC/CEIL/FLOOR instructions; interpret a source operand (as a float), and round it to an integer (s32 or s64).
		   Also, check for the "validity" of the rounding (check for NaN etc.). */
		auto Round = [&] <typename Format_Float, typename Output_Int>
		{
			const Format_Float source = FGR.InterpretAs<Format_Float>(fs);

			if constexpr (sizeof Format_Float == sizeof f32)
			{
				if (source < std::numeric_limits<s32>::min() ||
					source > std::numeric_limits<s32>::max() ||
					std::isnan(source) || std::isinf(source))
				{
					// TODO: if invalid operation exception is disabled, return 2^32 - 1
					InvalidOperationException();
					return;
				}
			}
			else if constexpr (sizeof Format_Float == sizeof f64)
			{
				if (source < std::numeric_limits<s64>::min() ||
					source > std::numeric_limits<s64>::max() ||
					std::isnan(source) || std::isinf(source))
				{
					// TODO: if invalid operation exception is disabled, return 2^64 - 1
					InvalidOperationException();
					return;
				}
			}
			else
			{
				static_assert(false, "Incorrectly sized float given as an argument to the \"FPU_Convert\" function template.");
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
}