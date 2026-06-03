#pragma once

#include "settings/Proctoring/ImageFormat.h"
#include "settings/Proctoring/ImageQuantization.h"

namespace seb::settings::proctoring {

struct ScreenProctoringSettings
{
    bool Enabled = false;
    ImageFormat ImageFormat = ImageFormat::Jpeg;
    ImageQuantization Quantization = ImageQuantization::Medium;
};

}  // namespace seb::settings::proctoring
