//
// Created by Wesly Zhong on 2025/11/13.
//

#ifndef GAME_MODULECONTAINER_H
#define GAME_MODULECONTAINER_H

#include <functional>
#include<vector>
class BaseModule;

class ModuleContainer {
public:
    void registerModule(BaseModule *module);

    void forEach(std::function<void(BaseModule *)> onCall) {
        for (auto it: modules) {
            onCall(it);
        }
    }

private:
    std::vector<BaseModule *> modules;

};


#endif //GAME_MODULECONTAINER_H
