#pragma once

#include "settings/Proctoring/MetaDataSettings.h"
#include "settings/Proctoring/ScreenProctoringSettings.h"

namespace seb::settings::proctoring {

struct ProctoringSettings
{
    bool Enabled = false;
    MetaDataSettings MetaData;
    ScreenProctoringSettings ScreenProctoring;
};

}  // namespace seb::settings::proctoring
