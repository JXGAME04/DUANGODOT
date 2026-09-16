#include "stdafx.h"
#include "planedoodad.h"


///////////////////////////////////////////////////////////////////////////////////////
//table of the plane
///////////////////////////////////////////////////////////////////////////////////////

CString g_aStrTablePlaneName[TP_NUM+1] = 
{
	"planetype","planelightingtype","planelightedtype","planelightedradius",
	"sorttype","anglexy","anglez",
	"centerx","centery","height",
	"obstacletype",
	"dynamictype",
	"textureuse",
	"kind",
	"versort",
	"anispeed",
	"aniloop",
	"sprwidth","sprheight","sprcenterx","sprcentery",
	"obstaclenum",
	"",
};

CString g_aaStrTablePlaneValue[TP_NUM+1][MAX_VALUE_ITEM] = 
{
	{"平行于地表","平行且切分","垂直不切分","垂直于地表","",},
	{"不受光","按照切分受光","按照重心受光","平行受光","",},
	{"不挡光","按照底边挡光","按照一个圆挡光","",},
	{"0.2","0.3","0.4","0.5",},

	{"低于地表","地表","低于ver","ver","高于ver","天空",""},
	{"00","45","135",""},
	{"00","30","60","90",""},

	{"00",""},
	{"00",""},
	{"00",""},
	{"非阻挡","一般阻挡","阻挡但是可以飞行","阻挡但是可以跳跃","阻挡可以跳跃和飞行",""},
	{"静态物体","npc","box","trap","clientonlynpc","clientonlybox","clientonlytrap","player","nosave",""},
	{"使用局部贴图","使用全部贴图",""},
	{"一般类型","室内地表","室外道路",
	"墙壁","树木","山洞洞壁",				//垂直的plane
	"森林","房屋","平台", "河流","山脉",	//平行的obstacle
	"组件的垂直部分","组件的阴影",			//组件
	//特殊存储的部分
	"声音","飞鸟",
	},
	{"按照点排序", "按照边排序","按照树排序"},
	{"20","30","40","50"},
	{"1","0"},
	{"00",""},
	{"00",""},
	{"00",""},
	{"00",""},
	{"00",""},
	{""},
};

_declspec(dllexport) int StringToPlaneTableValue(int table, const CString& strValue)
{
	return NameToID(g_aaStrTablePlaneValue[table],strValue);
}

_declspec(dllexport) CString GetTablePlaneName(int table)
{
	return g_aStrTablePlaneName[table];
}

_declspec(dllexport) LPCSTR GetTablePlaneValue(int table, int index)
{
	return g_aaStrTablePlaneValue[table][index];
}

_declspec(dllexport) CString* GetTablePlaneValue(int e)
{
	return g_aaStrTablePlaneValue[e];
}

CString g_aStrTableDoodadName[TD_NUM+1] = 
{
	"doodadtype","geometry","height","obstacletype",
	"",
};

CString g_aaStrTableDoodadValue[TD_NUM+1][MAX_VALUE_ITEM] = 
{
	{"只切分成垂直的面","切分成垂直的面和水平的面","等分平行的面","等分垂直的面",""},
	{"45度角转折","任意转折",""},
	{"400","350","300","250"},
	{"非阻挡","一般阻挡","阻挡但是可以飞行","阻挡但是可以跳跃","阻挡可以跳跃和飞行",""},
	{"",},
};

CString g_aStrTableDoodadDefaultValue[TD_NUM+1] = 
{
	"只切分成垂直的面","45度角转折","300","一般阻挡",
	"",
};

_declspec(dllexport) int StringToDoodadTableValue(int table, const CString& strValue)
{
	return NameToID(g_aaStrTableDoodadValue[table],strValue);
}

_declspec(dllexport) CString GetTableDoodadName(int table)
{
	return g_aStrTableDoodadName[table];
}

_declspec(dllexport) CString* GetTableDoodadValue(int e)
{
	return g_aaStrTableDoodadValue[e];
}

_declspec(dllexport) CString GetTableDoodadDefaultValue(int e)
{
	return g_aStrTableDoodadDefaultValue[e];
}


////////////--add--start--///////////////////////////////
//yujiang add 2003 2 15
//功能：将spr的信息添加到模板文件中,并且将模板转换成二进制
//打包可以使用winzip
/*
*/
