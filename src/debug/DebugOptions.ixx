export module DebugOptions;

export
{
	constexpr bool logging_is_enabled = false;

	constexpr bool log_cpu_instructions = logging_is_enabled && false;
	constexpr bool log_cpu_exceptions   = logging_is_enabled && true;
	constexpr bool log_dma              = logging_is_enabled && true;
	constexpr bool log_io_all           = logging_is_enabled && true;
	constexpr bool log_io_ai            = logging_is_enabled && (log_io_all || true);
	constexpr bool log_io_mi            = logging_is_enabled && (log_io_all || true);
	constexpr bool log_io_pi            = logging_is_enabled && (log_io_all || true);
	constexpr bool log_io_rdram         = logging_is_enabled && (log_io_all || true);
	constexpr bool log_io_ri            = logging_is_enabled && (log_io_all || true);
	constexpr bool log_io_si            = logging_is_enabled && (log_io_all || true);
	constexpr bool log_io_vi            = logging_is_enabled && (log_io_all || true);
	constexpr bool log_io_rdp           = logging_is_enabled && (log_io_all || true);
	constexpr bool log_io_rsp           = logging_is_enabled && (log_io_all || true);
	constexpr bool log_rsp_instructions = logging_is_enabled && false;

	constexpr bool skip_boot_rom = true;
}