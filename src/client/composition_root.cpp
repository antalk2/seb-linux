#include "client/composition_root.h"

#include "applications/external_application.h"
#include "browser/BrowserApplication.h"
#include "seb_session.h"
#include "client/client_controller.h"
#include "client/operations/application_operation.h"
#include "client/operations/browser_operation.h"
#include "client/operations/shell_operation.h"
#include "client/responsibilities/browser_responsibility.h"
#include "client/responsibilities/shell_responsibility.h"

namespace seb::client {

CompositionRoot::CompositionRoot(SebSession &session)
    : session_(session)
{
    context_.session = &session_;
    browser_ = std::make_shared<seb::browser::BrowserApplication>(session_);
    context_.browser = browser_;
    for (auto *application : session_.externalApplications()) {
        context_.applications.push_back(application);
    }

    controller_ = std::make_unique<ClientController>(context_);
    controller_->operations().add(new operations::ApplicationOperation(context_, controller_.get()));
    controller_->operations().add(new operations::ShellOperation(context_, controller_.get()));
    controller_->operations().add(new operations::BrowserOperation(context_, controller_.get()));

    context_.responsibilities.push_back(new responsibilities::BrowserResponsibility(context_));
    context_.responsibilities.push_back(new responsibilities::ShellResponsibility(context_));
}

ClientController &CompositionRoot::controller()
{
    return *controller_;
}

}  // namespace seb::client
