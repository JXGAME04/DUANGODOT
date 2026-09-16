#pragma once

/************************************************************
class declare
/************************************************************/
class iObj;
typedef std::list<iObj*>				iObjList;
typedef iObjList::iterator		itiObj;

//////////////////////////////////////////////////////////////
//id
//////////////////////////////////////////////////////////////
/************************************************************
//stObjParam
/************************************************************/
enum MaskEnum {MASK_NULL,MASK_OBJ,MASK_GROUNDOBJ,MASK_SCREENOBJ,
				MASK_PLANE,MASK_LIGHT,MASK_OBSTACLE,MASK_SELECT,
				MASK_MAP,MASK_MAPSMALL,MASK_PICTURE,
};
struct stObjParam
{
private:
	MaskEnum				eMask;
	long					mask;
public:
	void					SetMask(MaskEnum e, long l){eMask = e; mask = l;}
	void					ClearMask(){eMask = MASK_NULL; mask = 0;}

	BOOL					IsEMask(MaskEnum e){return eMask == e;}
	MaskEnum				GetEMask(){return eMask;}

	void					AddMask(long l){mask |= l;}
	BOOL					IsSetMask(long l){return mask & l;}
	BOOL					IsMask(long l){return mask == l;}
	BOOL					IsMaskEmpty(){return mask == 0;}
	long					GetMask(){return mask;}
	

	long					size;
	long					lParam, wParam;
	long					eCommand;		

	ElementEnum				eElement;				//graphics产生的类型
	cColor					crColor;				//颜色值，包括alpha
	
	BitOPEnum				eOPMapedit;				//mapedit的参数
	long					lMapeditEnum;

	enum 
	{
		MASK_OBJ_COMMAND		= 0x01, 
		MASK_OBJ_ELEMENT		= 0x02,
		MASK_OBJ_COLOR			= 0x04,
		MASK_OBJ_MAPEDIT		= 0x08,
	};

	stObjParam()
	{
		ZeroMemory(this,sizeof(stObjParam));
		size = sizeof(stObjParam);
	}
};

/************************************************************
//stGroundObjParam
/************************************************************/
struct stGroundObjParam : public stObjParam
{
	enum
	{
		MASK_GROUNDOBJ_OFFSET					= 0x01,
		MASK_GROUNDOBJ_GROUPID					= 0x02,
		MASK_GROUNDOBJ_GROUPFILE				= 0x04,

//动态物体
		MASK_GROUNDOBJ_DYNAMIC					= 0x08,
		MASK_GROUNDOBJ_EVENT					= 0x10,
		MASK_GROUNDOBJ_ID						= 0x20,

//静态物体
		MASK_GROUNDOBJ_TEMPLATE					= 0x40,
		MASK_GROUNDOBJ_KIND						= 0x80,
		MASK_GROUNDOBJ_MOVING					= 0x100,

	};
	enum
	{
		COM_GROUNDOBJ_OFFSET,					
		COM_GROUNDOBJ_END,
	};

	cPoint					ptOffset;				//地图的Offset
	long					lGroupID;				//编组ID
	char					szGroup[MAX_CHAR];		//编组的文件
//动态物体
	DynamicEnum				eDynamic;				//动态物体，按照事件点存档
	char					szEvent[MAX_NAME];		//事件点的名称
	long					lID;					//if = 0 自动生成一个唯一id
//静态物体
	char					szTemplate[MAX_CHAR];	//模板文件
	int						iTemplate;				//模板文件的Section
	KindEnum				eKind;					//地图编辑器适用
//一个标记
	BOOL					bMoving;

	stGroundObjParam()
	{
		ZeroMemory(this,sizeof(stGroundObjParam));
		size = sizeof(stGroundObjParam);
	}
};

//////////////////////////////////////////////////////////////
//plane
//////////////////////////////////////////////////////////////
enum TextureUseEnum{TEXTURE_USE_PART,TEXTURE_USE_WHOLE};

