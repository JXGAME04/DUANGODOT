#pragma once

//use to create a verson file at server
class iZip
{
public:
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;
	virtual bool load(LPCSTR szZipFile) = 0;		//load a .zip file
	virtual bool create(LPCSTR szZipFile) = 0;		//create a .zip file
	virtual bool add(LPCSTR szFile) = 0;			//add a file to zip file
//	bool remove(LPCSTR szFile);				//remove a file from zip file
};

//use to autoupdate get a zip from server and extra to client
class iZipExtract
{
public:
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;
	virtual bool load(LPCSTR szZipFile) = 0;		//load a .zip file from disk
//	virtual bool load_buffer(char* buffer) = 0;		//load a .zip file from buffer
	virtual bool extract(LPCSTR szFolder) = 0;		//extrace to a folder like d:\test
	virtual DWORD get_uncompressedsize() = 0;		//get size of uncompressed
};

//may be game used it
class iZipRead						
{
public:
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;

	virtual bool load(LPCSTR szZipFile) = 0;		//load a .zip file

	virtual bool locate(LPCSTR szFile)= 0;			//locate a file then can read
	virtual int	length() = 0;						//length of the file located
	virtual int	read(void* buf, int* bufsize) = 0;

	virtual BOOL Open(LPCSTR szFile) = 0;
	virtual DWORD GetLength() = 0;
	virtual UINT Read(void* lpBuf, UINT nCount) = 0;
};

//mapconvert use it
class iZipWrite
{
public:
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;
	virtual bool create(LPCSTR szZipFile) = 0;					//create a .zip file
	virtual bool add(LPCSTR szFile,LPCSTR szZipName) = 0;		//add a file to zip file
	virtual bool close() = 0;			//save
};

enum ZipEnum{zip_default,zip_normal,};
_declspec(dllexport) iZipExtract* Zip_CreateZipExtract(ZipEnum verson);
_declspec(dllexport) iZipRead* Zip_CreateZipRead(ZipEnum verson);
_declspec(dllexport) iZipWrite* Zip_CreateZipWrite(ZipEnum verson);
