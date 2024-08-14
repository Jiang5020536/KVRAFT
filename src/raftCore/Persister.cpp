#include <Persister.h>
#include <util.h>


void Persister::Save(cosnt std::string raftstate, const std::string Snapshot){
    std::lock_guard<std::mutex> lg(m_mtx);
    clearRaftStateAndSnapshot();
    m_raftStateOutStream << raftstate;
    m_snapshotOutStream  << Snapshot;
}

std::string Persister::ReadSnapshot(){
    std::lock_guard<std::mutex> lg(m_mtx);
    if(m_snapshotOutStream.is_open()){
        m_snapshotOutStream.close();
    }

    DEEF{
        m_snapshotOutStream.open(m_snapshotFileName); //默认是追加
    };
    std::fstream ifs(m_snapshotFileName, std::ios_base::in);        //以输入模式打开，不存在则操作时报
    if(!ifs.good()){
        retrun "";
    }
    std::string snapshot;
    ifs >> snapshot;
    ifs.close();
    return snapshot;
}

void Persister::SaveRaftState(const std::string &data) {        //注意这里传入的是字符串
  std::lock_guard<std::mutex> lg(m_mtx);
  // 将raftstate和snapshot写入本地文件
  clearRaftState();
  m_raftStateOutStream << data;
  m_raftStateSize += data.size();
}

long long Persister::RaftStateSize() {
  std::lock_guard<std::mutex> lg(m_mtx);

  return m_raftStateSize;
}

std::string Persister::ReadRaftState() {
  std::lock_guard<std::mutex> lg(m_mtx);

  std::fstream ifs(m_raftStateFileName, std::ios_base::in);
  if (!ifs.good()) {
    return "";
  }
  std::string snapshot;
  ifs >> snapshot;
  ifs.close();
  return snapshot;
}




Persister::Persister(const int me) : 
                    m_raftStateFileName("raftstatePersist" + std::to_string(me) + ".txt"),
                    m_snapshotFileName("snapshotPersist" + std::to_string(me) + ".txt"),
                    m_raftStateSize(0){
    bool fileOpenFlag = true;

    //检查文件能否打得开
    std::fstream file(m_raftStateFileName, std::ios::out | std::ios::trunc);    //out以写入方式打开，不存在则创建，存在则准备好接受新的数据
    if(file.is_open()){                                                         //trunc文件已存在则清空
        file.close();
    }else{
        fileOpenFlag = false;
    }

    file = std::fstream(m_snapshotFileName, std::ios::out | std::ios::trunc);
    if (file.is_open()) {
        file.close();
    } else {
        fileOpenFlag = false;
    }
    if(!fileOpenFlag){
        DPrintf("[func-Persister::Persister] file open error");
    }

    //流的绑定
    m_raftStateOutStream.open(m_raftStateFileName);
    m_snapshotOutStream.open(m_snapshotFileName);

}


Persister::~Persister(){
   if (m_raftStateOutStream.is_open()) {
    m_raftStateOutStream.close();
  }
  if (m_snapshotOutStream.is_open()) {
    m_snapshotOutStream.close();
  } 
}

void Persister::clearRateState(){
    m_raftStateSize = 0;
    if(m_raftStateOutStream.is_open()){
        m_raftStateOutStream.close();
    }
    m_raftStateOutStream.open(m_raftStateFileName, std::ios::out | std::ios::trunc);    //重新打开流，并清空
}

void Persister::clearSnapshot(){
    if (m_snapshotOutStream.is_open()) {
        m_snapshotOutStream.close();
    }
  m_snapshotOutStream.open(m_snapshotFileName, std::ios::out | std::ios::trunc);
}

void Persister::clearRaftStateAndSnapshot() {
  clearRaftState();
  clearSnapshot();
}
