#pragma once

#include "settings/Applications/ApplicationSettings.h"
#include "settings/Browser/BrowserSettings.h"
#include "settings/ConfigurationMode.h"
#include "settings/Logging/LogLevel.h"
#include "settings/Monitoring/DisplaySettings.h"
#include "settings/Monitoring/KeyboardSettings.h"
#include "settings/Monitoring/MouseSettings.h"
#include "settings/Proctoring/ProctoringSettings.h"
#include "settings/Security/SecuritySettings.h"
#include "settings/Server/ServerSettings.h"
#include "settings/Service/ServiceSettings.h"
#include "settings/SessionMode.h"
#include "settings/System/SystemSettings.h"
#include "settings/SystemComponents/AudioSettings.h"
#include "settings/SystemComponents/PowerSupplySettings.h"
#include "settings/UserInterface/UserInterfaceSettings.h"

namespace seb::settings {

struct AppSettings
{
    applications::ApplicationSettings Applications;
    systemcomponents::AudioSettings Audio;
    browser::BrowserSettings Browser;
    ConfigurationMode ConfigurationMode = ConfigurationMode::ConfigureClient;
    monitoring::DisplaySettings Display;
    monitoring::KeyboardSettings Keyboard;
    logging::LogLevel LogLevel = logging::LogLevel::Info;
    monitoring::MouseSettings Mouse;
    systemcomponents::PowerSupplySettings PowerSupply;
    proctoring::ProctoringSettings Proctoring;
    security::SecuritySettings Security;
    server::ServerSettings Server;
    service::ServiceSettings Service;
    SessionMode SessionMode = SessionMode::Server;
    system::SystemSettings System;
    userinterface::UserInterfaceSettings UserInterface;
};

}  // namespace seb::settings
