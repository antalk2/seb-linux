#pragma once

namespace seb::client::contracts {

/* Coordinates concurrent operations of the client application. */
class ICoordinator {
 public:
  virtual ~ICoordinator() = default;

  // reconfiguration lock

  /* Indicates whether the reconfiguration lock is currently occupied. */
  virtual bool isReconfigurationLocked() const = 0;
  
  /* Releases the reconfiguration lock. */
  virtual void releaseReconfigurationLock() = 0;
  
  /* Attempts to acquire the unique reconfiguration lock. Returns
   * <c>true</c> if successful, otherwise <c>false</c>.
   */
  virtual bool requestReconfigurationLock() = 0;

  // session lock
  
  /* Indicates whether the session lock is currently occupied. */
  virtual bool isSessionLocked() const = 0;

  /* Releases the session lock. */
  virtual void releaseSessionLock() = 0;

  /*
   * Attempts to acquire the unique session lock. Returns <c>true</c>
   * if successful, otherwise <c>false</c>.
   */
  virtual bool requestSessionLock() = 0;
};

}  // namespace seb::client::contracts
