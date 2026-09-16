/*****************************************************************************************
//	界面窗口体系结构--图形窗口
//	Copyright : Kingsoft 2002
//	Author	:   Wooy(Wu yue)
//	CreateTime:	2002-7-25
------------------------------------------------------------------------------------------
	显示(spr)图形的窗口。
*****************************************************************************************/
#pragma once
#include "WndWindow.h"
#include "UiImage.h"

#define	WNDIMG_ES_EXCLUDE_TRANS		0x0001		//以图形的形状来作为窗口的形状

class KWndImage : public KWndWindow
{
public:
	virtual int		Init(KIniFile* pIniFile, const char* pSection);//初始化
	virtual int		PtInWindow(int x, int y);				//判断一个点是否在窗口范围内,传入的是绝对坐标
	virtual void	PaintWindow();							//窗体绘制
	void			Clone(KWndImage* pCopy);
	
	void	SetFrame(int nFrame);							//设置图形帧
	int		NextFrame();									//图形换帧
	void	SetImage(short nType, const char* pszImgName, bool bAdjustWndSize = false);
	void	UpdateTimer();
	KWndImage();
	const char* GetImage() {return &m_Image.szImage[0];};
	void SetImageOffset(int x, int y) {m_ImgOffX = x; m_ImgOffY = y;};
	KUiImageRef* GetImageFactor() {return &m_Image;};
	void	SetSprByFrame(const char* pszImgName, int nFrame, int* pX = NULL, int* pY = NULL);
	void	GetImageOffset(int& x, int& y) {x = m_ImgOffX; y = m_ImgOffY;};
	int		GetCurFrame() {return m_Image.nFrame;}; //guve
private:
	int m_ImgOffX;
	int m_ImgOffY;
protected:
	KUiImageRef		m_Image;
};