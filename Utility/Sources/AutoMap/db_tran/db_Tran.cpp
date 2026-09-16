// db_Tran.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <STDIO.H>
#include "DBTable.h"

using namespace std;


#pragma pack( push, 1 )

struct TRoleBaseInfo
{
	//RoleBaseInfo
	unsigned long	dwId;
	char	szName[32];
	bool	bSex;
	char	szAlias[32];		
	char	caccname[32];
	unsigned char	nFirstSect;
	unsigned char	nSect;
	unsigned char	cFightMode;
	unsigned char	cUseRevive;
	unsigned char	cIsExchange;
// --------------------- add by spe 2003/07/19 -----------------------
	//unsigned char	cPkStatus;
// -------------------------------------------------------------------
	int		ijoincount;
	int		isectrole;
	int		igroupcode;
	int		igrouprole;
	int		irevivalid;
	int		irevivalx;
	int		irevivaly;
	int		ientergameid;
	int		ientergamex;
	int		ientergamey;
	char	cpartnercode[32];
	int		isavemoney;
	int		imoney;
	int		ifiveprop;
	int		iteam;
	int		ifightlevel;
	int		fightexp;
	int		ileadlevel;
	int		ileadexp;
	int		iliveexp;
	int		ipower;
	int		iagility;
	int		iouter;
	int		iinside;
	int		iluck;
	int		imaxlife;
	int		imaxstamina;
	int		imaxinner;
// --------------------- add by spe 2003/07/19 -----------------------
	//int		icurlife;
	//int		icurstamina;
	//int		icurinner;
	//int		ipkvalue;
// -------------------------------------------------------------------
	int		ileftprop;
	int		ileftfight;
	int		ileftlife;
	int		ifinishgame;
	short	iarmorres;
	short	iweaponres;
	short	ihelmres;
};

struct TRoleData
{
	TRoleBaseInfo BaseInfo;
	bool	bBaseNeedUpdate;//通知是否需要更新
	//当存取时如果Offset等于-1，表示不更新该数据
	//Additional Info
	short	nFightSkillCount;
	short	nLiveSkillCount;
	unsigned char	nTaskCount;
	short	nItemCount;
	short	nFriendCount;
	int		dwTaskOffset;
	int		dwLSkillOffset;
	int		dwFSkillOffset;
	int		dwItemOffset;
	int		dwFriendOffset;
	int		dwDataLen;
	unsigned char	pBuffer[1];
};

struct TRoleBaseInfoNew
{
	//RoleBaseInfo
	unsigned long	dwId;
	char	szName[32];
	bool	bSex;
	char	szAlias[32];		
	char	caccname[32];
	unsigned char	nFirstSect;
	unsigned char	nSect;
	unsigned char	cFightMode;
	unsigned char	cUseRevive;
	unsigned char	cIsExchange;
// --------------------- add by spe 2003/07/19 -----------------------
	unsigned char	cPkStatus;
// -------------------------------------------------------------------
	int		ijoincount;
	int		isectrole;
	int		igroupcode;
	int		igrouprole;
	int		irevivalid;
	int		irevivalx;
	int		irevivaly;
	int		ientergameid;
	int		ientergamex;
	int		ientergamey;
	char	cpartnercode[32];
	int		isavemoney;
	int		imoney;
	int		ifiveprop;		//五行属性
	int		iteam;
	int		ifightlevel;	//等级
	int		fightexp;
	int		ileadlevel;
	int		ileadexp;
	int		iliveexp;
	int		ipower;
	int		iagility;
	int		iouter;
	int		iinside;
	int		iluck;
	int		imaxlife;		//生命
	int		imaxstamina;	//体力
	int		imaxinner;		//内力
// --------------------- add by spe 2003/07/19 -----------------------
	int		icurlife;
	int		icurstamina;
	int		icurinner;
	int		ipkvalue;
// -------------------------------------------------------------------
	int		ileftprop;
	int		ileftfight;
	int		ileftlife;
	int		ifinishgame;
	short	iarmorres;
	short	iweaponres;
	short	ihelmres;
};

struct TRoleDataNew
{
	TRoleBaseInfoNew BaseInfo;
	bool	bBaseNeedUpdate;//通知是否需要更新
	//当存取时如果Offset等于-1，表示不更新该数据
	//Additional Info
	short	nFightSkillCount;
	short	nLiveSkillCount;
	unsigned char	nTaskCount;
	short	nItemCount;
	short	nFriendCount;
	int		dwTaskOffset;
	int		dwLSkillOffset;
	int		dwFSkillOffset;
	int		dwItemOffset;
	int		dwFriendOffset;
	int		dwDataLen;
	unsigned char	pBuffer[1];
};

