/*****************************************************************************
					中文显示系统之字体接口

应用：
	界面系统对外接口：iKFont。
	外部客户引用此系统要包含头文件iKFont.h。

接口功能：
	绘制显示字符串。

使用：
	1 接口使用的字库文件资源：
		其格式规范与金山西山居剑二引擎的字库文件的格式规范相同。
		具体格式规范可参考KFontRes.h文件。
	2 接口使用前的初始化：
		使用接口前需先调用其接口方法Init，如果是使用Direct3D Device，则初始化时
		传入Direct3D Device接口的实例的指针。调用时间最好是在Direct3D Device已
		初始化之后。如果是使用DirectGraph，则...
		初始化将使已经载入字库数据丢失。
    3 载入字库
		载入字库需要在成功的初始化操作之后，否则载入字库失败。在已经载入字库
		之后可以直接重复调用此接口函数，以更换字库，不需要在调用初始化函数。
	4 使用后接口的关闭：
		不需再使用此接口实例时调用其接口方法Terminate关闭它。
		关闭之后，如重新被初始化，则又可以被使用。

	5 输出（绘制）字符串，调用时传入字符串及其开始绘制的位置，还可以指定字符串
	  的颜色，如果没有指定颜色，会默认为黑色。还提供了一个特别功能：此字符串的
	  输出紧接在前一次输出的字符串的后面，如需要此功能把字符串开始绘制位置的
	  X与Y坐标设为KFL_FOLLOW就行了。
	  TextOut方法的参数nLineWidth(像点数)如果传入值为大于等于一个全角字符宽度
	  的宽度，则显示字符串时，将根据行宽自动换行。
	6 此模块引用了的绘图设备要由lost state重设恢复为operational state之前，要调
	  用iKFont的接口函数InvalidateDeviceObjects使Device状态切换会受影响的绘图设
	  备相关内容失效。重设之后要调用iKFont接口的RestoreDeviceObjects方法作一些
	  设备相关内容的恢复操作。

/*******************************************************************************
// FileName			:	KFont.h
// FileAuthor		:	Wooy
// FileCreateDate	:	2002-9-5
// FileDescription	:	字体接口
// Revision Count	:	
*******************************************************************************/

#define	pure = 0
#define	KF_FOLLOW	(-2147483647 - 1)
#define	KF_ZERO_END	-1

struct iKFont
{
	//初始化
	virtual bool	Init(void* pDrawDevice) pure;
	//载入字库
	virtual bool	Load(const char* pszFontFile) pure;
	//结束，清除操作
	virtual void	Terminate() pure;
	//显示字符串
	virtual void	TextOut(const char* pszText, int nCount = KF_ZERO_END,
					int nX = KF_FOLLOW, int nY = KF_FOLLOW,
					unsigned int nColor = 0xff000000, int nLineWidth = 0) pure;
	//使Device设备相关/依赖内容失效
	virtual void	InvalidateDeviceObjects() pure;
	//在绘图设备由lost state恢复为operational state后iKFont作设备相关内容的恢复操作。
	virtual bool	RestoreDeviceObjects() pure;				
	//释放接口实例对象
	virtual void	Release() pure;
};
