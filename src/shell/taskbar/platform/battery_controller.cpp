#include "shell/taskbar/platform/battery_controller.h"

#include "shell/taskbar/platform/command_helper.h"

#include <QRegularExpression>
#include <QTimer>

namespace seb::shell::taskbar::platform {

BatteryController::BatteryController(QObject *parent)
    : QObject(parent)
{
    timer_ = new QTimer(this);
    timer_->setInterval(10000);
    connect(timer_, &QTimer::timeout, this, &BatteryController::refresh);
    timer_->start();
    refresh();
}

const BatteryState &BatteryController::state() const
{
    return state_;
}

void BatteryController::refresh() {
  BatteryState next;
  const QString output = runCommand(
      QStringLiteral("upower")
    , { QStringLiteral("-i")
      , QStringLiteral("/org/freedesktop/UPower/devices/DisplayDevice")
      }
  );
  
  if ( !output.isEmpty() ) {
    const QStringList lines = output.split('\n');
    for ( const QString &line : lines ) {
      const QString trimmed = line.trimmed();
      if ( trimmed.startsWith(QStringLiteral( "percentage:" )) ) {
        const QRegularExpression re( QStringLiteral("([0-9]+)%") );
        const auto match = re.match(trimmed);
        if ( match.hasMatch() ) {
          next.percentage = match.captured(1).toInt();
        }
      } else if ( trimmed.startsWith(QStringLiteral("state:")) ) {
        // https://github.com/Jvr2022/seb-linux/pull/21/changes/0267fa0a5b54c824fa4f7adbb13b7ad6c06298af
        next.charging = !trimmed.contains(QStringLiteral("discharging"));
      } else if (  trimmed.startsWith(QStringLiteral("time to empty:") )
                || trimmed.startsWith(QStringLiteral("time to full:")) )
      {
        next.timeRemaining = trimmed.section(':', 1).trimmed();
        // Only consider available if informative.
        // upower shows "percentage:          0%" on a PC
        next.available = true;
      }
    }
  }

  if ( next.available     != state_.available  ||
       next.charging      != state_.charging   ||
       next.percentage    != state_.percentage ||
       next.timeRemaining != state_.timeRemaining )
  {
    state_ = next;
    emit stateChanged();
  }
}

}  // namespace seb::shell::taskbar::platform
