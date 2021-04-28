#ifndef ORIGINTABWIDGET_H
#define ORIGINTABWIDGET_H

#include <QTabWidget>
#include <QTabBar>
#include <QStackedWidget>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

class Ui_OriginTabWidget;

namespace Origin {
namespace UIToolkit {

class OriginTabWidget;
class OriginTab;

class UITOOLKIT_PLUGIN_API OriginTabBar: public QTabBar
{
    Q_OBJECT

    static const int MAX_TAB_WIDTH = 175;
    static const int MIN_TAB_WIDTH = 31; // fits 15 tabs
    static const int TAB_BAR_HEIGHT = 27;
    static const int HIDE_TITLE_DX = 15;
    static const int HIDE_ICON_DX = 3;
    static const int MAX_TABS = 15;

public:
    friend class OriginTabWidget;

public:
    OriginTabBar(QWidget* parent) : QTabBar(parent), mAddButtonWidth(0) {}

    void setAddButtonWidth( int width ) { mAddButtonWidth = width + 4; /* '+ 4' to compensate for tab bar left margin */ }
    void setTabButtonText( int index, const QString& text );
    QString tabButtonText( int index ) const;
    void setTabButtonIcon( int index, const QIcon& icon );

protected:
    virtual void resizeEvent( QResizeEvent * );
    virtual QSize tabSizeHint( int index ) const;

private:
    void causeResize();
    void adjustTabSizes();
    int computeTabWidth() const;

private:
    int mAddButtonWidth;
};

class UITOOLKIT_PLUGIN_API OriginTabWidget : public QWidget, public OriginUIBase
{
    Q_OBJECT

public:
	/// \brief Constructor
	/// \param parent Pointer to the parent widget, defaults to NULL
    OriginTabWidget(QWidget* parent = 0);

	/// \brief Destructor
    ~OriginTabWidget();

    void setMovable( bool movable );
    void setDocumentMode( bool set );
    void setTabsClosable( bool closeable );
    void setTabBarVisible( bool visible );
    void setUsesScrollButtons( bool usesButtons );
    void setDrawBase( bool drawTheBase );
    void setExpanding( bool enabled );

    int indexOf( QWidget* widget ) const { return mStackedWidget->indexOf(widget); }
    void setTabText( int index, const QString& text );
    QString tabText( int index ) const;
    void setTabIcon( int index, const QIcon& icon );

    void addTab( QWidget* page, const QString& label );
    int insertTab( int index, QWidget* page, const QString& label );
    void closeTab(int index);
    void setCurrentIndex(int index) { mTabBar->setCurrentIndex(index); }
    int currentIndex() { return mTabBar->currentIndex(); }
    int count() const;
    QWidget* widget( int index ) const { return mStackedWidget->widget(index);}

    QWidget* tabBar();

    bool allowAddTab() const { return (mTabBar->count() < OriginTabBar::MAX_TABS); }

private slots:
    void onTabMoved(int from, int to);
    void onTabCloseRequested(OriginTab* tab);
    void onTabCloseRequested(int index);
    void onCurrentChanged(int index);
    void onEnterTab();
    void onLeaveTab();

signals:
    void currentChanged(int);
    void addTabClicked();
    void lastTabClosed();
    void tabClosed(int);

protected:
    QString& getElementStyleSheet();
    void prepForStyleReload();
    bool event(QEvent* event);
    void paintEvent(QPaintEvent* event);
    QWidget* getSelf() {return this;}

private:
    void setTransparentBackground();
    void updateAddTabButtonPosition();

private:
     static QString mPrivateStyleSheet;
     Ui_OriginTabWidget* ui;
     QStackedWidget* mStackedWidget;
     OriginTabBar* mTabBar;
};
}
}
#endif