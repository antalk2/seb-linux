#pragma once

#include "communication/contracts/i_communication_host.h"

namespace seb::communication::contracts::hosts {

class IServiceHost : public contracts::ICommunicationHost
{
public:
    ~IServiceHost() override = default;
};

}  // namespace seb::communication::contracts::hosts