struct TDBSkillData
{
	short	m_nSkillId;
	short	m_nSkillLevel;
};

#pragma pack( pop )

int ExpTable[101] = {0,50,450,1350,2550,4550,7050,10050,14050,19050,49050,
						115050,190650,276550,373750,483050,605550,742150,894050,1062450,1248550,
						1419550,1607650,1814150,2040350,2287750,2557950,2852550,3173350,3522250,3847050,
						4199550,4581550,4995250,5442750,5926450,6448850,7012650,7620650,8275850,8981450,
						9645850,10360550,11128850,11954350,12840850,13792350,14813150,15907750,17081050,18338150,
						19684450,21125850,22668450,24318750,26083650,27970450,29986950,32141450,34442650,36730850,
						39289850,42150950,45348850,48922450,52914850,57374050,62353450,67912550,74117450,81041850,
						89118650,98537750,109520150,122322950,137245150,154634550,174895450,198498150,225989250,254802650,
						288352250,327410650,372875550,425790150,487366450,559012250,642362950,739318050,852083450,983220750,
						1135704350,1312987450,1519079050,1758632850,2037050450,2360601050,2736560050,3173370150,3658764250,4198091050
};
int LevelCount[100] = {0};
int get_account(DB *db, const DBT *pkey, const DBT *pdata, DBT *ikey) 
{
	//给定一个完整的buffer，得到account作为索引
	memset( ikey, 0, sizeof( DBT ) );
	TRoleDataNew *pRoleData = (TRoleDataNew *)pdata->data;

	ikey->data = pRoleData->BaseInfo.caccname;
	ikey->size = strlen( pRoleData->BaseInfo.caccname ) + 1;

	return 0;
}

