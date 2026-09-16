// GMDebugDoc.h : interface of the CGMDebugDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_GMDEBUGDOC_H__0B71D6B7_2E4B_4A40_8480_01F67A6D5617__INCLUDED_)
#define AFX_GMDEBUGDOC_H__0B71D6B7_2E4B_4A40_8480_01F67A6D5617__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CGMDebugDoc : public CDocument
{
protected: // create from serialization only
	CGMDebugDoc();
	DECLARE_DYNCREATE(CGMDebugDoc)

// Attributes
public:
	CString m_strDocName;
	DWORD m_docIP;
	DWORD m_docRelayID;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGMDebugDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGMDebugDoc();
	void Trace(LPCTSTR strMessage);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	DWORD m_nMessage;

// Generated message map functions
protected:
	//{{AFX_MSG(CGMDebugDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GMDEBUGDOC_H__0B71D6B7_2E4B_4A40_8480_01F67A6D5617__INCLUDED_)
