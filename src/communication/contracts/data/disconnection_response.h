#pragma once

#include "communication/contracts/data/response.h"

namespace seb::communication::contracts::data {

struct DisconnectionResponse : Response
{
    bool connectionTerminated = false;
};

}  // namespace seb::communication::contracts::data
