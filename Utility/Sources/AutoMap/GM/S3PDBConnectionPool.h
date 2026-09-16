//-----------------------------------------//
//                                         //
//  File		: S3PDBConnectionPool.h    //
//	Author		: Yang Xiaodong            //
//	Modified	: [wxb 2003-7-10]          //
//                                         //
//-----------------------------------------//
#ifndef _S3PDBCONNECTIONPOOL_H_
#define _S3PDBCONNECTIONPOOL_H_

#include <list>
#include <vector>
#include "S3PDBConVBC.h"
#include "KThread.h"

class S3PDBAccessConnectionPool : public K_CS
{
public:
	static S3PDBAccessConnectionPool* Instance();
	static void ReleaseInstance();

	BOOL Init(LPCSTR szAccessFile, DWORD dwConLimits);
	BOOL RemoveDBCon(S3PDBConVBC** ppInfo);
	BOOL ReturnDBCon(S3PDBConVBC* pInfo);

protected:
	static S3PDBAccessConnectionPool* m_pInstance;

	DWORD m_dwConLimits;

	std::vector<S3PDBConVBC*> m_ConVBCPool;
	
	BOOL CreateConnection(LPCSTR szAccessFile, S3PDBConVBC** ppInfo);

protected:
	S3PDBAccessConnectionPool();
	virtual ~S3PDBAccessConnectionPool();
};

class S3PDBSQLServerConnectionPool : public S3PDBAccessConnectionPool
{
public:
	static S3PDBSQLServerConnectionPool* m_pInstance;
	static S3PDBSQLServerConnectionPool* Instance();
	static void ReleaseInstance();

	BOOL Init(_LPDATABASEINFO pDatabase, DWORD dwConLimits);

protected:
	BOOL CreateConnection(_LPDATABASEINFO pDatabase, S3PDBConVBC** ppInfo);
};

#endif	// _S3PDBCONNECTIONPOOL_H_
