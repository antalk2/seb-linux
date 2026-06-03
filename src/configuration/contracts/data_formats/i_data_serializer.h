#pragma once

#include "configuration/contracts/cryptography/encryption_parameters.h"
#include "configuration/contracts/data_formats/format_type.h"
#include "configuration/contracts/data_formats/serialize_result.h"

#include <QVariantMap>

namespace seb::configuration::contracts::data_formats {

class IDataSerializer
{
public:
    virtual ~IDataSerializer() = default;
    virtual bool canSerialize(FormatType format) const = 0;
    virtual SerializeResult trySerialize(const QVariantMap &data, const cryptography::EncryptionParameters *encryption = nullptr) const = 0;
};

}  // namespace seb::configuration::contracts::data_formats
