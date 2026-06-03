#pragma once

#include "configuration/contracts/cryptography/encryption_parameters.h"
#include "configuration/contracts/load_status.h"
#include "configuration/contracts/data_formats/format_type.h"

#include <QVariantMap>

namespace seb::configuration::contracts::data_formats {

struct ParseResult
{
    cryptography::EncryptionParameters *encryption = nullptr;
    FormatType format = FormatType::Xml;
    QVariantMap rawData;
    LoadStatus status = LoadStatus::UnexpectedError;
};

}  // namespace seb::configuration::contracts::data_formats
