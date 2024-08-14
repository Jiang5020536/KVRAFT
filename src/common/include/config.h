#ifndef CONFIG_H
#define CONFIG_H

const bool Debug = true;
const int debugMul = 1;  //时间单位， 不同网络环境rpc速度不同，因此需要乘以一个系数
const int HeartBeaTimeout = 25 * debugMul;      //心跳时间
const int ApplyInterval = 10 * debugMul;        //日志应用到状态机

const int minRandomizedElectionTime = 300 * debugMul;   
const int maxRandomizedElectionTime = 500 * debugMul;       //防止同时选举的时间

const int CONSENSUS_TIMEOUT = 500 * debugMul;   //节点达成公式的等待时间


const int FIBER_THREAD_NUM = 1;                 // 协程库中的线程池大小
const bool FIBER_USE_CALLER_THREAD = false;     //是否使用caller_thread执行调度任务


#endif