#include "kvServer.h"
#include <rpcprovider.h>
#include <mprpcconfig.h>

void KvServer::KvServer(int me, int maxraftstate, std::string nodeInforFileName, short port) : 
                            m_skipList(6){
    std::shared_ptr<Persister> persister = std::make_shared<Persister>(me);
    
    m_me = me
    m_maxRaftState = maxraftstate;
    applyChan = std::make_shared<LockQueue<ApplyMsg>>();
    m_raftNode = std::make_shared<Raft>();
    ////////////////clerk层面 kvserver开启rpc接受功能
    //    同时raft与raft节点之间也要开启rpc功能，因此有两个注册
    std::thread t([this, port]() -> void {
    // provider是一个rpc网络服务对象。把UserService对象发布到rpc节点上
        RpcProvider provider;
        provider.NotifyService(this);
        provider.NotifyService(this->m_raftNode.get());  // todo：这里获取了原始指针，后面检查一下有没有泄露的问题 或者 shareptr释放的问题
        // 启动一个rpc服务发布节点   Run以后，进程进入阻塞状态，等待远程的rpc调用请求
        provider.Run(m_me, port);
        });
    t.detach();

    std::cout << "raftServer node:" << m_me << " start to sleep to wait all ohter raftnode start!!!!" << std::endl;
    sleep(6);
    std::cout << "raftServer node:" << m_me << " wake up!!!! start to connect other raftnode" << std::endl;
    Mprpcconfig config;
    config.LoadConfigFile(nodeInforFileName.c_str());       
    std::vector<std::pair<std::string, short>> ipPortVt;
    for(int i = 0; i <INT_MAX-1;i++){
        std::string node  = "node" + std::to_string(i);
        std::string nodeIp = config.Load(node + "ip");
        std::string nodePortStr = config.Load(node + "port");
        if(nodeIp.empty()){
            break;
        }
        ipPortVt.emplace_back(nodeIp, atoi(nodePortStr.c_str()));
    }
    std::vector<std::shared_ptr<RaftRpcUtil>> servers;
    for(int i = 0; i < ipPortVt.size(); i++){
        if(i == m_me){
            servers.push_back(nullptr);
            continue;
        }
        std::string otherNodeIp = ipPortVt[i].first;
        short otherNodePort = ipPortVt[i].second;
        auto *rpc = new RaftRpcUtil(otherNodeIp, otherNodePort);        //同其他人建立的代理
        servers.push_back(std::shared_ptr<RaftRpcUtil>(rpc));
        std::cout << "node" << m_me << " 连接node" << i << "success!" << std::endl;
    }
    sleep(ipPortVt.size() - me);  //等待所有节点相互连接成功，再启动raft
    m_raftNode->init(servers, m_me, persister, applyChan);

}

void KvServer::DprintfKVDB(){
    if(!Debug){
        return;
    }
    std::lock_guard<std::mutex> lg(m_mtx);
    DEEF{
        m_skipList.display_list();
    };
}

void KvServer::ExecuteAppendOpOnKVDB(Op op){
    m_mtx.lock();
    m_skipList.insert_set_element(op.Key, op.Value);
    m_lastRequestId[op.ClientId] = op.RequestId;
    m_mtx.unlock();
    DprintfKVDB();
}

void KvServer::ExecuteGetOpOnKVDB(Op op, std::string *value, bool *exist){
    m_mtx.lock();
    *value = "";
    *exist = false;
    if(m_skipList.search_element(op.Key, *value)){
        *exist = true;
    }
    m_lastRequestId[op.ClientId] = op.RequestId;
    m_mtx.unlock();

    DprintfKVDB();
}

void KvServer::ExecutePutOpOnKVDB(Op op){
    m_mtx.lock();
    m_skipList.insert_set_element(op.Key, op.Value);
    m_lastRequestId[op.ClientId] = op.RequestId;
    m_mtx.unlock();
    DprintfKVDB();
}

