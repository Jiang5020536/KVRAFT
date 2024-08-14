#ifndef UTIL_H
#define UTIL_H

#include <config.h>
#include <string>
#include <chrono>
#include <cstdlib>      //EXIT_FAILURE
#include <mutex>
#include <arpa/inet.h>  //socket
#include <queue>
#include <random>
#include <thread>
#include <functional>  //functional
#include <iostream>  //cerr
#include <condition_variable>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/access.hpp>
#include <sstream>

template<class F>       //Rall风格的资源管理，作用域结束后自动执行一个函数(Deffer pattern 延迟模式，SCOPR GUARD 作用域守卫)
class DeferClass
{
private:
    F m_func;
public:
    DeferClass(F&& f) : m_func(std::forward<F>(f)) {}
    DeferClass(const F& f) : m_func(f) {}
    DeferClass(const DeferClass& e) = delete;
    DeferClass &operator=(const DeferClass& e) = delete;
    ~DeferClass() {m_func();}
};

#define _CONCAT(a,b) a##b   //将a和b连在一起
#define _MAKE_DEFFER_(line) DeferClass _CONCAT(defer_placeholder, line) = [&]()

#undef DEFER
#define DEFER _MAKE_DEFFER_(__LINE__)

void DPrintf(const char* format, ...); //格式化带调试的函数

void myAssert(bool condition, std::string message = "Assertion failed!");

template<typename... Args>
std::string format(const char* format_str, Args... args){
    std::stringstream ss;
    int _[] = {((ss << args), 0)...};
    (void)_;
    return ss.str();
}

std::chrono::_V2::system_clock::time_point now();
std::chrono::milliseconds getRandomizedElectionTimeout();

void sleepNMilliseconds(int N); //线程休眠时间

//*************异步写日志queue */
template<typename T>
class LockQueue
{
   
public:
    void Push(const T& data) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(data);
        m_condvariable.notify_one();
    }

    T pop(){
        std::unique_lock<std::mutex> lock(m_mutex);
        while(m_queue.empty()){
            m_condvariable.wait(lock);
        }
        T data = m_queue.front();
        m_queue.pop();
        return data;
    }
private:
    std::mutex m_mutex;
    std::queue<T> m_queue;
    std::condition_variable m_condvariable;

};

class Op{
public:
    std::string Operation;
    std::string Key;
    std::string Value;
    std::string ClientId;
    int RequestId;

public:
    std::string asString() const{
        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        oa << *this;
        return ss.str();
    }
    bool parseFromString(std::string str){
        try {
        std::stringstream iss(str);
        boost::archive::text_iarchive ia(iss);

        // 尝试将字符串反序列化为对象
        ia >> *this;

        // 如果反序列化成功，返回 true
        return true;
        } catch (const boost::archive::archive_exception& e) {
            // 如果捕获到反序列化异常，输出错误信息并返回 false
            std::cerr << "Failed to parse string: " << e.what() << std::endl;
            return false;
        } catch (const std::exception& e) {
            // 捕获其他可能的异常
            std::cerr << "An error occurred: " << e.what() << std::endl;
            return false;
        } catch (...) {
            // 捕获所有其他未被捕获的异常
            std::cerr << "An unknown error occurred during parsing." << std::endl;
            return false;
        }
    } 
public:
    friend std::ostream& operator <<(std::ostream& os, const Op& obj){
        os << "[MyClass:Operation{" + obj.Operation + "},Key{" + obj.Key + "},Value{" + obj.Value + "},ClientId{" +
            obj.ClientId + "},RequestId{" + std::to_string(obj.RequestId) + "}";  // 在这里实现自定义的输出格式
        return os;
    }
    
private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version){
        ar& Operation;
        ar& Key;
        ar& Value;
        ar& ClientId;
        ar& RequestId;
    }

};

const std::string ok = "OK";
const std::string ErrNoKey = "ErrNoKey";
const std::string ErrWrongLeader = "ErrWrongLeader";



//获取端口号
bool isReleasePort(unsigned short usPort);

bool getReleasePort(short& port);






#endif
