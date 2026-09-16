#pragma once

#include "../Elem/WndImage.h"
#include "../Elem/UiImage.h"

class UiCaveMap : protected KWndImage
{
public:
	static UiCaveMap* OpenWindow();		//打开窗口，返回唯一的一个类对象实例
	static void			CloseWindow();		//关闭窗口
	static UiCaveMap*	GetIfVisible();
	void				LoadScheme(const char* pScheme);	//载入界面方案

private:
	UiCaveMap() {}
	~UiCaveMap() {}
	void	Initialize();
	int		WndProc(unsigned int uMsg, unsigned int uParam, int nParam);
	void	Breathe();				//活动函数
	void	UpdateData();
private:
	static UiCaveMap* m_pSelf;
	KWndImage			m_Sign;
};