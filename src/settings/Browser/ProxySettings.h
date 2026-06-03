#pragma once

#include "settings/Browser/Proxy/ProxyConfiguration.h"
#include "settings/Browser/Proxy/ProxyPolicy.h"

#include <QList>
#include <QString>
#include <QStringList>

namespace seb::settings::browser {

struct ProxySettings
{
    bool AutoConfigure = false;
    bool AutoDetect = false;
    QString AutoConfigureUrl;
    QStringList BypassList;
    proxy::ProxyPolicy Policy = proxy::ProxyPolicy::System;
    QList<proxy::ProxyConfiguration> Proxies;
};

}  // namespace seb::settings::browser
