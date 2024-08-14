#ifndef SKIP_LIST_ON_RAFT_CLERK_H
#define SKIP_LIST_ON_RAFT_CLERK_H




#include <sys/socket.h>
#include <vector>
#include <string>
#include "kvServerRPC.pb.h"
#include "aftServerRpcUtil.h"
#include "mprpcconfig.h"


class Clerk{
    private:
        std::vector<std::shared_ptr<raftServerRpcUtil>> m_servers;
        std::string m_clientId;
        int m_reuqestId;
        int m_recentLeaderId;

        //返回随机的客户id
        std::string Uuid(){
            return std::to_string(rand()) + std::to_string(rand()) + std::to_string(rand() + std::to_string(rand()))
        }
        void PutAppend(std::string key, std::string value, std::string op);

    public:
        void Init(std::string configFileName);
        std::string Get(std::string key);
        void Put(std::string key, std::string value);
        void Append(std::string key, std::string value);
    public:
        Clerk();
}

#endif