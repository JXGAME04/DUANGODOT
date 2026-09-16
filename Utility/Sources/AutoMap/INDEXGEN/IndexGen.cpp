// IndexGen.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "stdio.h"
#include "Data.h"


int main(int argc, char* argv[])
{

    if (argc < 7)
    {
        printf("SourceFile_Path Index_FileName Version RemotePath CommentFileName InfoFileName\r\n");
        
        printf("SourceFile_Path: 升级文件路径\r\n");
        printf("Index_FileName:  生成的Index.txt全路径名\r\n");
        printf("RemotePath：     指定该目录下的文件在服务器上的目录名称\r\n");
        printf("Version:         1: 标准版    2: OEM    8: DBUAII ...\r\n\\r\n");
        printf("CommonInfo_FileName:   公共部分的说明描述\r\n");
        printf("FilesComment_FileName: 各个文件的说明描述\r\n");        
        printf("InfoFileName：   提供给测试用的生成Index中的文件的信息   \r\n");        

        return false;
    }

    int nVersion      = _StrToInt(argv[3]);

    IndexGen(
        argv[1],    // source path
        argv[2],    // destional file
        nVersion,   // Version
        argv[4],    // Remote Path
        argv[5],    // Common inforamtion file name
        argv[6],    // File comment file name
        argv[7]     // file information file name
    );
    
    return 0;
}
