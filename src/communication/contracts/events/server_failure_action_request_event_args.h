#pragma once

#include "communication/contracts/events/communication_event_args.h"

#include <QString>
#include <QUuid>

namespace seb::communication::contracts::events {

struct ServerFailureActionRequestEventArgs : CommunicationEventArgs
{
    QString message;
    bool showFallback = false;
    QUuid requestId;
};

}  // namespace seb::communication::contracts::events
