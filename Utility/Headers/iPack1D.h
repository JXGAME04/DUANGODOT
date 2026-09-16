#pragma once

//对于一个2维数据打包

class iZipRead;

class iPack1D
{
public:
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;
//在图形引擎中使用
	virtual BOOL Open(LPCSTR szTemplate,LPCSTR szIndex,iZipRead* p = NULL) = 0;
	virtual void* LocateToFrame(LPCSTR szFile,int frame) = 0;

	virtual void Refresh(LPCSTR szTemplate) = 0;

	virtual void* LocateTo(LPCSTR szFile) = 0;
//在转换工具使用
	virtual void SetConvertNum(int n) = 0;
	virtual BOOL Convert(LPCSTR szTxt) = 0;
	virtual BOOL Save(LPCSTR szTemplate) = 0;
};

enum Pack1DEnum{Pack1D_Plane,Pack1D_Animation,
				Pack1D_Obj,
				Pack1D_Npc,};

_declspec(dllexport) iPack1D* Common_CreatePack1D(Pack1DEnum e);
_declspec(dllexport) iPack1D* Common_GetPack1D(Pack1DEnum e);
