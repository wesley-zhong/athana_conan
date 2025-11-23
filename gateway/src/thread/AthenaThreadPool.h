//
// Created by Wesly Zhong on 2025/10/12.
//

#ifndef ATHENA_ATHENATHREAD_H
#define ATHENA_ATHENATHREAD_H

#include "core/common/ThreadPool.h"

class AthenaThread : public  Thread::Worker{

};

class AthenaThreadPool : public Thread::ThreadPool {
public:
    Thread::Worker *createThread() override;

    void deleteThread(Thread::Worker *t) override;

    void completeTask(Thread::TaskPtr task) ;
};


#endif //ATHENA_ATHENATHREAD_H
