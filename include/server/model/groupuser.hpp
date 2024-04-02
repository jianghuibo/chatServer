#ifndef GROUPUSER_H
#define GROUPUSER_H
#include"user.hpp"
#include<string>
//不仅仅要呈现出User的信息，还要呈现出role的信息，（创建者还是普通用户）

//
class GroupUser : public User
{
    public:
        void setRole(string role){this->role = role;}
        string getRole(){return this->role;}
    
    private:
        string role;
};




#endif