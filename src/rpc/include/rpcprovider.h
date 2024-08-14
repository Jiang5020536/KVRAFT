#pragma once
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <google/protobuf/descriptor.h>
#include "google/protobuf/service.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/message.h>
#include <google/protobuf/io/coded_stream.h>

#include <string>
#include <iostream>
#include <unordered_map>



class RpcProvider{
public:
    //发布rpc方法的函数接口
    void NotifyService(google::protobuf::Service *service);

    //启动Rpc服务节点，开始提供Rpc远程调用服务
    void Run(int nodeIndex, short port);



private:
    muduo::net::EventLoop m_eventLoop;
    std::shared_ptr<muduo::net::TcpServer> m_muduo_server;
    struct ServiceInfo
    {
        google::protobuf::Service *m_service;
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> m_methodMap;
    };
    std::unordered_map<std::string, ServiceInfo> m_serviceMap;          //服务里面有许多方法
    
    //新的socket连接回调
    void OnConnection(const muduo::net::TcpConnectionPtr&);
    // 已建立连接用户的读写事件回调
    void OnMessage(const muduo::net::TcpConnectionPtr &, muduo::net::Buffer *, muduo::Timestamp);

    // Closure的回调操作，用于序列化rpc的响应和网络发送
    void SendRpcResponse(const muduo::net::TcpConnectionPtr &, google::protobuf::Message *);

public:
    ~RpcProvider();
};