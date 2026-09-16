// S3PDB_MSAccess_Connection.cpp: implementation of the S3PDB_MSAccess_Connection class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "S3PDB_MSAccess_Connection.h"
#include "S3AccessResult.h"
#include <string>

WINOLEAPI  CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static int g_nAccessServer = 0;

S3PDB_MSAccess_Connection::S3PDB_MSAccess_Connection()
	: m_pCon(NULL), m_pCmd(NULL)
{
//	if (g_nAccessServer == 0)
//		CoInitializeEx(NULL, 0);
	g_nAccessServer++;
}

S3PDB_MSAccess_Connection::~S3PDB_MSAccess_Connection()
{
	ASSERT(m_pCon == NULL);

	g_nAccessServer--;

//	if (g_nAccessServer == 0)
//		CoUninitialize();
}

void S3PDB_MSAccess_Connection::CloseConnect()
{
	if (NULL != m_pCon)
	{
		m_pCon->Close();
		m_pCon = NULL;
	}

	m_pCmd = NULL;
}

bool S3PDB_MSAccess_Connection::OpenConnect(LPCSTR szAccessFile)
{
	ASSERT(szAccessFile && strlen(szAccessFile));
	ASSERT(NULL == m_pCon);
	bool bRet = true;
	if (NULL == m_pCon)
	{
		bRet = false;
		_ConnectionPtr pCon;
		HRESULT hr = pCon.CreateInstance("ADODB.Connection");
		if ((SUCCEEDED(hr))
			&& (NULL != pCon))
		{
			std::string strSource = "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=";
			strSource += szAccessFile;
			try
			{
				hr = pCon->Open(strSource.c_str(),L"", L"", -1L);
				if (SUCCEEDED(hr))
				{
					m_pCon = pCon;
					m_pCmd.CreateInstance("ADODB.Command");

					if (m_pCmd)
						m_pCmd->ActiveConnection = m_pCon;

					bRet = true;
				}
			}
			catch( _com_error &e )
			{
				TRACE("[S3PDB_MSAccess_Connection::OpenConnect]COM error: %s", e.ErrorMessage());
			}
		}
	}
	return bRet;
}

bool S3PDB_MSAccess_Connection::OpenConnect(_LPDATABASEINFO)
{
	return false;
}

bool S3PDB_MSSQLServer_Connection::OpenConnect(LPCSTR)
{
	return false;
}

bool S3PDB_MSSQLServer_Connection::OpenConnect(_LPDATABASEINFO lpDBIdentifier)
{
	ASSERT(lpDBIdentifier);
	ASSERT(NULL == m_pCon);
	bool bRet = true;
	if (NULL == m_pCon)
	{
		bRet = false;
		_ConnectionPtr pCon;
		HRESULT hr = pCon.CreateInstance("ADODB.Connection");
		if ((SUCCEEDED(hr))
			&& (NULL != pCon))
		{
			std::string strSource = "driver={SQL Server};";
			std::string strTemp = "Server=";
			strTemp += lpDBIdentifier->strServer;
			strTemp += ";";
			strSource += strTemp;
			strTemp = "Database=";
			strTemp += lpDBIdentifier->strDataBase;
			strTemp += ";";
			strSource += strTemp;
			lpDBIdentifier->strDataBase.c_str();
			try
			{
				pCon->CursorLocation = adUseClient;
				hr = pCon->Open(strSource.c_str(),
								lpDBIdentifier->strUser.c_str(),
								lpDBIdentifier->strPassword.c_str(),
								adConnectUnspecified);
				if (SUCCEEDED(hr))
				{
					m_pCon = pCon;

					m_pCmd.CreateInstance("ADODB.Command");

					if (m_pCmd)
						m_pCmd->ActiveConnection = m_pCon;

					bRet = true;
				}
			}
			catch( _com_error &e )
			{
				TRACE("[S3PDBConnectionPool::OpenConnect]COM error: %s", e.ErrorMessage());
			}
		}
	}
	return bRet;
}

bool S3PDB_MSAccess_Connection::Do(const char* lpszSql)
{
	BOOL bRet = false;
	if (m_pCon == NULL)
		return bRet;

	try
	{
		if (m_pCmd)
		{
			m_pCmd->CommandText = lpszSql;
			m_pCmd->Execute(NULL, NULL, adCmdText);
			bRet = TRUE;
		}
		else
		{
			_Recordset* pRes = NULL;
			S3PResultVBC* pResult = NULL;
			if (GetFreeResult(&pResult, &pRes))
			{
				if (pRes)
				{
					try
					{
						HRESULT hr =
						pRes->Open(lpszSql,
						_variant_t( (IDispatch*)(m_pCon), true),
						adOpenDynamic,
						adLockOptimistic,
						adCmdText);
						if (SUCCEEDED(hr))
						{
							bRet = TRUE;
						}
					}
					catch (_com_error &e)
					{
						TRACE("[S3PDB_MSAccess_Connection::Do COM error: %s", e.ErrorMessage());
					}
				}

				pResult->unuse();
			}
		}
	}
	catch (_com_error &e)
    {
		TRACE("[S3PDB_MSAccess_Connection::Do COM error: %s", e.ErrorMessage());
    }

	return bRet;
}

bool S3PDB_MSAccess_Connection::DoSql(const char* lpszSql, _Recordset* pRes)
{
	BOOL bRet = false;
	if ((NULL != m_pCon) &&
		(NULL != lpszSql) &&
		(NULL != pRes))
	{
		try
		{
			HRESULT hr =
					pRes->Open(lpszSql,
					_variant_t( (IDispatch*)(m_pCon), true),
					adOpenStatic,
					adLockReadOnly,
					adCmdText);
			if (SUCCEEDED(hr))
			{
				bRet = TRUE;
			}
		}
		catch ( _com_error &e )
		{
#ifdef _DEBUG
			TRACE("SQL Error: %s", lpszSql);
#endif
			TRACE("[S3PDB_MSAccess_Connection::DoSql COM error: %s", e.ErrorMessage());
			bRet = FALSE;
		}
	}

	return bRet;
}

bool S3PDB_MSAccess_Connection::QuerySql(const char* lpszSql, S3PResultVBC** ppResult)
{
	*ppResult = NULL;
	BOOL bRet = FALSE;
	_Recordset* pRes = NULL;
	S3PResultVBC* pResult = NULL;
	if (GetFreeResult(&pResult, &pRes))
	{
		if (pRes)
		{
			bRet = DoSql(lpszSql, pRes);
		}

		if (bRet)
		{
			*ppResult = pResult;
		}
		else
		{
			pResult->unuse();
		}
	}

	return bRet;
}

bool S3PDB_MSAccess_Connection::GetFreeResult(S3PResultVBC** ppResult, _Recordset** ppRes)
{
	ASSERT(ppResult && ppRes);
	for (int i = 0; i < RESULT_BUFFER_SIZE; i++)
	{
		if (m_Result[i].m_nAddRef == 0)
		{
			if (m_Result[i].GetResult(ppRes) > 0)
			{
				*ppResult = &m_Result[i];
				return true;
			}
		}
	}
	ASSERT(0);
	return false;
}