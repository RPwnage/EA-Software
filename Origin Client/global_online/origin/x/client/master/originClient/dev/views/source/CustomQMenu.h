#ifndef CUSTOMQMENU_H
#define CUSTOMQMENU_H

#include <QMenu>
#include "services/plugin/PluginAPI.h"
#include "UIScope.h"

namespace Origin
{
namespace Client
{
class ORIGIN_PLUGIN_API CustomQMenu : public QMenu
{
    Q_OBJECT

public:
    CustomQMenu(const UIScope& scope, QWidget* parent = 0, const bool& flatTop = false);
    CustomQMenu(const UIScope& scope, const QString& title, QWidget* parent = 0, const bool& flatTop = false);


protected:
    bool event(QEvent*);


private:
    const UIScope mScope;
    const bool mFlatTop;
};
}
}
#endif //CUSTOMQMENU_H