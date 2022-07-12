export module MemoryAccess;

export namespace MemoryAccess
{
	enum class Alignment {
		Aligned,
		UnalignedLeft, /* Load/Store (Double)Word Left instructions */
		UnalignedRight /* Load/Store (Double)Word Right instructions */
	};

	enum class Operation {
		Read, InstrFetch, Write
	};
}