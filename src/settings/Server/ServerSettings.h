#pragma once

#include "settings/Server/InvigilationSettings.h"

#include <QString>

namespace seb::settings::server {

struct ServerSettings
{
    bool Enabled = false;
    InvigilationSettings Invigilation;
    QString Url;
};

}  // namespace seb::settings::server
