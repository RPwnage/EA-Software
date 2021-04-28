#ifndef ORIGINTRANSFERBAR_H_
#define ORIGINTRANSFERBAR_H_

#include <QWidget.h>
#include "originuiBase.h"
#include "originprogressbar.h"
#include "uitoolkitpluginapi.h"

namespace Ui{
class OriginTransferBar;
}
    namespace Origin {
    namespace UIToolkit {
/// \brief Provides a wrapper around OriginProgressBar that adds additional UI typically used to display a disk install, internet download, installation progress, etc.
/// 
/// <A HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginTransferBar">Click here for more OriginTransferBar documentation</A>

class UITOOLKIT_PLUGIN_API OriginTransferBar : public QWidget, public OriginUIBase
{
    Q_OBJECT
    Q_PROPERTY(QString state READ getState WRITE setState DESIGNABLE true)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle DESIGNABLE true)
    Q_PROPERTY(QString completed READ getCompleted WRITE setCompleted DESIGNABLE true)
    Q_PROPERTY(QString rate READ getRate WRITE setRate DESIGNABLE true)
    Q_PROPERTY(QString timeRemaining READ getTimeRemaining WRITE setTimeRemaining DESIGNABLE true)
    Q_PROPERTY(double value READ getValue WRITE setValue)

public:
    /// \brief Gets the state label text that appears in the upper right
    /// 
    /// Design recommends that all state labels end with ellipsis (eg "Downloading...")
    /// \return QString Contains the text of the state label
    const QString& getState() const { return state; }
    /// \brief Sets the state label text that appears in the upper right
    /// 
    /// Design recommends that all state labels end with ellipsis (eg "Downloading...")
    /// \param aState QString that contains the text of the label
    void setState(QString const& aState);

    /// \brief Gets the title label text that appears in the upper left
    /// 
    /// Examples include the name of game being transferred
    /// \return QString Contains the text of the title label
    const QString& getTitle() const { return title; }
    /// \brief Sets the title label text that appears in the upper left
    /// 
    /// Examples include the name of game being transferred
    /// \param aTitle QString that contains the text of the title label
    void setTitle(QString const& aTitle);

    /// \brief Gets the Amount Completed Label that appears in the lower left
    /// 
    /// Example: "1.23 MB / 5,344 GB"
    /// 
    /// Note: the design calls for this label's text to be selectively bolded (refer to the widget screenshot). The label supports limited HTML markup, so this can be achieved using the following as an example: "<b>1.23</b> MB / <b>5,344</b> MB"
    /// \return QString Contains the text of the completed label
    const QString& getCompleted() const { return completed; }
    /// \brief Sets the Amount Completed Label that appears in the lower left
    /// 
    /// Example: "1.23 MB / 5,344 GB"
    /// 
    /// Note: the design calls for this label's text to be selectively bolded (refer to the widget screenshot). The label supports limited HTML markup, so this can be achieved using the following as an example: "<b>1.23</b> MB / <b>5,344</b> MB"
    /// \param aCompleted QString that contains the text of the completed label
    void setCompleted(QString const& aCompleted);

    /// \brief Gets the transfer rate label text that appears in lower center area
    /// 
    /// Example:  "951.6 KB/S"
    /// 
    /// Note: the design calls for this label's text to be selectively bolded (refer to the widget screenshot). The label supports limited HTML markup, so this can be achieved using the following as an example: "<b>951.6</b> KB/S"
    /// \return QString Contains the text of the transfer rate label
    const QString& getRate() const { return rate; }
    /// \brief Sets the transfer rate label text that appears in lower center area
    /// 
    /// Example:  "951.6 KB/S"
    /// 
    /// Note: the design calls for this label's text to be selectively bolded (refer to the widget screenshot). The label supports limited HTML markup, so this can be achieved using the following as an example: "<b>951.6</b> KB/S"
    /// \return aRate QString that contains the text of the transfer rate label
    void setRate(QString const& aRate);

    /// \brief Gets the remaining time label text that appears in the lower right area
    /// 
    /// Example:  "1h:23m:00 Remaining"
    /// \return QString Contains the text of the remaining time label
    const QString& getTimeRemaining() const { return timeRemaining; }
    /// \brief Sets the remaining time label text that appears in the lower right area
    /// 
    /// Example:  "1h:23m:00 Remaining"
    /// \return aTimeRemaining QString that contains the text of the remaining time label
    void setTimeRemaining(QString const& aTimeRemaining);

    /// \brief Gets the OriginProgressBar that is in the center of the widget
    /// \return OriginProgressBar Pointer to the progress bar
    OriginProgressBar* getProgressBar();
    /// \brief Gets the current value of the progress bar
    /// 
    /// This property holds the progress bar's current value. Fractional values are supported. The "filled amount"
    /// of the progress bar is dependent on the value, minimum, and maximum properties of the progress bar.
    /// \return float The current value of the progress bar
    float getValue() const;

public slots:
    /// \brief Sets the value of the progress bar
    /// 
    /// This property holds the progress bar's current value. Fractional values are supported. The "filled amount"
    /// of the progress bar is dependent on the value, minimum, and maximum properties of the progress bar.
    /// \param double The value to set.
    void setValue(double);
    /// \brief Sets the value of the progress bar
    /// 
    /// This property holds the progress bar's current value. The "filled amount" of the progress bar is dependent
    /// on the value, minimum, and maximum properties of the progress bar.
    /// \param int The value to set. 
    void setValue(int);

public:
    /// \brief Constructor
    /// \param parent Pointer to the parent widget, defaults to NULL
    OriginTransferBar(QWidget *parent = 0);
    /// \brief Destructor
    ~OriginTransferBar();

protected:
     void paintEvent(QPaintEvent* event);
     bool event(QEvent* event);

     QWidget* getSelf() {return this;}
     QString&  getElementStyleSheet();

     void changeLang(QString newLang);
     QString& getCurrentLang();
     void prepForStyleReload();

private:
    Ui::OriginTransferBar *ui;
    static QString mPrivateStyleSheet;

    QString state;
    QString title;
    QString completed;
    QString rate;
    QString timeRemaining;

};
    }
}

#endif
