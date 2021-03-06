#ifndef _GAME_SERVER_MANAGER_H_INCLUDE_
#define _GAME_SERVER_MANAGER_H_INCLUDE_

#include <string>
#include <map>

#include "MolServerCom.h"

/** 
 * 用于存储所有的游戏服务器信息
 */
struct GameServerInfo
{
	GameServerInfo()
		: ConnId(0),GameId(0),ServerPort(0),OnlinePlayerCount(0),lastMoney(0)
	{
	}
	GameServerInfo(int ci,int gi,std::string sn,std::string si,int sp,int opc,int money)
		: ConnId(ci),GameId(gi),ServerName(sn),ServerIp(si),ServerPort(sp),OnlinePlayerCount(opc),lastMoney(money)
	{
	}

	int ConnId;                 // 连接ID
	int GameId;                 // 游戏ID
	std::string ServerName;     // 服务器名称
	std::string ServerIp;       // 服务器IP
	int ServerPort;             // 服务器端口
	int OnlinePlayerCount;      // 在线人数
	int lastMoney;                // 游戏进入最少金币值
};

class GameServerManager : public Singleton<GameServerManager>
{
public:
	/// 构造函数
	GameServerManager();
	/// 析构函数
	~GameServerManager();

	/// 添加一个游戏服务器到管理列表中
	int AddGameServer(GameServerInfo pGameServerInfo);
	/// 删除指定连接ID的服务器从管理列表中
	void DeleteGameServerByConnId(int connId);
	/// 更新指定连接ID的服务器人数
	void UpdateGameServerByConnId(int connId,int playerCount);
	/// 得到指定连接ID的服务器信息
	GameServerInfo* GetGameServerByConnId(int connId);

	/// 得到游戏服务器列表信息
	inline std::map<int,GameServerInfo>& GetGameServerInfo(void) { return m_GameServerInfoList; }
	/// 锁住游戏服务器列表
	inline void LockGameServerList(void) { m_GameServerLock.Acquire(); }
	/// 解锁游戏服务器列表
	inline void UnlockGameServerList(void) { m_GameServerLock.Release(); }

private:
	std::map<int,GameServerInfo> m_GameServerInfoList;      /**< 用于保存所有的游戏服务器信息 */
	Mutex m_GameServerLock;                                 /**< 用于保护所有的游戏服务器 */
};

#define ServerGameServerManager GameServerManager::getSingleton()

#endif