// ***************************************************************************************
// 场景地图中内建对象模板类的申明
// Copyright : Kingsoft 2002
// Author    : wooy(wu yue)
// CreateTime: 2002-11-18
// ---------------------------------------------------------------------------------------
// ***************************************************************************************

#ifndef KSPBIOTEMPLATE_H_HEADER_INCLUDED_C222B712
#define KSPBIOTEMPLATE_H_HEADER_INCLUDED_C222B712

struct KBuildinObj;


#define		PI						3.14159265f
#define		PI_PER_ANGLE_DEGREE		0.0174532925f
#define		ONE_OF_COS_30			1.1547005383792515290182975610039

//##ModelId=3DDC990003BA
//##Documentation
//## 场景地图的内建对象的模板
class KSpbioTemplate
{
  public:

	//##ModelId=3DDD1236014C
	KSpbioTemplate();

	//##ModelId=3DDD1BD60159
	//##Documentation
	//## 从文件载入数据
	bool Load(
		//##Documentation
		//## 文件名
		const char* pszFile, 
		//##Documentation
		//## 载入模板文件里的第几个模板
		int nIndex);

	//##ModelId=3DDD33C103B6
	//##Documentation
	//## 获知是模板里面的哪个子模板
	inline int GetIndex() const
		{ return m_nIndex; }

	//##ModelId=3DE1385502EA
	//##Documentation
	//## 复制模板数据到一个对象实例
	void Instance(
		//##Documentation
		//## 指向实例结构的指针
		KBuildinObj* pInstance, int& nCentreX, int& nCentreY);

  private:

	//##ModelId=3DDD1F4D00B4
	//##Documentation
	//## 从ini的一个段落读取数据
	bool LoadFromIniSection(
		//##Documentation
		//## ini文件
		class KIniFile* pIni, 
		//##Documentation
		//## 段落标记
		const char* pszSection,
		const char* pszPath);


  private:

	//##ModelId=3DDCA2E10149
	//##Documentation
	//## 引用的图形文件的名字
	char m_szImageFile[128];

	//##ModelId=3DDCA42601F1
	//##Documentation
	//## 图形的属性
	unsigned int m_uProps;

	//##ModelId=3DDD11A9030C
	//##Documentation
	//## 中心点横坐标
	short m_nCentreX;

	//##ModelId=3DDD11C00265
	//##Documentation
	//## 中心点纵坐标
	short m_nCentreY;

	short m_nImgWidth;
	short m_nImgHeight;

	//##ModelId=3DDCA3E3037B
	//##Documentation
	//## 此对象引用图形动画的帧数目
	short m_nNumFrames;

	short m_bSetSortManner;

	//##ModelId=3DDD335900B4
	//##Documentation
	//## 模板文件里面的子模板索引
	short m_nIndex;

	//##ModelId=3DE2940D038B
	//##Documentation
	//## 线条与x轴的夹角
	float m_fAngleXY;

	unsigned short	m_nAniSpeed;
};

#endif /* KSPBIOTEMPLATE_H_HEADER_INCLUDED_C222B712 */
