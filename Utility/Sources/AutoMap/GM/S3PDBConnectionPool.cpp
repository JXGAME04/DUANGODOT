//-----------------------------------------//
//                                         //
//  File		: S3PDBConnectionPool.cpp  //
//	Author		: Yang Xiaodong            //
//	Modified	: [wxb 2003-7-10]          //
//                                         //
//-----------------------------------------//

#include "stdafx.h"
#include "S3PDBConnectionPool.h"

#include "S3PDB_MSAccess_Connection.h"

S3PDBAccessConnectionPool* S3PDBAccessConnectionPool::m_pInstance = NULL;

S3PDBAccessConnectionPool::S3PDBAccessConnectionPool()
{
	m_dwConLimits = 0;
}

S3PDBAccessConnectionPool::~S3PDBAccessConnectionPool()
{
	for (int i = 0; i < m_ConVBCPool.size(); i++)
	{
		if (m_ConVBCPool[i])
		{
			m_ConVBCPool[i]->CloseConnect();
			delete m_ConVBCPool[i];
		}
	}
}

BOOL S3PDBAccessConnectionPool::Init(LPCSTR szAccessFile, DWORD dwConLimits)
{
	m_dwConLimits = dwConLimits;

	S3PDBConVBC* pVBC = NULL;
	for (int i = 0; i < m_dwConLimits; i++)
	{
		if (CreateConnection(szAccessFile, &pVBC))
		{
			m_ConVBCPool.push_back(pVBC);
		}
		else
			break;
	}

	return m_ConVBCPool.size() == m_dwConLimits;
}

BOOL S3PDBAccessConnectionPool::RemoveDBCon(S3PDBConVBC** ppInfo)
{
	while (IsLocked())
		Sleep(1);
	Lock();
	BOOL b = FALSE;
	for (int nIndex = 0; nIndex < m_ConVBCPool.size(); nIndex++)
	{
		if (m_ConVBCPool[nIndex] && !m_ConVBCPool[nIndex]->IsLocked())
		{
			*ppInfo = m_ConVBCPool[nIndex];
			m_ConVBCPool[nIndex]->Lock();
			b = TRUE;
			TRACE("DB Connect %d is busy !\n", nIndex);
			break;
		}
	}
	Unlock();

	return b;
}

BOOL S3PDBAccessConnectionPool::ReturnDBCon(S3PDBConVBC* pInfo)
{
	while (IsLocked())
		Sleep(1);
	Lock();
	BOOL b = FALSE;
	for (int nIndex = 0; nIndex < m_ConVBCPool.size(); nIndex++)
	{
		if (m_ConVBCPool[nIndex] == pInfo && m_ConVBCPool[nIndex]->IsLocked())
		{
			m_ConVBCPool[nIndex]->Unlock();
			b = TRUE;
			TRACE("DB Connect %d is free !\n", nIndex);
			break;
		}
	}
	Unlock();

	return b;
}

S3PDBAccessConnectionPool* S3PDBAccessConnectionPool::Instance()
{
	if ( NULL == m_pInstance )
	{
		m_pInstance = new S3PDBAccessConnectionPool;
	}
	return m_pInstance;
}

void S3PDBAccessConnectionPool::ReleaseInstance()
{
	if ( NULL != m_pInstance )
	{
		delete m_pInstance;
		m_pInstance = NULL;
	}
}

BOOL S3PDBAccessConnectionPool::CreateConnection(LPCSTR szAccessFile, S3PDBConVBC** ppInfo)
{
	S3PDBConVBC* pCon = new S3PDB_MSAccess_Connection();
	if (pCon->OpenConnect(szAccessFile))
	{
		*ppInfo = pCon;
		return TRUE;
	}
	delete pCon;
	pCon = NULL;
	return FALSE;
}

BOOL S3PDBSQLServerConnectionPool::Init(_LPDATABASEINFO lpDataBaseInfo, DWORD dwConLimits)
{
	m_dwConLimits = dwConLimits;

	S3PDBConVBC* pVBC = NULL;
	for (int i = 0; i < m_dwConLimits; i++)
	{
		if (CreateConnection(lpDataBaseInfo, &pVBC))
		{
			m_ConVBCPool.push_back(pVBC);
		}
		else
			break;
	}

	return m_ConVBCPool.size() == m_dwConLimits;
}

S3PDBSQLServerConnectionPool* S3PDBSQLServerConnectionPool::m_pInstance = NULL;
S3PDBSQLServerConnectionPool* S3PDBSQLServerConnectionPool::Instance()
{
	if ( NULL == m_pInstance )
	{
		m_pInstance = new S3PDBSQLServerConnectionPool;
	}
	return m_pInstance;
}

void S3PDBSQLServerConnectionPool::ReleaseInstance()
{
	if ( NULL != m_pInstance )
	{
		delete m_pInstance;
		m_pInstance = NULL;
	}
}

BOOL S3PDBSQLServerConnectionPool::CreateConnection(_LPDATABASEINFO pDatabase, S3PDBConVBC** ppInfo)
{
	S3PDBConVBC* pCon = new S3PDB_MSSQLServer_Connection();
	if (pCon->OpenConnect(pDatabase))
	{
		*ppInfo = pCon;
		return TRUE;
	}
	delete pCon;
	pCon = NULL;
	return FALSE;
}