struct stPlaneParam : public stGroundObjParam
{
	PlaneEnum				ePlane;
	SortEnum				eSort;					//用于排序
	cPoint					ptCenter;				//resourceManager设置的重心
	ObstacleEnum			eObstacle;				//对ver底边线段是obstacle,对于hor整个是obstacle,为了简化一些编辑操作
	float					fHeight;				//物体的基本高度
	stResourceA				stResA;
	int						nLayer;					
	int						wTex,hTex;
	int						xTex,yTex;
	int						nAniSpeed;
	BOOL					bAniLoop;

	float					fSortOffY;				//作为点排序有一个偏移
	float					fAngleXY;				//底边在z=0平面的角度
	TextureUseEnum			eTextureUse;			//当ver的时候，是否用整个的图作为贴图，还是局部作为贴图
	LightedEnum				eLighted;				//挡光类型
	float					fLightedRadius;			//Radius只对PLDT_CENTERCIRCLE有效
	LightingEnum			eLighting;				//受光类型
	VersortEnum				eVersort;				//怎么排序
	
	int						nDirection;

	enum
	{		
		MASK_PLANE_TYPE				= 0x01,
		MASK_PLANE_SORT				= 0x02,
		MASK_PLANE_VERSORT			= 0x04,
		MASK_PLANE_LAYER			= 0x08,
		MASK_PLANE_LIGHTING			= 0x10,
		MASK_PLANE_LIGHTED			= 0x20,
		MASK_PLANE_LIGHTEDRADIUS	= 0x30,
		MASK_PLANE_ANGLEXY			= 0x80,
		MASK_PLANE_ANGLEZ			= 0x100,
		MASK_PLANE_CENTER			= 0x200,
		MASK_PLANE_OBSTACLE			= 0x400,
		MASK_PLANE_HEIGHT			= 0x800,
		MASK_PLANE_TEXTUREUSE		= 0x1000,
		MASK_PLANE_RESFRAME			= 0x2000,
		MASK_PLANE_RESFILE			= 0x4000,
//NPC
		MASK_PLANE_RESOURCE			= 0x8000,
		MASK_PLANE_RESOURCEARRAY	= 0x10000,

		MASK_PLANE_SORTOFFY			= 0x20000,

		MASK_PLANE_ANISPEED			= 0x40000,
	};
	enum
	{
		COM_PLANE_GENERATEID	= COM_GROUNDOBJ_END,
		COM_PLANE_LOADFROMNAME,
		COM_PLANE_SAVETONAME,
		COM_PLANE_UPDATEVERTEX,
		COM_PLANE_UPDATETEXTURE,
		COM_PLANE_CHANGEDIRECTION,
	};
	stPlaneParam()
	{
		ZeroMemory(this,sizeof(stPlaneParam));
		size = sizeof(stPlaneParam);
	}
};

struct stPlaneHorParam : public stPlaneParam
{
	stPlaneHorParam()
	{
		ZeroMemory(this,sizeof(stPlaneHorParam));
		size = sizeof(stPlaneHorParam);
	}
};

struct stPlaneVerParam : public stPlaneParam
{
	stPlaneVerParam()
	{
		ZeroMemory(this,sizeof(stPlaneVerParam));
		size = sizeof(stPlaneVerParam);
	}
};

struct stPlaneVerDynamicParam : public stPlaneVerParam
{
	stResourceArray			stResArray;
	stPlaneVerDynamicParam()
	{
		ZeroMemory(this,sizeof(stPlaneVerDynamicParam));
		size = sizeof(stPlaneVerDynamicParam);
	}
};

/************************************************************
//stSelectParam
//地图编辑器专门使用！
/************************************************************/
struct stSelectParam : public stGroundObjParam
{
	enum 
	{
		MASK_SELECT_TYPE		= 0x01,
		MASK_SELECT_SIZE		= 0x02,
		MASK_SELECT_OFFSETEND	= 0x04,
	};
	SelectEnum			eSelect;
	float				wSelect,hSelect;
	cPoint				ptOffsetEnd;		//the other point of the ST_PARALLELOGRAM
	stSelectParam()
	{
		ZeroMemory(this,sizeof(stSelectParam));
		eElement = GT_SELECT;
		size = sizeof(stSelectParam);
	}
};

