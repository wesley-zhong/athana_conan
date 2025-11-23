//
// Created by Wesly Zhong on 2025/10/12.
//

#include "AthenaThreadPool.h"
Thread::Worker *AthenaThreadPool::createThread(){
    return new Thread::Worker();

}

void AthenaThreadPool::deleteThread(Thread::Worker *t){

}

void AthenaThreadPool::completeTask(Thread::TaskPtr task) {

}