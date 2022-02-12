export module MemoryAccess;

export namespace MemoryAccess
{
	enum class Operation {
		Read,
		Write,
		InstrFetch
	};

	enum class Alignment {
		Aligned,
		UnalignedLeft, /* Load/Store (Double)Word Left instructions */
		UnalignedRight /* Load/Store (Double)Word Right instructions */
	};
}