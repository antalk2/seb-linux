#pragma once

#include "communication/contracts/interlocutor.h"
#include "communication/contracts/data/message.h"

namespace seb::communication::contracts::data {

struct DisconnectionMessage : Message
{
    seb::communication::contracts::Interlocutor interlocutor = seb::communication::contracts::Interlocutor::Unknown;
};

}  // namespace seb::communication::contracts::data
