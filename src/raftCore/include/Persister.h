#ifndef SKIP_LIST_ON_RAFT_PERSISTER_H
#define SKIP_LIST_ON_RAFT_PERSISTER_H

#include <mutex>
#include <fstream>   //oftream

class Persister{
private:
    std::mutex m_mtx;
    std::string m_raftState;
    std::string m_snapshot;

    const std::string m_raftStateFileName;      //文件名
    const std::string m_snapshotFileName;       //快照文件名
    
    //两个输出流
    std::ofstream m_raftStateOutStream;
    std::ofstream m_snapshotOutStream;

    long long m_raftStateSize;

public:
    void Save(std::string raftstate, std::string Snapshot);
    std::string ReadSnapshot();
    void SaveRateState(const std::string& date);
    long long RaftStateSize();
    std::string ReadRaftState();
    explicit Persister(int me);     //防止隐式转换
    ~Persister();

private:
    void clearRateState();
    void clearSnapshot();
    void clearRateStateAndSnapshot();
};

#endif