/************************************************************
//Light 	//只支持点光源！
/************************************************************/
struct stLightParam: public stGroundObjParam
{
	enum 
	{
		MASK_LIGHT_RANGE				= 0x01,
		MASK_LIGHT_MAINLIGHT			= 0x02,

		MASK_LIGHT_VARRANGEMAX			= 0x04,
		MASK_LIGHT_VARRANGESPEED		= 0x08,
		MASK_LIGHT_VARRANGEFRAME		= 0x10,

		MASK_LIGHT_POSJUMPMAX			= 0x20,
		MASK_LIGHT_POSJUMPSPEED			= 0x40,
		MASK_LIGHT_POSJUMPFRAME			= 0x80,

		MASK_LIGHT_FLASHDARKFRAME		= 0x100,
		MASK_LIGHT_FLASHFRAME			= 0x200,
	};
	float						fRange;
	bool						bMainlight;

	float						fRangeVarMax;	//光源半径变化最大不超过
	float						fRangeVarSpeed; //光源半径变化的速度
	float						fRangeVarFrame;	//多少桢半径变一次

	float						fPosJumpMax;	//光源位置变化最大不超过
	cPoint						ptPosJumpSpeed;  //光源位置变化的速度
	float						fPosJumpFrame;	//多少桢位置变一次

	int							nFlashDarkFrame;//暗下来的时间
	float						fFlashFrame;	//多少桢闪烁一次
	stLightParam()
	{
		ZeroMemory(this,sizeof(stLightParam));
		eElement = GT_LIGHT;
		size = sizeof(stLightParam);
		bMainlight = false;

		fRangeVarMax = 1.f;
		fRangeVarSpeed = 0.1f;

		fPosJumpMax = 1.f;
		ptPosJumpSpeed = cPoint(0.1f,0.1f,0);

		nFlashDarkFrame = 1;

		fRangeVarFrame = 0;
		fPosJumpFrame = 0;
		fFlashFrame = 0;
	}
};

/************************************************************
//stObstacleParam 
/************************************************************/
struct stObstacleParam : public stGroundObjParam
{
	enum
	{
		MASK_OBSTACLE_TYPE			= 0x01,
		MASK_OBSTACLE_POINTINFO		= 0x02,
		MASK_OBSTACLE_SELECTPOINT	= 0x04,
		MASK_OBSTACLE_RADIUS			= 0x08,
	};
	enum					{MAX_POINT = 32};
	ObstacleEnum			eObstacle;
	int						nPoint;
	cPoint					aPoint[MAX_POINT];
	int						iPoint;
	float					fRadius;
	stObstacleParam()
	{
		ZeroMemory(this,sizeof(stObstacleParam));
		eElement = GT_OBSTACLE;
		size = sizeof(stObstacleParam);
	}
};

/************************************************************
//stScreenParam
//only game world use it
/************************************************************/
struct stScreenObjParam : public stObjParam
{
	POINT					ptOffset;
	enum 
	{
		MASK_SCREENOBJ_OFFSET			= 0x01,
	};
	enum
	{
		COM_SCREENOBJ_END,
	};
	stScreenObjParam()
	{
		ZeroMemory(this,sizeof(stScreenObjParam));
		size = sizeof(stScreenObjParam);
	}
};

/************************************************************
//stMapParam
//only game world use it
/************************************************************/
struct stMapSmallParam : public stScreenObjParam
{
	int wScreen,hScreen;				//screen长宽
	int xRegion,yRegion;				//中心(从ground得到)

	POINT ptScreenClick;				//click on the map will 
	cPoint ptGroundClick;				//return the ground position
	BOOL	bFull;						//全部显示？

