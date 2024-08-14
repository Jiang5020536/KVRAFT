#ifndef RAFTRPC_H
#define RAFTRPC_H

#include "raftRPC.pb.h"

class RaftRpcUtil {
 private:
  raftRpcProctoc::raftRpc_Stub *stub_;      //就像一个代理

 public:
  //主动调用其他节点的三个方法,可以按照mit6824来调用，但是别的节点调用自己的好像就不行了，要继承protoc提供的service类才行
  bool AppendEntries(raftRpcProctoc::AppendEntriesArgs *args, raftRpcProctoc::AppendEntriesReply *response);
  bool InstallSnapshot(raftRpcProctoc::InstallSnapshotRequest *args, raftRpcProctoc::InstallSnapshotResponse *response);
  bool RequestVote(raftRpcProctoc::RequestVoteArgs *args, raftRpcProctoc::RequestVoteReply *response);
  //响应其他节点的方法
  /**
   *
   * @param ip  远端ip
   * @param port  远端端口
   */
  RaftRpcUtil(std::string ip, short port);
  ~RaftRpcUtil();
};

#endif