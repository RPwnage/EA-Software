#include "CustomQMenu.h"
#include "services/debug/DebugService.h"
#include "services/qt/QtUtil.h"
#include <QEvent>
#include <QSysInfo>

namespace Origin
{
namespace Client
{

CustomQMenu::CustomQMenu(const UIScope& scope, QWidget* parent, const bool& flatTop)
: QMenu(parent)
, mScope(scope)
, mFlatTop(flatTop)
{
}


CustomQMenu::CustomQMenu(const UIScope& scope, const QString& title, QWidget* parent, const bool& flatTop)
: QMenu(title, parent)
, mScope(scope)
, mFlatTop(flatTop)
{
}


bool CustomQMenu::event(QEvent* e)
{
    if (mScope == ClientScope)
    {
        switch(e->type())
        {
        // EBIBUGS-14978
        // We are using a rounded rectangular mask for XP since we can't use translucency.  Recalculate every time we show or resize the menu.
        case QEvent::Show:
        case QEvent::Resize:
            Origin::Services::DrawRoundCornerForMenu(this, 6, mFlatTop);
            break;
        default:
            break;
        }
    }
    return QMenu::event(e);
}

}
}