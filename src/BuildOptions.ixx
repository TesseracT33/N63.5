export module BuildOptions;

export
{
	constexpr bool enable_logging = false;

	constexpr bool log_cpu_instructions = enable_logging && false;
	constexpr bool log_cpu_exceptions   = enable_logging && true;
	constexpr bool log_dma              = enable_logging && true;
	constexpr bool log_io_all           = enable_logging && true;
	constexpr bool log_io_ai            = enable_logging && (log_io_all || true);
	constexpr bool log_io_mi            = enable_logging && (log_io_all || true);
	constexpr bool log_io_pi            = enable_logging && (log_io_all || true);
	constexpr bool log_io_rdram         = enable_logging && (log_io_all || true);
	constexpr bool log_io_ri            = enable_logging && (log_io_all || true);
	constexpr bool log_io_si            = enable_logging && (log_io_all || true);
	constexpr bool log_io_vi            = enable_logging && (log_io_all || true);
	constexpr bool log_io_rdp           = enable_logging && (log_io_all || true);
	constexpr bool log_io_rsp           = enable_logging && (log_io_all || true);
	constexpr bool log_rsp_instructions = enable_logging && false;

	constexpr bool skip_boot_rom = true;

	constexpr bool interpret_cpu = false;
	constexpr bool recompile_cpu = !interpret_cpu;
}