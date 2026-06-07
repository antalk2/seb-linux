#include "ui/taskbar/controls/window_list_control.h"

#include "ui/taskbar/controls/window_list_popup.h"
#include "ui/taskbar/common/taskbar_button.h"
// #include "ui/taskbar/common/taskbar_style.h"

#include "seb_session.h"
#include "browser_window.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QTimer>

namespace seb::ui::taskbar {

WindowListControl::WindowListControl(SebSession &session, QWidget *parent)
    : QWidget(parent)
    , session_(session)
{
  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  button_ = new TaskbarButton(this);
  button_ ->setFixedWidth(50);
  button_ ->setToolTip(  QStringLiteral( "Safe Exam Browser Window List" ));
  button_ ->setIconPath( QStringLiteral( ":/assets/icons/safe-exam-browser.png" ));
  layout->addWidget(button_);

  windowListpopup_     = new WindowListPopup(this);
  hideTimer_ = new QTimer(this);
  hideTimer_ ->setSingleShot(true);
  hideTimer_ ->setInterval(250);

 connect( button_   , &QPushButton::clicked              , this
        , &WindowListControl::handleClicked
        );

 connect( button_   , &QPushButton::pressed              , hideTimer_
        , qOverload<>( &QTimer::stop )
        );

 connect( button_   , &QPushButton::released             , hideTimer_
          , qOverload<>( &QTimer::stop )
          );

 connect( &session_ , &SebSession::browserWindowsChanged , this
          , &WindowListControl::updateState
          );

 connect( windowListpopup_    , &WindowListPopup::windowSelected   , &session_
          , &SebSession::activateWindow
          );
 
 connect( windowListpopup_    , &TaskbarPopup::popupHidden         , this
          , [this] {
            button_ ->setHasPopupOpen(false);
          }
        );

 connect( hideTimer_ , &QTimer::timeout, this
          , &WindowListControl::hidePopupIfInactive
          );

    button_ ->installEventFilter(this);
    windowListpopup_  ->installEventFilter(this);

    updateState();
}

bool WindowListControl::eventFilter( QObject *watched, QEvent *event ) {

  if (watched == button_) {
    if ( event->type() == QEvent::Enter ) {
      hideTimer_ ->stop();
      showPopupIfNeeded();
    } else if ( event->type() == QEvent::Leave ) {
      hideTimer_ ->start();
    }
  } else if ( watched == windowListpopup_ ) {
    if (event->type() == QEvent::Enter) {
      hideTimer_ ->stop();
    } else if (event->type() == QEvent::Leave) {
      hideTimer_ ->start();
    }
  }

  return QWidget::eventFilter(watched, event);
}

void WindowListControl::handleClicked()
{
    const QList<BrowserWindow *> windows = session_.browserWindows();
    if (windows.isEmpty()) {
        return;
    }
    if (windows.size() == 1) {
        session_.activateWindow(windows.first());
        return;
    }

    windowListpopup_  ->setWindows(windows);
    button_ ->setHasPopupOpen(true);
    windowListpopup_  ->showAbove(button_);
}

void WindowListControl::showPopupIfNeeded()
{
    const QList<BrowserWindow *> windows = session_.browserWindows();
    if (windows.size() <= 1) {
        return;
    }
    windowListpopup_ ->setWindows(windows);
    button_ ->setHasPopupOpen(true);
    windowListpopup_ ->showAbove(button_);
}

void WindowListControl::hidePopupIfInactive()
{
    if (!button_ ->underMouse() && !windowListpopup_ ->underMouse()) {
        windowListpopup_ ->hide();
    }
}

void WindowListControl::updateState()
{
    const QList<BrowserWindow *> windows = session_.browserWindows();
    button_ ->setActiveLineVisible(!windows.isEmpty());
    windowListpopup_ ->setWindows(windows);
}

}  // namespace seb::ui::taskbar