void DBTran()
{
	ZDBTable DBOld("database", "roledb");
	if(!DBOld.open()){cout<<"打开旧数据库错误！";return;};
	ZDBTable DBNew("datanew", "roledb");
	DBNew.addIndex( get_account );

	if(!DBNew.open()){cout<<"打开新数据库错误！";return;};
	cout<<"打开数据库成功，开始转换..."<<endl;


	int i=0;
	int aCount=0;
	//数据库遍历，转换
	char* aBuffer;
	int size;
	char* aKey;
	int Keysize;

	aBuffer = DBOld.GetRecord(size, ZDBTable::cpFirst);
	aKey = DBOld.GetRecord_key(Keysize, ZDBTable::cpCurrent);
	while(aBuffer)
	{
		if(i>100)
		{
			i=0;cout<<'.';
		}else{
			++i;
		}
		++aCount;

		TRoleData* aRoleBuf = (TRoleData*)aBuffer;
		char* aNewBuffer = new char[64 * 1024];
		int aNewSize = sizeof(TRoleDataNew) + sizeof(TDBSkillData) * 3 - 1;	//新角色数据的大小
		TRoleDataNew* aRoleBufNew = (TRoleDataNew*)aNewBuffer;
		memset(aNewBuffer,0,64 * 1024);
		
		//导出（计算）经验等项目
		int aLevel = aRoleBuf->BaseInfo.ifightlevel;
		int aProp = aRoleBuf->BaseInfo.ifiveprop;
		
		if(aLevel > 80)	//如果人物大于80级就设为80级
		{
			cout<<endl<<aRoleBuf->BaseInfo.szName<<" 该人物"<<aLevel<<"级，设为80级。"<<endl;
			aLevel = 80;
		}
		++LevelCount[aLevel];

		memcpy(aRoleBufNew->BaseInfo.caccname, aRoleBuf->BaseInfo.caccname, 32);//账号名
		memcpy(aRoleBufNew->BaseInfo.szName, aRoleBuf->BaseInfo.szName, 32);	//角色名
		memcpy(aRoleBufNew->BaseInfo.szAlias, aRoleBuf->BaseInfo.szAlias, 32);	//别名
		aRoleBufNew->BaseInfo.dwId = aRoleBuf->BaseInfo.dwId;				//ID
		aRoleBufNew->BaseInfo.bSex = aRoleBuf->BaseInfo.bSex;					//性别
		aRoleBufNew->BaseInfo.ifightlevel = aLevel;								//等级
		aRoleBufNew->BaseInfo.ifiveprop = aProp;								//五行属性
		
		aRoleBufNew->BaseInfo.fightexp = ExpTable[aLevel - 1];
		aRoleBufNew->BaseInfo.ileftprop = (aLevel-1) * 5;		//潜力
		aRoleBufNew->BaseInfo.ileftfight = aLevel - 1;			//技能

		switch(aProp)
		{
			case 0://金
			{
				aRoleBufNew->BaseInfo.ipower = 35;		//力量
				aRoleBufNew->BaseInfo.iagility = 25;		//身法
				aRoleBufNew->BaseInfo.iouter = 25;		//外功
				aRoleBufNew->BaseInfo.iinside = 15;		//内功

				aRoleBufNew->BaseInfo.imaxlife = 204 + 4 * (aLevel-1);		//最大生命
				aRoleBufNew->BaseInfo.imaxinner = 16 + 1 * (aLevel-1);		//最大内力
				aRoleBufNew->BaseInfo.imaxstamina = 100 + 1 * (aLevel-1);		//最大体力

				aRoleBufNew->BaseInfo.irevivalid = 20;		//重生点
				aRoleBufNew->BaseInfo.irevivalx = 10;
				aRoleBufNew->BaseInfo.irevivaly = 0;
				break;
			}
			case 1://木
			{
				aRoleBufNew->BaseInfo.ipower = 20;		//力量
				aRoleBufNew->BaseInfo.iagility = 35;		//身法
				aRoleBufNew->BaseInfo.iouter = 20;		//外功
				aRoleBufNew->BaseInfo.iinside = 25;		//内功

				aRoleBufNew->BaseInfo.imaxlife = 103 + 3 * (aLevel-1);		//最大生命
				aRoleBufNew->BaseInfo.imaxinner = 77 + 2 * (aLevel-1);		//最大内力
				aRoleBufNew->BaseInfo.imaxstamina = 100 + 1 * (aLevel-1);		//最大体力

				aRoleBufNew->BaseInfo.irevivalid = 53;		//重生点
				aRoleBufNew->BaseInfo.irevivalx = 19;
				aRoleBufNew->BaseInfo.irevivaly = 0;
				break;
			}
			case 2://水
			{
				aRoleBufNew->BaseInfo.ipower = 25;		//力量
				aRoleBufNew->BaseInfo.iagility = 25;		//身法
				aRoleBufNew->BaseInfo.iouter = 25;		//外功
				aRoleBufNew->BaseInfo.iinside = 25;		//内功

				aRoleBufNew->BaseInfo.imaxlife = 153 + 3 * (aLevel-1);		//最大生命
				aRoleBufNew->BaseInfo.imaxinner = 77 + 2 * (aLevel-1);		//最大内力
				aRoleBufNew->BaseInfo.imaxstamina = 100 + 1 * (aLevel-1);		//最大体力

				aRoleBufNew->BaseInfo.irevivalid = 99;		//重生点
				aRoleBufNew->BaseInfo.irevivalx = 43;
				aRoleBufNew->BaseInfo.irevivaly = 0;
				break;
			}
			case 3://火
			{
				aRoleBufNew->BaseInfo.ipower = 30;		//力量
				aRoleBufNew->BaseInfo.iagility = 20;		//身法
				aRoleBufNew->BaseInfo.iouter = 30;		//外功
				aRoleBufNew->BaseInfo.iinside = 20;		//内功

				aRoleBufNew->BaseInfo.imaxlife = 213 + 3 * (aLevel-1);		//最大生命
				aRoleBufNew->BaseInfo.imaxinner = 41 + 1 * (aLevel-1);		//最大内力
				aRoleBufNew->BaseInfo.imaxstamina = 100 + 1 * (aLevel-1);		//最大体力

				aRoleBufNew->BaseInfo.irevivalid = 100;		//重生点
				aRoleBufNew->BaseInfo.irevivalx = 45;
				aRoleBufNew->BaseInfo.irevivaly = 0;
				break;
			}
			case 4://土
			{
				aRoleBufNew->BaseInfo.ipower = 20;		//力量
				aRoleBufNew->BaseInfo.iagility = 15;		//身法
				aRoleBufNew->BaseInfo.iouter = 25;		//外功
				aRoleBufNew->BaseInfo.iinside = 40;		//内功

				aRoleBufNew->BaseInfo.imaxlife =76 + 1 * (aLevel-1);		//最大生命
				aRoleBufNew->BaseInfo.imaxinner = 163 + 1 * (aLevel-1);		//最大内力
				aRoleBufNew->BaseInfo.imaxstamina = 100 + 3 * (aLevel-1);		//最大体力

				aRoleBufNew->BaseInfo.irevivalid = 101;		//重生点
				aRoleBufNew->BaseInfo.irevivalx = 47;
				aRoleBufNew->BaseInfo.irevivaly = 0;
				break;
			}
		}

		aRoleBufNew->BaseInfo.icurlife = aRoleBufNew->BaseInfo.imaxlife;	//当前生命
		aRoleBufNew->BaseInfo.icurinner = aRoleBufNew->BaseInfo.imaxinner;	//当前内力
		aRoleBufNew->BaseInfo.icurstamina = aRoleBufNew->BaseInfo.imaxstamina;	//当前体力
		
		//其他，每个角色都一样
		aRoleBufNew->BaseInfo.ileadlevel  = 1;		//统率力等级
		aRoleBufNew->BaseInfo.cUseRevive = 1;
		aRoleBufNew->BaseInfo.cPkStatus = 1;
		aRoleBufNew->BaseInfo.nSect = -1;
		aRoleBufNew->BaseInfo.nFirstSect = -1;
		aRoleBufNew->BaseInfo.isectrole = 61;
		aRoleBufNew->BaseInfo.imoney = 50;

		aRoleBufNew->dwDataLen = aNewSize;
		aRoleBufNew->bBaseNeedUpdate = 1;
		//初始化偏移值
		aRoleBufNew->dwTaskOffset = sizeof(TRoleDataNew)-1;
		aRoleBufNew->dwLSkillOffset = sizeof(TRoleDataNew)-1;
		aRoleBufNew->dwFSkillOffset = sizeof(TRoleDataNew)-1;
		aRoleBufNew->dwItemOffset = sizeof(TRoleDataNew)-1;
		aRoleBufNew->dwFriendOffset = sizeof(TRoleDataNew)-1;

		//新建技能点
		aRoleBufNew->nFightSkillCount = 3;
		TDBSkillData* SkillData;
		SkillData = (TDBSkillData*)aRoleBufNew->pBuffer;
		SkillData->m_nSkillId = 53;
		SkillData->m_nSkillLevel = 1;
		++SkillData;
		SkillData->m_nSkillId = 1;
		SkillData->m_nSkillLevel = 1;
		++SkillData;
		SkillData->m_nSkillId = 2;
		SkillData->m_nSkillLevel = 1;

		//拷贝到新的数据库上
		if(!DBNew.add(aKey,Keysize,aNewBuffer,aNewSize))
		{
			cout<<endl<<"插入“"<<aKey<<"”失败"<<endl;
			--aCount;
		}
		//清除当前缓存，游标指向下一个记录
		delete [] aNewBuffer;
		delete [] aBuffer;
		delete [] aKey;

		aBuffer = DBOld.GetRecord(size,ZDBTable::cpNext);
		aKey = DBOld.GetRecord_key(Keysize, ZDBTable::cpCurrent);
	}

	DBOld.close();
	DBNew.close();
	
	cout<<endl<<"数据库转换成功~~"<<endl;
	cout<<"共有"<<aCount<<"个记录转换"<<endl;

	cout<<"各个等级人数：";
	for(i=0;i<100;++i)
	{
		cout<<i<<"级："<<LevelCount[i]<<"人"<<endl;
	}
}