void KvServer::Get(const raftKVRpcProctoc::GetArgs *args, raftKVRpcProctoc::GetReply *reply){
    Op op;
    op.operator = "Get";
    op.Key = args->key();
    op.Value = "";
    op.ClientId = args->clientid();
    op.RequestId = args->requestid();
    int raftIndex = -1;
    int _ = -1;
    m_raftNode->Start(op, &raftIndex, &_, &isLeader);   //相当于把这个命令当道raft节点去了
    if(!isLeader){
        reply->set_err(ErrWrongLeader);
        return;
    }
    m_mtx.lock();
    if(waitApplyCh.find(raftIndex) == waitApplyCh.end()){
        waitApplyCh.insert(std::make_pair(raftIndex, new LockQueue<Op>()));
    }
    auto chForRaftIndex = waitApplyCh[raftIndex];
    m_mtx.unlock();
    Op raftCommitOp;
    if(!chForRaftIndex->timeOutPop(CONSENSUS_TIMEOUT, &raftCommitOp)){
        int _ = -1;
        bool isLeader = false;
        m_raftNode->GetState(&_, &isLeader);

        if(ifRequestDuplicate(op.ClientId, op.RequesetId) && isLeader){
            std::string value;      //对于get可以重复请求嘛？
            bool exist = false;
            ExecuteGetOpOnKVDB(op, &value, &exist);
            if (exist) {
                reply->set_err(OK);
                reply->set_value(value);
            } else {
                reply->set_err(ErrNoKey);
                reply->set_value("");
            }
        }else{
            reply->set_err(ErrWrongLeader);
        }
    }else{
    if(raftCommitOp.ClientId == op.ClientId && raftCommitOp.RequesetId == op.RequesetId){
        std::string value;
        bool exist = false;
        ExecuteAppendOpOnKVDB(op, &value, &exist);
        if (exist) {
            reply->set_err(OK);
            reply->set_value(value);
        } else {
            reply->set_err(ErrNoKey);
            reply->set_value("");
        }
    }else{
        reply->set_err(ErrWrongLeader);
    }
    }
    m_mtx = lock();
    auto tem = waitApplyCh[raftIndex];
    waitApplyCh.erase(raftIndex);
    delete tem;
    m_mtx.unlock();
}

// clerk 使用RPC远程调用
void PutAppend(const raftKVRpcProctoc::PutAppendArgs *args, raftKVRpcProctoc::PutAppendReply *reply); // 处理客户端的 PUT 或 APPEND 请求，通过 RPC 调用执行操作。
{
    Op op;
    op.Operation = args->op();
    op.Key = args->key();
    op.Value = args->value();
    op.ClientId = args->clientid();
    op.RequestId = args->requestid();
    int raftIndex = -1;
    int _ = -1;
    bool isleader = false;

    m_raftNode->Start(op, &raftIndex, &_, &isleader);       //进行分布式共识
    if (!isleader) {
        DPrintf(
        "[func -KvServer::PutAppend -kvserver{%d}]From Client %s (Request %d) To Server %d, key %s, raftIndex %d , but "
        "not leader",
        m_me, &args->clientid(), args->requestid(), m_me, &op.Key, raftIndex);

        reply->set_err(ErrWrongLeader);
        return;
    }
    DPrintf(
      "[func -KvServer::PutAppend -kvserver{%d}]From Client %s (Request %d) To Server %d, key %s, raftIndex %d , is "
      "leader ",
      m_me, &args->clientid(), args->requestid(), m_me, &op.Key, raftIndex);
    m_mtx.lock();
    if (waitApplyCh.find(raftIndex) == waitApplyCh.end()) {
        waitApplyCh.insert(std::make_pair(raftIndex, new LockQueue<Op>()));
    }
    auto chForRaftIndex = waitApplyCh[raftIndex];
    m_mtx.unlock();


    Op raftCommitOp;
    if (!chForRaftIndex->timeOutPop(CONSENSUS_TIMEOUT, &raftCommitOp)) {        //从队列弹出一个操作
        DPrintf(
            "[func -KvServer::PutAppend -kvserver{%d}]TIMEOUT PUTAPPEND !!!! Server %d , get Command <-- Index:%d , "
            "ClientId %s, RequestId %s, Opreation %s Key :%s, Value :%s",
            m_me, m_me, raftIndex, &op.ClientId, op.RequestId, &op.Operation, &op.Key, &op.Value);

        if (ifRequestDuplicate(op.ClientId, op.RequestId)) {    
        reply->set_err(OK);  // 超时了,但因为是重复的请求，返回ok，实际上就算没有超时，在真正执行的时候也要判断是否重复
        } else {
        reply->set_err(ErrWrongLeader);  ///这里返回这个的目的让clerk重新尝试
        }
    } else {
        DPrintf(
            "[func -KvServer::PutAppend -kvserver{%d}]WaitChanGetRaftApplyMessage<--Server %d , get Command <-- Index:%d , "
            "ClientId %s, RequestId %d, Opreation %s, Key :%s, Value :%s",
            m_me, m_me, raftIndex, &op.ClientId, op.RequestId, &op.Operation, &op.Key, &op.Value);
        if (raftCommitOp.ClientId == op.ClientId && raftCommitOp.RequestId == op.RequestId) {
        //可能发生leader的变更导致日志被覆盖，因此必须检查
            reply->set_err(OK);
        } else {
            reply->set_err(ErrWrongLeader);
        }
    }
  m_mtx.lock();

  auto tmp = waitApplyCh[raftIndex];
  waitApplyCh.erase(raftIndex);
  delete tmp;
  m_mtx.unlock();
}

