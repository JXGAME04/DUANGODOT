// cGraphics.h: interface for the cGraphics class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CGRAPHICS_H__E8A5065F_6C53_4DD2_BFE6_868079238526__INCLUDED_)
#define AFX_CGRAPHICS_H__E8A5065F_6C53_4DD2_BFE6_868079238526__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "iGraphics.h"
#include "d3dApp.h"

class cObj;
typedef std::list<cObj*>				cObjList;
typedef cObjList::iterator		itcObj;
class cLight;
typedef std::list<cLight*>			cLightList;
typedef cLightList::iterator	itcLight;
class cObstacle;
typedef std::list<cObstacle*>		cObstacleList;
typedef cObstacleList::iterator	itcObstacle;
class cNpc;
typedef std::list<cNpc*>		cNpcList;
typedef cNpcList::iterator	itcNpc;


class iMsg;
class iMsgQueue;
class cTextureCache;
class cThread;

const cRect c_rectMin((float)MAX_LONG,(float)MAX_LONG,(float)MIN_LONG,(float)MIN_LONG);
const cRect c_rectMax((float)MIN_LONG,(float)MIN_LONG,(float)MAX_LONG,(float)MAX_LONG);
class iFps;

class cGround;
class cScreen;
class cCamera;
struct iKFont;

enum {
	RESOURCE_GRAPHICS_START = 0x10,
	RESOURCE_MAP_START		= 0x100,
	RESOURCE_REGION_START	= 0x100000,
};

class cGraphics : public iGraphics
{
public:
	cGraphics();
	virtual ~cGraphics();

//com interface
	ULONG m_ulCount;
	virtual ULONG AddRef() ;
	virtual ULONG Release() ;
	HRESULT Cleanup();

//param 
	stParam m_param;
	HRESULT SetParam(stParam& param, bool bRunNow = false) ;
	HRESULT setParam(const stParam& param) ;
	HRESULT GetParam(stParam& param) ;
	LPCSTR GetWorkingFolder(){return m_param.szPath;}
	BOOL IsPausePaint(){return m_param.bPausePaint;}

	int m_nFaces;
	int IncreaseFace(int n){m_nFaces+=n;return m_nFaces;};
	int GetFaces(int n){return m_nFaces;};

//帧数保持在
	int GetFrame(){return m_param.nFrame;}
//得到显示方案
	long GetShowParam(){return m_param.lShowEnum;}
	BOOL IsShow(ShowEnum e);
	
//debug
	int m_nFlipTicks;
	int GetFlipTicks(){return m_nFlipTicks;}
	
//Trace 
	iKFont* m_pDebugFont;
	iKFont* GetDebugFont(){return m_pDebugFont;};
	CStringList m_listTrace;
	void Trace(LPCSTR str);
	void PrintTrace();
	void PrintTrace(LPCSTR s,int n);
	void Vaild(HRESULT hr);
	BOOL IsTrace(long mask){return m_param.lTraceEnum & mask;}

//begin and end
	virtual HRESULT Begin();
	virtual HRESULT End();
	
//thread 
	cThread* m_pThread;

	virtual HRESULT RunThread() ;
	virtual HRESULT EndThread() ;
	
	UINT ThreadRun();
	iFps* m_pFps;
	int GetFps();
	HRESULT Trace();
	HRESULT RunOnce();
	HRESULT TextureCacheRun();
	static UINT Thread( LPVOID pParam );

//graphics element!
	cGround* m_pGround;
	cGround* GetcGround(){return m_pGround;};
	iGround* GetiGround() ;  

	cScreen* m_pScreen;
	cScreen* GetcScreen(){return m_pScreen;};
	iScreen* GetiScreen() ;

	cCamera* m_pCamera;
	cCamera* GetcCamera(){return m_pCamera;};
	iCamera* GetiCamera();

//message Queue
	iMsgQueue* m_pMsgQueue;
	ULONG PushMessage(void* buffer,ULONG size,ElementEnum eMsg);
	ULONG HandleMessage();

//common resource only for mapedit
	enum eTexKind{
		TEX_WHITE,TEX_SELECT,TEX_GRID,//一般的选择图案
		TEX_NUM,};
	enum {MAX_TEX = 10};

//texture and image cache
	cTextureCache* m_aTextureCache[USAGE_NUM];
protected:
	iTexture* FindCommonTexture(eTexKind kind);
public:
	HRESULT SetCommonTexture(eTexKind kind, int frame = 0);
	iTexture* FindSprResource(LPCSTR szSprFile);
	HRESULT RefreshSpr(LPCSTR szSpr,bool bRunNow);

	CRITICAL_SECTION m_csTextureCache;
	iTexture* CreateiTexture(const stResourceA& resPos, int width, int height);
	void AddTexture(const stResourceA& resPos,iTexture* p);
	iTexture* FindTexture(const stResourceA& resPos, bool AutoCreate = true);
	HRESULT ClearTexture(const stResourceA& resPos);
/**************************************************************************/
//the below are different between 2d and 3d
/**************************************************************************/
//paint loop
	virtual BOOL IsPaint(){return FALSE;};
	virtual HRESULT BeginPaint();
	virtual HRESULT FlushPaint();
	virtual HRESULT EndPaint();
	ULONG m_nFrames;
	ULONG GetFrames(){return m_nFrames;}

//factory
	virtual HRESULT CreateGraphicsElement(ElementEnum id,LPVOID ppoint);
	virtual LPDIRECT3DDEVICE8 GetDevice3D() {return NULL;};
	
//create device
	virtual HRESULT CreateDevice(const stParam& param) = 0;
	virtual void GetScreen(int& x,int &y) = 0;

//save a bmp
	
// Overridable functions for the 3D scene created by the app
	virtual HRESULT ToggleFullscreen() = 0;
	virtual HRESULT ResizeEnvironment() = 0;
    virtual HRESULT CleanDevice() = 0;	

//error report
	virtual HRESULT DXError(HRESULT hr) = 0;

};

int MakeInt(float f);
void BitOP(long& param, BitOPEnum eOP,long lBit);

#endif // !defined(AFX_CGRAPHICS_H__E8A5065F_6C53_4DD2_BFE6_868079238526__INCLUDED_)
