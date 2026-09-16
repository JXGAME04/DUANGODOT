
#ifndef _KLE_BASEDEF_H_
#define _KLE_BASEDEF_H_

#define SAFE_DELETE(p)		{ if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELGRP(p)		{ if(p) { delete []p;     (p)=NULL; } }
#define SAFE_RELEASE(p)		{ if(p) { (p)->Release(); (p)=NULL; } }

//! KLE中用到的Short Name的长度
#define	KLESNAMELEN		32
//! KLE中用到的Middle Name的长度
#define KLEMNAMELEN		64
//! KLE中用到的Long Name的长度
#define KLELNAMELEN		256


//! 与stdlib.h 中的 _MAX_FNAME类似
#define KLEFMGR_MAXPATH   260			//!< 最大全路径长度
#define KLEFMGR_MAXDIR    256			//!< 最大目录长度
#define KLEFMGR_MAXFILE   256			//!< 最大文件名长度
#define KLEFMGR_MAXEXT    256			//!< 最大扩展名长度
#define KLEFMGR_MAXDRIVE    3			//!< 最大驱动器长度 

// 程序中用到的基本目录
#define KLEP_DATA			     "\\UpdateData"
#define KLEP_DATABK			     "\\UpdateDataBackUp"
#define KLEP_FILELIST		     "\\UpdateFileList"
#define KLEP_VERSIONDATA	     "\\VersionData"
#define KLEP_VERSIONFILE	     "\\Version.ini"
#define KLEP_TOOLS               "\\tools"
#define KLEP_INFO                "\\info"
#define KLEP_INDEX               "\\VersionData"
#define KLEP_UpdateRelativeFile  "\\UpdateRelativeFile.ini" 


#endif