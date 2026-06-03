#pragma once

#include "communication/contracts/i_communication.h"
#include "communication/contracts/hosts/i_host_object.h"

#include <memory>

namespace seb::communication::contracts::hosts {

class IHostObjectFactory
{
public:
    virtual ~IHostObjectFactory() = default;
    virtual std::unique_ptr<IHostObject> createObject(const QString &address, contracts::ICommunication *communicationObject) = 0;
};

}  // namespace seb::communication::contracts::hosts
