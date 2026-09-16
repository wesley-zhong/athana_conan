//
// Created by Wesly Zhong on 2025/11/15.
//

#ifndef ATHENA_BASEMODULE_H
#define ATHENA_BASEMODULE_H
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

protected:
    Player* owner;
};


#endif //ATHENA_BASEMODULE_H
