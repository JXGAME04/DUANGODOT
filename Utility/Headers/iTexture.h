#pragma once

/************************************************************
//class iTexture 
/************************************************************/
class iTexture;

enum UsageEnum
{
	//type						discription		video memory size
	USAGE_STATIC,				//静态图片		8M
	USAGE_MENU,					//菜单界面等	2M
	USAGE_ANIMATION_ACTOR,		//人物动画		10M
	USAGE_ANIMATION_SCENE,		//场景动画		2M
	USAGE_COMMON,				//公共资源		1M
	USAGE_EFFECT,				//特效动画		1M
	USAGE_REGION,				//地表德的		4M
	USAGE_NUM,					//use to get the number
	//all										27M
};

enum MediaEnum
{
	MEDIA_NULL = -1,
	MEDIA_VIDEO,				//spr
	MEDIA_VIDEOONE,
	MEDIA_MEMORY,
	MEDIA_SPR,
	MEDIA_VIDEOOTHER,			//JPG, tga. bmp format
	MEDIA_NUM,	
};

struct stResource
{
	long				idResource;				
	char				szResource[MAX_CHAR];
	int					iFrame;

	int					xOff,yOff;

	stResource()
	{
		szResource[0]	= 0;
		idResource		= 0;
	}
	bool operator == (const stResource& st)
	{
		if (idResource != 0 && st.idResource != 0)
			return (idResource == st.idResource);
		return strcmp(szResource,st.szResource) == 0;
	}
};

//仅仅cTexture的成员变量
struct stResourceDevice
{
	MediaEnum			eMedia;
	D3DFORMAT			format;
	D3DPOOL				pool;
	DWORD				usage;
	UsageEnum			eUsage;
	stResourceDevice()
	{
		format			= D3DFMT_A4R4G4B4;
		pool			= D3DPOOL_DEFAULT;
		usage			= D3DUSAGE_DYNAMIC;
		eUsage			= USAGE_STATIC;
		eMedia			= MEDIA_NULL;
	}
};

enum {MAX_RESOURCE= 16};
struct stResourceA
{
	stResourceDevice	stDevice;
	stResource			stRes;
	bool operator == (const stResourceA& st)
	{
		return stRes == st.stRes;
	}
};

//仅仅作为参数传递
struct stResourceArray
{
	stResourceDevice	stDevice;
	stResource			aResource[MAX_RESOURCE];
	int					nResource;

	stResourceArray()
	{
		nResource		= 0;
	}
	void operator = (const stResourceArray& st)
	{
		nResource = st.nResource;
		for (int i=0; i<nResource; i++)
		{
			aResource[i] = st.aResource[i];
		}
		stDevice = st.stDevice;
	}
};

enum UsedEnum{USED_JUSTCREATE, USED_OFTEN, USED_NORMAL, USED_SOLDEM, USED_NEVER,};

class iUsed
{
public:
	virtual void			Used() = 0;
	virtual UsedEnum		GetUsed() = 0;
};

struct stRender
{
	iTexture*		pTexSrc;
	RECT			rcSrc;
	int				nFrameSrc;

	iTexture*		pTexDes;
	POINT			ptDes;
	int				nFrameDes;
	D3DFORMAT		format;			//des format

	eRenderOp		eOp;			//op 
};

class cTextureCache;

class iTexture
{
public:
	virtual ULONG			AddRef() = 0;
	virtual ULONG			Release() = 0;
	virtual ULONG			GetCount() = 0;

	virtual cTextureCache*	GetTextureCache() = 0;
	virtual void			SetTextureCache(cTextureCache* p) = 0;

	virtual HRESULT			Create(const stResourceA& st, int width, int height)=0;

	//设置成无效的texture
	virtual BOOL			SetInvalid() = 0;
	virtual BOOL			IsValid() = 0;
	virtual void			Clear() = 0;				//用0填满
	virtual void			ClearRect(RECT& rc) = 0;	//用0填满
	virtual HRESULT			ChangeFormat(D3DFORMAT format) = 0;

	virtual HRESULT			SetResource(const stResourceA* st) = 0;
	virtual void			GetResource(stResourceA& st) = 0;
	virtual bool			IsResource(const stResourceA* st) = 0;
	virtual void			Used() = 0;
	virtual UsedEnum		GetUsed() = 0;
	virtual void			ResetUsed() = 0;

	virtual HRESULT			InitDeviceObjects()=0;
    virtual HRESULT			RestoreDeviceObjects()=0;                     
    virtual HRESULT			InvalidateDeviceObjects()=0;
    virtual HRESULT			DeleteDeviceObjects()=0;

	virtual HRESULT			Unlock(int frame) = 0;
	virtual HRESULT			Lock(int frame, D3DLOCKED_RECT& lr) = 0;
	virtual HRESULT			Render(stRender& render) = 0;
	virtual HRESULT			Update(RECT* pDirtyRect) = 0;
	virtual HRESULT			Refresh() = 0;

	virtual ULONG			GetFrames() = 0;
	virtual ULONG			GetDirections() = 0;
	virtual void			GetCenterToUpleft(int frame,int& x, int& y) = 0;

//必须重载的
    virtual HRESULT			FinalCleanup() = 0;	

	virtual long			Upload(MediaEnum type) = 0;
	virtual bool			IsUpload(MediaEnum type) = 0;
	virtual ULONG			Download(MediaEnum type) = 0;
	virtual ULONG			DownloadSomeFrame(MediaEnum type) = 0;
	virtual ULONG			GetSize(MediaEnum type) = 0;

	virtual void			GetTextureSize(int frame,ULONG& width, ULONG& height) = 0;
	virtual void			GetImageSize(int frame,ULONG& width, ULONG& height) = 0;
	virtual void			GetTextureRect(int frame, cRect& rc) = 0;
	
	virtual IDirect3DTexture8* GetD3DTexture(int frame = 0) = 0;
	
	virtual void			Trace() = 0;
};
