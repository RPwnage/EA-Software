#ifndef ORIGINSPINNER_H
#define ORIGINSPINNER_H

#include <QLabel>
#include <QHash>

#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Origin {
    namespace UIToolkit {
/// \brief A widget used to indicate a waiting or busy condition. Multiple styles and sizes are available.
/// 
/// <A HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginSpinner">Click here for more OriginSpinner documentation</A>

class UITOOLKIT_PLUGIN_API OriginSpinner : public QLabel, public OriginUIBase
{
    Q_OBJECT
    Q_ENUMS(SpinnerType)
    Q_PROPERTY(SpinnerType spinnerType READ getSpinnerType WRITE setSpinnerType DESIGNABLE true )
    Q_PROPERTY(bool autoStart READ getAutoStart WRITE setAutoStart)

public:
    /// \brief Constructor
    /// \param parent Pointer to the parent widget, defaults to NULL
    OriginSpinner(QWidget *parent = 0);
	/// \brief Destructor
    ~OriginSpinner();

    /// \brief Type of spinner 
    enum SpinnerType { 
        SpinnerSmall,       ///< Defines a small sized circular spinner.
        SpinnerMedium,      ///< Defines a medium sized circular spinner.
        SpinnerLarge,       ///< Defines a large sized circular spinner.
        SpinnerLarge_Dark,  ///< Defines a large sized circular spinner with a dark border.
        SpinnerSmallValidating ///< Defines a small spinner with animating arrows rotating.
    };

    /// \brief Set the type of spinner.
    /// \param spinnerSize SpinnerType to set the spinner to.
    void setSpinnerType(SpinnerType spinnerSize);
    /// \brief Get the type of spinner.
    /// \return SpinnerType Current Type of spinner.
    SpinnerType getSpinnerType() const { return mSpinnerType; }

    /// \brief Set whether the spinner begins animating on instantiation.
    /// \param aAutoStart True to begin at instantiation, False to not.
    void setAutoStart(bool aAutoStart);
    /// \brief Get whether the spinner begins animating on instantiation.
    /// \return bool True to begin at instantiation, False to not.
    bool getAutoStart() const { return autoStart; }

public slots:
    /// \brief Start the spinner animation
    void startSpinner();
    /// \brief Stop the spinner animation
    void stopSpinner();

protected:
    void paintEvent(QPaintEvent* event);
    bool event(QEvent* event);
    QLabel* getSelf() {return this;}
    QString&  getElementStyleSheet();
    void prepForStyleReload();

private:
    void createSpinner();

    static QString mPrivateStyleSheet;

    QHash<SpinnerType, QString> mSpinnerMovies;

    SpinnerType mSpinnerType;
    bool autoStart;
    QMovie* mSpinner;


};
    }
}

#endif
