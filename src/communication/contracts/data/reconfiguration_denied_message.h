#pragma once

#include "communication/contracts/data/message.h"

#include <QString>

namespace seb::communication::contracts::data {

struct ReconfigurationDeniedMessage : Message
{
    explicit ReconfigurationDeniedMessage(QString path = {})
        : filePath(std::move(path)) {}

    QString filePath;
};

}  // namespace seb::communication::contracts::data
