#include "ThreadPool.h"
#include <thread>
#include <utility>

using namespace Thread;

ITask::~ITask() {
}

Worker::Worker() : _thread(Worker::thread_func, this) {
    _thread.detach();
}

Worker::~Worker() {
}

void Worker::thread_func(Worker *t) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    t->onStart();
    while (t->_isrun) {
        TaskPtr task = t->popWaitTask();
        if (task != nullptr) {
            t->run(task);
            continue;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    t->onEnd();
}

void Worker::stop() {
    _isrun = false;
    _thread.join();
}

void Worker::run(TaskPtr task) {
    task->run();
}

void Worker::execute(TaskPtr task) {
    _waitTasks.push(std::move(task));
}

TaskPtr Worker::popWaitTask() {
    TaskPtr task;
    if (!_waitTasks.tryPop(task))
        return nullptr;
    return task;
}

ThreadPool::ThreadPool() = default;


ThreadPool::~ThreadPool() {
    exit();
}

void ThreadPool::create(int count) {
    for (int i = 0; i < count; ++i) {
        Worker *t = createThread();
        if (t) {
            _threads.push_back(t);
        }
    }
}

void ThreadPool::exit() {
    for (Worker *t: _threads) {
        t->stop();
        deleteThread(t);
    }
    _threads.clear();
}

void ThreadPool::executeTask(TaskPtr task, int threadHashCode) const {
    int threadIndex = threadHashCode % (int) _threads.size();
    _threads[threadIndex]->run(std::move(task));
}
