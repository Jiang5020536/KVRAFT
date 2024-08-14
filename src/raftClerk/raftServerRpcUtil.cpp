#include "raftServerRpcUtil.h"
raftServerRpcUtil::raftServerRpcUtil(std::string ip, short port){
    stub_ = new raftKVRpcProctoc::kvServer_Stub(new MprpcChannel(ip, port, false));
}

raftServerRpcUtil::~raftServerRpcUtil() { delete stub; }

bool raftServerRpcUtil::Get(raftKVRpcProctoc::GetArgs *GetArgs, raftKVRpcProctoc::GetReply *reply){
    MprpcController controller;
    sutb_->Get(&controller, GetArgs, reply, nullptr);
    return !controller.Failed();
}

bool PutAppend(raftKVRpcProctoc::PutAppendArgs* args, raftKVRpcProctoc::PutAppendReply* reply){
    MprpcController controller;
    sutb_->PutAppend(&controller, GetArgs, reply, nullptr);
    if (controller.Failed()) {
    std::cout << controller.ErrorText() << endl;
    }
    return !controller.Failed();
}