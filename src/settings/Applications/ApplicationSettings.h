#pragma once

#include "settings/Applications/BlacklistApplication.h"
#include "settings/Applications/WhitelistApplication.h"

#include <QList>

namespace seb::settings::applications {

struct ApplicationSettings
{
    QList<BlacklistApplication> Blacklist;
    QList<WhitelistApplication> Whitelist;
};

}  // namespace seb::settings::applications
