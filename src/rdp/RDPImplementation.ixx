export module RDPImplementation;

import Util;

export class RDPImplementation {
public:
	virtual ~RDPImplementation() = default;

	virtual void EnqueueCommand(int cmd_len, u32* cmd_ptr) = 0;
	virtual bool Initialize() = 0;
	virtual void OnFullSync() = 0;
	virtual void TearDown() = 0;
	virtual void UpdateScreen() = 0;

	virtual void SetOrigin(u32) {};
	virtual void SetWidth(u32) {};
	virtual void SetControl(u32) {};
};