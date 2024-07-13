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
#include <functional>
#include <unordered_map>

//Any 类型： 可以接收任意数据的类型 C++17中any类型的关键
class Any
{
public:
    Any() = default;
    ~Any() = default;
    Any(const Any&) = delete;
    Any& operator=(const Any&) = delete;//左值引用和右值引用的拷贝构造
    Any(Any&&) = default;//右值引用的拷贝构造
    Any& operator=(Any&&) = default;

    template<typename T>
    Any(T data) : base_(std::make_unique<Derive<T>>(data))
    {} 

    //这个方法又能把any里存储的对象数据取出来
    template<typename T>
    T cast_()
    {
        //怎么从base_中找到它所指向的Derive对象，从他里面取出data成员变量
        //基类指针转换成派生类指针 RTTI类型识别
        Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());//取出智能指针的裸指针
        if (pd == nullptr)
        {
            //转换失败，基类和派生类不对应
            throw "type is unmatch!";
        }
        return pd->data_;
    }

private:
    //基类类型
    class Base
    {
    public:
        virtual ~Base() = default;
    };

    //派生类类型//这个构造函数可以让any接收任意数据
    template<typename T>
    class Derive : public Base
    {
    public:
        Derive(T data) : data_(data)
        {}
        T data_; //基类指针的派生类对象里 保存了其他任意类型的
    };
    
private:
    //定义基类的指针
    std::unique_ptr<Base> base_;
};

// 实现信号量类
class Semaphore
{
public:
    Semaphore(int limit = 0)
        :resLimit_(limit)
    {}
    ~Semaphore() = default;

    // 获取一个信号量资源
    void wait()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        // 等待信号量有资源，没有资源的话，会阻塞当前线程
        cond_.wait(lock, [&]()->bool {return resLimit_ > 0; });
        resLimit_--;
    }

    // 增加一个信号量资源
    void post()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        resLimit_++;
        // linux下condition_variable的析构函数什么也没做
        // 导致这里状态已经失效，无故阻塞
        cond_.notify_all();  // 等待状态，释放mutex锁 通知条件变量wait的地方，可以起来干活了
    }
private:
    int resLimit_;
    std::mutex mtx_;
    std::condition_variable cond_;
};

//Task类型的前置声明
class Task;
//实现接收 提交到线程池的task任务执行完毕后的返回值类型 Result
class Result
{
public:
    Result(std::shared_ptr<Task> task, bool isValid = true);
    ~Result() = default;

    //setvalue 获取任务的返回值
    void setVal(Any any);

    //如何提供get方法给用户层调用
    Any get();
private:
    Any any_;//存储返回值
    Semaphore sema_;//信号量
    std::shared_ptr<Task>task_;//指向对应获取返回值的任务对象
    std::atomic_bool isValid_;//返回值是否有效，比如任务是否提交成功的情况
};

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
    Task();
    ~Task() = default;
    void exec();
    void setResult(Result* res);
    //用户自定义任务类型，从Task继承，重写run方法，实现自定义任务处理
    virtual Any run() = 0; //修饰虚函数

private:
    Result* result_;
};

//线程类型
class Thread
{
public:
    using ThreadFunc = std::function<void(int)>;//functional 头文件
    Thread(ThreadFunc func);
    ~Thread();
    //启动线程
    void start();
    int getId()const;

private:
    ThreadFunc func_; //存储一个线程函数的对象
    static int generateId;
    int threadId_;//保存线程id
};

/*
example:
ThreadPool pool;
pool.start(4);

class MyTask : public Task
{
    public:
        void run() { // 线程代码... }
};

pool.submitTask(std::make_shared<MyTask>());
*/

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

    //设置线程池cache模式下线程上限阈值
    void setthreadSizeThreshHold(int threshhold);

    //给线程池提交任务
    Result submitTask(std::shared_ptr<Task> sp);//让用户直接传智能指针进来，规避生命周期太短的任务。

    //开启线程池
    void start(int initThreadSize = 4);

    //禁止 不让用户这样 对线程池本身进行拷贝构造和赋值 //可以单独构造一个线程池对象
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    //定义线程函数
    void ThreadFunc(int threadid);

    //check pool运行状态
    bool checkRunningState() const;

private:
    //线程列表
    std::unordered_map<int, std::unique_ptr<Thread>> threads_;
    
    //threadpool(/* args */);因为需要把线程对象和id绑定 所以用map存储
    //std::vector<std::unique_ptr <Thread>> threads_;//线程列表//只能指针避免释放资源麻烦 把Thread*指针换成unique_ptr类型
    size_t initThreadSize_;   //初始线程数量  //size_t 无符号int
    int threadSizeThreshHold_;  //线程数量上限阈值
    std::atomic_int curThreadSize_;//记录当前线程池的总线程数量
    std::atomic_int idleThreadSize_; // 记录空闲线程的数量

    std::queue<std::shared_ptr<Task>> taskQue_;//任务队列 //基类指针//需要保持生命周期//run完才结束//因此使用智能指针

    /*记录任务数量//++--考虑线程安全和轻量*/
    std::atomic_int taskSize_; //任务数量
    int taskQueMaxThreshHold_;  //任务队列数量上限阈值//最好提供给用户接口

    std::mutex taskQueMtx_; //使用互斥锁保证任务队列的线程安全
    std::condition_variable notFull_;  //任务队列不满
    std::condition_variable notEmpty_; //任务队列不空

    PoolMode poolMode_; // 当前线程池的工作模式
    std::atomic_bool isPoolRunning_;//表示当前线程池的启动状态
};




#endif