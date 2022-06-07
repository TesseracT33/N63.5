module RSP:VectorUnit;

import :RSPOperation;
import :ScalarUnit;

import Host;
import SSEUtils;
import Utils;

#define vco control_reg[0]
#define vcc control_reg[1]
#define vce control_reg[2]

namespace RSP
{
	const __m128i m128i_all_ones = _mm_set1_epi16(0xFFFF);
	const __m128i m128i_all_zeroes = _mm_set1_epi16(0);


	void AddToAccumulator(const __m128i low, const __m128i mid, const __m128i high)
	{
		/* pseudo-code:
			for i in 0..7
				accumulator<i>(47..0) += high<i> << 32 | mid<i> << 16 | low<i>
			endfor
		*/
		const __m128i prev_acc_low = accumulator.low;
		accumulator.low = _mm_add_epi16(accumulator.low, low);
		const __m128i low_carry = _mm_srli_epi16(_mm_cmplt_epu16(accumulator.low, prev_acc_low), 15);
		__m128i prev_acc_mid = accumulator.mid;
		accumulator.mid = _mm_add_epi16(accumulator.mid, mid);
		__m128i mid_carry = _mm_cmplt_epu16(accumulator.mid, prev_acc_mid);
		prev_acc_mid = accumulator.mid;
		accumulator.mid = _mm_add_epi16(accumulator.mid, low_carry);
		mid_carry = _mm_or_si128(mid_carry, _mm_cmplt_epu16(accumulator.mid, prev_acc_mid));
		mid_carry = _mm_srli_epi16(mid_carry, 15);
		accumulator.high = _mm_add_epi16(accumulator.high, high);
		accumulator.high = _mm_add_epi16(accumulator.high, mid_carry);
	}


	void AddToAccumulator(const __m128i low)
	{
		/* pseudo-code:
			for i in 0..7
				accumulator<i>(47..0) += low<i>
			endfor
		*/
		const __m128i prev_acc_low = accumulator.low;
		accumulator.low = _mm_add_epi16(accumulator.low, low);
		const __m128i low_carry = _mm_srli_epi16(_mm_cmplt_epu16(accumulator.low, prev_acc_low), 15);
		const __m128i prev_acc_mid = accumulator.mid;
		accumulator.mid = _mm_add_epi16(accumulator.mid, low_carry);
		const __m128i mid_carry = _mm_srli_epi16(_mm_cmplt_epu16(accumulator.mid, prev_acc_mid), 15);
		accumulator.high = _mm_add_epi16(accumulator.high, mid_carry);
	}


	void AddToAccumulatorFromMid(const __m128i mid, const __m128i high)
	{
		/* pseudo-code:
			for i in 0..7
				accumulator<i>(47..0) += high<i> << 32 | mid<i> << 16
			endfor
		*/
		const __m128i prev_acc_mid = accumulator.mid;
		accumulator.mid = _mm_add_epi16(accumulator.mid, mid);
		const __m128i mid_carry = _mm_srli_epi16(_mm_cmplt_epu16(accumulator.mid, prev_acc_mid), 15);
		accumulator.high = _mm_add_epi16(accumulator.high, high);
		accumulator.high = _mm_add_epi16(accumulator.high, mid_carry);
	}


