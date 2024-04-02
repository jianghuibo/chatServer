#include"friendmodel.hpp"
#include"db.h"
void FriendModel:: insert(int userid,int friendid)
{
    char sql[1024] = {0};
    
    sprintf(sql,"insert into friend values(%d,%d)",userid,friendid);

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }


}

//返回用户好友列表 friendid name state
vector<User>FriendModel::query(int userid)
{
    char sql[1024] = {0};
    //这条SQL查询语句的意思是从名为"user"的表中选择id、name和state字段，同时与名为"friend"的表进行内连接（inner join）。
    //连接条件是"friend"表中的friendid等于"user"表中的id，并且"friend"表中的userid等于给定的用户ID。
    sprintf(sql,"select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid=%d",userid);
    vector<User>vec;
    MySQL mysql;

    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res!=nullptr)
        {
             MYSQL_ROW row;
             while((row = mysql_fetch_row(res))!=nullptr)
             {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
             }
             mysql_free_result(res);
             return vec;
        }
    }
    return vec;
}

