module VR4300:Cache;

import :COP0;
import :CPU;
import :Exceptions;
import :MMU;
import :Operation;

import BuildOptions;
import Logging;
import Memory;
import RDRAM;

namespace VR4300
{
	void CACHE(u32 rs, u32 rt, s16 imm16)
	{
		/* Cache op;
		   Sign-extends the 16-bit offset to 32 bits and adds it to register base to
		   generate a virtual address. The virtual address is converted into a physical
		   address by using the TLB, and a cache operation indicated by a 5-bit sub op
		   code is executed to that address. */
		uint cycles = 1;
		/* The below makes everything crash and burn */
#if 0
		if (!cop0.status.cu0 || operating_mode != OperatingMode::Kernel) {
			SignalException<Exception::CoprocessorUnusable>();
			AdvancePipeline(cycles);
			return;
		}
#endif
		auto cache = rt & 3;
		auto op = rt >> 2;
		auto virt_addr = gpr[rs] + imm16;
		bool cacheable_area;
		auto phys_addr = active_virtual_to_physical_fun_read(virt_addr, cacheable_area); /* may go unused below, but could also cause a TLB exception */
		if (exception_has_occurred) {
			AdvancePipeline(cycles);
			return;
		}

		auto HandleOp = [&](auto& cache_line) {
			static_assert(sizeof(DCacheLine) != sizeof(ICacheLine));
			static constexpr bool is_d_cache = sizeof(cache_line) == sizeof(DCacheLine);

			auto WriteBack = [&] {
				WritebackCacheLine(cache_line, phys_addr);
				cycles = 40;
			};
			/* TODO: in which situations do we abort if the cache line is tagged as invalid? */
			switch (op) { /* illegal 'op' and 'cache' combinations give UB */
			case 0: /* Index_Invalidate */
				if constexpr (is_d_cache) {
					if (cache_line.valid && cache_line.dirty) {
						WriteBack();
					}
				}
				cache_line.valid = false;
				break;

			case 1: /* Index_Load_Tag */
				cop0.tag_lo.ptag = cache_line.ptag >> 12;
				cop0.tag_lo.pstate = cache_line.valid << 1;
				if constexpr (is_d_cache) {
					cop0.tag_lo.pstate |= u32(cache_line.dirty);
				}
				break;

			case 2: /* Index_Store_Tag */
				cache_line.ptag = cop0.tag_lo.ptag << 12;
				cache_line.valid = cop0.tag_lo.pstate >> 1;
				if constexpr (is_d_cache) {
					cache_line.dirty = cop0.tag_lo.pstate & 1;
				}
				break;

			case 3: /* Create_Dirty_Exclusive */
				if constexpr (is_d_cache) {
					if (cache_line.dirty && (phys_addr & ~0xFFF) != cache_line.ptag) {
						WriteBack();
					}
					cache_line.dirty = cache_line.valid = true;
					cache_line.ptag = phys_addr & ~0xFFF;
				}
				break;

			case 4: /* Hit_Invalidate */
				if ((phys_addr & ~0xFFF) == cache_line.ptag) {
					cache_line.valid = false;
					if constexpr (is_d_cache) {
						cache_line.dirty = false;
					}
				}
				break;

			case 5: /* Hit_Write_Back_Invalidate (D-Cache), Fill (I-Cache) */
				if constexpr (is_d_cache) {
					if ((phys_addr & ~0xFFF) == cache_line.ptag) {
						if (cache_line.dirty) {
							WriteBack();
						}
						cache_line.valid = false;
					}
				}
				else {
					FillCacheLine(cache_line, phys_addr);
					cycles = 40;
				}
				break;

			case 6: /* Hit_Write_Back */
				if constexpr (is_d_cache) {
					if (!cache_line.dirty) {
						break;
					}
				}
				if ((phys_addr & ~0xFFF) == cache_line.ptag) {
					WriteBack();
				}
				break;

			case 7:
				break;

			default:
				std::unreachable();
			}
		};

		if (cache == 0) { /* I-Cache */
			ICacheLine& cache_line = i_cache[virt_addr >> 5 & 0x1FF];
			HandleOp(cache_line);
		}
		else if (cache == 1) { /* D-Cache */
			DCacheLine& cache_line = d_cache[virt_addr >> 4 & 0x1FF];
			HandleOp(cache_line);
		}

		AdvancePipeline(cycles);
	}


	void FillCacheLine(auto& cache_line, u32 phys_addr)
	{
		/* TODO: For now, we are lazy and assume that only RDRAM is cached. Other regions are too;
		https://discord.com/channels/465585922579103744/600463718924681232/1034605516900544582 */
		auto rdram_offset = phys_addr & ~(sizeof(cache_line.data) - 1);
		if (rdram_offset >= RDRAM::GetSize()) {
			Log(std::format("Attempted to fill cache line from p_addr {} (beyond RDRAM)", rdram_offset));
		}
		std::memcpy(cache_line.data, rdram_ptr + rdram_offset, sizeof(cache_line.data));
		cache_line.ptag = phys_addr & ~0xFFF;
		cache_line.valid = true;
		if constexpr (sizeof(cache_line) == sizeof(DCacheLine)) {
			cache_line.dirty = false;
		}
	}


