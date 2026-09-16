#include "stdafx.h"
#include "stringvalue.h"
///////////////////////////////////////////////////////////////////////////////////////
//string to value
///////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////
//compose obj
////////////////////////////////////////////////////////////////////
_declspec(dllexport) BOOL StringToValue(const CString& s, POINT& pt)
{
	int pos = s.Find(',');
	ASSERT(pos != -1);
	CString sX = s.Left(pos);
	CString sY = s.Mid(pos + 1);
	StringToValue(sX,pt.x);
	StringToValue(sY,pt.y);
	return TRUE;
}

_declspec(dllexport) void ValueToString(const POINT& pt,CString& s)
{
	s.Format("%05d,%05d",pt.x,pt.y);	
}

_declspec(dllexport) BOOL StringToValue(const CString& s, CPoint& pt)
{
	int pos = s.Find(',');
	ASSERT(pos != -1);
	CString sX = s.Left(pos);
	CString sY = s.Mid(pos + 1);
	StringToValue(sX,pt.x);
	StringToValue(sY,pt.y);
	return TRUE;
}

_declspec(dllexport) void ValueToString(const CPoint& pt,CString& s)
{
	s.Format("%05d,%05d",pt.x,pt.y);	
}

_declspec(dllexport) BOOL StringToValue(const CString& str, cPoint& pt)
{
	CString s = str;
	int pos = s.Find(',');
	ASSERT(pos != -1);
	CString sX = s.Left(pos);
	StringToValue(sX,pt.x);
	s = s.Mid(pos + 1);
	pos = s.Find(',');
	ASSERT(pos != -1);
	CString sY = s.Left(pos);
	StringToValue(sY,pt.y);
	CString sZ = s.Mid(pos+1);
	StringToValue(sZ,pt.z);
	return TRUE;
}

_declspec(dllexport) void ValueToString(const cPoint& pt,CString& s)
{
	s.Format("%f,%f,%f",pt.x,pt.y,pt.z);	
}

_declspec(dllexport) BOOL StringToValue(const CString& s,CRect& rc)
{
	CString sTemp = s;
	CString sn;
	int pos;
	for (int i=0; i<4; i++)
	{
		pos = sTemp.Find(',');
		if (pos != -1)
		{
			sn = sTemp.Left(pos);
			sTemp = sTemp.Mid(pos + 1);
		}
		else
		{
			if (i != 3)
			{
				//无效的字符串 
				rc = CRect(0,0,0,0);
				return FALSE;
			}
			sn = sTemp;
		}

		int n = atoi(sn);
		switch (i)
		{
		case 0:
			rc.left = n;
			break;
		case 1:
			rc.top = n;
			break;
		case 2:
			rc.right = n;
			break;
		case 3:
			rc.bottom = n;
			break;
		}
	}
	return TRUE;
}

_declspec(dllexport) void ValueToString(const CRect& rc,CString& s)
{
	s.Format("%d,%d,%d,%d",rc.left,rc.top,rc.right,rc.bottom);
}

_declspec(dllexport) BOOL StringToValue(const CString& s,cRect& rc)
{
	CString sTemp = s;
	CString sn;
	int pos;
	for (int i=0; i<4; i++)
	{
		pos = sTemp.Find(',');
		if (pos != -1)
		{
			sn = sTemp.Left(pos);
			sTemp = sTemp.Mid(pos + 1);
		}
		else
		{
			if (i != 3)
			{
				//无效的字符串 
				rc = cRect(0,0,0,0);
				return FALSE;
			}
			sn = sTemp;
		}

		float n = (float)atof(sn);
		switch (i)
		{
		case 0:
			rc.l= n;
			break;
		case 1:
			rc.t= n;
			break;
		case 2:
			rc.r= n;
			break;
		case 3:
			rc.b= n;
			break;
		}
	}
	return TRUE;
}

_declspec(dllexport) void ValueToString(const cRect& rc,CString& s)
{
	s.Format("%f,%f,%f,%f",rc.l,rc.t,rc.r,rc.b);
}

////////////////////////////////////////////////////////////////////
//base obj
////////////////////////////////////////////////////////////////////
_declspec(dllexport) BOOL StringToValue(const CString& str,CString& s)
{
	if (str == "null")
		s.Empty();
	else
		s = str;
	return TRUE;
}

