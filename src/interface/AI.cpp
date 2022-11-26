module AI;

import BuildOptions;
import Logging;
import MI;
import Memory;
import N64;
import Scheduler;
import UserMessage;

namespace AI
{
	void Initialize()
	{
		static constexpr int num_audio_channels = 2;
		static constexpr int sample_buffer_size_per_channel = 512;
		static constexpr int sample_buffer_size = sample_buffer_size_per_channel * num_audio_channels;;
		static constexpr int sample_rate = 44100;

		SDL_AudioSpec desired_spec;
		SDL_zero(desired_spec);
		desired_spec.freq = sample_rate;
		desired_spec.format = AUDIO_S16MSB;
		desired_spec.channels = num_audio_channels;
		desired_spec.samples = sample_buffer_size_per_channel;
		desired_spec.callback = nullptr;

		SDL_AudioSpec obtained_spec;
		audio_device_id = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, &obtained_spec, 0);
		if (audio_device_id == 0) {
			const char* error_msg = SDL_GetError();
			UserMessage::Show(std::format("Could not open audio device; {}", error_msg), UserMessage::Type::Warning);
		}
		SDL_PauseAudioDevice(audio_device_id, 0);

		ai.status = 1 << 20 | 1 << 24;
	}


	s32 ReadReg(u32 addr)
	{
		ai.status = 1 << 20 | 1 << 24;
		ai.status |= (dma_count > 1);
		ai.status |= (dma_count > 0) << 30;
		ai.status |= (dma_count > 1) << 31;

		static_assert(sizeof(ai) >> 2 == 8);
		u32 offset = addr >> 2 & 7;
		s32 ret;
		std::memcpy(&ret, (s32*)(&ai) + offset, 4);
		if constexpr (log_io_ai) {
			LogIoRead("AI", RegOffsetToStr(offset), ret);
		}
		return ret;
	}


	constexpr std::string_view RegOffsetToStr(u32 reg_offset)
	{
		switch (reg_offset) {
		case DramAddr: return "AI_DRAM_ADDR";
		case Len: return "AI_LEN";
		case Control: return "AI_CONTROL";
		case Status: return "AI_STATUS";
		case Dacrate: return "AI_DACRATE";
		case Bitrate: return "AI_BITRATE";
		default: return "UNKNOWN";
		}
	}


	void Sample()
	{
		if (dma_count != 0) {
			s32 data = Memory::Read<s32>(ai.dram_addr);
			// TODO output sample
			ai.dram_addr += 4;
			ai.len -= 4;
			if (ai.len == 0) {
				MI::SetInterruptFlag(MI::InterruptType::AI);
				if (--dma_count > 0) {
					ai.dram_addr = dma_address_buffer;
					ai.len = dma_length_buffer;
				}
			}
		}
		Scheduler::AddEvent(Scheduler::EventType::AudioSample, dac.period, Sample);
	}


	void WriteReg(u32 addr, s32 data)
	{
		static_assert(sizeof(ai) >> 2 == 8);
		u32 offset = addr >> 2 & 7;
		if constexpr (log_io_ai) {
			LogIoWrite("AI", RegOffsetToStr(offset), data);
		}

		switch (offset) {
		case Register::DramAddr:
			if (dma_count < 2) {
				dma_count == 0 ? ai.dram_addr = data & 0xFF'FFF8 : dma_address_buffer = data & 0xFF'FFF8;
			}
			break;

		case Register::Len: {
			s32 length = data & 0x3'FFF8;
			if (dma_count < 2 && length > 0) {
				dma_count == 0 ? ai.len = length : dma_length_buffer = length;
				++dma_count;
			}
			break;
		}

		case Register::Control: {
			auto prev_control = ai.control;
			ai.control = data & 1;
			if (prev_control ^ ai.control) {
				if (ai.control) {
					Scheduler::AddEvent(Scheduler::EventType::AudioSample, dac.period, Sample);
				}
				else {
					Scheduler::RemoveEvent(Scheduler::EventType::AudioSample);
				}
			}
		} break;

		case Register::Status:
			MI::ClearInterruptFlag(MI::InterruptType::AI);
			break;

		case Register::Dacrate:
			ai.dacrate = data & 0x3FFF;
			dac.frequency = std::max(1u, N64::cpu_cycles_per_second / (ai.dacrate + 1));
			dac.period = N64::cpu_cycles_per_second / dac.frequency;
			Scheduler::ChangeEventTime(Scheduler::EventType::AudioSample, dac.period);
			break;

		case Register::Bitrate:
			ai.bitrate = data & 0xF;
			break;

		default:
			Log(std::format("Unexpected write made to AI register at address ${:08X}", addr));
		}
	}
}