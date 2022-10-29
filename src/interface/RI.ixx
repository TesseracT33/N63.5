export module RI; /* RDRAM Interface */

import Util;

namespace RI
{
	export
	{
		void Initialize();
		s32 ReadWord(u32 addr);
		void WriteWord(u32 addr, s32 data);
	}
}