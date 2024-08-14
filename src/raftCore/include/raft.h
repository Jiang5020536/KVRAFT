#ifndef RAFT_H
#define RAFT_H

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include "boost/any.hpp"
#include "boost/serialization/serialization.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include "ApplyMsg.h"
#include "raft.h"
#include <mytex>
#include <vector>
#include <memory>
#include "Persister.h"
#include "raftRpcUtil.h"
#include "util.h"
#include <chrono>
#include "monsoon.h"


//根据此判断网络是否正常
constexpr int Disconnected = 0;     //网络异常为disconnect， 否则为AppNormal
constexpr int AppNormal = 1;

//根据此判断当前的投票状态
constexpr int Killed = 0;
constexpr int Voted = 1;        //已经投票
constexpr int Expire = 2;       //已经过期
constexpr int Normal = 3;

class Raft : public raftRpcProctoc::raftRPC{
private:
    std::mutex m_mtx;
    std::vector<std::shared_ptr<RaftRpcUtil>> m_peers;      //同其他节点之间的通信代理
    std::shared_ptr<Persister> m_persister;                 //本结点的持久化
    int m_me;
    int m_currentTerm;
    int m_votedFor;
    std::vector<raftRpcProctoc::LogEntry> m_logs;

    int m_commitIndex;      //日志提交的索引
    int m_lastApplied;      //已经应用的

    std::vector<int> m_nextIndex;       //其他节点下一个Index
    std::vector<int> m_matchIndex;      //追踪匹配

    enum Status{ Follower, Candidate, Leader};
    Status m_status;

    std::shared_ptr<LockQueue<ApplyMsg>> applyChan;  //状态机从这里去信息

    std::chrono::_V2::system_clock::time_point m_lastResetElectionTime;     //选举
    std::chrono::_V2::system_clock::time_point m_lastResetHearBeatTime;     //心跳

    /*最后一个快照相关的信息*/
    int m_lastSnapshotIncludeIndex;
    int m_lastSnapshotIncludeTerm;

    std::unique_ptr<moosoon::IOManager> m_ioManager = nullptr;

public:
    void AppendEntries1(const raftRpcProctoc::AppendEntries *args, raftRpcProctoc::AppendEntriesReply *reply);
    void applierTicker(); //定期应用
    bool CondInstallSnapshot(int lastIncludedTerm, int lastIncludedIndex, std::string snapshot);
    void doElection();

    void doHeartBeat();
    void electionTimeOutTicker();
    std::vector<ApplyMsg> getApplyLogs();
    int getNewCommandIndex();
    void getPreLogInfo(int server, int *preIndex, int *preTerm);
    void GetState(int *tem, bool *isleader);
    void InstallSnapshot(const raftRpcProctoc::InstallSnapshotRequest *args,
                                    raftRpcProctoc::InstallSnapshotResponse *reply);
    void leaderHertBeatTicker();
    void leaderSendSnapShot(int server);
    void leaderUpdateCommitIndex();
    bool matchLog(int logIndex, int logTerm);
    void persist();
    void RequestVote(const raftRpcProctoc::RequestVoteArgs *args, raftRpcProctoc::RequestVoteReply *reply);
    bool UpToDate(int index, int term);
    int getLastLogIndex();
    int getLastLogTerm();
    void getLastLogIndexAndTerm(int *lastLogIndex, int *lastLogTerm);
    int getLogTermFromLogIndex(int logIndex);
    int GetRaftStateSize();
    int getSlicesIndexFromLogIndex(int logIndex);
    bool sendRequestVote(int server, std::shared_ptr<raftRpcProctoc::RequestVoteArgs> args,
                            std::shared_ptr<raftRpcProctoc::RequestVoteReply> reply, std::shared_ptr<int> votedNum);
    bool sendAppendEntries(int server, std::shared_ptr<raftRpcProctoc::AppendEntriesArgs> args,
                            std::shared_ptr<raftRpcProctoc::AppendEntriesReply> reply, std::shared_ptr<int> appendNums);

    void pushMesgToKvServer(ApplyMsg msg);
    void readPersist(std::string date);
    std::string persistData();
    void Start(Op conmmand, int *newLogIndex, int *newLogTerm, bool *isleader);
    void Snapshot(int index, std::string snapshot);

public:
    //重写序列化的方法
    void AppendEntries(good::protobuf::RpcController *controller, const ::raftRpcProctoc::AppendEntries *request,
                            ::raftRpcProctoc::AppendEntriesReply *response, ::google::protobuf::Closure *done) override;
    void InstallSnapshot(google::protobuf::RpcController *controller, const ::raftRpcProctoc::InstallSnapshotRequest *request,
                            :;raftRpcProctoc::InstallSnapshotResponse *response, ::google::protobuf::Closure *done) override;                 
    void RequestVote(google::protobuf::RpcController *controller, const ::raftRpcProctoc::RequestVoteArgs *request,
                   ::raftRpcProctoc::RequestVoteReply *response, ::google::protobuf::Closure *done) override;
    
public:
    void init(std::vector<std::shared_ptr<RaftRpcUtil>> peers, int me, std::shared_ptr<Persister> persister,
                    std::shared_ptr<LockQueue<ApplyMsg>> applyCh);


private:
    class BoostPersistRaftNode{     //这保存序列化的数据
        public:
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive &ar, const unsigned int version){
                ar &m_currentTerm;
                ar &m_votedFor;
                ar &m_lastSnapshotIncludeIndex;
                ar &m_lastSnapshotIncludeTerm;
                ar &m_logs;
            }
            int m_currentTerm;
            int m_votedFor;
            int m_lastSnapshotIncludeIndex;
            int m_lastSnapshotIncludeTerm;
            std::vector<std::string> m_logs;
            std::unordered_map<std::string, int> umap;
        public:
    }

}
#endif