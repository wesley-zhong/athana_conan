//
// Created by Wesly Zhong on 2025/11/13.
//

#include "ModuleContainer.h"
#include "BaseModule.h"

void ModuleContainer::registerModule(BaseModule *module){
   modules.push_back(module);
}