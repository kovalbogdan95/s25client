// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CheatCommandTracker.h"
#include <memory>
#include <string>

class GameWorldBase;
struct KeyEvent;

class Cheats
{
public:
    Cheats(GameWorldBase& world);
    ~Cheats(); // = default - for unique_ptr

    void toggleCheatMode();
    bool isCheatModeOn() const { return isCheatModeOn_; }

    void toggleHumanAIPlayer();
    void armageddon();

private:
    bool areCheatsAllowed() const;

    bool isCheatModeOn_ = false;
    GameWorldBase& world_;
};
