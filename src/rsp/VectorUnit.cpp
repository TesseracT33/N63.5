module RSP:VectorUnit;

import :Operation;
import :ScalarUnit;

import DebugOptions;

#define vco ctrl_reg[0]
#define vcc ctrl_reg[1]
#define vce ctrl_reg[2]

/* For invoking a parameter-free lambda template */
#define INVOKE(LAMBDA, ...) LAMBDA.template operator() <__VA_ARGS__> ()

namespace RSP
{
	void AddToAcc(__m128i low)
	{
		/* pseudo-code:
			for i in 0..7
				accumulator<i>(47..0) += low<i>
			endfor
		*/
		__m128i prev_acc_low = acc.low;
		acc.low = _mm_add_epi16(acc.low, low);
		__m128i low_carry = _mm_srli_epi16(_mm_cmplt_epu16(acc.low, prev_acc_low), 15);
		acc.mid = _mm_add_epi16(acc.mid, low_carry);
		__m128i mid_carry = _mm_and_si128(low_carry, _mm_cmpeq_epi16(acc.mid, m128i_zero));
		acc.high = _mm_add_epi16(acc.high, mid_carry);
	}

	void AddToAcc(__m128i low, __m128i mid)
	{
		/* pseudo-code:
			for i in 0..7
				accumulator<i>(47..0) += mid<i> << 16 | low<i>
			endfor
		*/
		AddToAcc(low);
		__m128i prev_acc_mid = acc.mid;
		acc.mid = _mm_add_epi16(acc.mid, mid);
		__m128i mid_carry = _mm_cmplt_epu16(acc.mid, prev_acc_mid);
		mid_carry = _mm_srli_epi16(mid_carry, 15);
		acc.high = _mm_add_epi16(acc.high, mid_carry);
	}


	void AddToAcc(__m128i low, __m128i mid, __m128i high)
	{
		/* pseudo-code:
			for i in 0..7
				accumulator<i>(47..0) += high<i> << 32 | mid<i> << 16 | low<i>
			endfor
		*/
		AddToAcc(low, mid);
		acc.high = _mm_add_epi16(acc.high, high);
	}


	void AddToAccFromMid(__m128i mid, __m128i high)
	{
		/* pseudo-code:
			for i in 0..7
				accumulator<i>(47..0) += high<i> << 32 | mid<i> << 16
			endfor
		*/
		__m128i prev_acc_mid = acc.mid;
		acc.mid = _mm_add_epi16(acc.mid, mid);
		__m128i mid_carry = _mm_srli_epi16(_mm_cmplt_epu16(acc.mid, prev_acc_mid), 15);
		acc.high = _mm_add_epi16(acc.high, high);
		acc.high = _mm_add_epi16(acc.high, mid_carry);
	}


	__m128i ClampSigned(__m128i low, __m128i high)
	{
		/* pseudo-code:
			for i in 0..7
				value = high<i> << 16 | low<i>
				if value < -32768
					result<i> = -32768
				elif value > 32767
					result<i> = 32767
				else
					result<i> = value(15..0)
				endif
			endfor
		*/
		__m128i words1 = _mm_unpacklo_epi16(low, high);
		__m128i words2 = _mm_unpackhi_epi16(low, high);
		return _mm_packs_epi32(words1, words2);
	}


	template<VectorInstruction instr>
	__m128i ClampUnsigned(__m128i low, __m128i high)
	{
		/* pseudo-code (depends on 'instr'):
			for i in 0..7
				value = high<i> << 16 | low<i>
				if value < 0
					result<i> = 0
				elif value > 32767
					result<i> = 65535
				else
					result<i> = value(15..0)
				endif
			endfor
		*/
		__m128i words1 = _mm_unpacklo_epi16(low, high);
		__m128i words2 = _mm_unpackhi_epi16(low, high);
		/* Unsigned saturation in SSE can differ from that in the RSP in that
		the result is set to 65535 only if value >= 65535. It depends on the RSP instruction. */
		using enum VectorInstruction;
		__m128i clamp_sse = _mm_packus_epi32(words1, words2);
		if constexpr (instr == VMULU || instr == VMACU) {
			return _mm_blendv_epi8(clamp_sse, m128i_all_ones, _mm_srai_epi16(clamp_sse, 15));
		}
		else {
			return clamp_sse;
		}
	}


	__m128i GetVTBroadcast(uint vt /* 0-31 */, uint element /* 0-15 */)
	{
		return _mm_shuffle_epi8(vpr[vt], broadcast_mask[element]);
	}


