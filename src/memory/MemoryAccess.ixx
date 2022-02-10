export module MemoryAccess;

export namespace MemoryAccess
{
	enum class Operation { Read, Write, InstrFetch };

	enum class Alignment
	{
		Aligned,
		Unaligned
	};
}