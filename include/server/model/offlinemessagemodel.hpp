#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include<string>
using namespace std;
#include<vector>
//提供离线消息表的操作接口方法
class OfflineMegModel
{
public:
    //存储用户的离线消息
    void insert(int userid,string msg);

    //删除用户信息
    void remove(int userid);

    //查询用户的离线消息
    vector<string>query(int userid);
};
 

#endif