	template<VectorInstruction instr>
	void VectorLoadStore(u32 instr_code)
	{
		using enum VectorInstruction;

		s32 offset = SignExtend<s32, 7>(instr_code & 0x7F);
		auto element = instr_code >> 7 & 0xF;
		auto vt = instr_code >> 16 & 0x1F;
		auto base = instr_code >> 21 & 0x1F;

		/* LBV, LSV, LLV, LDV */
		auto LoadUpToDoubleword = [&] <std::signed_integral Int> {
			u8* vpr_dst = (u8*)(&vpr[vt]);
			auto addr = gpr[base] + offset * sizeof(Int);
			if constexpr (sizeof(Int) == 1) {
				*(vpr_dst + (element ^ 1)) = dmem[addr & 0xFFF];
			}
			else {
				u32 num_bytes = std::min((u32)sizeof(Int), 16 - element);
				for (u32 i = 0; i < num_bytes; ++i) {
					*(vpr_dst + (element + i ^ 1)) = dmem[addr + i & 0xFFF];
				}
			}

			if constexpr (log_rsp_instructions) {
				current_instr_log_output = std::format("{} {} e{}, ${:X}",
					current_instr_name, vt, element, MakeUnsigned(addr));
			}
		};

		/* SBV, SSV, SLV, SDV */
		auto StoreUpToDoubleword = [&] <std::signed_integral Int> {
			const u8* vpr_src = (u8*)(&vpr[vt]);
			auto addr = gpr[base] + offset * sizeof(Int);
			for (size_t i = 0; i < sizeof(Int); ++i) {
				dmem[addr + i & 0xFFF] = *(vpr_src + ((element + i ^ 1) & 0xF));
			}

			if constexpr (log_rsp_instructions) {
				current_instr_log_output = std::format("{} {} e{}, ${:X}",
					current_instr_name, vt, element, MakeUnsigned(addr));
			}
		};

		/* LPV, LUV, SPV, SUV */
		auto PackedLoadStore = [&] {
			/* 8-bit packed load;
			Pseudo-code (s == 8 for LPV; s == 7 for LUV):
				addr = GPR[base] + offset * 8
				for i in 0..7
					VPR<i>(15..0) = DMEM[(addr + i) & ~7] << s
				endfor
			*/
			/* 'element' selects the first lane, not byte, being accessed */
			s16* vpr_src = (s16*)(&vpr[vt]);
			auto addr = (gpr[base] + offset * 8) & 0xFFF;
			auto addr_dword_offset = addr;
			addr &= 0xFF8;
			auto current_lane = element;
			static constexpr auto shift_amount = [&] {
				if constexpr (instr == LPV || instr == SPV) return 8;
				else return 7;
			}();
			for (int i = 0; i < 8; ++i) {
				if constexpr (instr == LPV || instr == LUV) {
					*(vpr_src + (current_lane & 7)) = dmem[addr | addr_dword_offset & 7] << shift_amount;
				}
				else {
					dmem[addr | addr_dword_offset & 7] =
						*(vpr_src + (current_lane & 7)) >> shift_amount & 0xFF;
				}
				++current_lane;
				++addr_dword_offset;
			}

			if constexpr (log_rsp_instructions) {
				current_instr_log_output = std::format("{} {} e{}, ${:X}",
					current_instr_name, vt, element, MakeUnsigned(addr));
			}
		};

		if constexpr (instr == LBV)      INVOKE(LoadUpToDoubleword, s8);
		else if constexpr (instr == LSV) INVOKE(LoadUpToDoubleword, s16);
		else if constexpr (instr == LLV) INVOKE(LoadUpToDoubleword, s32);
		else if constexpr (instr == LDV) INVOKE(LoadUpToDoubleword, s64);
		else if constexpr (instr == LQV || instr == LRV) {
			u8* vpr_dst = (u8*)(&vpr[vt]);
			u32 addr = gpr[base] + offset * 16;

			if constexpr (log_rsp_instructions) {
				current_instr_log_output = std::format("{} {} e{}, ${:X}",
					current_instr_name, vt, element, MakeUnsigned(addr));
			}

			u32 num_bytes;
			if constexpr (instr == LQV) {
				num_bytes = 16 - std::max(addr & 0xF, element); // == std::min(16 - (addr & 0xF), 16 - element)
				addr &= 0xFFF;
			}
			else {
				element += 16 - (addr & 0xF);
				if (element >= 16) return;
				num_bytes = 16 - element;
				addr &= 0xFF0;
			}
			for (u32 i = 0; i < num_bytes; ++i) {
				*(vpr_dst + (element++ ^ 1)) = dmem[addr++];
			}
		}
		else if constexpr (instr == LTV) {
			const auto start_addr = gpr[base] + offset * 16;
			const auto wrap_addr = start_addr & 0xFF8;
			const auto num_bytes_until_addr_wrap = 16 - (start_addr & 7);
			auto current_addr = start_addr;
			auto current_reg = vt & 0x18;
			auto current_elem = (0x10 - element) & 0xE;
			bool on_odd_byte = 1;

			auto CopyNextByte = [&] {
				u8* vpr_dst = (u8*)(&vpr[current_reg]);
				*(vpr_dst + (current_elem & 0xE) + on_odd_byte) = dmem[current_addr & 0xFFF];
				on_odd_byte ^= 1;
				current_addr++;
				current_elem++; /* increment by two every other byte copy (achieved by ANDing with 0xE above) */
				current_reg += on_odd_byte; /* increment every other byte copy */
			};

			for (int i = 0; i < num_bytes_until_addr_wrap; ++i) {
				CopyNextByte();
			}
			current_addr = wrap_addr;
			for (int i = 0; i < 16 - num_bytes_until_addr_wrap; ++i) {
				CopyNextByte();
			}

			if constexpr (log_rsp_instructions) {
				current_instr_log_output = std::format("LTV {} e{}, ${:X}",
					vt & 0x18, element & 0xE, MakeUnsigned(start_addr));
			}
		}
		else if constexpr (instr == STV) {
			const auto start_addr = gpr[base] + offset * 16;
			const auto wrap_addr = start_addr & 0xFF8;
			const auto num_bytes_until_addr_wrap = 16 - (start_addr & 7);
			const auto base_reg = vt & 0x18;
			auto current_addr = start_addr;
			auto current_reg = base_reg | element >> 1;
			auto current_elem = 0;
			bool on_odd_byte = 1;

			auto CopyNextByte = [&] {
				u8* vpr_src = (u8*)(&vpr[current_reg]);
				dmem[current_addr & 0xFFF] = *(vpr_src + (current_elem & 0xE) + on_odd_byte);
				on_odd_byte ^= 1;
				current_addr++;
				current_elem++; /* increment by two every other byte copy (achieved by ANDing with 0xE above) */
				current_reg += on_odd_byte; /* increment every other byte copy */
				current_reg &= base_reg | current_reg & 7;
			};

			for (int i = 0; i < num_bytes_until_addr_wrap; ++i) {
				CopyNextByte();
			}
			current_addr = wrap_addr;
			for (int i = 0; i < 16 - num_bytes_until_addr_wrap; ++i) {
				CopyNextByte();
			}

			if constexpr (log_rsp_instructions) {
				current_instr_log_output = std::format("STV {} e{}, ${:X}",
					base_reg, element & 0xE, MakeUnsigned(start_addr));
			}
		}
		else if constexpr (instr == LPV || instr == LUV || instr == SPV || instr == SUV) {
			PackedLoadStore();
		}
		else if constexpr (instr == SBV) INVOKE(StoreUpToDoubleword, s8);
		else if constexpr (instr == SSV) INVOKE(StoreUpToDoubleword, s16);
		else if constexpr (instr == SLV) INVOKE(StoreUpToDoubleword, s32);
		else if constexpr (instr == SDV) INVOKE(StoreUpToDoubleword, s64);
		else if constexpr (instr == SQV || instr == SRV) {
			const u8* vpr_src = (u8*)(&vpr[vt]);
			auto addr = gpr[base] + offset * 16;

			if constexpr (log_rsp_instructions) {
				current_instr_log_output = std::format("{} {} e{}, ${:X}",
					current_instr_name, vt, element, MakeUnsigned(addr));
			}

			u32 num_bytes, base_element;
			if constexpr (instr == SQV) {
				num_bytes = 16 - (addr & 0xF);
				base_element = 0;
				addr &= 0xFFF;
			}
			else {
				num_bytes = addr & 0xF;
				base_element = 16 - (addr & 0xF);
				addr &= 0xFF0;
			}
			for (u32 i = 0; i < num_bytes; ++i) {
				dmem[addr++] = *(vpr_src + ((base_element + element++ & 0xF) ^ 1));
			}
		}
		else if constexpr (instr == LWV) {
			u8* vpr_dst = (u8*)(&vpr[vt]);
			auto addr = gpr[base] + offset * 16;

			if constexpr (log_rsp_instructions) {
				current_instr_log_output = std::format("LWV {} e{}, ${:X}",
					vt, element, MakeUnsigned(addr));
			}

			for (auto current_elem = 16 - element; current_elem < element + 16; ++current_elem) {
				*(vpr_dst + ((current_elem & 0xF) ^ 1)) = dmem[addr & 0xFFF];
				addr += 4;
			}
		}
		else if constexpr (instr == SWV) {
			u8* vpr_src = (u8*)(&vpr[vt]);
			auto addr = gpr[base] + offset * 16;

			if constexpr (log_rsp_instructions) {
				current_instr_log_output = std::format("SWV {} e{}, ${:X}",
					vt, element, MakeUnsigned(addr));
			}

			auto base = addr & 7;
			addr &= ~7;
			for (auto current_elem = element; current_elem < element + 16; ++current_elem) {
				dmem[(addr + (base++ & 0xF)) & 0xFFF] = *(vpr_src + ((current_elem & 0xF) ^ 1));
			}
		}
		else if constexpr (instr == LHV || instr == LFV || instr == SHV || instr == SFV) {
			/* TODO */
			assert(false);
		}
		else {
			static_assert(AlwaysFalse<instr>);
		}

		AdvancePipeline(1);
	}


