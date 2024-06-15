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
    , taskSize_(0)
    , taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
    , poolMode_(PoolMode::MODE_FIXED)
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

//给线程池提交任务   用户调用该接口，传入任务对象，生产任务
void ThreadPool::submitTask(std::shared_ptr<Task> sp)//让用户直接传智能指针进来，规避生命周期太短的任务。
{
    //获取锁
    std::unique_lock<std::mutex> lock(taskQueMtx_);

    //线程通信--等待任务队列有空余节点
    /*基本实现
    while (taskQue_.size() == taskQueMaxThreshHold_)
    {
        notFull_.wait(lock);//改变线程状态
    }
    */
    /*使用封装做*/ //设置wait的超时时间 否则阻塞用户进程太久
    if (!notFull_.wait_for(lock, std::chrono::seconds(1),
        [&]()->bool { return taskQue_.size() < (size_t)taskQueMaxThreshHold_; }))//通过正则表达式来判断  注意重载的wait的用法
    {
        //返回false ：等待一秒钟但仍不能满足notfull条件
        std::cerr << "task queue is full, task submit fail." << std::endl;
        return;
    }
    //若有空余，任务放入任务队列
    taskQue_.emplace(sp);
    taskSize_++;

    //放入任务后，线程队列至少有一个线程，必定通知 notEmpty，分配线程执行任务
    notEmpty_.notify_all();
}

//开启线程池
void ThreadPool::start(int initThreadSize)
{
    //记录初始线程个数
    initThreadSize_ = initThreadSize;

    //创建线程对象 同时把线程函数给到thread对象
    //考虑线程公平性 先集中创建再启动
    for (int i = 0; i < initThreadSize_; i++)
    {
        //创建thread线程对象时，把线程函数给到thread线程对象//注意指针 unique_ptr的使用
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::ThreadFunc, this));
        threads_.emplace_back(std::move(ptr)); //绑定器和函数对象的概念//设计cpp高级课程
    }

    //启动所有线程
    for (int i = 0; i < initThreadSize_; i++)
    {
        threads_[i]->start();//threads数组里的thread对象 它自己的启动函数
        //启动线程本身要有执行的线程函数的
    }
}

//定义线程函数  线程池的所有任务从队列消费任务
void ThreadPool::ThreadFunc()//
{
    //std::cout << "begin threadFunc" << std::endl;
    //std::cout << std::this_thread::get_id() << std::endl;
    //std::cout << "end threadFunc" << std::endl;
    for (;;)
    {
        std::shared_ptr<Task> task;
        {
            //获取锁
            std::unique_lock<std::mutex> lock(taskQueMtx_);

            //等待notempty
            notEmpty_.wait(lock, [&]()->bool { return taskQue_.size() > 0;  });
        
            //从任务队列取一个任务//取出
            auto task = taskQue_.front();
            taskQue_.pop();
            taskSize_--;

            // 若有队列任务剩余，则继续通知其他线程
            // 功能函数写完不要忘记条件变量的相应通知
            if (taskQue_.size() > 0)
            {
                notEmpty_.notify_all();
            }

            //取出任务需要通知,可以继续生产任务
            notFull_.notify_all();
        }//自动把锁释放

        if (task != nullptr) {
            //当前线程负责执行这个任务
            task->run();
        }
    }
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
    , taskSize_(0)
    , taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
    , poolMode_(PoolMode::MODE_FIXED)
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

//给线程池提交任务   用户调用该接口，传入任务对象，生产任务
void ThreadPool::submitTask(std::shared_ptr<Task> sp)//让用户直接传智能指针进来，规避生命周期太短的任务。
{
    //获取锁
    std::unique_lock<std::mutex> lock(taskQueMtx_);

    //线程通信--等待任务队列有空余节点
    /*基本实现
    while (taskQue_.size() == taskQueMaxThreshHold_)
    {
        notFull_.wait(lock);//改变线程状态
    }
    */
    /*使用封装做*/ //设置wait的超时时间 否则阻塞用户进程太久
    if (!notFull_.wait_for(lock, std::chrono::seconds(1),
        [&]()->bool { return taskQue_.size() < (size_t)taskQueMaxThreshHold_; }))//通过正则表达式来判断  注意重载的wait的用法
    {
        //返回false ：等待一秒钟但仍不能满足notfull条件
        std::cerr << "task queue is full, task submit fail." << std::endl;
        return;
    }
    //若有空余，任务放入任务队列
    taskQue_.emplace(sp);
    taskSize_++;

    //放入任务后，线程队列至少有一个线程，必定通知 notEmpty，分配线程执行任务
    notEmpty_.notify_all();
}

//开启线程池
void ThreadPool::start(int initThreadSize)
{
    //记录初始线程个数
    initThreadSize_ = initThreadSize;

    //创建线程对象 同时把线程函数给到thread对象
    //考虑线程公平性 先集中创建再启动
    for (int i = 0; i < initThreadSize_; i++)
    {
        //创建thread线程对象时，把线程函数给到thread线程对象//注意指针 unique_ptr的使用
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::ThreadFunc, this));
        threads_.emplace_back(std::move(ptr)); //绑定器和函数对象的概念//设计cpp高级课程
    }

    //启动所有线程
    for (int i = 0; i < initThreadSize_; i++)
    {
        threads_[i]->start();//threads数组里的thread对象 它自己的启动函数
        //启动线程本身要有执行的线程函数的
    }
}

//定义线程函数  线程池的所有任务从队列消费任务
void ThreadPool::ThreadFunc()//
{
    //std::cout << "begin threadFunc" << std::endl;
    //std::cout << std::this_thread::get_id() << std::endl;
    //std::cout << "end threadFunc" << std::endl;
    for (;;)
    {
        std::shared_ptr<Task> task;
        {
            //获取锁
            std::unique_lock<std::mutex> lock(taskQueMtx_);

            //等待notempty
            notEmpty_.wait(lock, [&]()->bool { return taskQue_.size() > 0;  });
        
            //从任务队列取一个任务//取出
            auto task = taskQue_.front();
            taskQue_.pop();
            taskSize_--;

            // 若有队列任务剩余，则继续通知其他线程
            // 功能函数写完不要忘记条件变量的相应通知
            if (taskQue_.size() > 0)
            {
                notEmpty_.notify_all();
            }

            //取出任务需要通知,可以继续生产任务
            notFull_.notify_all();
        }//自动把锁释放

        if (task != nullptr) {
            //当前线程负责执行这个任务
            task->run();
        }
    }
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

