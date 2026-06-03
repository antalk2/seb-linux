#pragma once

#include "configuration/contracts/cryptography/password_parameters.h"
#include "configuration/contracts/client_configuration.h"
#include "configuration/contracts/load_status.h"
#include "configuration/contracts/save_status.h"
#include "configuration/contracts/session_configuration.h"

#include "seb_settings.h"

#include <QUrl>

namespace seb::configuration::contracts {

class IConfigurationRepository
{
public:
    virtual ~IConfigurationRepository() = default;

    virtual SaveStatus configureClientWith(const QUrl &resource, const cryptography::PasswordParameters *password = nullptr) = 0;
    virtual AppConfig initializeAppConfig() = 0;
    virtual SessionConfiguration initializeSessionConfiguration() = 0;
    virtual seb::SebSettings loadDefaultSettings() = 0;
    virtual LoadStatus tryLoadSettings(const QUrl &resource, seb::SebSettings &settings, const cryptography::PasswordParameters *password = nullptr) = 0;
};

}  // namespace seb::configuration::contracts
