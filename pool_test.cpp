#include <iostream>
#include <chrono>
#include <thread>
using namespace std;

#include "threadpool.h"

/*
在部分场景，需要获取线程执行任务的返回值
e.g.
    多线程 流水线操作
    把一个事件拆分处理，最终合并出最终结果；
*/

using uLong = unsigned long long;

/*派生类 要 传入参数*/
class MyTask : public Task
{
public:
    MyTask(int begin, int end)
        : begin_(begin)
        , end_(end)
    {}
    //Ques1 如何设计run() 使其返回值可以表示任意类型；
    //在JAVA PY 中的Object 是所有其他类类型的基类，所有类都可以从它继承；
    //C++ 17 any类型
    Any run()  // run方法最终就在线程池分配的线程中去做执行了!
    {
        std::cout << "tid:" << std::this_thread::get_id()
            << "begin!" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        uLong sum = 0;
        for (uLong i = begin_; i <= end_; i++)
            sum += i;
        std::cout << "tid:" << std::this_thread::get_id()
            << "end!" << std::endl;
        return sum;
    }
    //设计到基类private性质
private:
    int begin_;
    int end_;
};

int main()
{
    {
        ThreadPool pool;
        pool.start(4);

        Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));

        uLong sum1 = res1.get().cast_<uLong>();
        std::cout << sum1 << std::endl;
    }
    std::cout << "main over" << std::endl;

#if 0
    //Ques 如果pool析构了，如何把线程池资源全部回收
    {
        ThreadPool pool;
        pool.setMode(PoolMode::MODE_CACHED);
        //用户自己设置线程池的工作模式
        pool.start(4);

        //Ques2 如何设计Result机制,未执行完时要阻塞用户需求;
        Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
        Result res2 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
        Result res3 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
        pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

        pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
        pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
        //Result res3 = pool.submitTask(std::make_shared<MyTask>(201, 300));

        uLong sum1 = res1.get().cast_<uLong>(); //get 返回的any类型如何转为其他类型 //提取什么类型
        uLong sum2 = res2.get().cast_<uLong>();
        uLong sum3 = res3.get().cast_<uLong>();
        //uLong sum3 = res1.get().cast_<uLong>();
        //cout << "--333-----------------------------------------------------" << endl;

        //Master-Slave 线程模型
        //Master 分解任务 给各个Slave线程分配任务
        //等待子线程全部执行完
        //Master合并结果输出
        cout << (sum1 + sum2 + sum3) << endl;
    }
    getchar();
    //std::this_thread::sleep_for(std::chrono::seconds(5));

#endif
}