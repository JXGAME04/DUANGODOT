// S3MSAccessResult.h: interface for the S3P_MSAccess_Result class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __S3MSACCESSRESULT_H__
#define __S3MSACCESSRESULT_H__

#include "S3PResultVBC.h"
#include "msado15.tlh"

class S3P_MSAccess_Result : public S3PResultVBC  
{
protected:
	_RecordsetPtr m_pResult;
public:
	int m_nAddRef;
	S3P_MSAccess_Result()
	{
		m_pResult = NULL;
		m_nAddRef = 0;
	}
	virtual ~S3P_MSAccess_Result();

	virtual int GetResult( _Recordset** ppResult);
	virtual int num_rows() const;
	virtual int num_fields() const;
	virtual void data_seek (unsigned int offset, int nType) const;
	virtual bool get_field_data(unsigned int nfieldindex, void* pData, unsigned long nsize);
	virtual bool set_field_data(unsigned int nfieldindex, const void* pData, unsigned long nsize);
	virtual bool addnew();
	virtual bool deleteall();
	virtual bool update();
	virtual int unuse();
};

#endif //__S3MSACCESSRESULT_H__
