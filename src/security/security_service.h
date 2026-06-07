#pragma once

#include "seb_settings.h"

#include <QObject>
#include <QStringList>
#include <QTimerEvent>

namespace seb::security {

class SecurityService : public QObject
{
    Q_OBJECT

public:
    explicit SecurityService(QObject *parent = nullptr);

    bool isVirtualMachine() const;
    bool isDebuggerAttached() const;
    bool isMultipleDisplaysActive() const;
    QStringList detectProhibitedProcesses() const;
    
    void startMonitoring();
    void stopMonitoring();

  void update_blacklist( const QList<BlacklistApplicationSettings>& blacklist );

protected:
    void timerEvent(QTimerEvent *event) override;

signals:
    void secureViolationDetected(const QString &reason);

private:
    void performCheck();
    int timerId_ = -1;

  QList<BlacklistApplicationSettings> blacklist_ ;
};

} // namespace seb::security
