#pragma once

#include "seb_settings.h"

#include <memory>

#include <QObject>

class BrowserWindow;
class RuntimeWindow;
class SebSession;
class QWidget;

/*
 * Load new settings, create new SebSession and BrowserWindow
 */
class AppController : public QObject {
  Q_OBJECT

 public:
  explicit AppController(QObject* parent = nullptr);
  ~AppController() override;

  /*
   *  Load settings from resource and pass it to this->applySettings()
   *
   * @param resource If resource=="", use seb::defaultSettings()
   *                 Otherwise passed to loadSettingsFromResource()
   * @param error OUT
   * @return false on failure, true on success.
   */
  bool launch( const QString& resource, QString* error );

  /*
   * Just calls applySettings(settings, warnings, error)
   *
   * The only added value is that launchResolved() is public.
   *
   * Only called from main()
   */
  bool launchResolved(const seb::SebSettings& settings,
                      const QStringList& warnings,
                      QString* error);

 private:
  /*
   *  Given new settings, repplace the session_ and mainWindow_
   *
   *  @param settings to apply
   *  @param warnings Warning messages loadSettingsFromResource()
   *  @param error  OUT, ignored if NULL
   *  @return true on success. false on failure (and set *error)
   *
   *  Finalizes settings. Show a RuntimeWindow. Create new SebSession
   *  and BrowserWindow. Hide RuntimeWindow, show the new BrowserWindow.
   *  Set ( runtimeWindow_, session_, mainWindow_ ) to these new values.
   */
  bool applySettings(const seb::SebSettings& settings,
                     const QStringList& warnings,
                     QString* error);

  std::unique_ptr<RuntimeWindow> runtimeWindow_;
  std::unique_ptr<SebSession>    session_;
  BrowserWindow*                 mainWindow_ = nullptr;
};
