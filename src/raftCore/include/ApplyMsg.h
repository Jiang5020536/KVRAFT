#ifndef APPLYMSG_H
#define APPLYMSG_H

#include <string>

class ApplyMsg{     //定义了一个信息结构
public:
    bool CommandValid;
    std::string Commond;
    int CommandIndex;
    bool SnapshotValid;
    std::string Snapshot;
    int SnapshotTerm;
    int SnapshotIndex;

public:
    : CommandValid(false),
    Commond(),
    CommandIndex(-1),
    SnapshotValid(false),
    SnapshotTerm(-1),
    SnapshotIndex(-1){};
}

#endif