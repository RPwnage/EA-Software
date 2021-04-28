#ifndef ORIGINDIVIDER_H_
#define ORIGINDIVIDER_H_

#include <QWidget>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Origin
{
namespace UIToolkit
{
/// \brief Close button for Origin
///
/// <A HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginDivider">Click here for more OriginDivider documentation</A>

class UITOOLKIT_PLUGIN_API OriginDivider : public QWidget, public OriginUIBase
{
    Q_OBJECT
    Q_ENUMS(LineStyle)
    Q_ENUMS(LineOrientation)
    Q_PROPERTY(LineStyle lineStyle READ lineStyle WRITE setLineStyle)
    Q_PROPERTY(LineOrientation lineOrientation READ lineOrientation WRITE setLineOrientation)

public:
    enum LineStyle
    {
        Invalid,
        DarkGrey_White,
        DarkGrey_LightGrey,
        LightGrey_White,
        DarkGreen_LightGreen,
        Dotted_DarkGrey,
        Dotted_LightGrey
    };

    enum LineOrientation
    {
        Horizontal,
        Vertical
    };

    OriginDivider(QWidget* parent = 0);
    ~OriginDivider();

    void setLineStyle(const LineStyle& lineStyle);
    LineStyle lineStyle() const { return mLineStyle; }

    void setLineOrientation(const LineOrientation& lineOrientation);
    LineOrientation lineOrientation() const { return mLineOrientation; }

protected:
     void paintEvent(QPaintEvent* event);
     bool event(QEvent* event);
     QWidget* getSelf() {return this;}
     QString&  getElementStyleSheet();
     void prepForStyleReload();

private:
    static QString mPrivateStyleSheet;
    LineStyle mLineStyle;
    LineOrientation mLineOrientation;
};

}
}

#endif