	template<std::signed_integral Int, MemOp mem_op>
	Int ReadCacheableArea(u32 phys_addr)
	{ /* Precondition: phys_addr is aligned to sizeof(Int) */
		static_assert(OneOf(mem_op, MemOp::InstrFetch, MemOp::Read));
		if constexpr (log_cpu_instructions && mem_op == MemOp::InstrFetch) {
			last_instr_fetch_phys_addr = phys_addr;
		}
		Int ret;
		if constexpr (mem_op == MemOp::InstrFetch) {
			ICacheLine& cache_line = i_cache[phys_addr >> 5 & 0x1FF];
			if (cache_line.valid && (phys_addr & ~0xFFF) == cache_line.ptag) { /* cache hit */
				p_cycle_counter += cache_hit_read_cycle_delay;
			}
			else { /* cache miss */
				FillCacheLine(cache_line, phys_addr);
				p_cycle_counter += cache_miss_cycle_delay;
			}
			std::memcpy(&ret, cache_line.data + (phys_addr & (sizeof(cache_line.data) - 1)), sizeof(Int));
		}
		else { /* MemOp::Read */
			DCacheLine& cache_line = d_cache[phys_addr >> 4 & 0x1FF];
			if (cache_line.valid && (phys_addr & ~0xFFF) == cache_line.ptag) { /* cache hit */
				p_cycle_counter += cache_hit_read_cycle_delay;
			}
			else { /* cache miss */
				if (cache_line.valid && cache_line.dirty) {
					WritebackCacheLine(cache_line, phys_addr);
				}
				FillCacheLine(cache_line, phys_addr);
				p_cycle_counter += cache_miss_cycle_delay;
			}
			std::memcpy(&ret, cache_line.data + (phys_addr & (sizeof(cache_line.data) - 1)), sizeof(Int));
		}
		return std::byteswap(ret);
	}


	template<size_t access_size, typename... MaskT>
	void WriteCacheableArea(u32 phys_addr, s64 data, MaskT... mask)
	{ /* Precondition: phys_addr is aligned to access_size if sizeof...(mask) == 0 */
		static_assert(std::has_single_bit(access_size) && access_size <= 8);
		static_assert(sizeof...(mask) <= 1);
		DCacheLine& cache_line = d_cache[phys_addr >> 4 & 0x1FF];
		if (cache_line.valid && (phys_addr & ~0xFFF) == cache_line.ptag) { /* cache hit */
			cache_line.dirty = true;
			p_cycle_counter += cache_hit_write_cycle_delay;
		}
		else { /* cache miss */
			if (cache_line.valid && cache_line.dirty) {
				WritebackCacheLine(cache_line, phys_addr);
			}
			FillCacheLine(cache_line, phys_addr);
			p_cycle_counter += cache_miss_cycle_delay;
		}
		auto to_write = [&] {
			if constexpr (access_size == 1) return u8(data);
			if constexpr (access_size == 2) return std::byteswap(u16(data));
			if constexpr (access_size == 4) return std::byteswap(u32(data));
			if constexpr (access_size == 8) return std::byteswap(data);
		}();
		constexpr static bool apply_mask = sizeof...(mask) == 1;
		if constexpr (apply_mask) {
			phys_addr &= ~(access_size - 1);
		}
		u8* cache = cache_line.data + (phys_addr & (sizeof(cache_line.data) - 1));
		if constexpr (apply_mask) {
			u64 existing;
			std::memcpy(&existing, cache, access_size);
			to_write |= existing & (..., mask);
		}
		std::memcpy(cache, &to_write, access_size);
		cache_line.dirty = true;
	}


	void WritebackCacheLine(auto& cache_line, u32 new_phys_addr)
	{
		/* The address in the main memory to be written is the address in the cache tag
			and not the physical address translated by using TLB */
		auto rdram_offset = cache_line.ptag | new_phys_addr & 0xFFF & ~(sizeof(cache_line.data) - 1);
		std::memcpy(rdram_ptr + rdram_offset, cache_line.data, sizeof(cache_line.data));
		if constexpr (sizeof(cache_line) == sizeof(DCacheLine)) {
			cache_line.dirty = false;
		}
	}

	template s32 ReadCacheableArea<s32, MemOp::InstrFetch>(u32);
	template s8 ReadCacheableArea<s8, MemOp::Read>(u32);
	template s16 ReadCacheableArea<s16, MemOp::Read>(u32);
	template s32 ReadCacheableArea<s32, MemOp::Read>(u32);
	template s64 ReadCacheableArea<s64, MemOp::Read>(u32);
	template void WriteCacheableArea<1>(u32, s64);
	template void WriteCacheableArea<2>(u32, s64);
	template void WriteCacheableArea<4>(u32, s64);
	template void WriteCacheableArea<8>(u32, s64);
	template void WriteCacheableArea<4, s64>(u32, s64, s64);
	template void WriteCacheableArea<8, s64>(u32, s64, s64);
}