// WorldEditorDoc.h : interface of the CWorldEditorDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WORLDEDITORDOC_H__13A6A8D2_9D13_48BE_8C80_44F5F7250981__INCLUDED_)
#define AFX_WORLDEDITORDOC_H__13A6A8D2_9D13_48BE_8C80_44F5F7250981__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CWorldEditorDoc : public CDocument
{
protected: // create from serialization only
	CWorldEditorDoc();
	DECLARE_DYNCREATE(CWorldEditorDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWorldEditorDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWorldEditorDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CWorldEditorDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WORLDEDITORDOC_H__13A6A8D2_9D13_48BE_8C80_44F5F7250981__INCLUDED_)
