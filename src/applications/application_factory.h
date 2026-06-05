#pragma once

#include "seb_settings.h"

#include <memory>

namespace seb::applications {

class ExternalApplication;

  /*
   *  Original implements IApplicationFactory
   */
class ApplicationFactory
{
public:
  /*
   * Original: FactoryResult TryCreate( WhitelistApplication settings,
   *                                    out IApplication<IApplicationWindow> application )
   */
    static std::unique_ptr<ExternalApplication> tryCreate(const seb::WhitelistApplicationSettings &settings);
};

}  // namespace seb::applications
