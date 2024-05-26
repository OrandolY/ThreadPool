#include "threadpool.h"
#include <functional>
#include <iostream>
#include <vector>
#include <stddef.h>
#include <thread>


const int TASK_MAX_THRESHHOLD = 1024;

/* 线程池构造 */
ThreadPool::ThreadPool()
    :initThreadSize_(0)
    ,taskSize_(0)
    ,taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
    ,poolMode_(PoolMode::MODE_FIXED)
{}

/* 线程池析构 */
ThreadPool::~ThreadPool()
{}

//设置线程池工作模式
void ThreadPool::setMode(PoolMode mode)
{
    poolMode_ = mode;
}

/*设置初始线程数量
void ThreadPool::setInitThreadSize(int size)
{
    initThreadSize_ = size;
}
*/

//设置task任务队列上限阈值
void ThreadPool::settaskQueMaxThreshHold(int threshhold)
{
    taskQueMaxThreshHold_ = threshhold;
}

//给线程池提交任务
void ThreadPool::submitTask(std::shared_ptr<Task> sp)//让用户直接传智能指针进来，规避生命周期太短的任务。
{

}

//开启线程池
void ThreadPool::start(int initThreadSize)
{
    //记录初始线程个数
    initThreadSize_ = initThreadSize;

    //创建线程对象 同时把线程函数给到thread对象
    //考虑线程公平性 先集中创建再启动
    for(int i = 0; i < initThreadSize_; i++)
    {
        threads_.emplace_back(new Thread(std::bind(&ThreadPool::ThreadFunc, this))); //绑定器和函数对象的概念//设计cpp高级课程
    }

    //启动所有线程
    for(int i = 0; i < initThreadSize_; i++)
    {
        threads_[i]->start();//threads数组里的thread对象 它自己的启动函数
        //启动线程本身要有执行的线程函数的
    }
}

//定义线程函数
void ThreadPool::ThreadFunc()//
{
    std::cout << "begin threadFunc" << std::endl;
    std::cout << std::this_thread::get_id() << std::endl;
    std::cout << "end threadFunc" << std::endl;
}

/////线程方案实现

    //线程构造
Thread::Thread(ThreadFunc func)
    : func_(func) //把传进来的函数变量用成员接收
{}
    //线程析构
Thread::~Thread() {}

void Thread::start()
{
    //创建线程 执行一个线程函数
    std::thread t(func_);  //cpp11 线程对象t 线程函数func_
    t.detach();  //设置分离线程  不让线程函数挂 ~~~  pthread_detach
}

