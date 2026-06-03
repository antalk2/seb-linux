#pragma once

#include "configuration/contracts/app_config.h"
#include "configuration/contracts/session_configuration.h"
#include "seb_settings.h"

namespace seb::configuration::configuration_data {

class DataValues
{
public:
    contracts::AppConfig initializeAppConfig() const;
    contracts::SessionConfiguration initializeSessionConfiguration() const;
    seb::SebSettings loadDefaultSettings() const;
    QString getAppDataFilePath() const;
};

}  // namespace seb::configuration::configuration_data