_declspec(dllexport) void ValueToString(const CString& s,CString& str)
{
	if (s.IsEmpty())
		str = "null";
	else
		str = s;
}

_declspec(dllexport) BOOL StringToValue(const CString& str,char* s)
{
	if (str == "null")
		s[0] = 0;
	else
	{
		strcpy(s,(LPCSTR)str);
	}
	return TRUE;
}

_declspec(dllexport) void ValueToString(const char* s,CString& str)
{
	if (strlen(s) == 0)
		str = "null";
	else
		str = s;
}

_declspec(dllexport) BOOL StringToValue(const CString& s,cColor& cr)
{
	CRect r;
	BOOL b = StringToValue(s,r);
	cr = cColor(r.left,r.top,r.right,r.bottom);
	return b;
}

_declspec(dllexport) void ValueToString(const cColor& cr,CString& s)
{
	CRect r = CRect(cr.r,cr.g,cr.b,cr.a);
	ValueToString(r,s);
}

_declspec(dllexport) BOOL StringToValue(const CString& s,int& n)
{
	n = atoi(s);
	return TRUE;
}

_declspec(dllexport) void ValueToString(const int& n, CString& s)
{
	s.Format("%d",n);
}

_declspec(dllexport) BOOL StringToValue(const CString& s,long& n)
{
	n = atoi(s);
	return TRUE;
}

_declspec(dllexport) void ValueToString(const long& n, CString& s)
{
	s.Format("%d",n);
}

_declspec(dllexport) BOOL StringToValue(const CString& s,float& n)
{
	n = (float)atof(s);
	return TRUE;
}

_declspec(dllexport) void ValueToString(const float& n, CString& s)
{
	s.Format("%f",n);
}

_declspec(dllexport) BOOL StringToValue(const CString& s,char& c)
{
	c = atoi(s);
	return TRUE;
}

_declspec(dllexport) void ValueToString(const char& c, CString& s)
{
	s.Format("%d",c);
}

_declspec(dllexport) BOOL StringToValue(const CString& s,bool& b)
{
	if (s == 'T')
		b = true;
	else if (s == 'F')
		b = false;
	else //无效的字符串 
	{
		b = false;		
		return FALSE;
	}
	return TRUE;
}

_declspec(dllexport) void ValueToString(const bool& b,CString& s )
{
	if (b == true)
		s = 'T';
	else
		s = 'F';
}

_declspec(dllexport) void PointArrayToString(const CPoint aPoint[], int nPoint, CString& sValue)
{
	sValue.Empty();
	for (int i=0; i<nPoint; i++)
	{
		CString s;
		s.Format("(%d,%d,) ",aPoint[i].x,aPoint[i].y);
		sValue += s;
	}
}

////////////////////////////////////////////////////////////////////
//more compose obj
////////////////////////////////////////////////////////////////////
_declspec(dllexport) void PointArrayToString(const cPoint aPoint[], int nPoint, CString& sValue)
{
	sValue.Empty();
	for (int i=0; i<nPoint; i++)
	{
		CString s;
		s.Format("(%f,%f,%f,) ",aPoint[i].x,aPoint[i].y,aPoint[i].z);
		sValue += s;
	}
}

_declspec(dllexport) void StringToPointArray(const CString& sValue,cPoint aPoint[])
{
	//sValue = (0,0,0,) (1,1,1,) (2,2,2,)
	int pos1 = 0;
	int pos2 = 0;
	int index = 0;
	while(1)
	{
		pos1 = sValue.Find('(',pos2);
		if (pos1 == -1)
			return;
		pos2 = sValue.Find(')',pos1);
		CString sVal = sValue.Mid(pos1+1,pos2-pos1-1);
	//sVal = 0,0,0,
		for (int i=0; i<3; i++)
		{
			int pos = sVal.Find(',');
			CString sF = sVal.Left(pos);
			sVal = sVal.Mid(pos + 1);
			float f = (float)atof(sF);
			switch(i)
			{
			case 0:
				aPoint[index].x = f;
				break;
			case 1:
				aPoint[index].y = f;
				break;
			case 2:
				aPoint[index].z = f;
				break;
			}			
		}
		index ++;
	}	
}

