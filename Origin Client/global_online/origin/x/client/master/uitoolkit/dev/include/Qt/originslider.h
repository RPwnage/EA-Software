#ifndef ORIGINSLIDER_H
#define ORIGINSLIDER_H

#include <QSlider>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Origin {
namespace UIToolkit {
/// \brief A widget used to change a value based on user interaction.
/// 
/// <A HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginSlider">Click here for more OriginSlider documentation</A>

class UITOOLKIT_PLUGIN_API OriginSlider : public QSlider, public OriginUIBase
{
    Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent Pointer to the parent widget, defaults to NULL
    OriginSlider(QWidget* parent = 0);
    
	/// \brief Destructor
    ~OriginSlider();


protected:
    void paintEvent(QPaintEvent* event);
    void mousePressEvent(QMouseEvent* event);
    bool event(QEvent* event);
    QSlider* getSelf() {return this;}
    QString& getElementStyleSheet();
    void prepForStyleReload();


private:
    static QString mPrivateStyleSheet;
};
}
}
#endif
