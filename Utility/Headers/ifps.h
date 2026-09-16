#pragma once

#include "ipublic.h"

class iFps
{
public:
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;
	virtual void Init() = 0;
	virtual void Frame() = 0;
	virtual void KeepFps(int frame) = 0;

	virtual unsigned long GetCount() = 0;
	virtual unsigned long GetTime() = 0;
	virtual unsigned long GetFps() = 0;
	virtual unsigned long GetNowFps() = 0;
};

_declspec(dllexport) iFps* Common_CreateFps();

