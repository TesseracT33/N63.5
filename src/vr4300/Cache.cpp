module VR4300:Cache;

import :COP0;
import :CPU;
import :Exceptions;
import :MMU;
import :Operation;

import DebugOptions;
import Logging;
import Memory;
import RDRAM;

namespace VR4300
{
	void Cache(u32 instr_code)
	{
		/* Cache op;
		   Sign-extends the 16-bit offset to 32 bits and adds it to register base to
		   generate a virtual address. The virtual address is converted into a physical
		   address by using the TLB, and a cache operation indicated by a 5-bit sub op
		   code is executed to that address. */
		if constexpr (log_cpu_instructions) {
			current_instr_log_output = current_instr_name;
		}
		uint cycles = 1;
		/* The below makes everything crash and burn */
#if 0
		if (!cop0.status.cu0 || operating_mode != OperatingMode::Kernel) {
			SignalException<Exception::CoprocessorUnusable>();
			AdvancePipeline(cycles);
			return;
		}
#endif
		s16 offset = instr_code & 0xFFFF;
		auto cache = instr_code >> 16 & 3;
		auto op = instr_code >> 18 & 7;
		auto base = instr_code >> 21 & 0x1F;
		auto virt_addr = gpr[base] + offset;
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


	template<std::signed_integral Int, Memory::Operation operation>
	Int ReadCacheableArea(u32 phys_addr)
	{ /* Precondition: phys_addr is aligned to sizeof(Int) */
		static_assert(OneOf(operation, Memory::Operation::InstrFetch, Memory::Operation::Read));
		if constexpr (log_cpu_instructions && operation == Memory::Operation::InstrFetch) {
			last_instr_fetch_phys_addr = phys_addr;
		}
		Int ret;
		if constexpr (operation == Memory::Operation::InstrFetch) {
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
		else { /* Memory::Operation::Read */
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


	template<size_t num_bytes>
	void WriteCacheableArea(u32 phys_addr, std::signed_integral auto data)
	{ /* Precondition: phys_addr may not be aligned to sizeof(Int),
		but phys_addr + number_of_bytes is at most the byte of the next boundary of sizeof(Int) */
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
		data = std::byteswap(data);
		std::memcpy(cache_line.data + (phys_addr & (sizeof(cache_line.data) - 1)), &data, num_bytes);
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

	template s32 ReadCacheableArea<s32, Memory::Operation::InstrFetch>(u32);
	template s8 ReadCacheableArea<s8, Memory::Operation::Read>(u32);
	template s16 ReadCacheableArea<s16, Memory::Operation::Read>(u32);
	template s32 ReadCacheableArea<s32, Memory::Operation::Read>(u32);
	template s64 ReadCacheableArea<s64, Memory::Operation::Read>(u32);
	template void WriteCacheableArea<1>(u32, s8);
	template void WriteCacheableArea<1>(u32, s16);
	template void WriteCacheableArea<1>(u32, s32);
	template void WriteCacheableArea<1>(u32, s64);
	template void WriteCacheableArea<2>(u32, s16);
	template void WriteCacheableArea<2>(u32, s32);
	template void WriteCacheableArea<2>(u32, s64);
	template void WriteCacheableArea<3>(u32, s32);
	template void WriteCacheableArea<3>(u32, s64);
	template void WriteCacheableArea<4>(u32, s32);
	template void WriteCacheableArea<4>(u32, s64);
	template void WriteCacheableArea<5>(u32, s64);
	template void WriteCacheableArea<6>(u32, s64);
	template void WriteCacheableArea<7>(u32, s64);
	template void WriteCacheableArea<8>(u32, s64);
}