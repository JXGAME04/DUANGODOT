/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Tue May 09 15:08:46 2006
 */
/* Compiler settings for F:\DownLoads\Utility\Sources\AutoMap\GM\GameMaster.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )
#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

const IID IID_IKRole = {0xAC961E16,0x2C9F,0x4dcf,{0x9B,0xCE,0x22,0x8B,0x12,0x18,0xD2,0xB6}};


const IID IID_IKPlayer = {0x98C49B84,0x9506,0x4c52,{0x95,0xE9,0x52,0x08,0xE5,0x02,0xB3,0x89}};


const IID IID_IKGameWorld = {0x4D98C1D8,0x9AEE,0x427b,{0xA9,0xD3,0xE2,0x7B,0xD4,0x7B,0x70,0x49}};


const IID IID_IKGameMaster = {0x5C75CC7B,0x3379,0x4a82,{0x9F,0xAB,0x94,0x54,0xC6,0xD2,0x24,0x34}};


const IID LIBID_GameMasterLib = {0x3D76430E,0x9308,0x4e9a,{0x9D,0xA8,0x24,0x21,0x5B,0x2D,0xB3,0x21}};


const CLSID CLSID_GameMaster = {0xD67E5BA4,0x2C4F,0x4ec4,{0xAC,0x21,0x50,0x47,0x03,0xF3,0x21,0x21}};


#ifdef __cplusplus
}
#endif

