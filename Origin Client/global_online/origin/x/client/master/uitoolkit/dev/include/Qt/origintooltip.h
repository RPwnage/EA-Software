#ifndef ORIGINTOOLTIP_H
#define ORIGINTOOLTIP_H

#include <QWidget>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Origin {
namespace UIToolkit {
/// \brief Tooltip
/// 
/// <A HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginTooltip">Click here for more OriginTooltip documentation</A>
class UITOOLKIT_PLUGIN_API OriginTooltip : public QWidget, public OriginUIBase
{
    Q_OBJECT

public:
	/// \brief Constructor
	/// \param parent Pointer to the parent widget, defaults to NULL
    OriginTooltip(QWidget *parent = 0);
	/// \brief Destructor
    ~OriginTooltip();


protected:
    void paintEvent(QPaintEvent* event);
    void enterEvent(QEvent* event);
    bool event(QEvent* event);
    QWidget* getSelf() {return this;}
    QString& getElementStyleSheet();
    void prepForStyleReload() {mPrivateStyleSheet = "";}


private:
    static QString mPrivateStyleSheet;
};

}
}
#endif