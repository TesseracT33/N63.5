#ifdef _WIN64
#include <windows.h>
#else
#include <sys/mman.h>
#endif

module VR4300:Recompiler;

import :COP0;
import :COP1;
import :COP2;
import :CPU;
import :MMU;
import :Operation;

import BuildOptions;
import UserMessage;

namespace VR4300::Recompiler
{
	void Block::Execute() const
	{
		auto fun_ptr = (void(*)())buffer;
		fun_ptr();
	}


	bool AllocateBuffer()
	{
#ifdef _WIN64
		/* prepare the memory in which the machine code will be put (it's not executable yet): */
		buffer = (u8*)VirtualAlloc(nullptr, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (!buffer) {
			std::cerr << "VirtualAlloc failed with error code " << GetLastError() << '\n';
			return false;
		}
		/* mark the memory as executable: */
		DWORD old_protect;
		if (!VirtualProtect(buffer, buffer_size, PAGE_EXECUTE_READWRITE, &old_protect)) {
			std::cerr << "VirtualProtect failed with error code " << GetLastError() << '\n';
			return false;
		}
#else
		buffer = (u8*)mmap(nullptr, buffer_size, PROT_READ | PROT_WRITE | PROT_EXEC,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (buffer == MAP_FAILED) {
			std::cerr << "mmap returned MAP_FAILED\n";
			return false;
		}
#endif
		buffer_allocated = true;
		buffer_end = buffer + buffer_size;
		return true;
	}


	void BreakupBlock()
	{
		if (current_block_buffer_pos == 0) { /* starting instruction in blocking currently being created not found yet */
			return;
		}
		ret();
		buffer_pos += current_block_buffer_pos;
		if (buffer_pos >= buffer_end) [[unlikely]] {
			UserMessage::Fatal("Out of memory for recompiler");
			std::cerr << "Out of memory for recompiler\n";
			std::exit(1);
		}
		current_block->cycle_len = current_block_cycle_counter;
		current_block->end_virtual_pc = pc;
		blocks.insert({ current_block_physical_start_pc, std::move(current_block) });
	}


	bool Initialize()
	{
		if (!buffer_allocated && !AllocateBuffer()) {
			return false;
		}
		std::memset(buffer, 0, buffer_size);
		buffer_pos = buffer;
		blocks.clear();
		current_block.reset();
	}


	void MakeBlock(u32 physical_start_pc)
	{
		current_block = std::make_unique<Block>();
		current_block->buffer = buffer_pos;
		current_block_buffer_pos = 0;
		current_block_cycle_counter = 0;
		current_block_physical_start_pc = physical_start_pc;
	}


	u64 Run(u64 cpu_cycles_to_run)
	{
		p_cycle_counter = 0;
		while (p_cycle_counter < cpu_cycles_to_run) {
			u32 physical_pc = GetPhysicalPC();
			if (auto block_it = blocks.find(physical_pc); block_it != blocks.end()) {
				block_it->second->Execute();
				pc = block_it->second->end_virtual_pc;
				p_cycle_counter += block_it->second->cycle_len;
			}
			else {
				if (!current_block) {
					MakeBlock(physical_pc);
				}
				while (current_block_buffer_pos < target_block_size) {
					FetchDecodeExecuteInstruction();
				}
				BreakupBlock();
			}
		}
		return p_cycle_counter - cpu_cycles_to_run;
	}


	bool Terminate()
	{
#ifdef _WIN64
		if (buffer && !VirtualFree(buffer, buffer_size, MEM_RELEASE)) {
			std::cerr << "VirtualFree failed with error code " << GetLastError() << '\n';
			return false;
		}
#else
		if (buffer & !munmap(buffer, buffer_size)) {
			std::cerr << "munmap failed\n";
			return false;
		}
#endif
		return true;
	}


	void emit(u8 byte)
	{
		current_block->buffer[current_block_buffer_pos++] = byte;
	}


	void call(const auto* fun_ptr)
	{
		if constexpr (Host::is_x64) {
			emit(0xFF);
		}
	}


	void ret()
	{
		if constexpr (Host::is_x64) {
			/* TODO */
		}
	}
}