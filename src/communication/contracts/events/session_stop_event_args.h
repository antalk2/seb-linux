#pragma once

#include "communication/contracts/events/communication_event_args.h"

#include <QUuid>

namespace seb::communication::contracts::events {

struct SessionStopEventArgs : CommunicationEventArgs
{
    QUuid sessionId;
};

}  // namespace seb::communication::contracts::events
