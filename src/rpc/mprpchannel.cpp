#include "mprpcchannel.h"
#include <cstring>  //c_str函数
#include "util.h"
#include "rpcheader.pb.h"
#include "mprpccontroller.h"
#include <cerrno>           //errno用于存储最后一个错误代码
#include <cstdio>           //sprint
#include <sys/socket.h>
#include <arpa/inet.h>      //处理internet地址函数 inet——portdeng 
#include <netinet/in.h>     //包含一些网络的协议的数据类型



void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method, google::protobuf::RpcController *controller, 
                                    const google::protobuf::Message *require, google::protobuf::Message *response, 
                                    google::protobuf::Closure *done){
    if(m_clientFd == -1){
        std::string errMsg;
        bool rt = newConnect(m_ip.c_str(), m_port, &errMsg);
        if(!rt){
            DPrintf("[func-MprpcChannel::CallMethod]重连接ip：{%s} port{%d}失败", m_ip.c_str(), m_port);
            controller->SetFailed(errMsg);
            return;
        }else{
            DPrintf("[func-MprpcChannel::CallMethod]连接ip：{%s} port{%d}成功", m_ip.c_str(), m_port);
        }
    }

    const google::protobuf::ServiceDescriptor* sd = method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();

    uint32_t args_size{};
    std::string args_str;
    if(require->SerializeToString(&args_str)){
        args_size = args_str.size();
    } else{
        controller->SetFailed("serialize request error!");
        return;
    }
    RPC::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    std::string rpc_header_str;
    if (!rpcHeader.SerializeToString(&rpc_header_str)) {
        controller->SetFailed("serialize rpc header error!");
        return;
    }

    std::string send_rpc_str;
    {
        google::protobuf::io::StringOutputStream string_output(&send_rpc_str);
        google::protobuf::io::CodedOutputStream coded_output(&string_output);

        coded_output.WriteVarint32(static_cast<uint32_t>(rpc_header_str.size()));
        coded_output.WriteString(rpc_header_str);
    }
        send_rpc_str += args_str;
        std::cout << "============================================" << std::endl;
        std::cout << "header_size: " << rpc_header_str.size() << std::endl;
        std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
        std::cout << "service_name: " << service_name << std::endl;
        std::cout << "method_name: " << method_name << std::endl;
        std::cout << "args_str: " << args_str << std::endl;
        std::cout << "============================================" << std::endl;


        //发送请求
        while(-1 == send(m_clientFd, send_rpc_str.c_str(), send_rpc_str.size(), 0)){
            char errtxt[512] = {0};
            sprintf(errtxt, "send error! errno:%d", errno);
            std::cout << "尝试重新连接，对方ip：" << m_ip << " 对方端口" << m_port << std::endl;
            close(m_clientFd);
            m_clientFd = -1;
            std::string errMsg;
            bool rt = newConnect(m_ip.c_str(), m_port, &errMsg);
            if (!rt) {
            controller->SetFailed(errMsg);
            return;
            }
        }

        //接受rpc请求的响应值
        char recv_buf[1024] = {0};
        int recv_size = 0;
        if (-1 == (recv_size = recv(m_clientFd, recv_buf, 1024, 0))) {
        close(m_clientFd);
        m_clientFd = -1;
        char errtxt[512] = {0};
        sprintf(errtxt, "recv error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
        }

        //反序列化
        if(!response->ParseFromArray(recv_buf, recv_size)){
            char errtxt[1050] = {0};
            sprintf(errtxt, "parse error! response_str:%s", recv_buf);
            controller->SetFailed(errtxt);
            }
}
        
bool MprpcChannel::newConnect(const char* ip, uint16_t port, string* errMsg){
        int clientfd = socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == clientfd) {
        char errtxt[512] = {0};
        sprintf(errtxt, "create socket error! errno:%d", errno);
        m_clientFd = -1;
        *errMsg = errtxt;
        return false;
        }
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(ip);
        if (-1 == connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
            close(clientfd);
            char errtxt[512] = {0};
            sprintf(errtxt, "connect fail! errno:%d", errno);
            m_clientFd = -1;
            *errMsg = errtxt;
            return false;
        }
        m_clientFd = clientfd;
        return true;
}

MprpcChannel::MprpcChannel(std::string ip, short port, bool connectNow) : m_ip(ip),m_port(port), m_clientFd(-1){
        if (!connectNow) {
            return;
        }  //可以允许延迟连接
        std::string errMsg;
        auto rt = newConnect(ip.c_str(), port, &errMsg);
        int tryCount = 3;		//重复尝试
        while (!rt && tryCount--) {
            std::cout << errMsg << std::endl;
            rt = newConnect(ip.c_str(), port, &errMsg);
        }
}

                                    
                            
    
    
    
    