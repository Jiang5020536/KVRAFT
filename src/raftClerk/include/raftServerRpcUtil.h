#ifndef RAFTSERVERRPC_H
#define RAFTSERVERRPC_H

#include "kvServerRPC.pb.h"
#include "mprpccontroller.h"
#include "mprpcchannel.h"


class raftServerRpcUtil{
private:
    raftKVRpcProctoc::kvServer_Stub* stub;

public:
    //主动调用其他节点的三个方法,别的调用自己的就不行，要继承
    //响应其他节点的方法
    bool Get(raftKVRpcProctoc::GetArgs* GetArgs, raftKVRpcProctoc:;GetReply* reply);
    bool PutAppend(raftKVRpcProctoc::PutAppendArgs* args, raftKVRpcProctoc::PutAppendReply* reply);

    raftServerRpcUtil(std::string ip, short port);
    ~raftServerRpcUtil();
}
#endif
