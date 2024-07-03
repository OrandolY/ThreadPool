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
    Any run()
    {
        std::cout << "tid:" << std::this_thread::get_id()
            << "begin!" << std::endl;
        int sum = 0;
        for (int i = begin_; i <= end_; i++)
            sum += i;
        //std::this_thread::sleep_for(std::chrono::seconds(2));
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
    ThreadPool pool;
    pool.start(4);

    //Ques2 如何设计Result机制,未执行完时要阻塞用户需求；
    Result res = pool.submitTask(std::make_shared<MyTask>());
    int sum = res.get().cast<int>(); //get 返回的any类型如何转为其他类型 //提取什么类型
    
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());

    getchar();
    //std::this_thread::sleep_for(std::chrono::seconds(5));
}