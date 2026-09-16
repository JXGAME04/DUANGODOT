#ifndef __GMCONSTS_H__
#define __GMCONSTS_H__

#define MAX_PING				999999	//ping操作失败时返回的数值
#define PING_TIMEOUT			2000	//ping操作允许的最大延迟,必须是 GM_TIMER_INTERVAL 的整数倍
#define SYSTEM_TIMEOUT			2000	//账号服务器获取信息的允许延迟
#define GM_TIMER_INTERVAL		1000	//GM 定时控制的核心
#define GM_PING_TIMEOUT_INTER	(PING_TIMEOUT / GM_TIMER_INTERVAL)

#define GM_INTERCOUNT_PING		58		//GM 定时 Ping 间隔
#define GM_INTERCOUNT_SHUTUP	60		//禁言定时处理间隔(一分钟)
#define GM_INTERCOUNT_JAIL		60		//天牢定时处理间隔(一分钟)
#define GM_INTERCOUNT_ONLINE	10		//在线人数更新间隔
#define GM_INTERCOUNT_REGISTER	30		//注册人数更新间隔

#define IS_GM_INTERCOUNT_REACHED(nTickCount, cInterval)				\
				(((cInterval) / 2) == ((nTickCount) % (cInterval)))	
#define IS_GM_INTERCOUNT_REACHED_ADD(nTickCount, cInterval, cAdd)	\
				(((((cInterval) / 2) + (cAdd)) % (cInterval)) == ((nTickCount) % (cInterval)))
#define IS_GM_INTERCOUNT_REACHED_EX(nTickCount, cInterval, cTest)	\
				(((cTest) % (cInterval)) == ((nTickCount) % (cInterval)))

#define GM_DB_NAME				"GMPrivate.mdb"
#define PING_MAX_RETRY			3	//连续几次 ping 失败后计为断线
									//PING_MAX_RETRY * GM_PING_TIMEOUT_INTER 必须
									//小于 GM_INTERCOUNT_PING
#endif //__GMCONSTS_H__
