#ifndef ORIGINWEBVIEW_H_
#define ORIGINWEBVIEW_H_

#include "originuiBase.h" 
#include "uitoolkitpluginapi.h"
#include <QtWidgets/QStackedWidget>

class QWebView;
class QWebPage;

namespace Origin {
    namespace UIToolkit {

class OriginSpinner;

/// \brief Web container that holds a web view and loading spinner.
///
/// Used for viewing web page within the Origin client. <A
/// HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginWebView">Click
/// here for more OriginWebView documentation</A>.
class UITOOLKIT_PLUGIN_API OriginWebView : public QStackedWidget, public OriginUIBase
{
    Q_OBJECT
    Q_PROPERTY(bool hasSpinner READ hasSpinner WRITE setHasSpinner DESIGNABLE true)
    Q_PROPERTY(bool usesOriginScrollbars READ usesOriginScrollbars WRITE setUsesOriginScrollbars DESIGNABLE true)
    Q_PROPERTY(bool isSelectable READ isSelectable WRITE setIsSelectable DESIGNABLE true)
    Q_PROPERTY(bool isContentSize READ isContentSize WRITE setIsContentSize DESIGNABLE true)
    Q_PROPERTY(QSize windowLoadingSize READ windowLoadingSize WRITE setWindowLoadingSize DESIGNABLE true)
    Q_PROPERTY(bool changeLoadSizeAfterInitLoad READ changeLoadSizeAfterInitLoad WRITE setChangeLoadSizeAfterInitLoad DESIGNABLE true)
    Q_PROPERTY(bool showSpinnerAfterInitLoad READ showSpinnerAfterInitLoad WRITE setShowSpinnerAfterInitLoad DESIGNABLE true)
    Q_PROPERTY(bool scaleSizeOnContentsSizeChanged READ scaleSizeOnContentsSizeChanged WRITE setScaleSizeOnContentsSizeChanged DESIGNABLE true)
    Q_PROPERTY(Qt::ScrollBarPolicy hScrollBar READ hScrollBarPolicy WRITE setHScrollPolicy DESIGNABLE true)
    Q_PROPERTY(Qt::ScrollBarPolicy vScrollBar READ vScrollBarPolicy WRITE setVScrollPolicy DESIGNABLE true)
    Q_PROPERTY(QSize contentsSize READ preferredContentsSize WRITE setPreferredContentsSize DESIGNABLE true)

public:
    /// \brief Constructor
    /// \param parent The parent of this object.
    OriginWebView(QWidget* parent = 0);

    /// \brief Destructor.
    ~OriginWebView();

    /// \brief Gets/Sets the horizontal scroll bar policy of the web page
    const Qt::ScrollBarPolicy hScrollBarPolicy(void) const;
    void setHScrollPolicy(const Qt::ScrollBarPolicy& policy);

    /// \brief Gets/Sets the vertical scroll bar policy of the web page
    const Qt::ScrollBarPolicy vScrollBarPolicy(void) const;
    void setVScrollPolicy(const Qt::ScrollBarPolicy& policy);

    /// \brief Gets/Sets whether the window containing this web 
    /// container should be the same size as the web pages. The size of the 
    /// top level widget will change it's size dynamically
    const bool& isContentSize(void) const {return mIsContentSize;}
    void setIsContentSize(const bool& isContentSize) {mIsContentSize = isContentSize;}

    /// \brief Gets/Sets the preferred size for this web view, if any.
    /// If set, the view will be resized to this size between refreshes.
    const QSize& preferredContentsSize(void) const {return mPreferredContentsSize;}
    void setPreferredContentsSize(const QSize& contentsSize) {mPreferredContentsSize = contentsSize;}

    /// \brief Gets/Sets whether the window/top level widget size will change
    /// after the first inital load. The default and inital load size is 200x400.
    /// If this value is set to true: after the inital load it the load size will be
    /// the same as the previous web site. The size can still change after load 
    /// the is complete
    const bool& changeLoadSizeAfterInitLoad(void) const {return mChangeLoadSizeAfterInitLoad;}
    void setChangeLoadSizeAfterInitLoad(const bool& changes) {mChangeLoadSizeAfterInitLoad = changes;}

    const bool showSpinnerAfterInitLoad(void) const {return mShowSpinnerAfterInitLoad;}
    void setShowSpinnerAfterInitLoad(const bool& shows) {mShowSpinnerAfterInitLoad = shows;}

