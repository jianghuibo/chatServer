#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include<unordered_map>
#include<functional>
#include<mutex>
#include<muduo/net/TcpConnection.h>
#include"json.hpp"
#include"usermodel.hpp"
#include"groupmodel.hpp"
#include"offlinemessagemodel.hpp"
#include"friendmodel.hpp"
#include"redis.hpp"
using json = nlohmann::json;
using namespace muduo;
using namespace muduo::net;
using namespace std;
//单例模式import
//表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn,json &s,Timestamp time)>;
//聊天服务器业务类
class ChatService
{
    public:
    //获取单例对象的接口函数
    static ChatService* instance();
    //服务器异常业务处理方法
    void reset();
    //处理登录业务
    void login(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    //一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //添加好友业务
    void addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //创建群组
    void createGroup(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //加入群组
    void addChat(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //群组聊天业务
    void groupChat(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //处理注销业务
    void loginout(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //在redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int userid,string msg);
    private:
    ChatService();
    
    //存储消息ID和其对应的业务处理方法
    unordered_map<int,MsgHandler> _msgHandlerMap;//提前已经添加好
    
    //数据操作类对象
    UserModel _userModel;
    OfflineMegModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;

    //存储在线用户的通信连接       随着用户的上线下线，要保证线程的安全
    //STL容器没有考虑线程安全
    unordered_map<int,TcpConnectionPtr>_userConnMap;

    //定义互斥锁，保证_userConnMap的线程安全    
    mutex _connMutex;

    //redis操作对象
    Redis _redis;
};







#endif
