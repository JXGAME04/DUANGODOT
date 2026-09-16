#include "stdafx.h"
#include "S3AccessResult.h"
#include "crtdbg.h"
#include <stdio.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
S3P_MSAccess_Result::~S3P_MSAccess_Result()
{
	m_pResult = NULL;
	_ASSERT(m_nAddRef == 0);
}

int S3P_MSAccess_Result::num_rows() const
{
	int iRet = 0;
	try
	{
		if (m_pResult)
			iRet = m_pResult->GetRecordCount();
	}
	catch( _com_error e )
	{
		//printf("[S3P_MSAccess_Result::num_rows]COM error: %s\r\n", e.ErrorMessage());
	}
	return iRet;
}

int S3P_MSAccess_Result::num_fields() const
{
	int iRet = 0;
	try
	{
		if (m_pResult)
		{
			FieldsPtr pFs = m_pResult->GetFields();
			iRet = pFs->GetCount();
		}
	}
	catch( _com_error e )
	{
		//printf("[S3P_MSAccess_Result::num_fields]COM error: %s\r\n", e.ErrorMessage());
	}
	return iRet;
}

void S3P_MSAccess_Result::data_seek(unsigned int offset, int nType) const
{
	try
	{
		if (NULL != m_pResult)
		{
			unsigned int iRows = num_rows();
			if (nType == begin)
			{
				m_pResult->MoveFirst();
				if (offset > 0 && offset < iRows)
				{
					m_pResult->Move(offset);
				}
			}
			else if (nType == end)
			{
				m_pResult->MoveLast();
			}
			else if (nType == next)
			{
				for (;offset > 0; offset--)
					m_pResult->MoveNext();
			}
			else if (nType == previous)
			{
				for (;offset > 0; offset--)
					m_pResult->MovePrevious();
			}
			else
				_ASSERT(0);
		}
	}
	catch ( _com_error e )
	{
		//printf("[S3P_MSAccess_Result::data_seek(%d, %d)]COM error: %s\r\n", offset, nType, e.ErrorMessage());
	}
}

bool S3P_MSAccess_Result::get_field_data(unsigned int nfieldindex, void* pData, unsigned long nsize)
{
	BOOL bRet = FALSE;
	try
	{
		if (NULL != m_pResult && nsize == sizeof(VARIANT))
		{
			if (num_rows() > 0 && nfieldindex >= 0 && nfieldindex < num_fields())
			{
				_variant_t varIndex;
				varIndex = (long)nfieldindex;
				FieldPtr pField = m_pResult->Fields->GetItem(varIndex);
				if (SUCCEEDED(pField->get_Value((VARIANT*)pData)))
					bRet = TRUE;
			}
		}
	}
	catch ( _com_error e )
	{
		//printf("[S3P_MSAccess_Result::get_field_data(%d)]COM error: %s\r\n", nfieldindex, e.ErrorMessage());
	}

	return bRet;
}

bool S3P_MSAccess_Result::set_field_data(unsigned int nfieldindex, const void* pData, unsigned long nsize)
{
	BOOL bRet = FALSE;
	try
	{
		if (NULL != m_pResult && nsize == sizeof(VARIANT))
		{
			if (nfieldindex >= 0 && nfieldindex < num_fields())
			{
				_variant_t varIndex;
				varIndex = (long)nfieldindex;
				const _variant_t* pvar = (const _variant_t*)pData;
				FieldPtr pField = m_pResult->Fields->GetItem(varIndex);
				if (SUCCEEDED(pField->put_Value(*pvar)))
					bRet = TRUE;
			}
		}
	}
	catch ( _com_error e )
	{
		//printf("[S3P_MSAccess_Result::get_field_data(%d)]COM error: %s\r\n", nfieldindex, e.ErrorMessage());
	}

	return bRet;
}

bool S3P_MSAccess_Result::addnew()
{
	BOOL bRet = FALSE;
	try
	{
		if (NULL != m_pResult)
		{
			if (SUCCEEDED(m_pResult->AddNew()))
				bRet = TRUE;
		}
	}
	catch ( _com_error e )
	{
		//printf("[S3P_MSAccess_Result::get_field_data(%d)]COM error: %s\r\n", nfieldindex, e.ErrorMessage());
	}

	return bRet;
}

bool S3P_MSAccess_Result::deleteall()
{
	BOOL bRet = FALSE;
	try
	{
		if (NULL != m_pResult)
		{
			if (SUCCEEDED(m_pResult->Delete(adAffectAll)))
				bRet = TRUE;
		}
	}
	catch ( _com_error e )
	{
		//printf("[S3P_MSAccess_Result::get_field_data(%d)]COM error: %s\r\n", nfieldindex, e.ErrorMessage());
	}

	return bRet;
}

bool S3P_MSAccess_Result::update()
{
	BOOL bRet = FALSE;
	try
	{
		if (NULL != m_pResult)
		{
			if (SUCCEEDED(m_pResult->Update()))
				bRet = TRUE;
		}
	}
	catch ( _com_error e )
	{
		//printf("[S3P_MSAccess_Result::get_field_data(%d)]COM error: %s\r\n", nfieldindex, e.ErrorMessage());
	}

	return bRet;
}


int S3P_MSAccess_Result::unuse()
{
	if (m_pResult)
	{
		try
		{
			if (adStateOpen ==
				(m_pResult->GetState() & adStateOpen))
			{
				m_pResult->Close();
			}
		}
		catch (_com_error e)
		{
			//printf("[S3PDB_MSAccess_Connection::Do(1)]COM error: %s\r\n", e.ErrorMessage());
		}
		m_nAddRef--;
	}

	return m_nAddRef;
}

int S3P_MSAccess_Result::GetResult( _Recordset** ppResult)
{
	if (ppResult == NULL)
		return m_nAddRef;
	if (m_pResult == NULL)
	{
		HRESULT hr = m_pResult.CreateInstance("ADODB.Recordset");
		if (FAILED(hr))
		{
			m_pResult = NULL;
		}
	}

	_ASSERT(m_nAddRef == 0);
	if (m_nAddRef == 0)
	{
		*ppResult = m_pResult;

		if (*ppResult)
			m_nAddRef++;
	}

	return m_nAddRef;
}
