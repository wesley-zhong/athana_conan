#pragma once

#include <vector>
#include <thread>
#include "TQueue.h"
#include "../log/XLog.h"
#include "BaseType.h"
#include "ObjectPool.hpp"

namespace Thread {
    class RunTask;

    class ITask {
    public:
        virtual ~ITask();

        virtual void run() = 0;

        virtual void release() =0;
    };

    typedef ITask *TaskPtr;

    class RunTask : public ITask, public ObjPool::PoolObjClass<RunTask> {
    public:
        explicit RunTask(VOID_FUN &&func) : _func(std::forward<VOID_FUN>(func)) {
        }

        explicit RunTask(const VOID_FUN &func) : _func(func) {
        }

        explicit RunTask() {
        }

        void run() override {
            _func();
            release();
        }

        void release() override {
            recycle(this);
        }

        ~RunTask() override {
        }

    private:
        VOID_FUN _func;
    };

    class ThreadPool;

    class Worker {
    public:
        Worker();

        virtual ~Worker();

        static void thread_func(Worker *t);

        void stop();

        void execute(const VOID_FUN &fun) {
            RunTask *it = RunTask::create(static_cast<VOID_FUN>(fun));
            execute(it);
        }
        virtual void run(TaskPtr task);

    protected:
        void execute(TaskPtr task);

        virtual void onStart() {
        };

        virtual void onEnd() {
        };

        TaskPtr popWaitTask();

        std::thread _thread;
        std::atomic<bool> _isrun{true};
        TQueue<TaskPtr> _waitTasks;
    };

    class ThreadPool {
    public:
        ThreadPool();

        virtual ~ThreadPool();

        void create(int count);

        void exit();

        void executeTask(TaskPtr task, int threadHashCode = 0) const;


        void execute(const VOID_FUN &run, int threadHashCode = 0) {
            int threadIndex = threadHashCode % (int) _threads.size();
            _threads[threadIndex]->execute(run);
        }

    protected:
        virtual Worker *createThread() = 0;

        virtual void deleteThread(Worker *t) {
            auto it = std::find(_threads.begin(), _threads.end(), t);
            if (it != _threads.end()) {
                _threads.erase(it);
            }
        }

    private:
        std::vector<Worker *> _threads;
    };
}