void KvServer::ReadRaftApplyCommandLoop(){
    while(true){
        auto message = applyChan->Pop();
        DPrintf(
        "---------------tmp-------------[func-KvServer::ReadRaftApplyCommandLoop()-kvserver{%d}] 收到了下raft的消息",
        m_me);
    
        if (message.CommandValid) {
        GetCommandFromRaft(message);
        }
        if (message.SnapshotValid) {
        GetSnapShotFromRaft(message);
        }
    }
}

void KvServer::ReadSnapShotToInstall(std::string snapshot){
    if(snapshot.empty()){
        return;
    }
    parseFromString(snapshot);
}

bool KvServer::ifRequestDuplicate(std::string ClientId, int RequesetId){
    std::lock_guard<std::mutex> lg(m_mtx);
    if(m_lastRequestId.find(ClientId) == m_lastRequestId.end()){
        return false;
    }
    return RequesetId <= m_lastRequestId[ClientId];
}

void GetCommandFromRaft(ApplyMsg message){
  Op op;
  op.parseFromString(message.Command);

  DPrintf(
      "[KvServer::GetCommandFromRaft-kvserver{%d}] , Got Command --> Index:{%d} , ClientId {%s}, RequestId {%d}, "
      "Opreation {%s}, Key :{%s}, Value :{%s}",
      m_me, message.CommandIndex, &op.ClientId, op.RequestId, &op.Operation, &op.Key, &op.Value);
  if (message.CommandIndex <= m_lastSnapShotRaftLogIndex) {
    return;
  }

  // State Machine (KVServer solute the duplicate problem)
  // duplicate command will not be exed
  if (!ifRequestDuplicate(op.ClientId, op.RequestId)) {
    // execute command
    if (op.Operation == "Put") {
      ExecutePutOpOnKVDB(op);
    }
    if (op.Operation == "Append") {
      ExecuteAppendOpOnKVDB(op);
    }
    //  kv.lastRequestId[op.ClientId] = op.RequestId  在Executexxx函数里面更新的
  }
  //到这里kvDB已经制作了快照
  if (m_maxRaftState != -1) {
    IfNeedToSendSnapShotCommand(message.CommandIndex, 9);       //9都没用到
    //如果raft的log太大（大于指定的比例）就把制作快照
  }

  // Send message to the chan of op.ClientId
  SendMessageToWaitChan(op, message.CommandIndex);      //还要走等待队列嘛？？？
}

void KvServer::GetSnapShotFromRaft(ApplyMsg message) {
  std::lock_guard<std::mutex> lg(m_mtx);
  if (m_raftNode->CondInstallSnapshot(message.SnapshotTerm, message.SnapshotIndex, message.Snapshot)) {
    ReadSnapShotToInstall(message.Snapshot);
    m_lastSnapShotRaftLogIndex = message.SnapshotIndex;
  }
}


bool KvServer::SendMessageToWaitChan(const Op &op, int raftIndex) {
  std::lock_guard<std::mutex> lg(m_mtx);
  DPrintf(
      "[RaftApplyMessageSendToWaitChan--> raftserver{%d}] , Send Command --> Index:{%d} , ClientId {%d}, RequestId "
      "{%d}, Opreation {%v}, Key :{%v}, Value :{%v}",
      m_me, raftIndex, &op.ClientId, op.RequestId, &op.Operation, &op.Key, &op.Value);

  if (waitApplyCh.find(raftIndex) == waitApplyCh.end()) {
    return false;
  }
  waitApplyCh[raftIndex]->Push(op);
  DPrintf(
      "[RaftApplyMessageSendToWaitChan--> raftserver{%d}] , Send Command --> Index:{%d} , ClientId {%d}, RequestId "
      "{%d}, Opreation {%v}, Key :{%v}, Value :{%v}",
      m_me, raftIndex, &op.ClientId, op.RequestId, &op.Operation, &op.Key, &op.Value);
  return true;
}

void KvServer::IfNeedToSendSnapShotCommand(int raftIndex, int proportion) {
  if (m_raftNode->GetRaftStateSize() > m_maxRaftState / 10.0) {
    // Send SnapShot Command
    auto snapshot = MakeSnapShot();
    m_raftNode->Snapshot(raftIndex, snapshot);      //相当于去更新raft的快照信息了
  }
}

std::string KvServer::MakeSnapShot() {
  std::lock_guard<std::mutex> lg(m_mtx);
  std::string snapshotData = getSnapshotData();
  return snapshotData;
}

void KvServer::PutAppend(google::protobuf::RpcController *controller, const ::raftKVRpcProctoc::PutAppendArgs *request,
                         ::raftKVRpcProctoc::PutAppendReply *response, ::google::protobuf::Closure *done) {
  KvServer::PutAppend(request, response);
  done->Run();
}   //这里是一个虚函数，是需要进行重写的

void KvServer::Get(google::protobuf::RpcController *controller, const ::raftKVRpcProctoc::GetArgs *request,
                   ::raftKVRpcProctoc::GetReply *response, ::google::protobuf::Closure *done) {
  KvServer::Get(request, response);
  done->Run();
}
