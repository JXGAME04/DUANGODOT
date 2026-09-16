// FilePath.cpp: implementation of the FilePath class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "VersionMaker.h"
#include "FilePath.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//DEL FilePath::FilePath(const ci_string&  FileFullPath,const ci_string&  FileHalfPath)
//DEL :FullPath(FileFullPath)
//DEL ,HalfPath(FileHalfPath)
//DEL {
//DEL 
//DEL 	
//DEL 
//DEL 
//DEL }

FilePath::~FilePath()
{

}

FilePath::FilePath(const string &FileFullPath, const string &FileHalfPath)
:FullPath(FileFullPath)
,HalfPath(FileHalfPath)
{

}
