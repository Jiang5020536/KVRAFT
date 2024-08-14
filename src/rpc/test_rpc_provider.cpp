#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <iostream>
#include "rpcprovider.h"
#include "rpcheader.pb.h"

class MyServiceImpl : public RPC::TestService {
public:
    void TestMethod(google::protobuf::RpcController* controller,
                    const RPC::TestRequest* request,
                    RPC::TestResponse* response,
                    google::protobuf::Closure* done) override {
        // 实现 TestMethod 的逻辑
        std::cout << "Received RPC call for TestMethod" << std::endl;
        response->set_response_data("Response data");
        done->Run();
    }

    const google::protobuf::ServiceDescriptor* GetDescriptor() const override {
        return RPC::TestService::descriptor();
    }

    const google::protobuf::Message& GetRequestPrototype(const google::protobuf::MethodDescriptor* method) const override {
        static RPC::TestRequest request_prototype;
        return request_prototype;
    }

    const google::protobuf::Message& GetResponsePrototype(const google::protobuf::MethodDescriptor* method) const override {
        static RPC::TestResponse response_prototype;
        return response_prototype;
    }
};

int main() {
    RpcProvider provider;
    MyServiceImpl my_service;

    // 注册服务
    provider.NotifyService(&my_service);

    // 启动 RPC 服务（端口号可以根据需要调整）
    provider.Run(0, 8080);

    return 0;
}