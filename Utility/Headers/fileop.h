#pragma once

_declspec(dllexport) void AppParsePath(LPCSTR sFullPath, CString& sPath);
_declspec(dllexport) void AppParseDisk(LPCSTR sFullPath, CString& sDisk);
_declspec(dllexport) void AppParseFileName(LPCSTR sFile, CString& sName, CString& sExt);
_declspec(dllexport) void AppParsePathName(LPCSTR sFullPath, CString& sPath, CString& sName);
_declspec(dllexport) void AppParsePathNameExt(LPCSTR sFullPath, CString& sPath, CString& sName, CString& sExt);

_declspec(dllexport) void AppGetWorkingFolder(CString& strWorkingFolder);
_declspec(dllexport) void AppGetWorkingDisk(CString& strAppName);
_declspec(dllexport) void AppGetExeName(CString& strAppName);
_declspec(dllexport) void AppGetModuleFile(CString& strFile);

_declspec(dllexport) BOOL AppGetShortPath(CString& sPath);
_declspec(dllexport) BOOL AppGetFullPath(CString& sPath);
_declspec(dllexport) BOOL AppAddPath(CString& sPath,LPCSTR szFolder);
_declspec(dllexport) BOOL AppBuildPath(LPCSTR sDes);

_declspec(dllexport) BOOL AppCopyFile(LPCSTR sSrc,LPCSTR sDes, BOOL bFailIfExists = FALSE);
_declspec(dllexport) BOOL AppDeleteFolder(LPCSTR lpszFolderPathName);
_declspec(dllexport) BOOL AppCopyFolder(LPCSTR lpszSrc, LPCSTR lpszDest);
_declspec(dllexport) BOOL AppRenameFolder(LPCSTR lpszSrc, LPCSTR lpszDest);
_declspec(dllexport) BOOL AppMoveFolder(LPCSTR lpszSrc, LPCSTR lpszDest);

_declspec(dllexport) BOOL AppFindFile(LPCSTR sFileName,LPCSTR sDir,CString& sPathName);

_declspec(dllexport) BOOL AppGetPathDialog(CString& strPath, LPCSTR szRoot = NULL);
_declspec(dllexport) BOOL AppGetFileDialog(CString& strFile, LPCSTR szPath = NULL, LPCSTR szFilter = "*.*", BOOL Load = TRUE) ;

_declspec(dllexport) BOOL AppOpenFile(CFile& file, LPCSTR szFile, long flag, LPCSTR szLocate);

_declspec(dllexport) BOOL AppGetHostName(CString& sName);
_declspec(dllexport) BOOL AppWriteSimpleLogFile(LPCSTR szLogFile);
_declspec(dllexport) BOOL AppGetDynamicNum(LPCSTR szKey, long& num);

//该文档是否比exe旧
_declspec(dllexport) BOOL AppIsFileOld(LPCSTR szFile);


