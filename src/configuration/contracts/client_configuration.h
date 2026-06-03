#pragma once

#include "seb_settings.h"
#include "configuration/contracts/app_config.h"

#include <QUuid>

namespace seb::configuration::contracts {

struct ClientConfiguration
{
    AppConfig appConfig;
    QUuid sessionId;
    seb::SebSettings settings;
};

}  // namespace seb::configuration::contracts
