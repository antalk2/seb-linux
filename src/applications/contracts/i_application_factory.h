#pragma once

#include "seb_settings.h"

namespace seb::applications::contracts {

/*
 * Defines all possible results of an attempt to create an application.
 */
enum class FactoryResult
{
  /* Error: An error occurred while trying to create the application. */
  Error,
  
  /* Invalid: The application has been found but is invalid
   * (e.g. because it is not the correct version or has been
   * manipulated).
   */
  Invalid,
  
  /* The application could not be found on the system.
   */
  NotFound,
  
  /*
   * The application has been created successfully.
   */
  Success
};

}  // namespace seb::applications::contracts


namespace seb::applications::contracts {

class IApplication;

  /*
   * IApplicationFactory
   *
   *
   * Desc: Provides functionality to create external applications.
   * 
   * Original:
   * seb-win-refactoring/SafeExamBrowser.Applications.Contracts/IApplicationFactory.cs interface IApplicationFactory
   *
   */
class IApplicationFactory
{
public:
    virtual ~IApplicationFactory() = default;
  
  /*
   * Desc: Attempts to create an application according to the given
   * settings.
   *
   * Original: TryCreate
   */
    virtual FactoryResult tryCreate(const seb::WhitelistApplicationSettings &settings, IApplication *&application) = 0;
};

}  // namespace seb::applications::contracts
