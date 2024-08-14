#include "rpcprovider.h"
#include <unistd.h> //gethostname
#include <netdb.h> //gethostbyname
#include <arpa/inet.h>
#include <fstream>
#include "rpcheader.pb.h"
#include "util.h"

void RpcProvider::NotifyService(google::protobuf::Service *service){
    ServiceInfo service_info;
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
    std::string service_name = pserviceDesc->name();
    int methodCnt = pserviceDesc->method_count();

    std::cout << "service_name:" << service_name << std::endl;

    for (int i = 0; i < methodCnt; i++)
    {
        const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name, pmethodDesc});
    }
    service_info.m_service = service;
    m_serviceMap.insert({service_name, service_info});
}

void RpcProvider::Run(int nodeIndex, short port){
    char *ipC;
    char hname[128];
    struct hostent* hent;
    gethostname(hname, sizeof(hname)); //获取主机名字
    hent = gethostbyname(hname);       //通过主机名获取主机信息，ip
    for (int i = 0; hent->h_addr_list[i] != NULL; i++) {
      ipC = inet_ntoa(*(struct in_addr *)(hent->h_addr_list[i]));  // 地址转换成字符串
    }
    std::string ip = std::string(ipC);
    // 获取端口
    if(getReleasePort(port)) //在port的基础上获取一个可用的port，不知道为何没有效果
    {
        std::cout << "可用的端口号为：" << port << std::endl;
    }
    else
    {
        std::cout << "获取可用端口号失败！" << std::endl;
    }
    //写入 test.conf
    std::string node = "node" + std::to_string(nodeIndex);
    std::ofstream outfile;
    outfile.open("test.conf", std::ios::app);
    if(!outfile.is_open()){
        std::cout << "打开文件失败！" << std::endl;
        exit(EXIT_FAILURE);
    }
    outfile << node + "ip=" + ip << std::endl;
    outfile << node + "port=" + std::to_string(port) << std::endl;
    outfile.close();

    //创建服务器
    muduo::net::InetAddress address(ip, port);
    m_muduo_server = std::make_shared<muduo::net::TcpServer> (&m_eventLoop, address, "RpcProvider");
    m_muduo_server->setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
    m_muduo_server->setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    m_muduo_server->setThreadNum(4);

    std::cout << "RpcProvider start service at ip:" << ip << "port:" << port << std::endl;

    m_muduo_server->start();
    m_eventLoop.loop();
}

void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn){
    if(!conn->connected()){
        conn->shutdown();       //在新的连接建立或者已有连接断开才执行
    }
}
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer, muduo::Timestamp) {
    std::string recv_buf = buffer->retrieveAllAsString();

    /*这段代码通常用于从网络或文件接收二进制数据，并将其
    解析为 Protocol Buffers 消息的场景。例如，在一个 RPC 服务中，
    服务器接收到来自客户端的二进制请求数据后，需要使用这段代码来将二进制数据转换为 
    Protocol Buffers 消息对象，以便进一步处理。*/
    google::protobuf::io::ArrayInputStream array_input(recv_buf.data(), recv_buf.size());
    google::protobuf::io::CodedInputStream coded_input(&array_input);
    
    uint32_t header_size{};
    coded_input.ReadVarint32(&header_size);     //读取一个32位的变长整数

    std::string rpc_header_str;
    RPC::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;

    google::protobuf::io::CodedInputStream::Limit msg_limit = coded_input.PushLimit(header_size);   //设置一个字节限制
    coded_input.ReadString(&rpc_header_str, header_size);


    coded_input.PopLimit(msg_limit);        //回复之前的字节限制
    uint32_t args_size{};
    if(rpcHeader.ParseFromString(rpc_header_str)){
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
    }else {
    // 数据头反序列化失败
    std::cout << "rpc_header_str:" << rpc_header_str << " parse error!" << std::endl;
    return;
  }
  // 获取rpc方法参数的字符流数据
  std::string args_str;
  // 直接读取args_size长度的字符串数据
  bool read_args_success = coded_input.ReadString(&args_str, args_size);

  if (!read_args_success) {
    // 处理错误：参数数据读取失败
    return;
  }

  // 打印调试信息
     std::cout << "============================================" << std::endl;
     std::cout << "header_size: " << header_size << std::endl;
     std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
     std::cout << "service_name: " << service_name << std::endl;
     std::cout << "method_name: " << method_name << std::endl;
     std::cout << "args_str: " << args_str << std::endl;
     std::cout << "============================================" << std::endl;

  // 获取service对象和method对象
  auto it = m_serviceMap.find(service_name);
  if (it == m_serviceMap.end()) {
    std::cout << "服务：" << service_name << " is not exist!" << std::endl;
    std::cout << "当前已经有的服务列表为:";
    for (auto item : m_serviceMap) {
      std::cout << item.first << " ";
    }
    std::cout << std::endl;
    return;
  }

  auto mit = it->second.m_methodMap.find(method_name);
  if (mit == it->second.m_methodMap.end()) {
    std::cout << service_name << ":" << method_name << " is not exist!" << std::endl;
    return;
  }

  google::protobuf::Service *service = it->second.m_service;       // 获取service对象  new UserService
  const google::protobuf::MethodDescriptor *method = mit->second;  // 获取method对象  Login

  // 生成rpc方法调用的请求request和响应response参数,由于是rpc的请求，因此请求需要通过request来序列化
  google::protobuf::Message *request = service->GetRequestPrototype(method).New();
  if (!request->ParseFromString(args_str)) {
    std::cout << "request parse error, content:" << args_str << std::endl;
    return;
  }
  google::protobuf::Message *response = service->GetResponsePrototype(method).New();

  // 给下面的method方法的调用，绑定一个Closure的回调函数
  // closure是执行完本地方法之后会发生的回调，因此需要完成序列化和反向发送请求的操作
  google::protobuf::Closure *done =
      google::protobuf::NewCallback<RpcProvider, const muduo::net::TcpConnectionPtr &, google::protobuf::Message *>(
          this, &RpcProvider::SendRpcResponse, conn, response);

  // 在框架上根据远端rpc请求，调用当前rpc节点上发布的方法
  // new UserService().Login(controller, request, response, done)

  /*
  为什么下面这个service->CallMethod 要这么写？或者说为什么这么写就可以直接调用远程业务方法了
  这个service在运行的时候会是注册的service
  // 用户注册的service类 继承 .protoc生成的serviceRpc类 继承 google::protobuf::Service
  // 用户注册的service类里面没有重写CallMethod方法，是 .protoc生成的serviceRpc类 里面重写了google::protobuf::Service中
  的纯虚函数CallMethod，而 .protoc生成的serviceRpc类 会根据传入参数自动调取 生成的xx方法（如Login方法），
  由于xx方法被 用户注册的service类 重写了，因此这个方法运行的时候会调用 用户注册的service类 的xx方法
  真的是妙呀
  */
  //真正调用方法
  service->CallMethod(method, nullptr, request, response, done);
}

void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message* response){
    std::string response_str;
    if(response->SerializeToString(&response_str)){
        conn->send(response_str);
    }else{
        std::cout << "serialize response_str error!" << std::endl;
    }
}

RpcProvider::~RpcProvider() {
  std::cout << "[func - RpcProvider::~RpcProvider()]: ip和port信息:" << m_muduo_server->ipPort() << std::endl;
  m_eventLoop.quit();
  //    m_muduo_server.   怎么没有stop函数，奇奇怪怪，看csdn上面的教程也没有要停止，甚至上面那个都没有
}

