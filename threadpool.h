/*完成所有对象和功能的声明*/

#ifndef THREADPOOL_H
#define THREADPOOL_H
/*防止重复包含头文件*/

#include <vector>
#include <stddef.h>
#include <thread>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>//互斥锁
#include <condition_variable>//条件变量

/*实现选择模式*/
enum class PoolMode    //enum枚举类型//给枚举项加上类型，可杜绝枚举类型不同但同名冲突的情况
{
    MODE_FIXED,  //固定数量线程
    MODE_CACHED, //可变数量 动态增长
};

/*任务抽象 基类*/
//用户自定义任务类型，从Task继承，重写run方法，实现自定义任务处理
class Task
{
public:
    //用户自定义任务类型，从Task继承，重写run方法，实现自定义任务处理
    virtual void run() = 0; //修饰虚函数
};

class Thread
{
public:
    //启动线程
    void start();
private:
};

/*线程池类型*/
class ThreadPool
{
public:
    ThreadPool();/* 构造 */
    ~ThreadPool();/* 析构 */

    //设置线程池工作模式
    void setMode(PoolMode mode);
    
    /*
    //设置初始线程数量
    void setInitThreadSize(int size);
    */

    //设置task任务队列上限阈值
    void settaskQueMaxThreshHold(int threshhold);

    //给线程池提交任务
    void submitTask(std::shared_ptr<Task> sp);//让用户直接传智能指针进来，规避生命周期太短的任务。

    //开启线程池
    void start(int initThreadSize = 2);

    //禁止 不让用户这样 对线程池本身进行拷贝构造和赋值 //可以单独构造一个线程池对象
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    //定义线程函数
    void threadFunc();

private:
    //threadpool(/* args */);
    std::vector<Thread*> threads_;//线程列表
    size_t initThreadSize_;   //初始线程数量  //size_t 无符号int

    std::queue<std::shared_ptr<Task>> taskQue;//任务队列 //基类指针//需要保持生命周期//run完才结束//因此使用只能指针
    
    /*记录任务数量//++--考虑线程安全和轻量*/
    std::atomic_uint taskSize_; //任务数量
    int taskQueMaxThreshHold_;  //任务队列数量上限阈值//最好提供给用户接口

    std::mutex taskQueMtx_; //使用互斥锁保证任务队列的线程安全
    std::condition_variable notFull_;  //任务队列不满
    std::condition_variable notEmpty_; //任务队列不空

    PoolMode poolMode_; // 当前线程池的工作模式
};




#endif