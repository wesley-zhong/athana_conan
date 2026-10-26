//
// Created by Wesly Zhong on 2025/11/15.
//

#ifndef ATHENA_BASEMODULE_H
#define ATHENA_BASEMODULE_H

// 这里只放前向声明，不能 #include "objs/Player.h"：
// Player.h 反向包含 module/Module.h（模块挂在 Player 上），含进来构成循环包含，
// 先包含 Player.h 的编译单元走到这里时 guard 已生效，拿到的是未声明的 Player
class Player;

class BaseModule
{
public:
    BaseModule(Player* player)
    {
        owner = player;
    }

    virtual void onLogin() = 0;

    virtual void onLogout() = 0;

    virtual void loadDataFromDB() =0;

    virtual void saveDataToDB() =0;

    virtual void markDirty()
    {
        is_dirty = true;
    }

protected:
    Player* owner;
    // 必须就地初始化：未初始化时 saveDataToDB 的 if (!is_dirty) 读到随机值，可能凭空落库
    bool is_dirty = false;
};


#endif //ATHENA_BASEMODULE_H
