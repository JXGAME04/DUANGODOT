//-----------------------------------------//
//                                         //
//  File		: S3PDBConVBC.h			   //
//	Author		: Yang Xiaodong            //
//	Modified	: [wxb 2003-7-9]           //
//                                         //
//-----------------------------------------//

#ifndef __S3PDBCONVBC_H__
#define __S3PDBCONVBC_H__

#include "KThread.h"
#include <string>

typedef struct tag_DATABASEINFO
{
	std::string strServer;
	std::string strDataBase;
	std::string strUser;
	std::string strPassword;
}_DATABASEINFO, *_LPDATABASEINFO;

class S3PResultVBC;
class S3PDBConVBC : public K_CS
{
public:
	virtual bool OpenConnect(_LPDATABASEINFO) = 0;
	virtual bool OpenConnect(LPCSTR szAccessFile) = 0;
	virtual void CloseConnect() = 0;
	virtual bool QuerySql(const char* lpszSql, S3PResultVBC** ppResult) = 0;
	virtual bool Do(const char* lpszSql) = 0;

	~S3PDBConVBC();
	
protected:
	S3PDBConVBC();
};

#endif //__S3PDBCONVBC_H__
