#pragma once

#include <QFrame>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace seb::ui::taskbar {

class TaskbarPopup : public QFrame
{
    Q_OBJECT

public:
    explicit TaskbarPopup(QWidget *parent = nullptr);

  /*
   * side : 0 -> center horizontally, negative -> align left edges , positive -> align right edges
   */
  void showAbove(QWidget *anchor, int side=0);

signals:
    void popupHidden();

protected:
    void hideEvent(QHideEvent *event) override;
};

}  // namespace seb::ui::taskbar
