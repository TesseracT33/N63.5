#pragma once

import NumericalTypes;

#define ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(FUNCTION, ARG_TYPE) \
	template u8 FUNCTION<u8>(ARG_TYPE); \
	template s8 FUNCTION<s8>(ARG_TYPE); \
	template u16 FUNCTION<u16>(ARG_TYPE); \
	template s16 FUNCTION<s16>(ARG_TYPE); \
	template u32 FUNCTION<u32>(ARG_TYPE); \
	template s32 FUNCTION<s32>(ARG_TYPE); \
	template u64 FUNCTION<u64>(ARG_TYPE); \
	template s64 FUNCTION<s64>(ARG_TYPE);

#define ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(FUNCTION, ARG_TYPE) \
	template void FUNCTION<1>(ARG_TYPE, const u8); \
	template void FUNCTION<1>(ARG_TYPE, const u16); \
	template void FUNCTION<1>(ARG_TYPE, const u32); \
	template void FUNCTION<1>(ARG_TYPE, const u64); \
	template void FUNCTION<2>(ARG_TYPE, const u8); \
	template void FUNCTION<2>(ARG_TYPE, const u16); \
	template void FUNCTION<2>(ARG_TYPE, const u32); \
	template void FUNCTION<2>(ARG_TYPE, const u64); \
	template void FUNCTION<3>(ARG_TYPE, const u8); \
	template void FUNCTION<3>(ARG_TYPE, const u16); \
	template void FUNCTION<3>(ARG_TYPE, const u32); \
	template void FUNCTION<3>(ARG_TYPE, const u64); \
	template void FUNCTION<4>(ARG_TYPE, const u8); \
	template void FUNCTION<4>(ARG_TYPE, const u16); \
	template void FUNCTION<4>(ARG_TYPE, const u32); \
	template void FUNCTION<4>(ARG_TYPE, const u64); \
	template void FUNCTION<5>(ARG_TYPE, const u8); \
	template void FUNCTION<5>(ARG_TYPE, const u16); \
	template void FUNCTION<5>(ARG_TYPE, const u32); \
	template void FUNCTION<5>(ARG_TYPE, const u64); \
	template void FUNCTION<6>(ARG_TYPE, const u8); \
	template void FUNCTION<6>(ARG_TYPE, const u16); \
	template void FUNCTION<6>(ARG_TYPE, const u32); \
	template void FUNCTION<6>(ARG_TYPE, const u64); \
	template void FUNCTION<7>(ARG_TYPE, const u8); \
	template void FUNCTION<7>(ARG_TYPE, const u16); \
	template void FUNCTION<7>(ARG_TYPE, const u32); \
	template void FUNCTION<7>(ARG_TYPE, const u64); \
	template void FUNCTION<8>(ARG_TYPE, const u8); \
	template void FUNCTION<8>(ARG_TYPE, const u16); \
	template void FUNCTION<8>(ARG_TYPE, const u32); \
	template void FUNCTION<8>(ARG_TYPE, const u64); \
	template void FUNCTION<1>(ARG_TYPE, const s8); \
	template void FUNCTION<1>(ARG_TYPE, const s16); \
	template void FUNCTION<1>(ARG_TYPE, const s32); \
	template void FUNCTION<1>(ARG_TYPE, const s64); \
	template void FUNCTION<2>(ARG_TYPE, const s8); \
	template void FUNCTION<2>(ARG_TYPE, const s16); \
	template void FUNCTION<2>(ARG_TYPE, const s32); \
	template void FUNCTION<2>(ARG_TYPE, const s64); \
	template void FUNCTION<3>(ARG_TYPE, const s8); \
	template void FUNCTION<3>(ARG_TYPE, const s16); \
	template void FUNCTION<3>(ARG_TYPE, const s32); \
	template void FUNCTION<3>(ARG_TYPE, const s64); \
	template void FUNCTION<4>(ARG_TYPE, const s8); \
	template void FUNCTION<4>(ARG_TYPE, const s16); \
	template void FUNCTION<4>(ARG_TYPE, const s32); \
	template void FUNCTION<4>(ARG_TYPE, const s64); \
	template void FUNCTION<5>(ARG_TYPE, const s8); \
	template void FUNCTION<5>(ARG_TYPE, const s16); \
	template void FUNCTION<5>(ARG_TYPE, const s32); \
	template void FUNCTION<5>(ARG_TYPE, const s64); \
	template void FUNCTION<6>(ARG_TYPE, const s8); \
	template void FUNCTION<6>(ARG_TYPE, const s16); \
	template void FUNCTION<6>(ARG_TYPE, const s32); \
	template void FUNCTION<6>(ARG_TYPE, const s64); \
	template void FUNCTION<7>(ARG_TYPE, const s8); \
	template void FUNCTION<7>(ARG_TYPE, const s16); \
	template void FUNCTION<7>(ARG_TYPE, const s32); \
	template void FUNCTION<7>(ARG_TYPE, const s64); \
	template void FUNCTION<8>(ARG_TYPE, const s8); \
	template void FUNCTION<8>(ARG_TYPE, const s16); \
	template void FUNCTION<8>(ARG_TYPE, const s32); \
	template void FUNCTION<8>(ARG_TYPE, const s64);