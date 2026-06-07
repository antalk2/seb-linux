#pragma once

#include "shell/taskbar/taskbar_model.h"
#include "seb_settings.h"

namespace seb::shell::taskbar {

class TaskbarService {
public:
    static TaskbarModel buildModel( const seb::SebSettings& settings
                                  , bool hasBattery );
};

}  // namespace seb::shell::taskbar
