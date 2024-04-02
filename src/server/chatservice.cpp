#include "chatservice.hpp"
#include"public.hpp"
#include<string>
#include<mutex>
#include"usermodel.hpp"
#include<map>
#include<muduo/base/Logging.h>
using namespace muduo;
using namespace std;
#include<vector>
void ChatService::reset()
{
    //把online状态的用户，设置为Offline
    _userModel.resetState();
}

ChatService *ChatService::instance()
{
    //静态变量是与类关联的变量，它在所有对象之间共享。每个类只有一个静态变量的副本。
    static ChatService service;
    return &service;
}

//注册消息以及对应handler的回调操作
//绑定器与回调
ChatService::ChatService()
{                                    //代表一个符合 void(const TcpConnectionPtr &conn, json &s, Timestamp time) 签名要求的可调用对象
    _msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
    _msgHandlerMap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});
    _msgHandlerMap.insert({LOGINOUT_MSG,std::bind(&ChatService::loginout,this,_1,_2,_3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG,std::bind(&ChatService::createGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG,std::bind(&ChatService::addChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG,std::bind(&ChatService::groupChat,this,_1,_2,_3)});
    //连接redis服务器
    if(_redis.connect())
    {
        //设置上报消息的回调  注册一个回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
    }

}

//处理登录业务    业务模块跟数据模块分开  id pwd 
void ChatService::login(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];
    User user = _userModel.query(id);
    if(user.getId() == id && user.getPwd() == pwd)
    {
        if(user.getState() == "online")
        {
            //该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "id is login,plese select other id";
            conn->send(response.dump());
        }
        else{
            //登录成功，记录用户连接信息     保证线程安全
            {
                lock_guard<mutex>lock(_connMutex);
                _userConnMap.insert({id,conn});//存储用户的ID和连接
            }
            //id用户登录成功后,向redis订阅channel(id)
            _redis.subscribe(id);

            //登陆成功 更新用户状态信息
            user.setState("online"); 
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;//登录成功
            response["id"] = user.getId();
            response["name"] = user.getNmae();
            //查询该用户是否有离线消息
            vector<string>vec = _offlineMsgModel.query(id);
            if(!vec.empty())
            {
                response["offlinemsg"] = vec;
                //读取该用户的离线消息后，把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }

            //查询用户的好友信息并返回
            vector<User>uservec = _friendModel.query(id);
            if(!uservec.empty())
            {
                vector<string>vec2;
                for(User &user:uservec)
                {
                    json js;//将user的信息转成字符串
                    js["id"] = user.getId();
                    js["name"] = user.getNmae();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }
            //查询用户群组信息
            vector<Group>groupuserVec = _groupModel.queryGroup(id);
            if(!groupuserVec.empty())
            {
                vector<string>groupV;
                for(Group &group:groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string>userV;
                    for(GroupUser &user:group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getNmae();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }
            
            conn->send(response.dump());
        }
    }
    else{
        //登录失败,该用户不存在，用户存在但是密码错误
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;//登录成功
        response["errmsg"] = "id or password fail";
        conn->send(response.dump());
    }
        
}

//处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];
    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if(state)
    {
        //注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;//注册成功
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    {
        //注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;//注册失败
        conn->send(response.dump());
    }

}

//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    //记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end())
    {
        //返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn,json &js,Timestamp time){
            LOG_ERROR<<"msgid:"<<msgid<<"can not find handler";
        };
    }
    else{
        return _msgHandlerMap[msgid];
    }
    
}

void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex>lock(_connMutex);
        for(auto it = _userConnMap.begin();it!=_userConnMap.end();it++)
        {
            if(it->second == conn)
            {
                //从map表中删除用户的连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    //用户注销，相当于就是下线在redis中取消订阅通道
    _redis.unsubscribe(user.getId());
    //更新用户的状态信息
    if(user.getId()!=-1)
    {
        user.setState("offline");
        _userModel.updateState(user);//根据ID
    }

}
//一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int toid = js["toid"].get<int>();

    //表示用户是否在线
    {
        lock_guard<mutex>lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end())
        {
            //toid在线，转发消息
            it->second->send(js.dump());//服务器主动推送消息给toid用户
            return;
        }
    }
    //查询toid是否在线  在线，但是不在一台服务器上
    User user = _userModel.query(toid);
    if(user.getState() == "online")
    {
        _redis.publish(toid,js.dump());
        return;
    }

    //toid不在线，存储离线消息
    _offlineMsgModel.insert(toid,js.dump());

}

//添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    //存储好友信息
    _friendModel.insert(userid,friendid);

}

//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();//n哪个用户创建群组
    string name = js["groupname"];//群名
    string desc = js["groupdesc"];//群描述

    //存储新创建的群组信息
    Group group(-1,name,desc);
    if(_groupModel.createGroup(group))
    {
        //存储群组创建人的信息
        _groupModel.addGroup(userid,group.getId(),"creator");
    }
}

//加入群组业务
void ChatService::addChat(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid,groupid,"normal");
}

//群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int>useridVec = _groupModel.queryGroupUsers(userid,groupid);
    lock_guard<mutex> lock(_connMutex);
    for(int id : useridVec)
    { 
        auto it = _userConnMap.find(id);
        if(it!=_userConnMap.end())
        {
            //转发消息
            it->second->send(js.dump());
        }
        else
        {
            User user = _userModel.query(id);
            if(user.getState() == "online")
            {
                _redis.publish(id,js.dump());
            }
            else
            {
                //存储离线群消息
                _offlineMsgModel.insert(id,js.dump());
            }
        }
    }
}

void ChatService::loginout(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex>lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it!=_userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }
    //用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    //更新状态的用户信息
    User user(userid,"","","offline");
    _userModel.updateState(user);
}

//从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid,string msg)
{
    lock_guard<mutex>lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if(it!=_userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    //存储用户的离线消息
    _offlineMsgModel.insert(userid,msg);
}
//注册消息{"msgid":1,"name":"li si","password":"666666"}
//登录消息{"msgid":1,"id":4,"password":"666666"}
//聊天消息{"msgid":5,"id":1,"from1":"zhang san","to":4,"msg":"hello222"}
//添加好友{"msgid":6,"id":4,"friendid":1}
//update user set state = 'offline'