	template<VectorInstruction instr>
	void Move(u32 instr_code)
	{
		using enum VectorInstruction;

		auto vs_elem = instr_code >> 8 & 0x7;
		auto vs = instr_code >> 11 & 0x1F;
		auto rt = instr_code >> 16 & 0x1F;

		if constexpr (log_rsp_instructions) {
			current_instr_log_output = std::format("{} GPR[{}] VPR[{}] e{}",
				current_instr_name, rt, vs, vs_elem);
		}

		if constexpr (instr == MTC2) {
			/* Pseudo-code:
				VS<elem>(15..0) = GPR[rt](15..0)
			*/
			_mm_setlane_epi16(&vpr[vs], vs_elem, s16(gpr[rt]));
		}
		else if constexpr (instr == MFC2) {
			/* Pseudo-code:
				GPR[rt](31..0) = sign_extend(VS<elem>(15..0))
			*/
			gpr.Set(rt, _mm_getlane_epi16(&vpr[vs], vs_elem));
		}
		else if constexpr (instr == CTC2) {
			/* Pseudo-code:
				CTRL(15..0) = GPR(15..0)
			*/
			/* Control registers (16-bit) are encoded in two __m128i. Each lane represents one bit. */
			assert(vs < 3);
			static constexpr std::array lanes = {
				s64(0x0000'0000'0000'0000), s64(0x0000'0000'0000'FFFF), s64(0x0000'0000'FFFF'0000), s64(0x0000'0000'FFFF'FFFF),
				s64(0x0000'FFFF'0000'0000), s64(0x0000'FFFF'0000'FFFF), s64(0x0000'FFFF'FFFF'0000), s64(0x0000'FFFF'FFFF'FFFF),
				s64(0xFFFF'0000'0000'0000), s64(0xFFFF'0000'0000'FFFF), s64(0xFFFF'0000'FFFF'0000), s64(0xFFFF'0000'FFFF'FFFF),
				s64(0xFFFF'FFFF'0000'0000), s64(0xFFFF'FFFF'0000'FFFF), s64(0xFFFF'FFFF'FFFF'0000), s64(0xFFFF'FFFF'FFFF'FFFF)
			};
			s32 r = gpr[rt];
			ctrl_reg[vs].low = _mm_set_epi64x(lanes[r >> 4 & 0xF], lanes[r >> 0 & 0xF]);
			ctrl_reg[vs].high = _mm_set_epi64x(lanes[r >> 12 & 0xF], lanes[r >> 8 & 0xF]);
		}
		else if constexpr (instr == CFC2) {
			/* Pseudo-code:
				GPR(31..0) = sign_extend(CTRL(15..0))
			*/
			assert(vs < 3);
			int lo = _mm_movemask_epi8(_mm_packs_epi16(ctrl_reg[vs].low, m128i_zero));
			int hi = _mm_movemask_epi8(_mm_packs_epi16(ctrl_reg[vs].high, m128i_zero));
			gpr.Set(rt, s16(hi << 8 | lo));
		}
		else {
			static_assert(AlwaysFalse<instr>);
		}

		AdvancePipeline(1);
	}


