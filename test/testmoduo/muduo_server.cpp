/*
    Mudo网络库主要给用户提供了两个主要的类
    TcpServer:用于编写服务器程序
    TcpClient:用于编写客户端程序
    epoll+线程池：
        好处：能够把网络IO代码和业务代码区分开
                               用户的连接和断开  用户的可读写事件
    
*/
//回调函数被当作参数给别的函数使用

//回调函数：让系统在恰当的时机通知应用程序做一件事。
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<iostream>
#include<string>
#include<functional>//绑定器
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;//参数占位符
//基于muduo网络库开发的服务器程序
//1 组合TCPserverd对象
//2创建EventLoop事件循环对象的指针
//3明确TCPserver构造函数需要什么参数，输出chatserver的构造函数
//4在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
//5设置合适的服务端线程数量，muduo库会自己分配io线程和worker线程
class ChatServer
{
    public:
        ChatServer(EventLoop *loop,//事件循环
                    const InetAddress &listenAddr,//ip地址+端口
                    const string &nameArg)//服务器的名字
                    :_server(loop,listenAddr,nameArg)
                    ,_loop(loop)
        {
            //给服务器注册用户连接的创建和断开回调
            //为什么绑定，是因为成员方法onConnection会有一个隐藏的this指针，_1，表示实际使用的
            //时候所表示的参数，（this,绑定到onConnection）
            //当有客户端连接创建和断开的时候，调用onConnection(具体实现在muduo库)
            _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));
            //给服务器注册用户读写事件回调
            _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));

            //设置服务器的线程数量
            _server.setThreadNum(4);//一个IO，3个worker
        }

        //开启事件循环
        void start()
        {
            _server.start();
        }
    private:
    //专门处理用户的连接创建和断开 epoll listened accept 
    void onConnection(const TcpConnectionPtr&conn)
    {
        if(conn->connected())//是否已经连接
        {
            //打印IP地址+端口号
            cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()
            <<"state:online"<<endl;
        }
        else{
            cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()
            <<"state:offonline"<<endl;
            conn->shutdown();//close(fd)
        
            //_loop->quit();//连接断开
        }
    }
    //专门用于处理用户的读写时间
    //连接、缓冲区、接收到数据的时间信息
    void onMessage(const TcpConnectionPtr &conn,Buffer *buffer,Timestamp time)
    {
        string buf = buffer->retrieveAllAsString();//接收到的消息
        cout<<"recv data:"<<buf<<"time:"<<time.toString()<<endl;
        conn->send(buf);//回复的消息
    }

        TcpServer _server;//1
        EventLoop *_loop;//2 epoll
    
};

int main()
{
    EventLoop loop;//epoll 事件循环
    InetAddress addr("127.0.0.1",6000);
    ChatServer server(&loop,addr,"chatserver");//创建server对象
    server.start();//启动服务  listenfd epoll_ctl=>epoll
    loop.loop();//epoll_wait,以阻塞的方式等待新用户连接，已连接用户的读写时间等

    return 0;
}

