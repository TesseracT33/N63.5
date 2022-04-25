export module DebugOptions;

export
{
	constexpr inline bool logging_is_enabled = false;

	constexpr inline bool log_cpu_instructions = logging_is_enabled && true;
	constexpr inline bool log_cpu_memory       = logging_is_enabled && true;
	constexpr inline bool log_cpu_exceptions   = logging_is_enabled && true;
	constexpr inline bool log_dma              = logging_is_enabled && true;
	constexpr inline bool log_rsp_instructions = logging_is_enabled && true;

	constexpr inline bool skip_boot_rom = true;

	enum class MemoryLoggingMode { All, OnlyIO };
	constexpr inline MemoryLoggingMode cpu_memory_logging_mode = MemoryLoggingMode::All;
}