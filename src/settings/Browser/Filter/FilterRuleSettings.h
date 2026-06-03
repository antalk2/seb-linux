#pragma once

#include "settings/Browser/Filter/FilterResult.h"
#include "settings/Browser/Filter/FilterRuleType.h"

#include <QString>

namespace seb::settings::browser::filter {

struct FilterRuleSettings
{
    bool Active = true;
    QString Expression;
    FilterResult Result = FilterResult::Block;
    FilterRuleType Type = FilterRuleType::Simplified;
};

}  // namespace seb::settings::browser::filter
