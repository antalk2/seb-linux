#pragma once

#include "client/client_context.h"
#include "client/responsibilities/client_task.h"

namespace seb::client::responsibilities {

class ClientResponsibility
{
public:
    explicit ClientResponsibility(ClientContext &context);
    virtual ~ClientResponsibility() = default;

    virtual void assume(ClientTask task) = 0;

protected:
    ClientContext &context_;
};

}  // namespace seb::client::responsibilities
