#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threadnum, const std::string &threadtype) : stop_(false), threadtype_(threadtype)
{
    // 启动threadnum个线程，每个线程将阻塞在条件变量上
    for(size_t i = 0; i < threadnum; i ++)
    {
        // 用lambda函数创建线程
        // emplace_back和push_back的区别
        threads_.emplace_back([this] 
        {
            printf("create %s thread(%ld).\n", threadtype_.c_str(), syscall(SYS_gettid));

            while(stop_ == false)
            {
                std::function<void()> task;

                {   // 锁的作用域开始
    
                    std::unique_lock<std::mutex> lock(this -> mutex_);

                    // 等待生产者的条件变量
                    this -> condition_.wait(lock, [this] 
                    {
                        return ((this -> stop_ == true) || (this -> taskqueue_.empty() == false));
                    });

                    // 在线程池结束之前，如果队列中还没任务，执行完再退出
                    if((this -> stop_ == true) && (this -> taskqueue_.empty() == true)) return;

                    // 出队一个任务
                    task = std::move(this -> taskqueue_.front());
                    this -> taskqueue_.pop();

                }   // 锁的作用域结束

                // printf("%s(%ld) execute task.\n", threadtype_.c_str(), syscall(SYS_gettid));
                task();     // 执行任务
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    stop();
}

void ThreadPool::stop()        // 停止线程
{
    if(stop_) return;
    
    stop_ = true;
    condition_.notify_all();            // 唤醒全部线程

    // 等待线程退出
    for(std::thread &th : threads_)
    {
        th.join();
    }
}

void ThreadPool::addtask(std::function<void()> task)
{
    {   // 锁的作用域开始

        std::unique_lock<std::mutex> lock(mutex_);
        taskqueue_.push(task);

    }   // 锁的作用域结束

    condition_.notify_one();        // 唤醒一个线程
}

size_t ThreadPool::size()          // 获取线程池的大小
{
    return threads_.size();
}