	template<VectorInstruction instr>
	void SingleLaneInstr(u32 instr_code)
	{
		using enum VectorInstruction;

		auto vd = instr_code >> 6 & 0x1F;
		auto vd_elem = instr_code >> 11 & 7; /* vd_elem is 4 bits, but the highest bit is always ignored */
		auto vt = instr_code >> 16 & 0x1F;
		/* When vt_elem(3) is 0, a hardware bug is triggered and portions of the lower bits of vt_elem are replaced with portion of the bits of vd_elem
		while computing se. Specifically, all bits in vt_elem from the topmost set bit and higher are replaced with the same-position bits in vd_elem. */
		auto vt_elem = instr_code >> 21 & 0xF;

		//if (!(vt_elem & 8)) {
		//	     if (vt_elem & 4) vt_elem = vd_elem & 4 | vt_elem & 3;
		//	else if (vt_elem & 2) vt_elem = vd_elem & 6 | vt_elem & 1;
		//	else if (vt_elem & 1) vt_elem = vd_elem;
		//	else                  vt_elem = 0;
		//}

		if (vt_elem & 7) {
			bool vt_elem_bit_3 = vt_elem & 8;
			vt_elem &= 7;
			if (vt_elem_bit_3 == 0) {
				if (vt_elem & 4) vt_elem = vd_elem & 4 | vt_elem & 3;
				else if (vt_elem & 2) vt_elem = vd_elem & 6 | vt_elem & 1;
				else                  vt_elem = vd_elem;
			}
		}
		else {
			vt_elem = 0; /* effectively clear bit 3 */
		}


		if constexpr (log_rsp_instructions) {
			current_instr_log_output = [&] {
				if constexpr (instr == VNOP || instr == VNULL) {
					return current_instr_name;
				}
				else {
					return std::format("{} VD {} e{} VT {} e{}",
						current_instr_name, vt, vt_elem, vd, vd_elem);
				}
			}();
		}

		auto Rcp = [&](s32 input) {
			auto mask = input >> 31;
			auto data = input ^ mask;
			if (input > -32768) {
				data -= mask;
			}
			if (data == 0) {
				return 0x7FFF'FFFF;
			}
			else if (input == -32768) {
				return s32(0xFFFF'0000);
			}
			else {
				auto shift = std::countl_zero(MakeUnsigned(data));
				auto index = (u64(data) << shift & 0x7FC0'0000) >> 22;
				s32 result = rcp_rom[index];
				result = (0x10000 | result) << 14;
				return (result >> (31 - shift)) ^ mask;
			}
		};

		auto Rsq = [&](s32 input) {
			if (input == 0) {
				return 0x7FFF'FFFF;
			}
			else if (input == -32768) {
				return s32(0xFFFF'0000);
			}
			else {
				auto unsigned_input = MakeUnsigned(std::abs(input));
				auto lshift = std::countl_zero(unsigned_input) + 1;
				auto rshift = (32 - lshift) >> 1;
				auto index = (unsigned_input << lshift) >> 24;
				auto rom = rsq_rom[(index | ((lshift & 1) << 8))];
				s32 result = ((0x10000 | rom) << 14) >> rshift;
				if (unsigned_input != input) {
					return ~result;
				}
				else {
					return result;
				}
			}
		};

		if constexpr (instr == VMOV) {
			_mm_setlane_epi16(&vpr[vd], vd_elem, _mm_getlane_epi16(&vpr[vt], vt_elem));
			acc.low = vpr[vt];
		}
		else if constexpr (instr == VRCP || instr == VRSQ) {
			s32 input = _mm_getlane_epi16(&vpr[vt], vt_elem);
			auto result = [&] {
				if constexpr (instr == VRCP) return Rcp(input);
				else return Rsq(input);
			}();
			_mm_setlane_epi16(&vpr[vd], vd_elem, s16(result));
			div_out = result >> 16 & 0xFFFF;
			div_dp = 0;
			acc.low = vpr[vt];
		}
		else if constexpr (instr == VRCPH || instr == VRSQH) {
			_mm_setlane_epi16(&vpr[vd], vd_elem, div_out);
			div_in = _mm_getlane_epi16(&vpr[vt], vt_elem);
			div_dp = 1;
			acc.low = vpr[vt];
		}
		else if constexpr (instr == VRCPL || instr == VRSQL) {
			s32 input = div_in << 16 | _mm_getlane_epi16(&vpr[vt], vt_elem);
			auto result = [&] {
				if constexpr (instr == VRCPL) return Rcp(input);
				else return Rsq(input);
			}();
			_mm_setlane_epi16(&vpr[vd], vd_elem, s16(result));
			div_out = result >> 16 & 0xFFFF;
			div_in = 0;
			div_dp = 0;
			acc.low = vpr[vt];
		}
		else if constexpr (instr == VRNDN) {
			assert(false); /* TODO */
		}
		else if constexpr (instr == VRNDP) {
			assert(false); /* TODO */
		}
		else if constexpr (instr == VNOP || instr == VNULL) {

		}
		else {
			static_assert(AlwaysFalse<instr>);
		}

		AdvancePipeline(1);
	}


	template<VectorInstruction instr>
	void ComputeInstr(u32 instr_code)
	{
		using enum VectorInstruction;

		auto vd = instr_code >> 6 & 0x1F;
		auto vs = instr_code >> 11 & 0x1F;
		auto vt = instr_code >> 16 & 0x1F;
		auto element = instr_code >> 21 & 0xF;

		if constexpr (log_rsp_instructions) {
			current_instr_log_output = std::format("{} {} {} {} e{}",
				current_instr_name, vd, vs, vt, element);
		}

		/* Determine which lanes of vpr[vt] to access */
		__m128i vt_operand = GetVTBroadcast(vt, element);

		if constexpr (instr == VADD) {
			/* Pseudo-code:
				for i in 0..7
					result(16..0) = VS<i>(15..0) + VT<i>(15..0) + VCO(i)
					ACC<i>(15..0) = result(15..0)
					VD<i>(15..0) = clamp_signed(result(16..0))
					VCO(i) = 0
					VCO(i + 8) = 0
				endfor
			*/
			__m128i vco_op = _mm_and_si128(vco.low, m128i_one);
			acc.low = _mm_add_epi16(vpr[vs], _mm_add_epi16(vt_operand, vco_op));
			vpr[vd] = _mm_adds_epi16(vpr[vs], _mm_adds_epi16(vt_operand, vco_op));
			std::memset(&vco, 0, sizeof(vco));
		}
		else if constexpr (instr == VSUB) {
			/* Pseudo-code:
				for i in 0..7
					result(16..0) = VS<i>(15..0) - VT<i>(15..0) - VCO(i)
					ACC<i>(15..0) = result(15..0)
					VD<i>(15..0) = clamp_signed(result(16..0))
					VCO(i) = 0
					VCO(i + 8) = 0
				endfor
			*/
			__m128i vco_op = _mm_and_si128(vco.low, m128i_one);
			acc.low = _mm_sub_epi16(vpr[vs], _mm_add_epi16(vt_operand, vco_op));
			vpr[vd] = _mm_subs_epi16(vpr[vs], _mm_adds_epi16(vt_operand, vco_op));
			std::memset(&vco, 0, sizeof(vco));
		}
		else if constexpr (instr == VADDC) {
			/* Pseudo-code:
				for i in 0..7
					result(16..0) = VS<i>(15..0) + VT<i>(15..0)
					ACC<i>(15..0) = result(15..0)
					VD<i>(15..0) = result(15..0)
					VCO(i) = result(16)
					VCO(i + 8) = 0
				endfor
			*/
			vpr[vd] = acc.low = _mm_add_epi16(vpr[vs], vt_operand);
			vco.low = _mm_cmplt_epu16(vpr[vd], vt_operand); /* check carry */
			std::memset(&vco.high, 0, sizeof(vco.high));
		}
		else if constexpr (instr == VSUBC) {
			/* Pseudo-code:
				for i in 0..7
					result(16..0) = VS<i>(15..0) - VT<i>(15..0)
					ACC<i>(15..0) = result(15..0)
					VD<i>(15..0) = result(15..0)
					VCO(i) = result(16)
					VCO(i + 8) = result(16..0) != 0
				endfor
			*/
			vco.low = _mm_cmplt_epu16(vpr[vs], vt_operand); /* check borrow */
			vpr[vd] = acc.low = _mm_sub_epi16(vpr[vs], vt_operand);
			vco.high = _mm_or_si128(vco.low, _mm_cmpneq_epi16(vpr[vd], m128i_zero));
		}
		else if constexpr (instr == VMULF || instr == VMULU) {
			/* Pseudo-code:
				for i in 0..7
					prod(32..0) = VS<i>(15..0) * VT<i>(15..0) * 2   // signed multiplication
					ACC<i>(47..0) = sign_extend(prod(32..0) + 0x8000)
					VD<i>(15..0) = clamp(ACC<i>(47..16))
				endfor
			*/
			/* Tests indicate the following behavior. The product is 33-bits, and product + 0x8000 is
			   also kept in 33 bits. Example:
			   oper1 = 0xFFEE, oper2 = 0x0011 => prod(32..0) = 0x1'FFFF'FD9C.
			   prod(32..0) + 0x8000 = 0x0'0000'7D9C. */
			__m128i low = _mm_mullo_epi16(vpr[vs], vt_operand);
			__m128i high = _mm_mulhi_epi16(vpr[vs], vt_operand);
			/* multiply by two */
			__m128i low_carry_mul = _mm_srli_epi16(low, 15); /* note: either 0 or 1 */
			__m128i high_carry_mul = _mm_srai_epi16(high, 15); /* note: either 0 or 0xFFFF */
			low = _mm_slli_epi16(low, 1);
			high = _mm_slli_epi16(high, 1);
			high = _mm_add_epi16(high, low_carry_mul);
			/* add $8000 */
			low = _mm_add_epi16(low, m128i_epi16_sign_mask);
			__m128i low_carry_add = _mm_srli_epi16(_mm_cmpgt_epi16(low, m128i_zero), 15); /* carry if low >= 0 */
			high = _mm_add_epi16(high, low_carry_add);
			__m128i high_carry_add = _mm_and_si128(_mm_cmpeq_epi16(high, m128i_zero), _mm_cmpeq_epi16(low_carry_add, m128i_one));
			acc.low = low;
			acc.mid = high;
			/* The XOR achieves the correct 33-bit overflow behaviour and subsequent sign-extension to 48 bits.
			   E.g., if prod(32) = 1, but the mid lane overflowed when adding 0x8000, then acc(47..32) = 0.
			   Notice that high carries are always computed as either 0 or 0xFFFF. */
			acc.high = _mm_xor_si128(high_carry_mul, high_carry_add);
			if constexpr (instr == VMULF) {
				vpr[vd] = ClampSigned(acc.mid, acc.high);
			}
			else {
				vpr[vd] = ClampUnsigned<instr>(acc.mid, acc.high);
			}
		}
		else if constexpr (instr == VMULQ) {
			__m128i low = _mm_mullo_epi16(vpr[vs], vt_operand);
			__m128i high = _mm_mulhi_epi16(vpr[vs], vt_operand);
			/* add 31 to product if product < 0 */
			__m128i addend = _mm_and_si128(_mm_srai_epi16(high, 15), _mm_set1_epi16(0x1F));
			low = _mm_add_epi16(low, addend);
			__m128i low_carry = _mm_srli_epi16(_mm_cmplt_epu16(low, addend), 15);
			high = _mm_add_epi16(high, low_carry);
			acc.low = m128i_zero;
			acc.mid = low;
			acc.high = high;
			vpr[vd] = _mm_and_si128(_mm_set1_epi16(~0xF),
				ClampSigned(_mm_srai_epi16(acc.mid, 1), _mm_srai_epi16(acc.high, 1)));
		}
		else if constexpr (instr == VMACQ) {
			__m128i low = _mm_mullo_epi16(acc.mid, acc.high);
			__m128i high = _mm_mulhi_epi16(acc.mid, acc.high);
			/* if bit 5 of product is clear, add 32 if product < 0, else if product >= 32, subtract 32. */
			const __m128i mask = _mm_set1_epi16(32);
			const __m128i addend = _mm_and_si128(_mm_not_si128(low), mask); /* 0 or 32 */
			/* Possibly add 32. No carry in low, since bit 5 is clear. */
			__m128i pos_addend = _mm_and_si128(addend,
				_mm_cmplt_epi16(high, m128i_zero)); /* prod < 0 */
			low = _mm_add_epi16(low, pos_addend);
			/* Possibly subtract 32. Explanation: If bit 5 is clear, then product >= 64.
			   First, subtract 32 from low. This subtracts one from the upper 11 bits of low.
			   If the result underflows (low & 0xFFE0 == 0xFFE0), subtract one from high. This cannot
			   lead to underflow in high, since product >= 64, and if low underflowed, then 0 <= low <= 31
			   before the subtraction. Thus, at least one bit of high must be set. */
			__m128i neg_addend = _mm_and_si128(addend,
				_mm_and_si128(_mm_cmpge_epi16(high, m128i_zero), /* prod >= 0 */
					_mm_or_si128(_mm_cmpgt_epu16(low, mask), _mm_cmpgt_epi16(high, m128i_zero)))); /* low > 32 or high > 0 */
			low = _mm_sub_epi16(low, neg_addend);
			__m128i low_borrow = _mm_srli_epi16(_mm_cmpeq_epi16(
				_mm_and_si128(low, _mm_set1_epi16(0xFFE0)), _mm_set1_epi16(0xFFE0)), 15);
			high = _mm_sub_epi16(high, low_borrow);
			AddToAccFromMid(low, high);
			__m128i clamp_input_low = _mm_or_si128(_mm_srli_epi16(acc.mid, 1), _mm_slli_epi16(acc.high, 15));
			__m128i clamp_input_high = _mm_srai_epi16(acc.high, 1);
			vpr[vd] = _mm_and_si128(_mm_set1_epi16(~0xF), ClampSigned(clamp_input_low, clamp_input_high));
		}
		else if constexpr (instr == VMACF || instr == VMACU) {
			/* Pseudo-code:
				for i in 0..7
					prod(32..0) = VS<i>(15..0) * VT<i>(15..0) * 2   // signed multiplication
					ACC<i>(47..0) += sign_extend(prod(32..0))
					VD<i>(15..0) = clamp(ACC<i>(47..16))
				endfor*/
			__m128i low = _mm_mullo_epi16(vpr[vs], vt_operand);
			__m128i high = _mm_mulhi_epi16(vpr[vs], vt_operand);
			/* multiply by two to get a 33-bit product. Sign-extend to 48 bits, and add to the accumulator. */
			__m128i low_carry = _mm_srli_epi16(low, 15);
			__m128i high_carry = _mm_srai_epi16(high, 15);
			low = _mm_slli_epi16(low, 1);
			high = _mm_slli_epi16(high, 1);
			high = _mm_add_epi16(high, low_carry);
			AddToAcc(low, high, high_carry);
			if constexpr (instr == VMACF) {
				vpr[vd] = ClampSigned(acc.mid, acc.high);
			}
			else {
				vpr[vd] = ClampUnsigned<instr>(acc.mid, acc.high);
			}
		}
		else if constexpr (instr == VMUDN || instr == VMADN) {
			/* Pseudo-code:
				for i in 0..7
					prod(31..0) = VS<i>(15..0) * VT<i>(15..0)   // unsigned by signed
					ACC<i>(47..0) (+)= sign_extend(prod(31..0))
					VD<i>(15..0) = ACC<i>(15..0)
				endfor */
			__m128i low = _mm_mullo_epi16(vpr[vs], vt_operand);
			__m128i high = _mm_mulhi_epu16_epi16(vpr[vs], vt_operand);
			__m128i sign_ext = _mm_srai_epi16(high, 15);
			if constexpr (instr == VMUDN) {
				acc.low = low;
				acc.mid = high;
				acc.high = sign_ext;
			}
			else {
				AddToAcc(low, high, sign_ext);
			}
			vpr[vd] = acc.low;
		}
		else if constexpr (instr == VMUDL || instr == VMADL) {
			/* Pseudo-code:
				for i in 0..7
					prod(31..0) = VS<i>(15..0) * VT<i>(15..0)   // unsigned multiplication
					ACC<i>(47..0) (+)= prod(31..16)
					VD<i>(15..0) = clamp_unsigned(ACC<i>(31..0))
				endfor
			*/
			__m128i high = _mm_mulhi_epu16(vpr[vs], vt_operand);
			if constexpr (instr == VMUDL) {
				acc.low = high;
			}
			else {
				AddToAcc(high);
			}
			vpr[vd] = ClampUnsigned<instr>(acc.low, acc.mid);
		}
		else if constexpr (instr == VMUDM || instr == VMADM) {
			/* Pseudo-code:
				for i in 0..7
					prod(31..0) = VS<i>(15..0) * VT<i>(15..0)   // signed by unsigned
					ACC<i>(47..0) (+)= sign_extend(prod(31..0))
					VD<i>(15..0) = clamp_signed(ACC<i>(47..16))
				endfor
			*/
			__m128i low = _mm_mullo_epi16(vpr[vs], vt_operand);
			__m128i high = _mm_mulhi_epu16_epi16(vt_operand, vpr[vs]);
			__m128i sign_ext = _mm_srai_epi16(high, 15);
			if constexpr (instr == VMUDM) {
				acc.low = low;
				acc.mid = high;
				acc.high = sign_ext;
			}
			else {
				AddToAcc(low, high, sign_ext);
			}
			vpr[vd] = ClampSigned(acc.mid, acc.high);
		}
		else if constexpr (instr == VMUDH || instr == VMADH) {
			/* Pseudo-code:
				for i in 0..7
					prod(31..0) = VS<i>(15..0) * VT<i>(15..0)   // signed multiplication
					ACC<i>(47..16) (+)= prod(31..0)
					VD<i>(15..0) = clamp_signed(ACC<i>(47..16))
				endfor*/
			__m128i low = _mm_mullo_epi16(vpr[vs], vt_operand);
			__m128i high = _mm_mulhi_epi16(vpr[vs], vt_operand);
			if constexpr (instr == VMUDH) {
				acc.low = m128i_zero; /* seems necessary given tests */
				acc.mid = low;
				acc.high = high;
			}
			else {
				AddToAccFromMid(low, high);
			}
			vpr[vd] = ClampSigned(acc.mid, acc.high);

		}
		else if constexpr (instr == VABS) {
			/* If a lane is 0x8000, store 0x7FFF to vpr[vd], and 0x8000 to the accumulator. */
			__m128i eq0 = _mm_cmpeq_epi16(vpr[vs], m128i_zero);
			__m128i slt = _mm_srai_epi16(vpr[vs], 15);
			vpr[vd] = _mm_andnot_si128(eq0, vt_operand);
			vpr[vd] = _mm_xor_si128(vpr[vd], slt);
			acc.low = _mm_sub_epi16(vpr[vd], slt);
			vpr[vd] = _mm_subs_epi16(vpr[vd], slt);
		}
		else if constexpr (instr == VSAR) {
			vpr[vd] = [&] {
				switch (element) {
				case 0x8: return acc.high;
				case 0x9: return acc.mid;
				case 0xA: return acc.low;
				default: return m128i_zero;
				}
			}();
		}
		/* Pseudo-code VAND/VNAND/VOR/VNOR/VXOR/VNXOR:
			for i in 0..7
				ACC<i>(15..0) = VS<i>(15..0) <LOGICAL_OP> VT<i>(15..0)
				VD<i>(15..0) = ACC<i>(15..0)
			endfor
		*/
		else if constexpr (instr == VAND) {
			vpr[vd] = acc.low = _mm_and_si128(vpr[vs], vt_operand);
		}
		else if constexpr (instr == VNAND) {
			vpr[vd] = acc.low = _mm_nand_si128(vpr[vs], vt_operand);
		}
		else if constexpr (instr == VOR) {
			vpr[vd] = acc.low = _mm_or_si128(vpr[vs], vt_operand);
		}
		else if constexpr (instr == VNOR) {
			vpr[vd] = acc.low = _mm_nor_si128(vpr[vs], vt_operand);
		}
		else if constexpr (instr == VXOR) {
			vpr[vd] = acc.low = _mm_xor_si128(vpr[vs], vt_operand);
		}
		else if constexpr (instr == VNXOR) {
			vpr[vd] = acc.low = _mm_nxor_si128(vpr[vs], vt_operand);
		}

		AdvancePipeline(1);
	}


	template<VectorInstruction instr>
	void SelectInstr(u32 instr_code)
	{
		using enum VectorInstruction;

		auto vd = instr_code >> 6 & 0x1F;
		auto vs = instr_code >> 11 & 0x1F;
		auto vt = instr_code >> 16 & 0x1F;
		auto element = instr_code >> 21 & 0xF;

		if constexpr (log_rsp_instructions) {
			current_instr_log_output = std::format("{} {} {} {} e{}",
				current_instr_name, vd, vs, vt, element);
		}

		/* Determine which lanes (0-7) of vpr[vt] to access */
		__m128i vt_operand = GetVTBroadcast(vt, element);

		if constexpr (instr == VLT || instr == VGE || instr == VEQ || instr == VNE) {
			vcc.low = [&] {
				__m128i eq = _mm_cmpeq_epi16(vpr[vs], vt_operand);
				if constexpr (instr == VLT) {
					/* Pseudo-code:
						for i in 0..7
							eql = VS<i>(15..0) == VT<i>(15..0)
							neg = VCO(i + 8) & VCO(i) & eql
							VCC(i) = neg | (VS<i>(15..0) < VT<i>(15..0))
							ACC<i>(15..0) = VCC(i) ? VS<i>(15..0) : VT<i>(15..0)
							VD<i>(15..0) = ACC<i>(15..0)
							VCC(i + 8) = VCO(i + 8) = VCO(i) = 0
						endfor
					*/
					__m128i neg = _mm_and_si128(_mm_and_si128(vco.low, vco.high), eq);
					__m128i lt = _mm_cmplt_epi16(vpr[vs], vt_operand);
					return _mm_or_si128(neg, lt);
				}
				else if constexpr (instr == VGE) {
					/* Pseudo-code;
						for i in 0..7
							eql = VS<i>(15..0) == VT<i>(15..0)
							neg = !(VCO(i + 8) & VCO(i)) & eql
							VCC(i) = neg | (VS<i>(15..0) > VT<i>(15..0))
							ACC<i>(15..0) = VCC(i) ? VS<i>(15..0) : VT<i>(15..0)
							VD<i>(15..0) = ACC<i>(15..0)
							VCC(i + 8) = VCO(i + 8) = VCO(i) = 0
						endfor
					*/
					__m128i neg = _mm_and_si128(_mm_nand_si128(vco.low, vco.high), eq);
					__m128i gt = _mm_cmpgt_epi16(vpr[vs], vt_operand);
					return _mm_or_si128(neg, gt);
				}
				else if constexpr (instr == VEQ) {
					/* Pseudo-code:
						for i in 0..7
							VCC(i) = !VCO(i + 8) & (VS<i>(15..0) == VT<i>(15..0))
							ACC<i>(15..0) = VCC(i) ? VS<i>(15..0) : VT<i>(15..0)
							VD<i>(15..0) = ACC<i>(15..0)
							VCC(i + 8) = VCO(i + 8) = VCO(i) = 0
						endfor
					*/
					return _mm_and_si128(_mm_not_si128(vco.high), eq);
				}
				else if constexpr (instr == VNE) {
					/* Pseudo-code:
						for i in 0..7
							VCC(i) = VCO(i + 8) | (VS<i>(15..0) != VT<i>(15..0))
							ACC<i>(15..0) = VCC(i) ? VS<i>(15..0) : VT<i>(15..0)
							VD<i>(15..0) = ACC<i>(15..0)
							VCC(i + 8) = VCO(i + 8) = VCO(i) = 0
						endfor
					*/
					return _mm_or_si128(vco.high, _mm_cmpneq_epi16(vpr[vs], vt_operand));
				}
				else {
					static_assert(AlwaysFalse<instr>);
				}
			}();
			vpr[vd] = acc.low = _mm_blendv_epi8(vt_operand, vpr[vs], vcc.low); /* epi8 blending works because each 16-bit lane in vcc is either 0 or $FFFF */
			std::memset(&vco, 0, sizeof(vco));
			std::memset(&vcc.high, 0, sizeof(vcc.high));
		}
		else if constexpr (instr == VCR) {
			__m128i sign = _mm_srai_epi16(_mm_xor_si128(vpr[vs], vt_operand), 15);
			__m128i dlez = _mm_add_epi16(_mm_and_si128(vpr[vs], sign), vt_operand);
			vcc.low = _mm_srai_epi16(dlez, 15);
			__m128i dgez = _mm_min_epi16(_mm_or_si128(vpr[vs], sign), vt_operand);
			vcc.high = _mm_cmpeq_epi16(dgez, vt_operand);
			__m128i nvt = _mm_xor_si128(vt_operand, sign);
			__m128i mask = _mm_blendv_epi8(vcc.high, vcc.low, sign);
			acc.low = _mm_blendv_epi8(vpr[vs], nvt, mask);
			vpr[vd] = acc.low;
			vco.low = vco.high = vce.low = vce.high = m128i_zero;
		}
		else if constexpr (instr == VCH) {
			__m128i neg_vt = _mm_neg_epi16(vt_operand);
			__m128i signs_different = _mm_cmplt_epi16(_mm_xor_si128(vpr[vs], vt_operand), m128i_zero);
			vcc.high = _mm_blendv_epi8(vcc.high, m128i_all_ones, _mm_cmplt_epi16(vt_operand, m128i_zero));
			vco.low = signs_different;
			__m128i vt_abs = _mm_blendv_epi8(vt_operand, neg_vt, signs_different);
			vce.low = _mm_and_si128(vco.low, _mm_cmpeq_epi16(vpr[vs], _mm_sub_epi16(neg_vt, m128i_one)));
			vco.high = _mm_not_si128(_mm_or_si128(vce.low, _mm_cmpeq_epi16(vpr[vs], vt_abs)));
			vcc.low = _mm_cmple_epi16(vpr[vs], neg_vt);
			vcc.high = _mm_cmpge_epi16(vpr[vs], vt_operand);
			__m128i clip = _mm_blendv_epi8(vcc.high, vcc.low, vco.low);
			vpr[vd] = acc.low = _mm_blendv_epi8(vpr[vs], vt_abs, clip);
		}
		else if constexpr (instr == VCL) {
			/* Pseudo-code:
				for i in 0..7
					if !VCO(i) & !VCO(i + 8)
						VCC(i + 8) = VS<i>(15..0) >= VT<i>(15..0)
					endif
					if VCO(i) & !VCO(i + 8)
						lte = VS<i>(15..0) <= -VT<i>(15..0)
						eql = VS<i>(15..0) == -VT<i>(15..0)
						VCC(i) = VCE(i) ? lte : eql
					endif
					clip = VCO(i) ? VCC(i) : VCC(i + 8)
					vt_abs(15..0) = VCO(i) ? -VT<i>(15..0) : VT<i>(15..0)
					ACC<i>(15..0) = clip ? vt_abs(15..0) : VS<i>(15..0)
					VD<i>(15..0) = ACC<i>(15..0)
				endfor
				Note: all comparisons are unsigned
			*/
			vcc.high = _mm_blendv_epi8(
				_mm_cmpge_epu16(vpr[vs], vt_operand),
				vcc.high,
				_mm_or_si128(vco.low, vco.high));
			__m128i neg_vt = _mm_neg_epi16(vt_operand);
			__m128i le = _mm_cmple_epu16(vpr[vs], neg_vt);
			__m128i eq = _mm_cmpeq_epi16(vpr[vs], neg_vt);
			vcc.low = _mm_blendv_epi8(
				vcc.low,
				_mm_blendv_epi8(eq, le, vce.low),
				_mm_and_si128(vco.low, _mm_not_si128(vco.high)));
			__m128i clip = _mm_blendv_epi8(vcc.high, vcc.low, vco.low);
			__m128i vt_abs = _mm_blendv_epi8(vt_operand, neg_vt, vco.low);
			vpr[vd] = acc.low = _mm_blendv_epi8(vpr[vs], vt_abs, clip);
		}
		else if constexpr (instr == VMRG) {
			/* Pseudo-code:
				for i in 0..7
					ACC<i>(15..0) = VCC(i) ? VS<i>(15..0) : VT<i>(15..0)
					VD<i>(15..0) = ACC<i>(15..0)
				endfor
			*/
			vpr[vd] = acc.low = _mm_blendv_epi8(vt_operand, vpr[vs], vcc.low);
		}
		else {
			static_assert(AlwaysFalse<instr>);
		}

		AdvancePipeline(1);
	}


	template void VectorLoadStore<VectorInstruction::LBV>(u32);
	template void VectorLoadStore<VectorInstruction::LSV>(u32);
	template void VectorLoadStore<VectorInstruction::LLV>(u32);
	template void VectorLoadStore<VectorInstruction::LDV>(u32);
	template void VectorLoadStore<VectorInstruction::LQV>(u32);
	template void VectorLoadStore<VectorInstruction::LRV>(u32);
	template void VectorLoadStore<VectorInstruction::LPV>(u32);
	template void VectorLoadStore<VectorInstruction::LUV>(u32);
	template void VectorLoadStore<VectorInstruction::LTV>(u32);
	template void VectorLoadStore<VectorInstruction::LHV>(u32);
	template void VectorLoadStore<VectorInstruction::LFV>(u32);
	template void VectorLoadStore<VectorInstruction::LWV>(u32);

	template void VectorLoadStore<VectorInstruction::SBV>(u32);
	template void VectorLoadStore<VectorInstruction::SSV>(u32);
	template void VectorLoadStore<VectorInstruction::SLV>(u32);
	template void VectorLoadStore<VectorInstruction::SDV>(u32);
	template void VectorLoadStore<VectorInstruction::SQV>(u32);
	template void VectorLoadStore<VectorInstruction::SRV>(u32);
	template void VectorLoadStore<VectorInstruction::SPV>(u32);
	template void VectorLoadStore<VectorInstruction::SUV>(u32);
	template void VectorLoadStore<VectorInstruction::STV>(u32);
	template void VectorLoadStore<VectorInstruction::SHV>(u32);
	template void VectorLoadStore<VectorInstruction::SFV>(u32);
	template void VectorLoadStore<VectorInstruction::SWV>(u32);

	template void Move<VectorInstruction::MFC2>(u32);
	template void Move<VectorInstruction::MTC2>(u32);
	template void Move<VectorInstruction::CFC2>(u32);
	template void Move<VectorInstruction::CTC2>(u32);

	template void SingleLaneInstr<VectorInstruction::VMOV>(u32);
	template void SingleLaneInstr<VectorInstruction::VRCP>(u32);
	template void SingleLaneInstr<VectorInstruction::VRSQ>(u32);
	template void SingleLaneInstr<VectorInstruction::VRCPH>(u32);
	template void SingleLaneInstr<VectorInstruction::VRSQH>(u32);
	template void SingleLaneInstr<VectorInstruction::VRCPL>(u32);
	template void SingleLaneInstr<VectorInstruction::VRSQL>(u32);
	template void SingleLaneInstr<VectorInstruction::VRNDN>(u32);
	template void SingleLaneInstr<VectorInstruction::VRNDP>(u32);
	template void SingleLaneInstr<VectorInstruction::VNOP>(u32);
	template void SingleLaneInstr<VectorInstruction::VNULL>(u32);

	template void ComputeInstr<VectorInstruction::VMULF>(u32);
	template void ComputeInstr<VectorInstruction::VMULU>(u32);
	template void ComputeInstr<VectorInstruction::VMULQ>(u32);
	template void ComputeInstr<VectorInstruction::VMUDL>(u32);
	template void ComputeInstr<VectorInstruction::VMUDM>(u32);
	template void ComputeInstr<VectorInstruction::VMUDN>(u32);
	template void ComputeInstr<VectorInstruction::VMUDH>(u32);
	template void ComputeInstr<VectorInstruction::VMACF>(u32);
	template void ComputeInstr<VectorInstruction::VMACU>(u32);
	template void ComputeInstr<VectorInstruction::VMACQ>(u32);
	template void ComputeInstr<VectorInstruction::VMADL>(u32);
	template void ComputeInstr<VectorInstruction::VMADM>(u32);
	template void ComputeInstr<VectorInstruction::VADMN>(u32);
	template void ComputeInstr<VectorInstruction::VADMH>(u32);
	template void ComputeInstr<VectorInstruction::VADD>(u32);
	template void ComputeInstr<VectorInstruction::VABS>(u32);
	template void ComputeInstr<VectorInstruction::VADDC>(u32);
	template void ComputeInstr<VectorInstruction::VSUB>(u32);
	template void ComputeInstr<VectorInstruction::VSUBC>(u32);
	template void ComputeInstr<VectorInstruction::VMADN>(u32);
	template void ComputeInstr<VectorInstruction::VMADH>(u32);
	template void ComputeInstr<VectorInstruction::VSAR>(u32);
	template void ComputeInstr<VectorInstruction::VAND>(u32);
	template void ComputeInstr<VectorInstruction::VNAND>(u32);
	template void ComputeInstr<VectorInstruction::VOR>(u32);
	template void ComputeInstr<VectorInstruction::VNOR>(u32);
	template void ComputeInstr<VectorInstruction::VXOR>(u32);
	template void ComputeInstr<VectorInstruction::VNXOR>(u32);

	template void SelectInstr<VectorInstruction::VLT>(u32);
	template void SelectInstr<VectorInstruction::VEQ>(u32);
	template void SelectInstr<VectorInstruction::VNE>(u32);
	template void SelectInstr<VectorInstruction::VGE>(u32);
	template void SelectInstr<VectorInstruction::VCH>(u32);
	template void SelectInstr<VectorInstruction::VCR>(u32);
	template void SelectInstr<VectorInstruction::VCL>(u32);
	template void SelectInstr<VectorInstruction::VMRG>(u32);
}