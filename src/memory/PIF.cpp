module PIF;

import UserMessage;

namespace PIF
{
	void ChallengeProtection()
	{

	}


	void ChecksumVerification()
	{
		memory[command_byte_index] |= 0x80;
	}


	void ClearRam()
	{
		std::fill(memory.begin() + ram_start, memory.end(), 0);
	}


	size_t GetNumberOfBytesUntilMemoryEnd(u32 offset)
	{
		return memory_size - (offset & (memory_size - 1)); /* Size is 0x800 bytes */
	}


	size_t GetNumberOfBytesUntilRamStart(u32 offset)
	{
		offset &= memory_size - 1;
		return offset < ram_start ? ram_start - offset : 0;
	}


	u8* GetPointerToMemory(u32 address)
	{
		return memory.data() + (address & (memory_size - 1));
	}


	bool LoadIPL12(const std::filesystem::path& path)
	{
		std::optional<std::array<u8, memory_size>> optional_rom = ReadFileIntoArray<memory_size>(path);
		if (!optional_rom.has_value()) {
			UserMessage::Warning("Failed to open boot rom (IPL) file.");
			return false;
		}
		memory = optional_rom.value();
		return true;
	}


	template<bool press>
	void OnButtonAction(N64::Control control)
	{
		auto OnShoulderOrStartChanged = [] {
			if constexpr (press) {
				if (joypad_status.l && joypad_status.r && joypad_status.s) {
					joypad_status.rst = 1;
					joypad_status.s = joypad_status.x_axis = joypad_status.y_axis = 0;
				}
			}
			else {
				joypad_status.rst = 0;
			}
		};
		switch (control) {
		case N64::Control::A: joypad_status.a = press; break;
		case N64::Control::B: joypad_status.b = press; break;
		case N64::Control::CUp: joypad_status.cU = press; break;
		case N64::Control::CDown: joypad_status.cD = press; break;
		case N64::Control::CLeft: joypad_status.cL = press; break;
		case N64::Control::CRight: joypad_status.cR = press; break;
		case N64::Control::DUp: joypad_status.dU = press; break;
		case N64::Control::DDown: joypad_status.dD = press; break;
		case N64::Control::DLeft: joypad_status.dL = press; break;
		case N64::Control::DRight: joypad_status.dR = press; break;
		case N64::Control::ShoulderL: joypad_status.l = press; OnShoulderOrStartChanged(); break;
		case N64::Control::ShoulderR: joypad_status.r = press; OnShoulderOrStartChanged(); break;
		case N64::Control::Start: joypad_status.s = press; OnShoulderOrStartChanged(); break;
		case N64::Control::Z: joypad_status.z = press; break;
		default: std::unreachable();
		}
	}


	void OnJoystickMovement(N64::Control control, s16 value)
	{
		u8 adjusted_value = u8(value >> 8);
		if (control == N64::Control::JX) {
			joypad_status.x_axis = adjusted_value;
		}
		else if (control == N64::Control::JY) {
			joypad_status.y_axis = adjusted_value;
		}
		else {
			std::unreachable();
		}
	}


	template<std::signed_integral Int>
	Int ReadMemory(u32 addr)
	{ /* CPU precondition: addr is aligned */
		Int ret;
		std::memcpy(&ret, memory.data() + (addr & 0x7FF), sizeof(Int));
		return std::byteswap(ret);
	}


	void RomLockout()
	{

	}


	void RunJoybusProtocol()
	{
		switch (memory[ram_start]) { /* joybus command */
		case 0x00:/* Info */
		case 0xFF: /* Reset & Info */
			/* Device: controller */
			memory[ram_start] = 0x05;
			memory[ram_start + 1] = 0x00;
			/* Pak installed */
			memory[ram_start + 2] = 0x01;
			break;

		case 0x01: /* Controller State */
			std::memcpy(&memory[ram_start], &joypad_status, sizeof(joypad_status));
			break;

		case 0x02: /* Read Controller Accessory */
			break;

		case 0x03: /* Write Controller Accessory */
			break;

		case 0x04: /* Read EEPROM */
			break;

		case 0x05: /* Write EEPROM */
			break;

		case 0x06: /* Real-Time Clock Info */
			std::memset(&memory[ram_start], 0, 3); /* clock does not exist */
			break;
		}
	}


	void TerminateBootProcess()
	{

	}


	template<size_t access_size>
	void WriteMemory(u32 addr, s64 data)
	{ /* CPU precondition: write does not go to the next boundary */
		addr &= 0x7FF;
		if (addr < ram_start) return;
		s32 to_write = [&] {
			if constexpr (access_size == 1) return data << (8 * (3 - (addr & 3)));
			if constexpr (access_size == 2) return data << (8 * (2 - (addr & 2)));
			if constexpr (access_size == 4) return data;
			if constexpr (access_size == 8) return data >> 32; /* TODO: not confirmed; could cause console lock-up? */
		}();
		to_write = std::byteswap(to_write);
		addr &= ~3;
		std::memcpy(&memory[addr], &to_write, 4);
		
		if (addr == command_byte_index - 3) {
			if (memory[command_byte_index] & 1) {
				RunJoybusProtocol();
				memory[command_byte_index] &= ~1;
			}
			if (memory[command_byte_index] & 2) {
				ChallengeProtection();
				memory[command_byte_index] &= ~2;
			}
			if (memory[command_byte_index] & 8) {
				TerminateBootProcess();
				memory[command_byte_index] &= ~8;
			}
			if (memory[command_byte_index] & 0x10) {
				RomLockout();
				memory[command_byte_index] &= ~0x10;
			}
			if (memory[command_byte_index] & 0x20) {
				ChecksumVerification();
				memory[command_byte_index] &= ~0x20;
			}
			if (memory[command_byte_index] & 0x40) {
				ClearRam();
				memory[command_byte_index] &= ~0x40;
			}
		}
	}

	template void OnButtonAction<true>(N64::Control);
	template void OnButtonAction<false>(N64::Control);
	template s8 ReadMemory<s8>(u32);
	template s16 ReadMemory<s16>(u32);
	template s32 ReadMemory<s32>(u32);
	template s64 ReadMemory<s64>(u32);
	template void WriteMemory<1>(u32, s64);
	template void WriteMemory<2>(u32, s64);
	template void WriteMemory<4>(u32, s64);
	template void WriteMemory<8>(u32, s64);
}