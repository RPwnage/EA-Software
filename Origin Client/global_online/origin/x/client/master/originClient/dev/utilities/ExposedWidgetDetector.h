#ifndef _EXPOSEDWIDGETDETECTOR_H
#define _EXPOSEDWIDGETDETECTOR_H

#include <QObject>

#include "services/plugin/PluginAPI.h"

class QWidget;
class QAbstractScrollArea;

namespace Origin
{
namespace Client
{

///
/// \brief Helper to heuristically determine if a widget is exposed
///
/// Exposed in this context means directly visible to the end user. This is different from a QWidget's visibility 
/// which is merely a flag indicating if a widget has been explicitly hidden or not
/// 
class ORIGIN_PLUGIN_API ExposedWidgetDetector : public QObject
{
    Q_OBJECT
public:
    ///
    /// \brief Constructs an instance watching the passed widget
    /// \param watched  Widget to watch. This is also used as the QObject parent
    ///
    ExposedWidgetDetector(QWidget *watched);
    
    ///
    /// \brief Constructs an instance watching the passed abstract scroll area
    /// \param watched  QAbstractScrollArea to watch. This is also used as the QObject parent
    ///
    ExposedWidgetDetector(QAbstractScrollArea *watched);

    ///
    /// \brief Returns if the watched widget is exposed
    /// This is conservative. An unexposed widget might be detected as exposed but not the converse.
    ///
    bool isExposed() const;

signals:
    /// \brief Fired when the watched widget is exposed
    void exposed();

    /// \brief Fired when the watched widget is unexposed
    void unexposed();

    ///
    /// \brief Fired when the watched widget's exposure changed
    ///
    /// \param  exposed Indicates if the widget is now exposed
    /// 
    void exposureChanged(bool exposed);
    
private:
    void init();
    
    bool eventFilter(QObject *, QEvent *event);

    void setExposed(bool exposed);
    bool isHiddenOrMinimized() const;

    QWidget *mPaintWidget;
    QWidget *mVisibilityWidget;
    QWidget *mWindow;

    bool mExposed;
};

}
}

#endif

