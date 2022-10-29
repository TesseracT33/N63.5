export module RI; /* RDRAM Interface */

import Util;

namespace RI
{
	export
	{
		void Initialize();
		s32 ReadReg(u32 addr);
		void WriteReg(u32 addr, s32 data);
	}
}