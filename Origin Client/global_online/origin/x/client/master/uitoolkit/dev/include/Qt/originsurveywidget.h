#ifndef ORIGINSURVEYWIDGET_H
#define ORIGINSURVEYWIDGET_H

#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

class Ui_OriginSurveyWidget;

namespace Origin {
    namespace UIToolkit {
        
/// \brief Creates a survey widget
///
/// This is a simple survey widget that can return a value for the survey

class UITOOLKIT_PLUGIN_API OriginSurveyWidget: public QWidget, public OriginUIBase
{
    Q_OBJECT
    Q_PROPERTY(short result READ getResult WRITE setResult)
  
public:
    /// \brief Constructor
    ///
    /// Creates the survey widget
    /// \param parent The QWidget that is the parent of this object, default is NULL
    OriginSurveyWidget(QWidget* parent = NULL);

    ///\brief destructor
    ~OriginSurveyWidget();

    /// \brief Sets the default value for the result.
    /// \param result unsigned short 0-10 where 0 is not at all unlikely and 10 is extremely likely
    /// any value outside the range [0,10] unselects all radio buttons and sets the result to -1.
    void setResult(short result);

    /// \brief Gets the value for the result.
    /// \param result short between -1 to 10 where -1 if not initialized, 0 to 10 otherwise. 0 is not at all unlikely and 10 is extremely likely
    short getResult() const;

signals:
    void resultChanged(int);

protected:
    void paintEvent(QPaintEvent* event);
    void resizeEvent(QResizeEvent* event);
    bool event(QEvent* event);

    QWidget* getSelf() {return this;}
    QString& getElementStyleSheet();
    void prepForStyleReload();

private slots:
    void btnToggled(bool);

private:
    static QString mPrivateStyleSheet;
    short mResult;
    Ui_OriginSurveyWidget* ui;

    
}; // OriginSurveyWidget

    } // uiToolkit
} // origin
#endif
