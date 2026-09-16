#pragma once

#include "iPublic.h"
#include "iColor.h"

typedef long MsgType;

#define MAX_ACCOUNT 32
#define MAX_PASSWORD 32
#define MAX_WORD 128

struct stMsg
{
public:
	MsgType lType;
	stMsg(MsgType l){lType = l;}
	MsgType GetType() const {return lType;}
	char* GetBuffer() const{return (char*)(&lType);}
//	__declspec(dllexport) DWORD GetSize() const;
//	BOOL IsValid() const;
//	void operator = (const stMsg& msg){memcpy(GetBuffer(),msg.GetBuffer(),msg.GetSize());}
	static stMsg* CreateMsg(const stMsg& msg,DWORD size){
		char* p = new char[size]; 
		memcpy(p,msg.GetBuffer(),size);
		return (stMsg*)p;
	};
};

