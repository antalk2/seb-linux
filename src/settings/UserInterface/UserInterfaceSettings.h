#pragma once

#include "settings/UserInterface/ActionCenterSettings.h"
#include "settings/UserInterface/LockScreenSettings.h"
#include "settings/UserInterface/TaskbarSettings.h"
#include "settings/UserInterface/UserInterfaceMode.h"

namespace seb::settings::userinterface {

struct UserInterfaceSettings
{
    ActionCenterSettings ActionCenter;
    LockScreenSettings LockScreen;
    TaskbarSettings Taskbar;
    UserInterfaceMode Mode = UserInterfaceMode::Desktop;
};

}  // namespace seb::settings::userinterface