int main(int argc, char **argv)
{
	cout<<"-=角色数据库转换工具=-"<<endl;
	cout<<endl;
	cout<<"请先确认数旧据库放在database目录内，并命名为roledb"<<endl;
	cout<<"新数据库将保存在datanew目录内，名称为roledb"<<endl;
	cout<<"开始转换请安y，其他键退出：";
	char charin;cin>>charin;
	cout<<endl;
	if (charin!='y')return 0;
	
	DBTran();
/*

	ZDBTable DBNew("datanew", "roledb");
	int size;
	if(!DBNew.open()){cout<<"打开新数据库错误！";return 1;};
	char* aBuffer = DBNew.GetRecord(size, ZDBTable::cpFirst);
	while(aBuffer)
	{
		TRoleDataNew* pRoleData = (TRoleDataNew*)aBuffer;
		cout<<pRoleData->BaseInfo.ifightlevel<<"||"<<pRoleData->BaseInfo.szName<<"||"<<pRoleData->BaseInfo.fightexp<<"||"<<pRoleData->BaseInfo.imaxlife<<endl;
		TDBSkillData* aSkillData;
		aSkillData = (TDBSkillData*)pRoleData->pBuffer;
		cout<<aSkillData->m_nSkillId<<"_"<<aSkillData->m_nSkillLevel<<"||";
		++aSkillData;
		cout<<aSkillData->m_nSkillId<<"_"<<aSkillData->m_nSkillLevel<<"||";
		++aSkillData;
		cout<<aSkillData->m_nSkillId<<"_"<<aSkillData->m_nSkillLevel<<endl;
		delete[]aBuffer;
		aBuffer = DBNew.GetRecord(size, ZDBTable::cpNext);
	}
	DBNew.close();

*/

	int a;cin>>a;
	return 0;
}