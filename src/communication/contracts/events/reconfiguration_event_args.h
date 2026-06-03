#pragma once

#include "communication/contracts/events/communication_event_args.h"

#include <QString>

namespace seb::communication::contracts::events {

struct ReconfigurationEventArgs : CommunicationEventArgs
{
    QString configurationPath;
    QString resourceUrl;
};

}  // namespace seb::communication::contracts::events
