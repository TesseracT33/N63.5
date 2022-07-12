export module DebugOptions;

export
{
	enum class MemoryLoggingMode { All, OnlyIO };

	constexpr bool logging_is_enabled = false;

	constexpr bool log_cpu_instructions = logging_is_enabled && true;
	constexpr bool log_cpu_memory       = logging_is_enabled && true;
	constexpr bool log_cpu_exceptions   = logging_is_enabled && true;
	constexpr bool log_dma              = logging_is_enabled && true;
	constexpr bool log_rsp_instructions = logging_is_enabled && true;
	constexpr bool skip_boot_rom = true;

	constexpr MemoryLoggingMode cpu_memory_logging_mode = MemoryLoggingMode::All;
}