#include "app_controller.h"
#include "security/security_service.h"
#include "browser/webengine_environment.h"
#include "seb_settings.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QIcon>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QTextStream>
#include <QUrl>
#include <QProcess>
#include <QProcessEnvironment>
#include <QFileInfo>
#include <QDebug>
#include <cstdio>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <signal.h>

namespace {

int g_tty0_fd     = -1; /* /dev/tty0 */
int g_new_tty_fd  = -1; /* /dev/ttyN  */
int g_original_vt = -1; /* Which virtual terminal to return to */

void cleanup_vt_and_exit() {
    if ( g_new_tty_fd >= 0 ) {
        /* back to text mode */
        ioctl( g_new_tty_fd, KDSETMODE, KD_TEXT );
    }

    if (g_tty0_fd >= 0) {
        /* Switch back to the original virtual terminal */
        int target_vt = (g_original_vt > 0) ? g_original_vt : 1;
        ioctl(g_tty0_fd, VT_ACTIVATE, target_vt);
        ioctl(g_tty0_fd, VT_WAITACTIVE, target_vt);
    }

    /* Close the tty devices */
    if (g_new_tty_fd >= 0){ close(g_new_tty_fd); }
    if (g_tty0_fd    >= 0){ close(g_tty0_fd);    }
}

void barebones_sig_handler(int signum) {
    cleanup_vt_and_exit();
    if (signum == SIGSEGV || signum == SIGABRT || signum == SIGFPE || signum == SIGILL) {
        signal(signum, SIG_DFL);
        raise(signum);
    } else {
        _exit(1);
    }
}

/*
 * @return true on success
 */
bool setup_barebones_vt() {
    signal( SIGINT , barebones_sig_handler );
    signal( SIGTERM, barebones_sig_handler );
    signal( SIGSEGV, barebones_sig_handler );
    signal( SIGABRT, barebones_sig_handler );
    signal( SIGILL , barebones_sig_handler );
    signal( SIGFPE , barebones_sig_handler );

    // Open tty0
    g_tty0_fd = open("/dev/tty0", O_RDWR);
    if (g_tty0_fd < 0) {
        qWarning() << "Could not open /dev/tty0. Did you run the app as root?";
        return false;
    }

    // Find a free virtual terminal
    int free_vt = -1;
    if (ioctl(g_tty0_fd, VT_OPENQRY, &free_vt) < 0 || free_vt == -1) {
        qWarning() << "Could not find a free VT.";
        close(g_tty0_fd);
        g_tty0_fd = -1;
        return false;
    }

    // Open the corresponding tty
    char vt_name[20];
    snprintf(vt_name, sizeof(vt_name), "/dev/tty%d", free_vt);
    g_new_tty_fd = open(vt_name, O_RDWR);
    if (g_new_tty_fd < 0) {
        qWarning() << "Could not open VT" << vt_name;
        close(g_tty0_fd);
        g_tty0_fd = -1;
        return false;
    }

    // Remember which VT did we start from
    struct vt_stat vts;
    if (ioctl(g_tty0_fd, VT_GETSTATE, &vts) == 0) {
        g_original_vt = vts.v_active;
    }

    // Switch to new VT
    if (ioctl(g_tty0_fd, VT_ACTIVATE, free_vt) < 0 ||
        ioctl(g_tty0_fd, VT_WAITACTIVE, free_vt) < 0) {
        qWarning() << "Could not switch to VT" << free_vt;
        close(g_new_tty_fd);
        g_new_tty_fd = -1;
        close(g_tty0_fd);
        g_tty0_fd = -1;
        return false;
    }

    // Set new VT to graphics mode
    if (ioctl(g_new_tty_fd, KDSETMODE, KD_GRAPHICS) < 0) {
        qWarning() << "Error setting graphics mode on VT" << free_vt;
        int target_vt = (g_original_vt > 0) ? g_original_vt : 1;
        ioctl(g_tty0_fd, VT_ACTIVATE, target_vt);
        ioctl(g_tty0_fd, VT_WAITACTIVE, target_vt);
        close(g_new_tty_fd);
        g_new_tty_fd = -1;
        close(g_tty0_fd);
        g_tty0_fd = -1;
        return false;
    }

    // Restore original VT on exit.
    atexit(cleanup_vt_and_exit);
    return true;
}

#if 0
/*
 * Find and return --config <file> or <resource>
 *
 * Scan argv for (("-c" or "--config") followed by another argument)
 * or an argument not starting with "-".
 *
 * At the *first hit*, return "another argument" or the (argument not starting with "-").
 *
 */
QString findConfigPath_firstHit( int argc, char *argv[] ) {
    const QString c1 = QStringLiteral("--config");
    const QString c2 = QStringLiteral("-c");
    //
    QString res = {};
    for (int index = 1; index < argc; ++index) {
        const QString argument = QString::fromLocal8Bit( argv[index] );
        if ( (argument == c1 || argument == c2) && index + 1 < argc) {
            res = QString::fromLocal8Bit( argv[index+1] );
            break;
        }
        if ( !argument.startsWith('-') ) {
            res = argument;
            break;
        }
    }
    qWarning() << "findConfigPath_firstHit result: " << res ; // xxx
    return res;
}
#endif

/*
 * Find and return --config <file> or <resource>
 *
 * Scan argv for (("-c" or "--config") followed by <file>)
 * or <resource> : an argument not starting with "-".
 *
 * At the *first hit*, return <file> or <resource>.
 *
 * Note: For `--config ""` returns `""` even if `resource` is available.
 *
 */
QString findConfigPath_preferConfig( int argc, char *argv[] ) {
    const QString c1 = QStringLiteral("--config");
    const QString c2 = QStringLiteral("-c");
    //
    bool haveConfig   = false;
    bool haveResource = false;
    QString config   = {};
    QString resource = {};
    //
    for (int index = 1; index < argc; ++index) {
        const QString argument = QString::fromLocal8Bit( argv[index] );
        if ( (argument == c1 || argument == c2) && index + 1 < argc ) {
            if ( haveConfig ) {
              qWarning() << "Multiple { --config | -c } flags on the command line. Keeping the first.";
            } else {
              config     = QString::fromLocal8Bit( argv[index+1] );
              haveConfig = true;
            }
            index++;
        } else if ( !argument.startsWith('-') ) {
          if ( haveResource ) {
            qWarning() << "Multiple 'resource' arguments on the command line. Keeping the first.";
          } else {
            resource     = argument;
            haveResource = true;
          }
        }
    }
    QString result = ( haveConfig
                     ? config
                     : resource
                       );
    qWarning() << "findConfigPath_preferConfig result: " << result ; // xxx
    return result;
}


bool hasArgument( int argc, char *argv[], const QString& value ) {
    for (int index = 1; index < argc; ++index) {
        if (QString::fromLocal8Bit(argv[index]) == value) {
            return true;
        }
    }
    return false;
}

/*
 *  If environment variable with name `name` exists (and not empty),
 *  append "{name}={value}"  to `args`
 */
void appendPkexecEnvironmentVariable( QStringList& args, const char *name ) {
    const QByteArray value = qgetenv(name);
    if ( !value.isEmpty() ) {
        args << ( QString(name) + QLatin1Char('=') + QString::fromLocal8Bit(value) );
    }
}

/*
 * Run a  "pkexec" child process.
 *
 * Pkexec fails if it has no parent process, so it is ran without
 * detaching.
 *
 * @param child A  "pkexec" QProcess to run.
 */
int runNonDetachedPkexecChild( QProcess &child ) {
    // Do not detach. Forward outputs to parent.
    child.setProcessChannelMode( QProcess::ForwardedChannels );

    child.start();
    if (!child.waitForStarted()) {
        qWarning() << "pkexec child failed to start:" << child.errorString();
        return 1;
    }

    if (!child.waitForFinished(-1 /* no timeout */)) {
        qWarning() << "pkexec child failed:" << child.errorString();
        return 1;
    }

    if (child.exitStatus() != QProcess::NormalExit) {
        qWarning() << "pkexec child crashed";
        return 1;
    }

    const int exitCode = child.exitCode();
    qDebug() << "pkexec child finished with exit code" << exitCode;
    return exitCode;
}

/*
 * Prepare `window_settings` for "protected mode".
 *
 *  @param window_settings WindowSettings to be modified.
 *  @param fullScreen Goes into settings.fullScreenMode
 */
void applyProtectedWindowSettings( seb::WindowSettings &window_settings, bool fullScreen ) {
    window_settings.absoluteHeight = 0;
    window_settings.absoluteWidth  = 0;
    window_settings.relativeHeight = 100;
    window_settings.relativeWidth  = 100;
    //
    window_settings.allowAddressBar         = false;
    window_settings.allowBackwardNavigation = false;
    window_settings.allowDeveloperConsole   = false;
    window_settings.allowForwardNavigation  = false;
    window_settings.allowMinimize           = false;
    window_settings.allowReloading          = false;
    //
    window_settings.alwaysOnTop             = true;
    window_settings.frameless               = true;
    //
    window_settings.fullScreenMode          = fullScreen;
    //
    window_settings.showHomeButton    = false;
    window_settings.showReloadButton  = false;
    window_settings.showReloadWarning = false;
    window_settings.showToolbar       = false;
    window_settings.position          = seb::WindowPosition::Center;
}

/*
 * Prepare `seb_settings` for "protected mode"
 *
 * @param seb_settings To be modified
 * @param fullScreen For the main window
 *
 * @param allowConfiguredApps If false, clear application blacklist
 *        and whitelist, set
 *        seb_settings.taskbar.showProctoringNotification to false.
 *
 * @param allowTermination For seb_settings.security.allowTermination
 */
void applyProtectedSessionSettings( seb::SebSettings &seb_settings,
                                    bool fullScreen,
                                    bool allowConfiguredApps,
                                    bool allowTermination
    )
{
    applyProtectedWindowSettings( seb_settings.browser.mainWindow      , fullScreen);
    applyProtectedWindowSettings( seb_settings.browser.additionalWindow, false     );
    //
    seb_settings.browser.additionalWindow.relativeWidth = 100; /* Why repeat here? */
    //
    seb_settings.browser.popupPolicy         = seb::PopupPolicy::AllowSameWindow;
    seb_settings.browser.allowConfigurationDownloads      = false;
    seb_settings.browser.allowCustomDownAndUploadLocation = false;
    seb_settings.browser.allowDownloads                   = false;
    seb_settings.browser.allowFind                        = false;
    seb_settings.browser.allowPageZoom                    = false;
    seb_settings.browser.allowPrint                       = false;
    seb_settings.browser.allowSpellChecking               = false;
    seb_settings.browser.allowUploads                     = false;
    seb_settings.taskbar.showApplicationInfo = false;
    seb_settings.taskbar.showApplicationLog  = false;
    seb_settings.taskbar.showAudio           = false;
    seb_settings.taskbar.showKeyboardLayout  = false;
    seb_settings.taskbar.showNetwork         = false;
    seb_settings.security.allowTermination   = allowTermination;
    //
    if ( !allowConfiguredApps ) {
        seb_settings.applications.whitelist.clear();
        seb_settings.applications.blacklist.clear();
        seb_settings.taskbar.showProctoringNotification = false;
    }
}

/*
 * Purpose: Decide if we need an anti-cheat VT according to the
 * command line.
 *
 * This function fails if (not running as root, but "--menu-lockdown"
 * or "--anti-cheat" is in argv).
 *
 *  loadSettingsFromFile().
 *
 *  If "--menu-lockdown" or ("--anti-cheat" and have config), then
 *       setup_barebones_vt() // Note: this can fail if not root.
 *       setenv "QT_QPA_PLATFORM", "QT_QUICK_BACKEND"
 *
 *  Finally: seb::browser::applyWebEngineEnvironment( seb_settings )
 */
void applyEarlyEnvironment( int argc, char *argv[] ) {
    const QString configPath    = findConfigPath_preferConfig(argc, argv);
    const bool hasConfig        = !configPath.isEmpty();
    //
    seb::SebSettings seb_settings = seb::defaultSettings();
    if ( hasConfig ) {
        const seb::LoadResult loaded = seb::loadSettingsFromFile(configPath);
        if (loaded.ok) {
            seb_settings = loaded.settings;
        }
    }
    //
    const bool isExamAntiCheat = hasArgument(argc, argv, QStringLiteral("--anti-cheat"));
    const bool isMenuLockdown  = hasArgument(argc, argv, QStringLiteral("--menu-lockdown"));
    //
    if ( isExamAntiCheat || isMenuLockdown ) {
        if ( isMenuLockdown || (isExamAntiCheat && hasConfig) ) {
            if ( !setup_barebones_vt() ) {
                qCritical() << "Anti-cheat VT setup failed; aborting.";
                _exit(1);
            }
            qputenv("QT_QPA_PLATFORM" , "linuxfb");
            qputenv("QT_QUICK_BACKEND", "software");
        }
#if SEB_HAS_QTWEBENGINE
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--no-sandbox");
#endif
    }
    //
    // Add proxy setup flags to "QTWEBENGINE_CHROMIUM_FLAGS"
    seb::browser::applyWebEngineEnvironment( seb_settings );
}

/*
 *  Command line options can override some settings in `settings`
 *
 * @param parser Provides info on command line options.
 * @parser seb_settings Settings to be modified.
 */
void applyCommandLineOverrides( const QCommandLineParser &parser, seb::SebSettings &seb_settings ) {

#define MW_bool1( name, mainwindow_field, value ) \
    if ( parser.isSet(name) ) { seb_settings.browser.mainWindow.mainwindow_field = value; }

#define MW_bool2( name, mainwindow_field1, value1, mainwindow_field2, value2 )  \
    if ( parser.isSet(name) ) {                                                 \
        seb_settings.browser.mainWindow.mainwindow_field1 = value1;             \
        seb_settings.browser.mainWindow.mainwindow_field2 = value2;             \
    }

    if ( parser.isSet("url") ) {
        seb_settings.browser.startUrl = parser.value("url").trimmed();
    }

    MW_bool1( "show-toolbar"     , showToolbar    , true );

    MW_bool2( "allow-address-bar", allowAddressBar, true
                                 , showToolbar    , true );

    MW_bool2( "allow-navigation" , allowBackwardNavigation, true
                                 , allowForwardNavigation , true  );

    MW_bool2( "allow-reload"     , allowReloading         , true
                                 , showReloadButton       , true  );

    MW_bool1( "allow-devtools"   , allowDeveloperConsole  , true  );
    MW_bool1( "windowed"         , fullScreenMode         , false );
    MW_bool1( "fullscreen"       , fullScreenMode         , true  );
    MW_bool1( "always-on-top"    , alwaysOnTop            , true  );
    MW_bool1( "disable-minimize" , allowMinimize          , false );

    if ( parser.isSet("disable-quit") ) {
        seb_settings.security.allowTermination = false;
    }

#if defined(QT_DEBUG) || defined(SEB_DEV_BYPASS_OPTION)
    if ( parser.isSet("dev-bypass") ) {
        seb_settings.devBypass = true;
    }
#endif

#undef MW_bool1
#undef MW_bool2
}

}  // namespace


