#include "ui/taskbar/common/taskbar_popup.h"

#include "ui/taskbar/common/taskbar_style.h"

#include <QHideEvent>
#include <QPoint>
#include <QWidget>

namespace seb::ui::taskbar {

  TaskbarPopup::TaskbarPopup(QWidget *parent )
    : QFrame(parent, Qt::Popup | Qt::FramelessWindowHint)
{
    setAttribute(Qt::WA_DeleteOnClose, false);
    setStyleSheet(popupStyleSheet());
}

 
void TaskbarPopup::showAbove(QWidget *anchor, int side)
{
    if (!anchor) {
        return;
    }

    adjustSize();

    int xpos =  anchor->width()/2 - width()/2;
    if ( side < 0 ){
      xpos = 0;
    } else if ( side > 0 ){
      xpos = anchor->width() - width();
    }
    int ypos =  -height() - kPopupTopMargin ;
    const QPoint global = anchor->mapToGlobal( QPoint( xpos, ypos ) );
    move(global);
    show();
    raise();
}

void TaskbarPopup::hideEvent(QHideEvent *event)
{
    QFrame::hideEvent(event);
    emit popupHidden();
}

}  // namespace seb::ui::taskbar
