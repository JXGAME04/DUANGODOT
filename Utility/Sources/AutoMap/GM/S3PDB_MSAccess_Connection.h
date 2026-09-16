// S3PDB_MSAccess_Connection.h: interface for the S3PDB_MSAccess_Connection class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __S3PDB_MSACCESS_CONNECTION_H__
#define __S3PDB_MSACCESS_CONNECTION_H__

#include "S3PDBConVBC.h"
#include "S3AccessResult.h"

#include "msado15.tlh"

#define RESULT_BUFFER_SIZE	8
class S3PDB_MSAccess_Connection : public S3PDBConVBC  
{
public:
	S3PDB_MSAccess_Connection();
	virtual ~S3PDB_MSAccess_Connection();
	virtual bool OpenConnect(_LPDATABASEINFO);
	virtual bool OpenConnect(LPCSTR szAccessFile);
	virtual void CloseConnect();
	virtual bool QuerySql(const char* lpszSql, S3PResultVBC** ppResult);
	virtual bool Do(const char* lpszSql);
	virtual bool DoSql(const char* lpszSql, _Recordset* pRes);
	
protected:
	_ConnectionPtr m_pCon;
	_CommandPtr m_pCmd;
	S3P_MSAccess_Result m_Result[RESULT_BUFFER_SIZE];
	bool GetFreeResult(S3PResultVBC** ppResult, _Recordset** ppRes);
};

class S3PDB_MSSQLServer_Connection : public S3PDB_MSAccess_Connection
{
public:
	virtual bool OpenConnect(_LPDATABASEINFO);
	virtual bool OpenConnect(LPCSTR szAccessFile);
};

#endif //__S3PDB_MSACCESS_CONNECTION_H__