	enum 
	{
		COM_MAP_GETCLICKPOS		
	};
	enum 
	{
		MASK_MAP_SCREENSIZE		= 0x01,
		MASK_MAP_REGIONPOS		= 0x02,
		MASK_MAP_CLICKPOINT		= 0x10,
		MASK_MAP_FULL			= 0x20,
	};
	stMapSmallParam()
	{
		ZeroMemory(this,sizeof(stMapSmallParam));
		eElement = GT_SMALLMAP;
		size = sizeof(stMapSmallParam);
		bFull = FALSE;
	}
};

struct stMapParam : public stScreenObjParam
{
	int wScreen,hScreen;				//screen长宽
	int xRegion,yRegion;				//中心(从ground得到)
	int wRegion,hRegion;				//ground长宽，以region为单位
	enum {
		DISPLAY_NULL		= 0, 
		DISPLAY_OBSTACLE	= 0x01,		//显示障碍
	};
	long lDisplay;
	BitOPEnum eDisplayOp;	
	POINT ptScreenClick;				//click on the map will 
	cPoint ptGroundClick;				//return the ground position
	enum 
	{
		COM_MAP_GETCLICKPOS		
	};
	enum 
	{
		MASK_MAP_SCREENSIZE		= 0x01,
		MASK_MAP_REGIONPOS		= 0x02,
		MASK_MAP_REGIONSIZE		= 0x04,
		MASK_MAP_DISPLAY		= 0x08,
		MASK_MAP_CLICKPOINT		= 0x10,
	};
	stMapParam()
	{
		ZeroMemory(this,sizeof(stMapParam));
		eElement = GT_MAP;
		size = sizeof(stMapParam);
	}
};

/************************************************************
//stPictureParam
//only game world use it
/************************************************************/
struct stPictureParam : public stScreenObjParam
{
	enum
	{
		COM_PICTURE_UPDATEVERTEX,
	};
	enum 
	{
		MASK_PICTURE_SUBRECT		= 0x01,
		MASK_PICTURE_RESOURCE		= 0x02,	//设置所有的res信息
		MASK_PICTURE_LAYER			= 0x04,	//设置先后顺序
	};
	stResourceA				stResA;
	RECT					rcSub;					//因为plane的图全部被裁开来了，所以没有提供这个变量
	int						nLayer;
	stPictureParam()
	{
		ZeroMemory(this,sizeof(stPictureParam));
		eElement = GT_PICTURE;
		size = sizeof(stPictureParam);
	}
};

class iItem;
class CFile;

class iObj
{
public:
	virtual ULONG				AddRef() = 0;
	virtual ULONG				Release() = 0;

	virtual void				Init(bool bCreateBuffer = false) = 0;

	virtual HRESULT				Load(const stSection* p) = 0;
	virtual HRESULT				Save(stSection* p) = 0;
	virtual HRESULT				LoadBinary(CFile& f) = 0;
	virtual HRESULT				SaveBinary(CFile& f) = 0;
	virtual BOOL				IsInGraphics() = 0;


	//包含屏幕的一个点
	virtual BOOL				Contain(POINT pt) = 0;
	virtual BOOL				Contain(cPoint pt) = 0;
	//包含在一个屏幕矩形中
	virtual BOOL				ContainedScreen(RECT rc) = 0;
	//包含在一个地图矩形中
	virtual BOOL				ContainedGround(cRect rc) = 0;
	//包含在一个菱形中
	virtual BOOL				ContainedDiamond(cRect rc) = 0;

	virtual void				GetOffset(POINT ptScreen,cPoint& ptGround) = 0;

	virtual HRESULT				GetParam(stObjParam& param) = 0;
	virtual HRESULT				SetParam(stObjParam& param, bool bRunNow = TRUE) = 0;//将消息放入队列
	virtual HRESULT				setParam(stObjParam& param) = 0;//立即执行
};



