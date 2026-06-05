#pragma once

#include "client/contracts/i_coordinator.h"

#include <QMutex>

namespace seb::client {

class Coordinator : public contracts::ICoordinator {
 public:
  // reconfiguration lock
  bool isReconfigurationLocked() const override;
  void releaseReconfigurationLock() override;
  bool requestReconfigurationLock() override;

  // session lock
  bool isSessionLocked() const override;
  void releaseSessionLock() override;
  bool requestSessionLock() override;

 private:
  mutable QMutex mutex_;
  bool reconfigurationLocked_ = false;
  bool sessionLocked_ = false;
};

}  // namespace seb::client
