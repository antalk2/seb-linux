#pragma once

namespace seb::client::contracts {

/* Coordinates concurrent operations of the client application. */
class ICoordinator {
 public:
  virtual ~ICoordinator() = default;

  /* Indicates whether the reconfiguration lock is currently occupied. */
  virtual bool isReconfigurationLocked() const = 0;
  virtual bool isSessionLocked() const = 0;
  virtual void releaseReconfigurationLock() = 0;
  virtual void releaseSessionLock() = 0;
  virtual bool requestReconfigurationLock() = 0;
  virtual bool requestSessionLock() = 0;
};

}  // namespace seb::client::contracts
