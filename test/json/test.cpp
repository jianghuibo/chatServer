#include"json.hpp"
using json = nlohmann::json;
#include<iostream>
using namespace std;
#include<string>
#include<vector>
#include<map>
string func1()
{
    json js;
    // 添加数组
    js["id"] = {1,2,3,4,5};
    // 添加key-value
    js["name"] = "zhang san";
    // 添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china";
    // 上面等同于下面这句一次性添加数组对象
    js["msg"] = {{"zhang san", "hello world"}, {"liu shuo", "hello china"}};
    cout << js << endl;
    string sendBuf = js.dump();//json对象序列化成 字符串
    return sendBuf;
}
string func3()
{
    json js;
    // 直接序列化一个vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);
    js["list"] = vec;
    // 直接序列化一个map容器
    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "华山"});
    m.insert({3, "泰山"});
    js["path"] = m;
    return js.dump();
}
int main()
{
    string recvBuf = func1();
    //数据反序列化  json字符串反序列化成数据对象（看作容器，方便访问）
    json jsbuf = json::parse(recvBuf);
    cout<<jsbuf["id"][1]<<endl;
    cout<<jsbuf["name"]<<endl;
    //cout<<jsbuf["to"]<<endl;
    cout<<jsbuf["msg"]<<endl;

    string recvBuf2 = func3();
    json jsbuf2 = json::parse(recvBuf2);
    vector<int>vec = jsbuf2["list"];//js对象里面的数据类型，直接放到vector容器中
    for(int &v:vec)
    {
        cout<<v<<endl;
    }
    map<int,string>mmp = jsbuf2["path"];
    for(auto &m : mmp)
    {
        cout<<m.first<<"  "<<m.second<<endl;
    }
    return 0;
}