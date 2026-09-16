#pragma once

class iEncode
{
public:
	virtual DWORD AddRef() = 0;
	virtual DWORD Release() = 0;
	virtual HRESULT Encode(const char* buffer, DWORD size, char* buf, DWORD& sz) = 0;
	virtual HRESULT Decode(const char* buffer, DWORD size, char* buf, DWORD& sz) = 0;
};
