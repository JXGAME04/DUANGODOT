#pragma once

class iPack
{
public:
	virtual DWORD AddRef() = 0;
	virtual DWORD Release() = 0;
	virtual HRESULT Pack(const char* buffer, DWORD size, char* buf, DWORD& sz) = 0;
	virtual HRESULT Unpack(const char* buffer, DWORD size, char* buf, DWORD& sz) = 0;
};
