#include "app_controller.h"

#include "browser_window.h"
#include "seb_session.h"
#include "ui/runtime_window.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QTextStream>
#include <QTimer>

AppController::AppController(QObject* parent) : QObject(parent) {}

AppController::~AppController() = default;

bool AppController::applySettings( const seb::SebSettings& settings,
                                   const QStringList&      warnings,
                                   QString*                error
                                   )
{
  seb::SebSettings finalSettings = settings;

  const bool currentBypass = (  this->session_
                              ? this->session_ -> settings().devBypass
                              : false
                              );
  
  if ( currentBypass || finalSettings.devBypass ) {
    seb::applyDevBypassOverrides( finalSettings );
  }

  if ( !finalSettings.browser.enableBrowser ) {
    if (error) {
      *error = QStringLiteral("The loaded configuration disables the browser.");
    }
    return false;
  }

  {
    QTextStream err(stderr);
    for (const QString& warning : warnings) {
      err << "warning: " << warning << Qt::endl;
    }
  }

  /*
   * Create this->runtimeWindow_
   */
  if ( !this->runtimeWindow_ ) {
    this->runtimeWindow_ = std::make_unique<RuntimeWindow>();
  }
  this->runtimeWindow_->setVersionText(
      QCoreApplication::applicationVersion(),
      QCoreApplication::applicationVersion()
  );
  this->runtimeWindow_ -> setStatusText( QStringLiteral("Starting Safe Exam Browser") );
  this->runtimeWindow_ -> show();
  this->runtimeWindow_ -> raise();
  this->runtimeWindow_ -> activateWindow();

  // startupTimer
  QElapsedTimer startupTimer;
  startupTimer.start();

  // Create nextSession
  SebSessionResourceOpener opener =  [this]( const QString& resource, QWidget* parent ) {
    QString error;
    if ( this->launch( resource, &error ) ) {
      return true;
    }

    QMessageBox::critical(
                          parent
                          , QStringLiteral("Failed to Open SEB Resource")
                          , error
                          );
    return false;
  };
  
  auto nextSession = std::make_unique<SebSession>(
      finalSettings
    , opener
    , this
  );

  BrowserWindow* nextWindow = nextSession->createWindow( nextSession->initialUrl()
                                                       , true
                              );

  this->runtimeWindow_ -> setStatusText(
      QStringLiteral("Opening secure browser window")
  );

  {
    // Make it at least 5 seconds from start()
    const qint64 remaining = 5000 - startupTimer.elapsed(); // number of milliseconds
    if ( remaining > 0 ) {
      QEventLoop loop;
      QTimer::singleShot( static_cast<int>(remaining), &loop, &QEventLoop::quit );
      loop.exec();
    }
  }

  nextWindow->show();
  this->runtimeWindow_->hide();

  if ( mainWindow_ ) {
    mainWindow_ ->hide();
    mainWindow_ ->deleteLater();
    mainWindow_ = nullptr;
  }

  if ( session_ ) {
    session_.release()->deleteLater();
  }

  mainWindow_ = nextWindow;
  session_    = std::move(nextSession);

  return true;
}


bool AppController::launch(
    const QString& resource,
    QString*       error)
{
  seb::ResourceLoadResult loaded;
  
  if ( resource.trimmed().isEmpty() ) {
    // No resource, use defaultSettings
    loaded.ok = true;
    loaded.settings = seb::defaultSettings();
  } else {
    // Ask password, return it.
    // Return "" on Cancel.
    seb::PasswordProvider passwordProvider =  [this](bool hashed) {
      bool          accepted = false;
      const QString text =
        QInputDialog::getText(
                              this -> mainWindow_ // parent
                              , // title
                              hashed
                              ? QStringLiteral("SEB Password Required")
                              : QStringLiteral("SEB Password Required")
                              , // label
                              hashed
                              ? QStringLiteral( "Enter the administrator password for this "
                                                "client-configuration SEB file.")
                              : QStringLiteral( "Enter the password for this SEB configuration file.")
                              , QLineEdit::Password /* EchoMode */
                              , QString()   /* text */
                              , &accepted   /* ok   */
                              );
      return accepted ? text : QString();
    };
    //
    loaded = seb::loadSettingsFromResource( resource, passwordProvider );
  }

  if (!loaded.ok) {
    if (error) {
      *error = loaded.error;
    }
    return false;
  }

  return applySettings( loaded.settings, loaded.warnings, error );
}

bool AppController::launchResolved(
    const seb::SebSettings& settings,
    const QStringList&      warnings,
    QString*                error
)
{
  return applySettings(settings, warnings, error);
}

