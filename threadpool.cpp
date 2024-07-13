#include "threadpool.h"
#include <functional>
#include <iostream>
#include <vector>
#include <stddef.h>
#include <thread>


const int TASK_MAX_THRESHHOLD = 4;
const int THREAD_MAX_THRESHHOLD = 10;
const int THREAD_MAX_IDLE_TIME = 60;

/* 线程池构造 */
ThreadPool::ThreadPool()
    : initThreadSize_(0)
    , taskSize_(0)
    , idleThreadSize_(0)
    , curThreadSize_(0)
    , taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
    , threadSizeThreshHold_(THREAD_MAX_THRESHHOLD)
    , poolMode_(PoolMode::MODE_FIXED)
    , isPoolRunning_(false)
{}

/* 线程池析构 */
ThreadPool::~ThreadPool()
{}

//设置线程池工作模式
void ThreadPool::setMode(PoolMode mode)
{
    if (checkRunningState())
        return;
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
    if (checkRunningState())
        return;
    taskQueMaxThreshHold_ = threshhold;
}
//设置线程数量上限阈值
void ThreadPool::setthreadSizeThreshHold(int threshhold)
{
    if (checkRunningState())
        return;
    threadSizeThreshHold_ = threshhold;
}

//给线程池提交任务   用户调用该接口，传入任务对象，生产任务
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)//让用户直接传智能指针进来，规避生命周期太短的任务。
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
        return Result(sp, false);//线程执行完后pop，task对象就被析构了，不可用task->getResult();
    }
    //若有空余，任务放入任务队列
    taskQue_.emplace(sp);
    taskSize_++;

    //放入任务后，线程队列至少有一个线程，必定通知 notEmpty，分配线程执行任务
    notEmpty_.notify_all();

    //cache模式 根据任务数量和空闲线程数量，判断是否创建新线程
    //cache模式 任务处理紧急 场景：小而快的任务
    if (poolMode_ == PoolMode::MODE_CACHED
        && taskSize_ > idleThreadSize_
        && curThreadSize_ < threadSizeThreshHold_)
    {
        //创建新线程
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::ThreadFunc, this, std::placeholders::_1));
        //threads_.emplace_back(std::move(ptr));
        int threadId = ptr->getId();
        threads_.emplace(threadId, (std::move(ptr)));
        curThreadSize_++;
    }

    //返回任务的result对象
    return Result(sp);
}

//开启线程池
void ThreadPool::start(int initThreadSize)
{
    //设置线程池启动状态
    isPoolRunning_ = true;
    //记录初始线程个数
    initThreadSize_ = initThreadSize;
    curThreadSize_  = initThreadSize;

    //创建线程对象 同时把线程函数给到thread对象
    //考虑线程公平性 先集中创建再启动
    for (int i = 0; i < initThreadSize_; i++)
    {
        //创建thread线程对象时，把线程函数给到thread线程对象//注意指针 unique_ptr的使用
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::ThreadFunc, this, std::placeholders::_1));//绑定器和函数对象的概念
        int threadId = ptr->getId();
        threads_.emplace(threadId, (std::move(ptr))); 
    }

    //启动所有线程
    for (int i = 0; i < initThreadSize_; i++)
    {
        threads_[i]->start();//threads数组里的thread对象 它自己的启动函数
        //启动线程本身要有执行的线程函数的
        idleThreadSize_++;//每启动一个线程 记录初始空闲线程数量
    }
}

//定义线程函数  线程池的所有任务从队列消费任务
void ThreadPool::ThreadFunc(int threadid)//线程函数执行完 对应线程即结束
{
    auto lastTime = std::chrono::high_resolution_clock().now();

    for (;;)
    {
        std::shared_ptr<Task> task;
        {
            //获取锁
            std::unique_lock<std::mutex> lock(taskQueMtx_);
            std::cout << "tid:" << std::this_thread::get_id()
                << "尝试获取任务!" << std::endl;

            //cache模式下有可能已经创建很多线程但是没有任务去使用 空闲时间超过某个时间
            //需要回收线程 超过initThreadSize 的要回收
            //记录当前时间，记录上一次线程执行时间 》 60s
            if (poolMode_ == PoolMode::MODE_CACHED)
            {
                //每秒返回一次：：判断是超时返回还是有任务待执行返回
                while (taskQue_.size() > 0)
                {
                    if (std::cv_status::timeout ==
                        notEmpty_.wait_for(lock, std::chrono::seconds(1)))
                    {
                        //条件变量超时返回
                        auto nowTime = std::chrono::high_resolution_clock().now();
                        auto dur = std::chrono::duration_cast<std::chrono::seconds>(nowTime - lastTime);
                        if (dur.count() >= THREAD_MAX_IDLE_TIME
                            && curThreadSize_ > initThreadSize_)
                        {
                            //回收当前线程
                            //记录线程数量的相关变量值的修改，匹配线程的变化
                            //把线程对象从线程vector删除 如何匹配thread_func 和thread对象//需要映射关系
                            //通过threadid 去把thread对象删除

                        }
                    }
                }
            }
            else
            {
                //等待notempty条件
                notEmpty_.wait(lock, [&]()->bool { return taskQue_.size() > 0;  });
            }

            idleThreadSize_--;

            std::cout << "tid:" << std::this_thread::get_id()
                << "获取任务成功!" << std::endl;

            //从任务队列取一个任务//取出
            task = taskQue_.front();
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

        if (task != nullptr) 
        {
            std::cout << "tid:" << std::this_thread::get_id()
                << "执行任务成功!" << std::endl;
            //当前线程负责执行这个任务
            //task->run();执行任务并把任务返回值通过setVal给到Result
            task->exec();
        }
        else {
            std::cout << "tid:" << std::this_thread::get_id()
                << "执行任务失败!" << std::endl;
        }
        idleThreadSize_++;//此线程已将任务执行完 该线程成为空闲线程
        lastTime = std::chrono::high_resolution_clock().now(); // 更新线程执行完时间戳
    }
}

bool ThreadPool::checkRunningState() const
{
    return isPoolRunning_;
}

////////////////////////////////////线程方案实现
int Thread::generateId = 0;


    //线程构造
Thread::Thread(ThreadFunc func)
    : func_(func) //把传进来的函数变量用成员接收
    , threadId_(generateId++)
{}
//线程析构
Thread::~Thread() {}

void Thread::start()
{
    //创建线程 执行一个线程函数
    std::thread t(func_, threadId_);  //cpp11 线程对象t 线程函数func_
    t.detach();  //设置分离线程  不让线程函数挂 ~~~  pthread_detach
}

//获取线程id
int Thread::getId()const
{
    return threadId_;
}

////////////////////////////////////task方法实现
Task::Task()
    : result_(nullptr)
{}

void Task::exec()
{
    if (result_ != nullptr) 
    {
        result_->setVal(run());//发生多态调用
    }
}

void Task::setResult(Result* res)
{
    result_ = res;
}

////////////////////////////////////Result 方法的实现
Result::Result(std::shared_ptr<Task> task, bool isValid)
    :isValid_(isValid)
    ,task_(task)
{
    task_->setResult(this);
}

Any Result::get()
{
    if (!isValid_)
    {
        return "";
    }
    sema_.wait();//等待task执行完，阻塞用户线程
    return std::move(any_);
}

void Result::setVal(Any any)
{
    //存储task返回值
    this->any_ = std::move(any);
    sema_.post(); //已经获取的任务返回值，增加信号量资源
}