	__m128i ClampSigned(const __m128i low, const __m128i high)
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
		const __m128i words1 = _mm_unpacklo_epi16(low, high);
		const __m128i words2 = _mm_unpackhi_epi16(low, high);
		return _mm_packs_epi32(words1, words2);
	}


	__m128i ClampUnsigned(const __m128i low, const __m128i high)
	{
		/* pseudo-code:
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
		const __m128i words1 = _mm_unpacklo_epi16(low, high);
		const __m128i words2 = _mm_unpackhi_epi16(low, high);
		return _mm_packus_epi32(words1, words2);
	}


	__m128i GetVTBroadcast(const int vt, const int element)
	{
		/* Determine which lanes (0-7) of vpr[vt] to access */
		auto CombineFourLanes = [&] <int mask>
		{
			static const __m128i x = _mm_set_epi64x(s64(-1), 0);
			static const __m128i y = _mm_set_epi64x(0, s64(-1));
			const __m128i bottom_half = _mm_and_si128(_mm_shufflelo_epi16(vpr[vt], mask), x);
			const __m128i top_half = _mm_and_si128(_mm_shufflehi_epi16(vpr[vt], mask), y);
			return _mm_or_si128(bottom_half, top_half);
		};

		switch (element)
		{
		case 0:
		case 1: /* 0,1,2,3,4,5,6,7 */
			return vpr[vt];
		case 2: /* 0,0,2,2,4,4,6,6 */
			return CombineFourLanes.template operator() < 0b1010'0000 > ();
		case 3: /* 1,1,3,3,5,5,7,7 */
			return CombineFourLanes.template operator() < 0b1111'0101 > ();
		case 4: /* 0,0,0,0,4,4,4,4 */
		case 5: /* 1,1,1,1,5,5,5,5 */
		case 6: /* 2,2,2,2,6,6,6,6 */
		case 7: /* 3,3,3,3,7,7,7,7 */
		{
			const u16* src = (u16*)(vpr.data() + vt) + element;
			u16 bottom_lane, top_lane;
			std::memcpy(&bottom_lane, src - 4, 2);
			std::memcpy(&top_lane, src, 2);
			return _mm_set_epi16(bottom_lane, bottom_lane, bottom_lane, bottom_lane, top_lane, top_lane, top_lane, top_lane);
		}
		default: /* 8x (element - 8) */
		{
			const u16 lane = *((u16*)(vpr.data() + vt) + element - 8);
			return _mm_set1_epi16(lane);
		}
		}
	}


	template<VectorInstruction instr>
	void VectorLoad(const u32 instr_code)
	{
		const signed offset = (instr_code & 0x1F) - 0x20 * bool(instr_code & 0x20); /* two's complement 6-bit number */
		const auto element = instr_code >> 7 & 0xF;
		const auto op_code = instr_code >> 11 & 0x1F;
		const auto vt = instr_code >> 16 & 0x1F;
		const auto base = instr_code >> 21 & 0x1F;

		auto ScalarLoad = [&] <std::integral Int>
		{
			u8* const vpr_dest = (u8*)(vpr.data() + vt) + element;
			const auto address = gpr[base] + offset * sizeof(Int);
			if constexpr (sizeof(Int) == 1)
			{
				*vpr_dest = dmem[address & 0xFFF];
			}
			else
			{
				const u32 num_bytes = std::min((u32)sizeof(Int), 16 - element);
				for (u32 i = 0; i < num_bytes; ++i)
					*(vpr_dest + i) = dmem[(address + i) & 0xFFF];
			}
		};

		using enum VectorInstruction;

		if constexpr (instr == LBV) ScalarLoad.template operator() < s8 > ();
		else if constexpr (instr == LSV) ScalarLoad.template operator() < s16 > ();
		else if constexpr (instr == LLV) ScalarLoad.template operator() < s32 > ();
		else if constexpr (instr == LDV) ScalarLoad.template operator() < s64 > ();
		else if constexpr (instr == LQV)
		{
			u8* const vpr_dest = (u8*)(vpr.data() + vt) + element;
			const u32 address = (gpr[base] + offset * 16) & 0xFFF;
			const u32 num_bytes = 16 - element;
			std::memcpy(vpr_dest, dmem.data() + address, num_bytes);
		}
		else if constexpr (instr == LRV)
		{
			u8* const vpr_dest = (u8*)(vpr.data() + vt) + element;
			const u32 address = gpr[base] + offset * 16;
			const u32 num_bytes = std::min(address & 0xF, 16 - element);
			std::memcpy(vpr_dest, dmem.data() + (address & 0xFF0), num_bytes);
		}
		else if constexpr (instr == LTV)
		{
			auto address = gpr[base] + offset * 16;
			auto reg_index = vt & 0x18;
			auto elem = element >> 1;
			const auto num_bytes_until_wrap = 16 - (address & 7);
			for (int i = 0; i < num_bytes_until_wrap; ++i)
			{
				std::memcpy((s16*)(vpr.data() + reg_index) + (elem & 7), dmem.data() + (address & 0xFFF), 2);
				++elem;
				++reg_index;
				address += 2;
			}
		}
		else if constexpr (instr == LPV || instr == LUV)
		{
			/* 'element' selects the first lane, not byte, being accessed */
			s16* const vpr_dest = (s16*)(vpr.data() + vt);
			const auto address = gpr[base] + offset * 8;
			auto address_dword_offset = address;
			auto elem = element;
			for (int i = 0; i < 8; ++i)
			{
				*(vpr_dest + (elem & 7)) = dmem[address & 0xFF8 | address_dword_offset & 7] << [&] {
					if constexpr (instr == LPV) return 8;
					else return 7;
				}();
				++elem;
				++address_dword_offset;
			}
		}
		else static_assert(instr != instr, "Invalid VectorInstruction given to function template \"ScalarLoadStore\"");
	}


	template<VectorInstruction instr>
	void VectorStore(const u32 instr_code)
	{
		const signed offset = (instr_code & 0x1F) - 0x20 * bool(instr_code & 0x20); /* two's complement 6-bit number */
		const auto element = instr_code >> 7 & 0xF;
		const auto op_code = instr_code >> 11 & 0x1F;
		const auto vt = instr_code >> 16 & 0x1F;
		const auto base = instr_code >> 21 & 0x1F;

		auto ScalarStore = [&] <std::integral Int>
		{
			const u8* const vpr_src = (u8*)(vpr.data() + vt);
			const auto address = gpr[base] + offset * sizeof(Int);
			if constexpr (sizeof(Int) == 1)
			{
				dmem[address & 0xFFF] = *(vpr_src + element);
			}
			else
			{
				auto elem = element;
				for (std::size_t i = 0; i < sizeof(Int); ++i)
				{
					dmem[(address + i) & 0xFFF] = *(vpr_src + elem);
					elem = (elem + 1) & 0xF;
				}
			}
		};

		using enum VectorInstruction;

		if constexpr (instr == SBV) ScalarStore.template operator() < s8 > ();
		else if constexpr (instr == SSV) ScalarStore.template operator() < s16 > ();
		else if constexpr (instr == SLV) ScalarStore.template operator() < s32 > ();
		else if constexpr (instr == SDV) ScalarStore.template operator() < s64 > ();
		else if constexpr (instr == SQV || instr == SRV)
		{
			const u8* const vpr_src = (u8*)(vpr.data() + vt);
			u32 address = gpr[base] + offset * 16;
			auto elem = element;
			const std::size_t num_bytes = [&] {
				if constexpr (instr == SQV) return 16 - (address & 0xF);
				else return address & 0xF;
			}();
			for (std::size_t i = 0; i < 16 - (address & 0xF); ++i)
			{
				static constexpr auto address_mask = [&] {
					if constexpr (instr == SQV) return 0xFFF;
					else return 0xFF0;
				}();
				dmem[(address & address_mask) + i] = *(vpr_src + elem);
				elem = (elem + 1) & 0xF;
			}
		}
		else if constexpr (instr == SRV)
		{
			const u8* const vpr_src = (u8*)(vpr.data() + vt);
			const u32 address = gpr[base] + offset * 16;
			auto elem = element;
			for (std::size_t i = 0; i < (address & 0xF); ++i)
			{
				dmem[(address & 0xFF0) + i] = *(vpr_src + elem);
				elem = (elem + 1) & 0xF;
			}
		}
		else if constexpr (instr == STV)
		{
			/* TODO */
		}
		else if constexpr (instr == SPV || instr == SUV)
		{
			/* 'element' selects the first lane, not byte, being accessed */
			s16* const vpr_src = (s16*)(vpr.data() + vt);
			const auto address = (gpr[base] + offset * 8) & 0xFFF;
			auto address_dword_offset = address & 7;
			auto elem = element & 7;
			const auto shift_count = 7 + ((instr == SPV) ^ (element >= 8));
			for (int i = 0; i < 8; ++i)
			{
				dmem[address & 0xFF8 | address_dword_offset] = *(vpr_src + elem) >> shift_count & 0xFF;
				elem = (elem + 1) & 7;
				address_dword_offset = (address_dword_offset + 1) & 7;
			}
		}
		else
		{
			static_assert(instr != instr);
		}
	}


	template<VectorInstruction instr>
	void Move(const u32 instr_code)
	{
		const auto vs_elem = instr_code >> 8 & 0x7;
		const auto vs = instr_code >> 11 & 0x1F;
		const auto rt = instr_code >> 16 & 0x1F;

		using enum VectorInstruction;

		if constexpr (instr == MTC2)
		{
			/* Pseudo-code:
				VS<elem> = GPR<rt>
			*/
			_mm_setlane_epi16(&vpr[vs], vs_elem, s16(gpr[rt]));
		}
		else if constexpr (instr == MFC2)
		{
			gpr.Set(rt, _mm_getlane_epi16(&vpr[vs], vs_elem));
		}
		else if constexpr (instr == CTC2)
		{
			assert(vs < 3);
			static constexpr std::array<s64, 16> lanes = {
				0x0000'0000'0000'0000,
				0x0000'0000'0000'FFFF,
				0x0000'0000'FFFF'0000,
				0x0000'0000'FFFF'FFFF,
				0x0000'FFFF'0000'0000,
				0x0000'FFFF'0000'FFFF,
				0x0000'FFFF'FFFF'0000,
				0x0000'FFFF'FFFF'FFFF,
				0xFFFF'0000'0000'0000,
				0xFFFF'0000'0000'FFFF,
				0xFFFF'0000'FFFF'0000,
				0xFFFF'0000'FFFF'FFFF,
				0xFFFF'FFFF'0000'0000,
				0xFFFF'FFFF'0000'FFFF,
				0xFFFF'FFFF'FFFF'0000,
				0xFFFF'FFFF'FFFF'FFFF
			};
			control_reg[vs].low = _mm_set_epi64x(lanes[gpr[rt] >> 4 & 0xF], lanes[gpr[rt] & 0xF]);
			control_reg[vs].high = _mm_set_epi64x(lanes[gpr[rt] >> 12 & 0xF], lanes[gpr[rt] >> 8 & 0xF]);
		}
		else if constexpr (instr == CFC2)
		{
			assert(vs < 3);
			const int low = _mm_movemask_epi8(_mm_packs_epi16(control_reg[vs].low, m128i_all_zeroes));
			const int high = _mm_movemask_epi8(_mm_packs_epi16(control_reg[vs].high, m128i_all_zeroes));
			gpr.Set(rt, s16(high << 8 | low));
		}
		else
		{
			static_assert(instr != instr);
		}
	}


	template<VectorInstruction instr>
	void SingleLaneInstr(const u32 instr_code)
	{
		const auto vd = instr_code >> 6 & 0x1F;
		const auto vd_elem = instr_code >> 11 & 0x07;
		const auto vt = instr_code >> 16 & 0x1F;
		const auto vt_elem = instr_code >> 21 & 0x07;

		auto Rcp = [&](const s32 src)
		{
			static constexpr std::array<u16, 512> rcp_rom =
			{
				0xFFFF, 0xFF00, 0xFE01, 0xFD04, 0xFC07, 0xFB0C, 0xFA11, 0xF918, 0xF81F, 0xF727, 0xF631, 0xF53B, 0xF446, 0xF352, 0xF25F, 0xF16D,
				0xF07C, 0xEF8B, 0xEE9C, 0xEDAE, 0xECC0, 0xEBD3, 0xEAE8, 0xE9FD, 0xE913, 0xE829, 0xE741, 0xE65A, 0xE573, 0xE48D, 0xE3A9, 0xE2C5,
				0xE1E1, 0xE0FF, 0xE01E, 0xDF3D, 0xDE5D, 0xDD7E, 0xDCA0, 0xDBC2, 0xDAE6, 0xDA0A, 0xD92F, 0xD854, 0xD77B, 0xD6A2, 0xD5CA, 0xD4F3,
				0xD41D, 0xD347, 0xD272, 0xD19E, 0xD0CB, 0xCFF8, 0xCF26, 0xCE55, 0xCD85, 0xCCB5, 0xCBE6, 0xCB18, 0xCA4B, 0xC97E, 0xC8B2, 0xC7E7,
				0xC71C, 0xC652, 0xC589, 0xC4C0, 0xC3F8, 0xC331, 0xC26B, 0xC1A5, 0xC0E0, 0xC01C, 0xBF58, 0xBE95, 0xBDD2, 0xBD10, 0xBC4F, 0xBB8F,
				0xBACF, 0xBA10, 0xB951, 0xB894, 0xB7D6, 0xB71A, 0xB65E, 0xB5A2, 0xB4E8, 0xB42E, 0xB374, 0xB2BB, 0xB203, 0xB14B, 0xB094, 0xAFDE,
				0xAF28, 0xAE73, 0xADBE, 0xAD0A, 0xAC57, 0xABA4, 0xAAF1, 0xAA40, 0xA98E, 0xA8DE, 0xA82E, 0xA77E, 0xA6D0, 0xA621, 0xA574, 0xA4C6,
				0xA41A, 0xA36E, 0xA2C2, 0xA217, 0xA16D, 0xA0C3, 0xA01A, 0x9F71, 0x9EC8, 0x9E21, 0x9D79, 0x9CD3, 0x9C2D, 0x9B87, 0x9AE2, 0x9A3D,
				0x9999, 0x98F6, 0x9852, 0x97B0, 0x970E, 0x966C, 0x95CB, 0x952B, 0x948B, 0x93EB, 0x934C, 0x92AD, 0x920F, 0x9172, 0x90D4, 0x9038,
				0x8F9C, 0x8F00, 0x8E65, 0x8DCA, 0x8D30, 0x8C96, 0x8BFC, 0x8B64, 0x8ACB, 0x8A33, 0x899C, 0x8904, 0x886E, 0x87D8, 0x8742, 0x86AD,
				0x8618, 0x8583, 0x84F0, 0x845C, 0x83C9, 0x8336, 0x82A4, 0x8212, 0x8181, 0x80F0, 0x8060, 0x7FD0, 0x7F40, 0x7EB1, 0x7E22, 0x7D93,
				0x7D05, 0x7C78, 0x7BEB, 0x7B5E, 0x7AD2, 0x7A46, 0x79BA, 0x792F, 0x78A4, 0x781A, 0x7790, 0x7706, 0x767D, 0x75F5, 0x756C, 0x74E4,
				0x745D, 0x73D5, 0x734F, 0x72C8, 0x7242, 0x71BC, 0x7137, 0x70B2, 0x702E, 0x6FA9, 0x6F26, 0x6EA2, 0x6E1F, 0x6D9C, 0x6D1A, 0x6C98,
				0x6C16, 0x6B95, 0x6B14, 0x6A94, 0x6A13, 0x6993, 0x6914, 0x6895, 0x6816, 0x6798, 0x6719, 0x669C, 0x661E, 0x65A1, 0x6524, 0x64A8,
				0x642C, 0x63B0, 0x6335, 0x62BA, 0x623F, 0x61C5, 0x614B, 0x60D1, 0x6058, 0x5FDF, 0x5F66, 0x5EED, 0x5E75, 0x5DFD, 0x5D86, 0x5D0F,
				0x5C98, 0x5C22, 0x5BAB, 0x5B35, 0x5AC0, 0x5A4B, 0x59D6, 0x5961, 0x58ED, 0x5879, 0x5805, 0x5791, 0x571E, 0x56AC, 0x5639, 0x55C7,
				0x5555, 0x54E3, 0x5472, 0x5401, 0x5390, 0x5320, 0x52AF, 0x5240, 0x51D0, 0x5161, 0x50F2, 0x5083, 0x5015, 0x4FA6, 0x4F38, 0x4ECB,
				0x4E5E, 0x4DF1, 0x4D84, 0x4D17, 0x4CAB, 0x4C3F, 0x4BD3, 0x4B68, 0x4AFD, 0x4A92, 0x4A27, 0x49BD, 0x4953, 0x48E9, 0x4880, 0x4817,
				0x47AE, 0x4745, 0x46DC, 0x4674, 0x460C, 0x45A5, 0x453D, 0x44D6, 0x446F, 0x4408, 0x43A2, 0x433C, 0x42D6, 0x4270, 0x420B, 0x41A6,
				0x4141, 0x40DC, 0x4078, 0x4014, 0x3FB0, 0x3F4C, 0x3EE8, 0x3E85, 0x3E22, 0x3DC0, 0x3D5D, 0x3CFB, 0x3C99, 0x3C37, 0x3BD6, 0x3B74,
				0x3B13, 0x3AB2, 0x3A52, 0x39F1, 0x3991, 0x3931, 0x38D2, 0x3872, 0x3813, 0x37B4, 0x3755, 0x36F7, 0x3698, 0x363A, 0x35DC, 0x357F,
				0x3521, 0x34C4, 0x3467, 0x340A, 0x33AE, 0x3351, 0x32F5, 0x3299, 0x323E, 0x31E2, 0x3187, 0x312C, 0x30D1, 0x3076, 0x301C, 0x2FC2,
				0x2F68, 0x2F0E, 0x2EB4, 0x2E5B, 0x2E02, 0x2DA9, 0x2D50, 0x2CF8, 0x2C9F, 0x2C47, 0x2BEF, 0x2B97, 0x2B40, 0x2AE8, 0x2A91, 0x2A3A,
				0x29E4, 0x298D, 0x2937, 0x28E0, 0x288B, 0x2835, 0x27DF, 0x278A, 0x2735, 0x26E0, 0x268B, 0x2636, 0x25E2, 0x258D, 0x2539, 0x24E5,
				0x2492, 0x243E, 0x23EB, 0x2398, 0x2345, 0x22F2, 0x22A0, 0x224D, 0x21FB, 0x21A9, 0x2157, 0x2105, 0x20B4, 0x2063, 0x2012, 0x1FC1,
				0x1F70, 0x1F1F, 0x1ECF, 0x1E7F, 0x1E2E, 0x1DDF, 0x1D8F, 0x1D3F, 0x1CF0, 0x1CA1, 0x1C52, 0x1C03, 0x1BB4, 0x1B66, 0x1B17, 0x1AC9,
				0x1A7B, 0x1A2D, 0x19E0, 0x1992, 0x1945, 0x18F8, 0x18AB, 0x185E, 0x1811, 0x17C4, 0x1778, 0x172C, 0x16E0, 0x1694, 0x1648, 0x15FD,
				0x15B1, 0x1566, 0x151B, 0x14D0, 0x1485, 0x143B, 0x13F0, 0x13A6, 0x135C, 0x1312, 0x12C8, 0x127F, 0x1235, 0x11EC, 0x11A3, 0x1159,
				0x1111, 0x10C8, 0x107F, 0x1037, 0x0FEF, 0x0FA6, 0x0F5E, 0x0F17, 0x0ECF, 0x0E87, 0x0E40, 0x0DF9, 0x0DB2, 0x0D6B, 0x0D24, 0x0CDD,
				0x0C97, 0x0C50, 0x0C0A, 0x0BC4, 0x0B7E, 0x0B38, 0x0AF2, 0x0AAD, 0x0A68, 0x0A22, 0x09DD, 0x0998, 0x0953, 0x090F, 0x08CA, 0x0886,
				0x0842, 0x07FD, 0x07B9, 0x0776, 0x0732, 0x06EE, 0x06AB, 0x0668, 0x0624, 0x05E1, 0x059E, 0x055C, 0x0519, 0x04D6, 0x0494, 0x0452,
				0x0410, 0x03CE, 0x038C, 0x034A, 0x0309, 0x02C7, 0x0286, 0x0245, 0x0204, 0x01C3, 0x0182, 0x0141, 0x0101, 0x00C0, 0x0080, 0x0040
			};

			if (src == 0)
				return u32(-1);
			const u32 src_abs = std::abs(src);
			const auto scale_out = std::bit_width(src_abs);
			const auto scale_in = 32 - scale_out;
			const auto result = (0x10000 | rcp_rom[src_abs >> (scale_in - 9) & 0x1FF]) << (scale_out - 16);
			return u32(src < 0 ? ~result : result);
		};

		auto Rsq = [&](const s32 src)
		{
			static constexpr std::array<u16, 512> rsq_rom =
			{
				0xFFFF, 0xFF00, 0xFE02, 0xFD06, 0xFC0B, 0xFB12, 0xFA1A, 0xF923, 0xF82E, 0xF73B, 0xF648, 0xF557, 0xF467, 0xF379, 0xF28C, 0xF1A0,
				0xF0B6, 0xEFCD, 0xEEE5, 0xEDFF, 0xED19, 0xEC35, 0xEB52, 0xEA71, 0xE990, 0xE8B1, 0xE7D3, 0xE6F6, 0xE61B, 0xE540, 0xE467, 0xE38E,
				0xE2B7, 0xE1E1, 0xE10D, 0xE039, 0xDF66, 0xDE94, 0xDDC4, 0xDCF4, 0xDC26, 0xDB59, 0xDA8C, 0xD9C1, 0xD8F7, 0xD82D, 0xD765, 0xD69E,
				0xD5D7, 0xD512, 0xD44E, 0xD38A, 0xD2C8, 0xD206, 0xD146, 0xD086, 0xCFC7, 0xCF0A, 0xCE4D, 0xCD91, 0xCCD6, 0xCC1B, 0xCB62, 0xCAA9,
				0xC9F2, 0xC93B, 0xC885, 0xC7D0, 0xC71C, 0xC669, 0xC5B6, 0xC504, 0xC453, 0xC3A3, 0xC2F4, 0xC245, 0xC198, 0xC0EB, 0xC03F, 0xBF93,
				0xBEE9, 0xBE3F, 0xBD96, 0xBCED, 0xBC46, 0xBB9F, 0xBAF8, 0xBA53, 0xB9AE, 0xB90A, 0xB867, 0xB7C5, 0xB723, 0xB681, 0xB5E1, 0xB541,
				0xB4A2, 0xB404, 0xB366, 0xB2C9, 0xB22C, 0xB191, 0xB0F5, 0xB05B, 0xAFC1, 0xAF28, 0xAE8F, 0xADF7, 0xAD60, 0xACC9, 0xAC33, 0xAB9E,
				0xAB09, 0xAA75, 0xA9E1, 0xA94E, 0xA8BC, 0xA82A, 0xA799, 0xA708, 0xA678, 0xA5E8, 0xA559, 0xA4CB, 0xA43D, 0xA3B0, 0xA323, 0xA297,
				0xA20B, 0xA180, 0xA0F6, 0xA06C, 0x9FE2, 0x9F59, 0x9ED1, 0x9E49, 0x9DC2, 0x9D3B, 0x9CB4, 0x9C2F, 0x9BA9, 0x9B25, 0x9AA0, 0x9A1C,
				0x9999, 0x9916, 0x9894, 0x9812, 0x9791, 0x9710, 0x968F, 0x960F, 0x9590, 0x9511, 0x9492, 0x9414, 0x9397, 0x931A, 0x929D, 0x9221,
				0x91A5, 0x9129, 0x90AF, 0x9034, 0x8FBA, 0x8F40, 0x8EC7, 0x8E4F, 0x8DD6, 0x8D5E, 0x8CE7, 0x8C70, 0x8BF9, 0x8B83, 0x8B0D, 0x8A98,
				0x8A23, 0x89AE, 0x893A, 0x88C6, 0x8853, 0x87E0, 0x876D, 0x86FB, 0x8689, 0x8618, 0x85A7, 0x8536, 0x84C6, 0x8456, 0x83E7, 0x8377,
				0x8309, 0x829A, 0x822C, 0x81BF, 0x8151, 0x80E4, 0x8078, 0x800C, 0x7FA0, 0x7F34, 0x7EC9, 0x7E5E, 0x7DF4, 0x7D8A, 0x7D20, 0x7CB6,
				0x7C4D, 0x7BE5, 0x7B7C, 0x7B14, 0x7AAC, 0x7A45, 0x79DE, 0x7977, 0x7911, 0x78AB, 0x7845, 0x77DF, 0x777A, 0x7715, 0x76B1, 0x764D,
				0x75E9, 0x7585, 0x7522, 0x74BF, 0x745D, 0x73FA, 0x7398, 0x7337, 0x72D5, 0x7274, 0x7213, 0x71B3, 0x7152, 0x70F2, 0x7093, 0x7033,
				0x6FD4, 0x6F76, 0x6F17, 0x6EB9, 0x6E5B, 0x6DFD, 0x6DA0, 0x6D43, 0x6CE6, 0x6C8A, 0x6C2D, 0x6BD1, 0x6B76, 0x6B1A, 0x6ABF, 0x6A64,
				0x6A09, 0x6955, 0x68A1, 0x67EF, 0x673E, 0x668D, 0x65DE, 0x6530, 0x6482, 0x63D6, 0x632B, 0x6280, 0x61D7, 0x612E, 0x6087, 0x5FE0,
				0x5F3A, 0x5E95, 0x5DF1, 0x5D4E, 0x5CAC, 0x5C0B, 0x5B6B, 0x5ACB, 0x5A2C, 0x598F, 0x58F2, 0x5855, 0x57BA, 0x5720, 0x5686, 0x55ED,
				0x5555, 0x54BE, 0x5427, 0x5391, 0x52FC, 0x5268, 0x51D5, 0x5142, 0x50B0, 0x501F, 0x4F8E, 0x4EFE, 0x4E6F, 0x4DE1, 0x4D53, 0x4CC6,
				0x4C3A, 0x4BAF, 0x4B24, 0x4A9A, 0x4A10, 0x4987, 0x48FF, 0x4878, 0x47F1, 0x476B, 0x46E5, 0x4660, 0x45DC, 0x4558, 0x44D5, 0x4453,
				0x43D1, 0x434F, 0x42CF, 0x424F, 0x41CF, 0x4151, 0x40D2, 0x4055, 0x3FD8, 0x3F5B, 0x3EDF, 0x3E64, 0x3DE9, 0x3D6E, 0x3CF5, 0x3C7C,
				0x3C03, 0x3B8B, 0x3B13, 0x3A9C, 0x3A26, 0x39B0, 0x393A, 0x38C5, 0x3851, 0x37DD, 0x3769, 0x36F6, 0x3684, 0x3612, 0x35A0, 0x352F,
				0x34BF, 0x344F, 0x33DF, 0x3370, 0x3302, 0x3293, 0x3226, 0x31B9, 0x314C, 0x30DF, 0x3074, 0x3008, 0x2F9D, 0x2F33, 0x2EC8, 0x2E5F,
				0x2DF6, 0x2D8D, 0x2D24, 0x2CBC, 0x2C55, 0x2BEE, 0x2B87, 0x2B21, 0x2ABB, 0x2A55, 0x29F0, 0x298B, 0x2927, 0x28C3, 0x2860, 0x27FD,
				0x279A, 0x2738, 0x26D6, 0x2674, 0x2613, 0x25B2, 0x2552, 0x24F2, 0x2492, 0x2432, 0x23D3, 0x2375, 0x2317, 0x22B9, 0x225B, 0x21FE,
				0x21A1, 0x2145, 0x20E8, 0x208D, 0x2031, 0x1FD6, 0x1F7B, 0x1F21, 0x1EC7, 0x1E6D, 0x1E13, 0x1DBA, 0x1D61, 0x1D09, 0x1CB1, 0x1C59,
				0x1C01, 0x1BAA, 0x1B53, 0x1AFC, 0x1AA6, 0x1A50, 0x19FA, 0x19A5, 0x1950, 0x18FB, 0x18A7, 0x1853, 0x17FF, 0x17AB, 0x1758, 0x1705,
				0x16B2, 0x1660, 0x160D, 0x15BC, 0x156A, 0x1519, 0x14C8, 0x1477, 0x1426, 0x13D6, 0x1386, 0x1337, 0x12E7, 0x1298, 0x1249, 0x11FB,
				0x11AC, 0x115E, 0x1111, 0x10C3, 0x1076, 0x1029, 0x0FDC, 0x0F8F, 0x0F43, 0x0EF7, 0x0EAB, 0x0E60, 0x0E15, 0x0DCA, 0x0D7F, 0x0D34,
				0x0CEA, 0x0CA0, 0x0C56, 0x0C0C, 0x0BC3, 0x0B7A, 0x0B31, 0x0AE8, 0x0AA0, 0x0A58, 0x0A10, 0x09C8, 0x0981, 0x0939, 0x08F2, 0x08AB,
				0x0865, 0x081E, 0x07D8, 0x0792, 0x074D, 0x0707, 0x06C2, 0x067D, 0x0638, 0x05F3, 0x05AF, 0x056A, 0x0526, 0x04E2, 0x049F, 0x045B,
				0x0418, 0x03D5, 0x0392, 0x0350, 0x030D, 0x02CB, 0x0289, 0x0247, 0x0206, 0x01C4, 0x0183, 0x0142, 0x0101, 0x00C0, 0x0080, 0x0040
			};

			if (src == 0)
				return u32(-1);
			const u32 src_abs = std::abs(src);
			u32 scale_out = std::bit_width(src_abs);
			const auto scale_in = 32 - scale_out;
			scale_out >>= 1;
			const auto result = (0x10000 | rsq_rom[(scale_in & 1) << 8 | src_abs >> (scale_in - 8) & 0xFF]) << (scale_out - 16);
			return u32(src < 0 ? ~result : result);
		};

		using enum VectorInstruction;

		if constexpr (instr == VMOV)
		{
			_mm_setlane_epi16(&vpr[vd], vd_elem, _mm_getlane_epi16(&vpr[vt], vt_elem));
		}
		else if constexpr (instr == VRCP)
		{
			s32 src = _mm_getlane_epi16(&vpr[vt], vt_elem);
			const u32 result = Rcp(src);
			_mm_setlane_epi16(&vpr[vd], vd_elem, s16(result));
			div_out = s16(result >> 16);
			accumulator.low = vpr[vt];
		}
		else if constexpr (instr == VRSQ)
		{
			s32 src = _mm_getlane_epi16(&vpr[vt], vt_elem);
			const u32 result = Rsq(src);
			_mm_setlane_epi16(&vpr[vd], vd_elem, s16(result));
			div_out = s16(result >> 16);
		}
		else if constexpr (instr == VRCPH || instr == VRSQH)
		{
			_mm_setlane_epi16(&vpr[vd], vd_elem, div_out);
			div_in = _mm_getlane_epi16(&vpr[vt], vt_elem);
			accumulator.low = vpr[vt]; // TODO accumulator loaded on VRSQH?
		}
		else if constexpr (instr == VRCPL || instr == VRSQL)
		{
			const s32 src = div_in << 16 | _mm_getlane_epi16(&vpr[vt], vt_elem);
			const u32 result = [&] {
				if constexpr (instr == VRCPL) return Rcp(src);
				else return Rsq(src);
			}();
			_mm_setlane_epi16(&vpr[vd], vd_elem, s16(result));
			div_out = s16(result >> 16);
			div_in = 0;
			accumulator.low = vpr[vt]; // TODO: accumulator loaded on VRSQL?
		}
		else if constexpr (instr == VRNDN)
		{
			/* TODO */
		}
		else if constexpr (instr == VRNDP)
		{
			/* TODO */
		}
		else if constexpr (instr == VNOP)
		{
			/* TODO */
		}
		else if constexpr (instr == VNULL)
		{
			/* TODO */
		}
		else
		{
			static_assert(instr != instr);
		}
	}


	template<VectorInstruction instr>
	void ComputeInstr(const u32 instr_code)
	{
		const auto vd = instr_code >> 6 & 0x1F;
		const auto vs = instr_code >> 11 & 0x1F;
		const auto vt = instr_code >> 16 & 0x1F;
		const auto element = instr_code >> 21 & 0xF;

		/* Determine which lanes (0-7) of vpr[vt] to access */
		const __m128i vt_operand = GetVTBroadcast(vt, element);

		using enum VectorInstruction;

		static const __m128i epi16_all_lanes_1 = _mm_set1_epi16(1);

		if constexpr (instr == VADD)
		{
			/* Pseudo-code:
				for i in 0..7
					result(16..0) = VS<i>(15..0) + VT<i>(15..0) + VCO(i)
					ACC<i>(15..0) = result(15..0)
					VD<i>(15..0) = clamp_signed(result(16..0))
					VCO(i) = 0
					VCO(i + 8) = 0
				endfor
			*/
			const __m128i vc0_operand = _mm_and_si128(vco.low, epi16_all_lanes_1);
			accumulator.low = _mm_add_epi16(vpr[vs], _mm_add_epi16(vt_operand, vc0_operand));
			vpr[vd] = _mm_adds_epi16(vpr[vs], _mm_adds_epi16(vt_operand, vc0_operand));
			std::memset(&vco, 0, sizeof(vco));
		}
		else if constexpr (instr == VSUB)
		{
			/* Pseudo-code:
				for i in 0..7
					result(16..0) = VS<i>(15..0) - VT<i>(15..0) - VCO(i)
					ACC<i>(15..0) = result(15..0)
					VD<i>(15..0) = clamp_signed(result(16..0))
					VCO(i) = 0
					VCO(i + 8) = 0
				endfor
			*/
			const __m128i vc0_operand = _mm_and_si128(vco.low, epi16_all_lanes_1);
			accumulator.low = _mm_sub_epi16(vpr[vs], _mm_add_epi16(vt_operand, vc0_operand));
			vpr[vd] = _mm_subs_epi16(vpr[vs], _mm_adds_epi16(vt_operand, vc0_operand));
			std::memset(&vco, 0, sizeof(vco));
		}
		else if constexpr (instr == VADDC)
		{
			/* Pseudo-code:
				for i in 0..7
					result(16..0) = VS<i>(15..0) + VT<i>(15..0)
					ACC<i>(15..0) = result(15..0)
					VD<i>(15..0) = result(15..0)
					VCO(i) = result(16)
					VCO(i + 8) = 0
				endfor
			*/
			vpr[vd] = accumulator.low = _mm_add_epi16(vpr[vs], vt_operand);
			vco.low = _mm_cmplt_epu16(_mm_sub_epi16(m128i_all_ones, vpr[vs]), vt_operand); /* check carry */
			std::memset(&vco.high, 0, sizeof(vco.high));
		}
		else if constexpr (instr == VSUBC)
		{
			/* Pseudo-code:
				for i in 0..7
					result(16..0) = VS<i>(15..0) - VT<i>(15..0)
					ACC<i>(15..0) = result(15..0)
					VD<i>(15..0) = result(15..0)
					VCO(i) = result(16)
					VCO(i + 8) = result(16..0) != 0
				endfor
			*/
			vpr[vd] = accumulator.low = _mm_sub_epi16(vpr[vs], vt_operand);
			vco.low = _mm_cmplt_epu16(vpr[vs], vt_operand); /* check borrow */
			vco.high = _mm_or_si128(vco.low, _mm_cmpneq_epi16(vpr[vd], m128i_all_zeroes));
		}
		else if constexpr (instr == VMULF || instr == VMULU)
		{
			/* Pseudo-code:
				for i in 0..7
					prod(32..0) = VS<i>(15..0) * VT<i>(15..0) * 2   // signed multiplication
					ACC<i>(47..0) = sign_extend(prod(32..0) + 0x8000)
					VD<i>(15..0) = clamp_signed(ACC<i>(47..16))
				endfor
			*/
			static const __m128i carry_mask = _mm_set1_epi16(0x8000);
			static const __m128i m128i_all_lanes_1_epi16 = _mm_set1_epi16(1);
			__m128i low = _mm_mullo_epi16(vpr[vs], vt_operand);
			__m128i high = _mm_mulhi_epi16(vpr[vs], vt_operand);
			__m128i carry = _mm_cmpeq_epi16(_mm_and_si128(high, carry_mask), carry_mask);
			low = _mm_slli_epi16(low, 1);
			high = _mm_slli_epi16(high, 1);
			// add $8000
			low = _mm_add_epi16(low, carry_mask);
			const __m128i low_carry = _mm_srli_epi16(_mm_cmpgt_epi16(low, m128i_all_ones), 15); // carry if low >= 0
			high = _mm_add_epi16(high, low_carry);
			const __m128i high_carry = _mm_and_si128(_mm_cmpeq_epi16(high, m128i_all_zeroes), _mm_cmpeq_epi16(low_carry, m128i_all_lanes_1_epi16));
			carry = _mm_add_epi16(carry, high_carry);
			accumulator.low = low;
			accumulator.mid = high;
			accumulator.high = carry;
			// set vd
			vpr[vd] = [&] {
				if constexpr (instr == VMULF) return ClampSigned(accumulator.mid, accumulator.high);
				else return ClampUnsigned(accumulator.mid, accumulator.high);
			}();
		}
		else if constexpr (instr == VMULQ)
		{
			/* TODO */
		}
		else if constexpr (instr == VMACF || instr == VMACU)
		{
			static const __m128i carry_mask = _mm_set1_epi16(0x8000);
			__m128i low = _mm_mullo_epi16(vpr[vs], vt_operand);
			__m128i high = _mm_mulhi_epi16(vpr[vs], vt_operand);
			__m128i carry = _mm_cmpeq_epi16(_mm_and_si128(high, carry_mask), carry_mask);
			low = _mm_slli_epi16(low, 1);
			high = _mm_slli_epi16(high, 1);
			AddToAccumulator(low, high, carry);
			// set vd
			vpr[vd] = [&] {
				if constexpr (instr == VMACF) return ClampSigned(accumulator.mid, accumulator.high);
				else return ClampUnsigned(accumulator.mid, accumulator.high);
			}();
		}
		else if constexpr (instr == VMACQ)
		{
			/* TODO */
		}
		else if constexpr (instr == VMUDN || instr == VMADN)
		{
			const __m128i low = _mm_mullo_epi16(vpr[vs], vt_operand);
			const __m128i high = _mm_mulhi_epu16_epi16(vpr[vs], vt_operand);
			// TODO
		}
		else if constexpr (instr == VMUDL || instr == VMADL)
		{
			const __m128i low = _mm_mullo_epi16(vpr[vs], vt_operand);
			const __m128i high = _mm_mulhi_epu16(vpr[vs], vt_operand);
			AddToAccumulator(high);
			vpr[vd] = ClampUnsigned(accumulator.low, accumulator.mid);
		}
		else if constexpr (instr == VMUDM || instr == VMADM)
		{
			const __m128i low = _mm_mullo_epi16(vpr[vs], vt_operand);
			const __m128i high = _mm_mulhi_epu16_epi16(vpr[vs], vt_operand);
			// TODO
		}
		else if constexpr (instr == VMUDH || instr == VMADH)
		{
			const __m128i low = _mm_mullo_epi16(vpr[vs], vt_operand);
			const __m128i high = _mm_mulhi_epi16(vpr[vs], vt_operand);
			AddToAccumulatorFromMid(low, high);
			vpr[vd] = ClampSigned(accumulator.mid, accumulator.high);
		}
		else if constexpr (instr == VABS)
		{
			vpr[vd] = _mm_abs_epi16(vpr[vs]);
		}
		else if constexpr (instr == VSAR)
		{
			/* Pseudo-code:
				for i in 0..7
					a = 16 * e + 15
					b = 16 * e
					VD<i>(15..0) = ACC<i>(a..b)
				endfor
			*/
			assert(element < 3);
			std::memcpy(vpr.data() + vd, (u8*)&accumulator.low + 16 * element, 16);
		}
		/* Pseudo-code VAND/VNAND/VOR/VNOR/VXOR/VNXOR:
			for i in 0..7
				ACC<i>(15..0) = VS<i>(15..0) <LOGICAL_OP> VT<i>(15..0)
				VD<i>(15..0) = ACC<i>(15..0)
			endfor
		*/
		else if constexpr (instr == VAND)
		{
			vpr[vd] = accumulator.low = _mm_and_si128(vpr[vs], vt_operand);
		}
		else if constexpr (instr == VNAND)
		{
			vpr[vd] = accumulator.low = _mm_nand_si128(vpr[vs], vt_operand);
		}
		else if constexpr (instr == VOR)
		{
			vpr[vd] = accumulator.low = _mm_or_si128(vpr[vs], vt_operand);
		}
		else if constexpr (instr == VNOR)
		{
			vpr[vd] = accumulator.low = _mm_nor_si128(vpr[vs], vt_operand);
		}
		else if constexpr (instr == VXOR)
		{
			vpr[vd] = accumulator.low = _mm_xor_si128(vpr[vs], vt_operand);
		}
		else if constexpr (instr == VNXOR)
		{
			vpr[vd] = accumulator.low = _mm_nxor_si128(vpr[vs], vt_operand);
		}
	}


	template<VectorInstruction instr>
	void SelectInstr(const u32 instr_code)
	{
		const auto vd = instr_code >> 6 & 0x1F;
		const auto vs = instr_code >> 11 & 0x1F;
		const auto vt = instr_code >> 16 & 0x1F;
		const auto element = instr_code >> 21 & 0xF;

		/* Determine which lanes (0-7) of vpr[vt] to access */
		__m128i vt_operand = GetVTBroadcast(vt, element);

		using enum VectorInstruction;

		if constexpr (instr == VLT || instr == VGE || instr == VEQ || instr == VNE)
		{
			vcc.low = [&] {
				const __m128i eq = _mm_cmpeq_epi16(vpr[vs], vt_operand);
				if constexpr (instr == VLT)
				{
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
					const __m128i neg = _mm_and_si128(_mm_and_si128(vco.low, vco.high), eq);
					const __m128i lt = _mm_cmplt_epi16(vpr[vs], vt_operand);
					return _mm_or_si128(neg, lt);
				}
				else if constexpr (instr == VGE)
				{
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
					const __m128i neg = _mm_and_si128(_mm_nand_si128(vco.low, vco.high), eq);
					const __m128i gt = _mm_cmpgt_epi16(vpr[vs], vt_operand);
					return _mm_or_si128(neg, gt);
				}
				else if constexpr (instr == VEQ)
				{
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
				else if constexpr (instr == VNE)
				{
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
				else
				{
					static_assert(instr != instr);
				}
			}();
			vpr[vd] = accumulator.low = _mm_blendv_epi8(vt_operand, vpr[vs], vcc.low); /* epi8 blending works because each 16-bit lane in vcc is either 0 or $FFFF */
			std::memset(&vco, 0, sizeof(vco));
			std::memset(&vcc.high, 0, sizeof(vcc.high));
		}
		else if constexpr (instr == VCH || instr == VCR)
		{
			/* Pseudo-code:
				for i in 0..7
					VCO(i) = VS<i>(15) != VT<i>(15)
					vt_abs(15..0) = VCO(i) ? -VT<i>(15..0) : VT<i>(15..0)
					VCE(i) = VCO(i) & (VS<i>(15..0) == -VT<i>(15..0) - 1)
					VCO(i + 8) = !VCE(i) & (VS<i>(15..0) != vt_abs(15..0))
					VCC(i) = VS<i>(15..0) <= -VT<i>(15..0)
					VCC(i + 8) = VS<i>(15..0) >= VT<i>(15..0)
					clip = VCO(i) ? VCC(i) : VCC(i + 8)
					ACC<i>(15..0) = clip ? vt_abs(15..0) : VS<i>(15..0)
					VD<i>(15..0) = ACC<i>(15..0)
				endfor
				VCR: works in one's complement (inputs, meaning of '-' operator)
			*/
			// TODO: VCR handle case when some number is signed $8000
			static const __m128i sign_mask = _mm_set1_epi16(0x8000);
			static const __m128i epi16_all_lanes_one = _mm_set1_epi16(1);
			/* Convert vpr[vs] and vt_operand to two's complement */
			const __m128i vs_operand = [&] {
				if constexpr (instr == VCH) return vpr[vs];
				else
				{
					__m128i correction = _mm_cmpeq_epi16(_mm_and_si128(vpr[vs], sign_mask), sign_mask);
					correction = _mm_srli_epi16(correction, 15);
					return _mm_add_epi16(vpr[vs], correction);
				}
			}();
			if constexpr (instr == VCR)
			{
				__m128i correction = _mm_cmpeq_epi16(_mm_and_si128(vt_operand, sign_mask), sign_mask);
				correction = _mm_srli_epi16(correction, 15);
				vt_operand = _mm_add_epi16(vt_operand, correction);
			}
			const __m128i neg_vt = [&] {
				if constexpr (instr == VCH) return _mm_sign_epi16(vt_operand, vt_operand);
				else return _mm_not_si128(vt_operand);
			}();
			vco.low = _mm_cmpneq_epi16(_mm_and_si128(vs_operand, sign_mask), _mm_and_si128(vs_operand, sign_mask));
			const __m128i vt_abs = _mm_blendv_epi8(vt_operand, neg_vt, vco.low);
			vce.low = _mm_and_si128(vco.low, _mm_cmpeq_epi16(vs_operand, _mm_sub_epi16(neg_vt, epi16_all_lanes_one)));
			vco.high = _mm_not_si128(_mm_or_si128(vce.low, _mm_cmpeq_epi16(vs_operand, vt_abs)));
			vcc.low = _mm_cmple_epi16(vs_operand, neg_vt);
			vcc.high = _mm_cmpge_epi16(vs_operand, vt_operand);
			const __m128i clip = _mm_blendv_epi8(vcc.high, vcc.low, vco.low);
			vpr[vd] = accumulator.low = _mm_blendv_epi8(vs_operand, vt_abs, clip);
		}
		else if constexpr (instr == VCL)
		{
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
			*/
			vcc.high = _mm_blendv_epi8(_mm_cmpgt_epi16(vpr[vs], vt_operand), vcc.high, _mm_or_si128(vco.low, vco.high));
			const __m128i neg_vt = _mm_sign_epi16(vt_operand, vt_operand);
			const __m128i lt = _mm_cmplt_epi16(vpr[vs], neg_vt);
			const __m128i eq = _mm_cmpeq_epi16(vpr[vs], neg_vt);
			vcc.low = _mm_blendv_epi8(
				vcc.low,
				_mm_blendv_epi8(eq, lt, vce.low),
				_mm_and_si128(vco.low, _mm_not_si128(vco.high)));
			const __m128i clip = _mm_blendv_epi8(vcc.high, vcc.low, vco.low);
			const __m128i vt_abs = _mm_blendv_epi8(vt_operand, neg_vt, vco.low);
			vpr[vd] = accumulator.low = _mm_blendv_epi8(vpr[vs], vt_abs, clip);
		}
		else if constexpr (instr == VMRG)
		{
			/* Pseudo-code:
				for i in 0..7
					ACC<i>(15..0) = VCC(i) ? VS<i>(15..0) : VT<i>(15..0)
					VD<i>(15..0) = ACC<i>(15..0)
				endfor
			*/
			vpr[vd] = accumulator.low = _mm_blendv_epi8(vt_operand, vpr[vs], vcc.low);
		}
		else
		{
			static_assert(instr != instr);
		}
	}


	template void VectorLoad<VectorInstruction::LBV>(u32);
	template void VectorLoad<VectorInstruction::LSV>(u32);
	template void VectorLoad<VectorInstruction::LLV>(u32);
	template void VectorLoad<VectorInstruction::LDV>(u32);
	template void VectorLoad<VectorInstruction::LQV>(u32);
	template void VectorLoad<VectorInstruction::LRV>(u32);
	template void VectorLoad<VectorInstruction::LPV>(u32);
	template void VectorLoad<VectorInstruction::LUV>(u32);
	template void VectorLoad<VectorInstruction::LTV>(u32);

	template void VectorStore<VectorInstruction::SBV>(u32);
	template void VectorStore<VectorInstruction::SSV>(u32);
	template void VectorStore<VectorInstruction::SLV>(u32);
	template void VectorStore<VectorInstruction::SDV>(u32);
	template void VectorStore<VectorInstruction::SQV>(u32);
	template void VectorStore<VectorInstruction::SRV>(u32);
	template void VectorStore<VectorInstruction::SPV>(u32);
	template void VectorStore<VectorInstruction::SUV>(u32);
	template void VectorStore<VectorInstruction::STV>(u32);

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