#ifndef ORIGINPROGRESSBAR_H_
#define ORIGINPROGRESSBAR_H_

#include <QtWidgets/QProgressBar>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Origin {
    namespace UIToolkit {
class OriginDivider;
/// \brief Provides an extremely flexible multi-use horizontal progress bar type control which can be used to give a graphic indication of progress, a ongoing task, percentage, etc.
/// 
/// <A HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginProgressBar">Click here for more OriginProgressBar documentation</A>

class UITOOLKIT_PLUGIN_API OriginProgressBar : public QProgressBar, public OriginUIBase
{
    Q_OBJECT
    Q_ENUMS(Color)
    Q_PROPERTY(bool paused READ getPaused WRITE setPaused)
    Q_PROPERTY(bool animated READ getAnimated WRITE setAnimated)
    Q_PROPERTY(bool fixed READ getFixed WRITE setFixed)
    Q_PROPERTY(bool completed READ getCompleted WRITE setCompleted)
    Q_PROPERTY(bool labelVisible READ getLabelVisible WRITE setLabelVisible)
    Q_PROPERTY(bool tiny READ getTiny WRITE setTiny)
    Q_PROPERTY(bool indeterminate READ getIndeterminate WRITE setIndeterminate)
    Q_PROPERTY(double value READ getValue WRITE setValue)
    Q_PROPERTY(QString format READ getFormat WRITE setFormat)
    Q_PROPERTY(int precision READ getPrecision WRITE setPrecision)
    Q_PROPERTY(double progressIndicatorValue READ getProgressIndicatorValue WRITE setProgressIndicatorValue)
    Q_PROPERTY(Color color READ getColor WRITE setColor)

public:
    /// \brief Defines the color of the progress bar
    enum Color
    {
        Color_Blue,
        Color_Green
    };

    /// \brief Constructor
    /// \param parent Pointer to the parent widget, defaults to NULL
    OriginProgressBar(QWidget *parent = 0);
    /// \brief Destructor
	~OriginProgressBar();

    /// \brief Gets whether or not the progress bar is paused
    /// 
    /// If set to true, this property will cause the progress bar animation effect {if enabled via the animated property) to stop, and
    /// will give the progress bar a "paused" appearance.
    /// \return bool True if paused, False if not paused
    bool getPaused() const { return paused; }
    /// \brief Sets whether or not the progress bar is paused
    /// 
    /// If set to true, this property will cause the progress bar animation effect {if enabled via the animated property) to stop, and
    /// will give the progress bar a "paused" appearance.
    /// \param  bool True if paused, False if not paused
    void setPaused(bool);
    
    /// \brief Gets the animated property
    /// 
    /// If set to true, this property will enable the progress bar animation effect. If set to false, the progress bar will not animate.
    /// \return bool True if animated, False if not
    bool getAnimated() const { return animated; }
    /// \brief Sets the animated property
    /// 
    /// If set to true, this property will enable the progress bar animation effect. If set to false, the progress bar will not animate.
    /// \param bool True if animated, False if not.
    void setAnimated(bool);

    /// \brief Gets whether or not the progress bar is not animated
    /// 
    /// If set to true, the progress bar animation indicators ("barbershop pole stripes") to not be shown. This is useful for indicating
    /// things besides progress, like percentage of disk space usage for example. If set to false, the progress bar will be rendered with
    /// the animation indicators.
    /// \return bool True if no animation is to be show, False if it is animated
    bool getFixed() const { return fixed; }
    /// \brief Sets whether or not the progress bar is not animated
    /// 
    /// If set to true, the progress bar animation indicators ("barbershop pole stripes") to not be shown. This is useful for indicating
    /// things besides progress, like percentage of disk space usage for example. If set to false, the progress bar will be rendered with
    /// the animation indicators.
    /// \param bool True if no animation is to be show, False if it is animated
    void setFixed(bool);

    /// \brief Gets whether or not the progress bar is in minimalistic mode
    /// 
    /// If set to true, the progress bar will change to a more minimalistic appearance to indicate to the user that the process has completed.
    /// \return bool True if set to minimalistic, False if not
	bool getCompleted() const { return completed; }
    /// \brief Sets whether or not the progress bar is in minimalistic mode
    /// 
    /// If set to true, the progress bar will change to a more minimalistic appearance to indicate to the user that the process has completed.
	/// \param bool True if set to minimalistic, False if not
    void setCompleted(bool);

    /// \brief Gets whether the label in the center of the progress bar is visible
    /// 
    /// If set to true, the label in the center of the progress bar will be visible. If set to false, no label will be rendered on the progress
    /// bar. This property also works when the completed property has been set.
    /// \return bool True if the label is visible, False if not
	bool getLabelVisible() const { return labelVisible; }
    /// \brief Sets whether the label in the center of the progress bar is visible
    /// 
    /// If set to true, the label in the center of the progress bar will be visible. If set to false, no label will be rendered on the progress
    /// bar. This property also works when the completed property has been set.
    /// \param bool True if the label is visible, False if not
    void setLabelVisible(bool);

    /// \brief Gets whether or not the progress bar is in tiny mode
    /// 
    /// If set to true, the progress bar will be rendered as a tiny version of the widget. If set the false, the progress bar will appear normally.
    /// (See screenshots.) Note that in tiny mode, the progress bar will have the following limitations:
    /// <ul>
    /// <li>The progress bar label will not be rendered.</li>
    /// <li>The "completed" state is not supported</li>
    /// <li>The "indeterminate" state is not currently supported</li>
    /// </ul>
    /// \return bool True if set to tiny, False if not
    bool getTiny() const { return tiny; }
    /// \brief Sets whether or not the progress bar is in tiny mode
    /// 
    /// If set to true, the progress bar will be rendered as a tiny version of the widget. If set the false, the progress bar will appear normally.
    /// (See screenshots.) Note that in tiny mode, the progress bar will have the following limitations:
    /// <ul>
    /// <li>The progress bar label will not be rendered.</li>
    /// <li>The "completed" state is not supported</li>
    /// <li>The "indeterminate" state is not currently supported</li>
    /// </ul>
    /// \param bool True if set to tiny, False if not
    void setTiny(bool);

    /// \brief Gets whether or not the progress bar is in an indeterminate state
    /// 
    /// If set to true, the progress bar will animate in the opposite direction and completely fill the bar area. This feature is used to indicate
    /// to the user that a potentially long operation is being performed, but for which an actual progress value cannot be established. 
    /// \return bool True if indeterminate, False if not
    bool getIndeterminate() const { return indeterminate; }
    /// \brief Sets whether or not the progress bar is in an indeterminate state
    /// 
    /// If set to true, the progress bar will animate in the opposite direction and completely fill the bar area. This feature is used to indicate
    /// to the user that a potentially long operation is being performed, but for which an actual progress value cannot be established. 
    /// \param bool True if indeterminate, False if not
    void setIndeterminate(bool);

    /// \brief Gets the current value of the progress bar
    /// 
    /// This property holds the progress bar's current value. Fractional values are supported. The "filled amount" of the progress bar is dependent
    /// on the value, minimum, and maximum properties.
    /// \return float The current value of the progress bar
    float getValue() const { return value; }

    /// \brief Sets the formatting of the text in the progress bar
    /// 
    /// This property holds the string used to generate the current text for the progress bar label. It supports the formatting macros listed below,
    /// but can also accept a simple string (eg "Updating...")
    ///
    /// Formatting Macros:
    /// <ul>
    /// <li>"%p" - is replaced by the percentage completed.</li>
    /// <li>"%v" - is replaced by the current value.</li>
    /// <li>"%m" - is replaced by the total number of steps.</li>
    /// <li>"%%" - is replaced by a percent symbol.</li>
    /// </ul>
    /// The default value is "%p %%".
    /// \param QString A reference to the QString containing the formatting.
    void setFormat(const QString &);
    /// \brief Gets the formatting of the text in the progress bar
    /// 
    /// This property holds the string used to generate the current text for the progress bar label. It supports the formatting macros listed below,
    /// but can also accept a simple string (eg "Updating...")
    /// Formatting Macros:
    /// <ul>
    /// <li>"%p" - is replaced by the percentage completed.</li>
    /// <li>"%v" - is replaced by the current value.</li>
    /// <li>"%m" - is replaced by the total number of steps.</li>
    /// <li>"%%" - is replaced by a percent symbol.</li>
    /// </ul>
    /// The default value is "%p %%".
    /// \return QString A reference to the QString containing the formatting.
    const QString& getFormat() const { return format; }
	
    /// \brief Sets the precision of the value
    /// 
    /// Specifies the number of digits following the decimal point that are shown in the label. Design recommends that all progress bars in the Origin
    /// client have a precision value of 2 (show 2 decimal places).
    /// \param int The number of digits following the decimal point.
    void setPrecision(int);
    /// \brief Gets the precision of the value
    /// 
    /// Specifies the number of digits following the decimal point that are shown in the label. Design recommends that all progress bars in the Origin
    /// client have a precision value of 2 (show 2 decimal places).
    /// \return int The number of digits following the decimal point.
    int getPrecision() const { return precision; }

    /// \brief Sets the color of the progress bar
    void setColor(const Color& aColor);
    /// \brief the color of the progress bar
    Color getColor() const { return color; }

    /// \brief Sets the value of where the progresses indicator will be show on the bar.
    void setProgressIndicatorValue(const double& value);
    /// \brief the color of the progress bar
    double getProgressIndicatorValue() const { return progressIndicatorValue; }

public slots:
    /// \brief Sets the value of the progress bar
    /// 
    /// This property holds the progress bar's current value. Fractional values are supported. The "filled amount" of the progress bar is dependent
    /// on the value, minimum, and maximum properties.
    /// \param double The value to set.
    void setValue(double);
    /// \brief Sets the value of the progress bar
    /// 
    /// This property holds the progress bar's current value. The "filled amount" of the progress bar is dependent on the value, minimum, and maximum
    /// properties.
    /// \param int The value to set. 
    void setValue(int);

protected:
    void paintEvent(QPaintEvent* event);
    bool event(QEvent* event);
    void resizeEvent(QResizeEvent* resizeEvent);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);

    QProgressBar* getSelf() {return this;}
    QString&  getElementStyleSheet();

    void changeLang(QString newLang);
    QString& getCurrentLang();
    void prepForStyleReload();

    void ensureBuffers();
    void updateLabel();
    void updateAnimation();

protected slots:
    void onAnimationTimer();

private:
    static QString mPrivateStyleSheet;

    // Static shared pixmaps
    static long mPixmapRefCount;

    static QPixmap* mPixmapGutter;
    static QPixmap* mPixmapBlueFillBar;
    static QPixmap* mPixmapGreenFillBar;
    static QPixmap* mPixmapFillBarPaused;
    static QPixmap* mPixmapFillBarBarberPoleOverlay;
    static QPixmap* mPixmapFillBarShadow;
    static QPixmap* mPixmapFillBarMask;
    static QPixmap* mPixmapFillBarIndeterminateMask;

    static QPixmap* mPixmapGutterSmall;
    static QPixmap* mPixmapBlueFillBarSmall;
    static QPixmap* mPixmapGreenFillBarSmall;
    static QPixmap* mPixmapFillBarPausedSmall;
    static QPixmap* mPixmapFillBarMaskSmall;
    static QPixmap* mPixmapFillBarIndeterminateMaskSmall;

    static QPixmap* mPixmapCompleted;

    static QFont* mLabelFont;

    // Instance stuff

    QPixmap* mFillBarBackbuffer;

    QTimer* mAnimationTimer;
    int mAnimationXPos;

    float mPercentage;
    QString mLabelValue;

    // Properties
    bool paused;
    bool animated;
    bool fixed;
    bool completed;
    bool labelVisible;
    bool tiny;
    bool indeterminate;
    double value;
    QString format;
    int precision;
	double progressIndicatorValue;
    Color color;
    OriginDivider* mIndicator;
};
}
}
#endif
