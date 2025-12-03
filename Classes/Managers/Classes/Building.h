/****************************************************************

 * Project Name:  Clash_of_Clans

 * File Name:     Building.h

 * File Function:

 * Author:        赵崇治

 * Update Date:   2025/11/29

 * License:       MIT License

 ****************************************************************/



#pragma once

#include "cocos2d.h"



class Building : public cocos2d::Sprite

{

public:

    static Building* create(const std::string& filename);

    virtual bool init(const std::string& filename);



    // 以后可以在这里加：upgrade(), produceGold() 等

};