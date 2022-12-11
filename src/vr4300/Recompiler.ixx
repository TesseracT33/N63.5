export module VR4300:Recompiler;

import Util;

import <bit>;
import <cstdlib>;
import <cstring>;
import <iostream>;
import <iterator>;
import <memory>;
import <unordered_map>;
import <utility>;

namespace VR4300::Recompiler
{
	export
	{
		bool Initialize();
		u64 Run(u64 cpu_cycles_to_run);
		bool Terminate();
	}

	bool AllocateBuffer();
	void BreakupBlock();
	void MakeBlock(u32 physical_start_pc);

	struct Block {
		u8* buffer;
		u64 cycle_len;
		u64 end_virtual_pc;
		void Execute() const;
	};

	constexpr size_t buffer_size = 32 * 1024 * 1024;
	constexpr size_t target_block_size = 256;

	u8* buffer;
	u8* buffer_end;
	u8* buffer_pos;
	bool buffer_allocated;
	u64 current_block_cycle_counter;
	u64 current_block_physical_start_pc;
	size_t current_block_buffer_pos;
	std::unique_ptr<Block> current_block; /* TODO: allocate all memory upfront */
	std::unordered_map<u64, std::unique_ptr<Block>> blocks; /* physical address => instruction block */

	void call(const auto* fun_ptr);
	void ret();
}