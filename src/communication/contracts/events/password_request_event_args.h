#pragma once

#include "communication/contracts/data/password_request_purpose.h"
#include "communication/contracts/events/communication_event_args.h"

#include <QUuid>

namespace seb::communication::contracts::events {

struct PasswordRequestEventArgs : CommunicationEventArgs
{
    seb::communication::contracts::data::PasswordRequestPurpose purpose =
        seb::communication::contracts::data::PasswordRequestPurpose::Settings;
    QUuid requestId;
};

}  // namespace seb::communication::contracts::events