    /// \brief The size of the window when the loading screen appears. The default is (450, 201)
    /// If setChangeLoadSizeAfterInitLoad is set to true this will be the initial load size of the window.
    const QSize windowLoadingSize(void) const {return mWindowLoadingSize;}
    void setWindowLoadingSize(const QSize& size) {mWindowLoadingSize = size;}

    /// \brief Will scale down to a minimum preferred size or a set preferred (setPreferredContentsSize) 
    /// when the content size of the web page changes.
    const bool scaleSizeOnContentsSizeChanged(void) const {return mScaleSizeOnContentsSizeChanged;}
    void setScaleSizeOnContentsSizeChanged(const bool& scale) {mScaleSizeOnContentsSizeChanged = scale;}

    /// \brief Will show the page after initial layout instead of waiting to load finishes
    bool showAfterInitialLayout() const {return mShowAfterInitialLayout;}
    void setShowAfterInitialLayout(const bool &show) {mShowAfterInitialLayout = show;}

    /// \brief Gets/Sets whether the user can select the content of an element
    /// on the web page
    const bool& isSelectable(void) const {return mIsSelectable;}
    void setIsSelectable(const bool& selectable);

    /// \brief Gets/Sets whether origin uses it's scroll bars.
    const bool usesOriginScrollbars(void) const {return mUsesOriginScrollbars;}
    void setUsesOriginScrollbars(const bool& usesOriginScrollbars);

    /// \brief Gets/Sets whether a loading spinner will display while a page
    /// is loading
    const bool hasSpinner(void) const {return mSpinner != NULL;}
    void setHasSpinner(const bool& hasSpinner);
    OriginSpinner* spinner() const {return mSpinner;}

    /// \brief Returns the containing web view
    QWebView* webview() const {return mWebView;}
    /// \brief Sets the containing web view. Used if there is a special need
    /// for the web view to have custom functionality. Avoid using this.
    void setWebview(QWebView* webview);
    
    /// \brief Sets the containing graphics web view widget. This functionality isn't fully supported
    /// yet.
    void setGraphicsWebview(QWidget* view, QWebPage* page);

    /// \brief Returns style sheet of our web page error
    static QString getErrorStyleSheet(const bool& alignCenter = true);
    /// \brief Returns style sheet we set on our web pages
    static QString getStyleSheet(const bool& hasFocusOutline, const bool& isSelectable, const bool& useOriginScrollbars = true);

   
public slots:
    /// \brief Updates the window/top level widget size after the page has 
    /// loaded or the size of the web page changed.
    void updateSize();

signals:
    /// \brief Emitted when the cursor of the web view changes. Used in OIG.
    void cursorChanged(int shape);

    /// \brief Signals that the size of the window has been changed. Result of updateSize()
    void sizeUpdated();

protected:
    void removeWebview();
    bool event(QEvent* event);
    bool eventFilter(QObject* obj, QEvent* event);
    void paintEvent(QPaintEvent* event);
    QStackedWidget* getSelf() {return this;}
    QString& getElementStyleSheet();
    void prepForStyleReload();
    void setStyleSheetDirty();

protected slots:
    /// \brief Changes the visible widget to the web view
    void showWebpage(bool ok = true);
    /// \brief Changes the visible widget to the web view
    void showGraphicsWebpage(bool ok = true);
    /// \brief Changes the visible widget to the loading spinner
    void showLoading();

private:
    /// \brief Converts our QSS to CSS.
    static const QString QSStoCSS(const QString& thisElementName,
                                  const QString& elementName,
                                  const QString& elementSubName = "",
                                  const QString& qssfile = "style.qss");

    QWebView* mWebView;
    QWidget* mGraphicsView;
    QWebPage* mGraphicsWebpage;
    OriginSpinner* mSpinner;

    QString mPrivateStyleSheet;
    QSize mPreferredContentsSize;
    QSize mWindowLoadingSize;

    // Todo: Turn these into flags.
    bool mIsSelectable;
    bool mUsesOriginScrollbars;
    bool mIsContentSize;
    bool mChangeLoadSizeAfterInitLoad;
    bool mInitalLoadComplete;
    bool mShowSpinnerAfterInitLoad;
    bool mScaleSizeOnContentsSizeChanged;
    bool mShowAfterInitialLayout;

	bool mOverridingCursor;
};
}
}
#endif