int main( int argc, char *argv[] ) {

    /*  For --menu-lockdown or --anti-cheat, we need to run as
     *  root. (Maybe group tty is enough?)
     */
    applyEarlyEnvironment(argc, argv);

    QApplication app(argc, argv);

    {
        const QIcon appIcon( QStringLiteral(":/assets/icons/safe-exam-browser.png") );
        app.setWindowIcon(appIcon);
    }

    app.setDesktopFileName(                  QStringLiteral("safe-exam-browser") );
    QCoreApplication::setApplicationName(    QStringLiteral("Safe Exam Browser") );
    QCoreApplication::setApplicationVersion( QStringLiteral("0.1.0") );

    QCommandLineParser parser;
    parser.setApplicationDescription( QStringLiteral(
         "Safe Exam Browser for Linux"
         " with .seb file support, exam link handling,"
         " and Qt WebEngine or WebKitGTK browser backends." ));
    parser.addHelpOption();
    parser.addVersionOption();

    // OPT211( name1, name2, desc, valueName ) defines a command line
    //            option that has two names, a description and a name
    //            for the corresponding value.
#define OPT211( name1, name2, desc, valueName  )       \
    parser.addOption( QCommandLineOption(              \
               QStringList{ QStringLiteral(name1)      \
                          , QStringLiteral(name2) }    \
             , QStringLiteral( desc )                  \
             , QStringLiteral( valueName )   ) )

    OPT211( "c", "config"
          , "Load settings from a JSON file or an unencrypted XML plist .seb file."
          , "file" );

    OPT211( "u", "url"
          , "Override the configured start URL."
          , "url" );

#undef OPT211

    parser.addPositionalArgument(
          QStringLiteral("resource")
        , QStringLiteral("Open a local .seb file, remote .seb URL, or seb:// / sebs:// resource.")
        );

    // OPT11( name, desc ) defines a command line option with a single
    //                     name and description.
#define OPT11( name, desc ) parser.addPositionalArgument( QStringLiteral(name), QStringLiteral(desc) )

    OPT11( "show-toolbar"      , "Show the browser toolbar.");
    OPT11( "allow-address-bar" , "Enable the address bar."  );
    OPT11( "allow-navigation"  , "Enable back and forward navigation in the main window.");
    OPT11( "allow-reload"      , "Enable reload in the main window."          );
    OPT11( "allow-devtools"    , "Enable the developer tools shortcut (F12)." );
    OPT11( "windowed"          , "Force the main window to stay windowed."    );
    OPT11( "fullscreen"        , "Force the main window to be fullscreen."    );
    OPT11( "always-on-top"     , "Keep the main window above other windows."  );
    OPT11( "disable-minimize"  , "Prevent minimizing the main exam window."   );
    OPT11( "disable-quit"      , "Disable manual termination even if the configuration allows it." );
    OPT11( "anti-cheat"        , "(*) Enable anticheat mode."    );
    OPT11( "menu-lockdown"     , "(*) Enable the protected start-menu lockdown mode." );

#if defined(QT_DEBUG) || defined(SEB_DEV_BYPASS_OPTION)
    OPT11( "dev-bypass"        , "(*) Skip strict lockdowns for development purposes." );
#endif

#undef OPT11

    parser.process( app );

    seb::SebSettings settings = seb::defaultSettings();
    QTextStream err(stderr);

#if !SEB_HAS_QTWEBENGINE
    err << "warning: This build was compiled without QtWebEngine support."
           " Safe Exam Browser will start in compatibility mode"
           " and cannot render exam pages."
        << Qt::endl;
#endif

    /*
     * Unlike in findConfigPath_firstHit(), here "--config <file>" is stronger then earlier <resource>
     *
     * findConfigPath_preferConfig() is more similar to this one.
     */
    const QString resource = parser.isSet("config")
                           ? parser.value("config")
                           : ( parser.positionalArguments().isEmpty()
                             ? QString()
                             : parser.positionalArguments().constFirst()
                             );
    qWarning() << "main resource is: " << resource ; // xxx

    /*
     * Get password from env SEB_PASSWORD or from GUI.
     */
    QString userPassword;
    bool    usedPassword = false;

    QStringList warnings;
    if ( !resource.isEmpty() ) {
        seb::PasswordProvider passwordProvider =  [&userPassword, &usedPassword](bool hashed) {
            if ( qEnvironmentVariableIsSet("SEB_PASSWORD") ) {
                userPassword = QString::fromUtf8(qgetenv("SEB_PASSWORD"));
                usedPassword = true;
                return userPassword;
            }
            //
            bool accepted = false;
            const QString password = QInputDialog::getText(
                nullptr
                , QStringLiteral("SEB Password Required")
                , ( hashed
                  ? QStringLiteral("Enter the administrator password for this client-configuration SEB file.")
                  : QStringLiteral("Enter the password for this SEB configuration file.")
                  )
                , QLineEdit::Password
                , QString()
                , &accepted
                );
            if (accepted) {
                userPassword = password;
                usedPassword = true;
            }
            return accepted ? password : QString();
        };

        const seb::ResourceLoadResult loaded =
            seb::loadSettingsFromResource( resource
                                         , passwordProvider  );

        if (!loaded.ok) {
            err << loaded.error << Qt::endl;
            return 1;
        }

        settings = loaded.settings;
        warnings = loaded.warnings;
    }

    applyCommandLineOverrides(parser, settings);


    const bool launchedWithoutExam = resource.isEmpty();
    const bool menuLockdown        = parser.isSet("menu-lockdown");
    const bool examAntiCheat       = parser.isSet("anti-cheat");
#if defined(QT_DEBUG) || defined(SEB_DEV_BYPASS_OPTION)
    bool devBypass = settings.devBypass || parser.isSet("dev-bypass");
#else
    bool devBypass = settings.devBypass;
#endif
#ifdef SEB_DEV_BYPASS_DEFAULT
    devBypass = true;
#endif

    if ( !devBypass && !examAntiCheat && !menuLockdown ) {
        if ( launchedWithoutExam ) {
            {
                // Get permission or exit
                const auto answer = QMessageBox::question(
                    nullptr,
                    QStringLiteral("Administrator Privileges Required"),
                    QStringLiteral(
                        "Safe Exam Browser needs administrator privileges "
                        "to apply lockdown protections.\n\nDo you want to continue?"),
                    QMessageBox::Yes | QMessageBox::Cancel,
                    QMessageBox::Yes);
                if (answer != QMessageBox::Yes) {
                    return 0;
                }
            }

            {
                // Run with privileges and exit
                QStringList args = QCoreApplication::arguments();
                args.removeFirst();

                // Possibly running from menu, do lockdown
                args.prepend(QStringLiteral("--menu-lockdown"));

                QProcess child;
                child.setProgram(QStringLiteral("pkexec"));

                QStringList pkexecArgs;
                pkexecArgs << QStringLiteral("--keep-cwd");
                pkexecArgs << QStringLiteral("env");
                appendPkexecEnvironmentVariable(pkexecArgs, "DISPLAY");
                appendPkexecEnvironmentVariable(pkexecArgs, "WAYLAND_DISPLAY");
                appendPkexecEnvironmentVariable(pkexecArgs, "XAUTHORITY");
                appendPkexecEnvironmentVariable(pkexecArgs, "XDG_RUNTIME_DIR");
                appendPkexecEnvironmentVariable(pkexecArgs, "XDG_SESSION_TYPE");
                appendPkexecEnvironmentVariable(pkexecArgs, "DBUS_SESSION_BUS_ADDRESS");
                appendPkexecEnvironmentVariable(pkexecArgs, "QT_QPA_PLATFORM");
                //
                // No password here
                //
                pkexecArgs << QCoreApplication::applicationFilePath();
                pkexecArgs << args;

                child.setArguments(pkexecArgs);
                return runNonDetachedPkexecChild(child);
            }
        }

        const bool requiresLockedExamShell = settings.browser.mainWindow.fullScreenMode
                                          && settings.browser.mainWindow.alwaysOnTop;

        if ( requiresLockedExamShell ) {
            {
                // Get permission or exit
                const auto answer = QMessageBox::question(
                    nullptr,
                    QStringLiteral("Administrator Permission Required"),
                    QStringLiteral(
                        "Safe Exam Browser needs administrator privileges"
                        "to apply exam locking and anti-cheat protections.\n\nDo you want to continue?"),
                    QMessageBox::Yes | QMessageBox::Cancel,
                    QMessageBox::Yes);
                if (answer != QMessageBox::Yes) {
                    return 0;
                }
            }

            {
                // Run with privileges and exit
                QStringList args = QCoreApplication::arguments();
                args.removeFirst(); // Remove executable path
                if (!args.contains(QStringLiteral("--anti-cheat"))) {
                    args.prepend(QStringLiteral("--anti-cheat"));
                }

                QProcess child;
                child.setProgram(QStringLiteral("pkexec"));

                QStringList pkexecArgs;
                pkexecArgs << QStringLiteral("--keep-cwd");
                pkexecArgs << QStringLiteral("env");
                appendPkexecEnvironmentVariable(pkexecArgs, "DISPLAY");
                appendPkexecEnvironmentVariable(pkexecArgs, "WAYLAND_DISPLAY");
                appendPkexecEnvironmentVariable(pkexecArgs, "XAUTHORITY");
                appendPkexecEnvironmentVariable(pkexecArgs, "XDG_RUNTIME_DIR");
                appendPkexecEnvironmentVariable(pkexecArgs, "XDG_SESSION_TYPE");
                appendPkexecEnvironmentVariable(pkexecArgs, "DBUS_SESSION_BUS_ADDRESS");
                appendPkexecEnvironmentVariable(pkexecArgs, "QT_QPA_PLATFORM");
                if (usedPassword) {
                    pkexecArgs << (QStringLiteral("SEB_PASSWORD=") + userPassword);
                }
                pkexecArgs << QCoreApplication::applicationFilePath();
                pkexecArgs << args;

                child.setArguments(pkexecArgs);
                return runNonDetachedPkexecChild(child);
            }
        } // if (requiresLockedExamShell)
    } // if ( !devBypass  && !examAntiCheat && !menuLockdown )

    //
    // If we get here, at least one of devBypass, examAntiCheat or menuLockdown
    // is set. We make no more attempt to get root privileges.
    //
    if ( devBypass ){
        seb::applyDevBypassOverrides(settings);
    } else {
        // !devBypass
        {
            const bool haveExamWithStrictSettings = (
                !launchedWithoutExam
                && settings.browser.mainWindow.fullScreenMode
                && settings.browser.mainWindow.alwaysOnTop
                );
            if ( menuLockdown && launchedWithoutExam ) {
                applyProtectedSessionSettings(settings
                                              , false // fullScreen
                                              , false // allowConfiguredApps
                                              , true  // allowTermination
                    );
            } else if ( examAntiCheat || haveExamWithStrictSettings ) {
                applyProtectedSessionSettings(settings
                                              , true  // fullScreen
                                              , true  // allowConfiguredApps
                                              , false // allowTermination
                    );
            } else {
                // We can get here if (!devBypass)
                //   and ( (menuLockdown && !launchedWithoutExam)
                //         or
                //         (...) )
                // Anyway, we go on without modifying `settings`.
            }
        }
    }

    // Open BrowserWindow with settings
    AppController controller;
    QString       launchError;
    if ( !controller.launchResolved( settings, warnings, &launchError ) ) {
        err << launchError << Qt::endl;
        return 1;
    }

    // Check security, start monitoring.
    seb::security::SecurityService security;
    if ( !devBypass ) {
        if ( security.isVirtualMachine() ) {
            err << "Error: Running in a virtual machine is not allowed." << Qt::endl;
            return 1;
        }
        if ( security.isDebuggerAttached() ) {
            err << "Error: A debugger is attached." << Qt::endl;
            return 1;
        }

        QObject::connect(  &security
                           , &seb::security::SecurityService::secureViolationDetected
                           , [&app](const QString &reason) {
                                 qCritical() << "Security Violation:" << reason;
                                 app.quit();
                           }
            );
        security.startMonitoring();
    } else {
        qDebug() << "Developer bypass active; security monitoring disabled.";
    }

    return app.exec();
}
