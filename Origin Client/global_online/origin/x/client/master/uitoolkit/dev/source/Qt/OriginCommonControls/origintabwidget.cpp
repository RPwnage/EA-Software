#include "OriginTabWidget.h"
#include "OriginTab.h"
#include "ui_OriginTabWidget.h"
#include <QTabBar>
#include <QMouseEvent>

namespace Origin {
namespace UIToolkit {

QString OriginTabWidget::mPrivateStyleSheet = QString("");

OriginTabWidget::OriginTabWidget(QWidget* parent)
: QWidget(parent)
, mStackedWidget(NULL)
, mTabBar(NULL)
{
    ui = new Ui::OriginTabWidget();
    ui->setupUi(this);
    setOriginElementName("OriginTabWidget");
    m_children.append(this);

    // set the custom tab bar
    mTabBar = new OriginTabBar(ui->tabBarContainer);
    ui->tabBarContainer->layout()->addWidget(mTabBar);

    // reparent the '+' button
    ui->btnPlusTab->setParent(mTabBar);

    setTransparentBackground();

    mStackedWidget = ui->stackedWidget;

    mTabBar->setAddButtonWidth(ui->btnPlusTab->width());
    // Make sure we call the stacked widget first, so that we can properly assign the focus afterwards
    connect(mTabBar, SIGNAL(currentChanged(int)), mStackedWidget, SLOT(setCurrentIndex(int)));
    connect(mTabBar, SIGNAL(currentChanged(int)), this, SLOT(onCurrentChanged(int)));
    connect(mTabBar, SIGNAL(tabMoved(int, int)), this, SLOT(onTabMoved(int, int)));
    connect(ui->btnPlusTab, SIGNAL(clicked()), this, SIGNAL(addTabClicked()));
}


OriginTabWidget::~OriginTabWidget()
{
	removeElementFromChildList(this);
    if( ui )
    {
        delete ui;
        ui = NULL;
    }
    if( mTabBar )
    {
        mTabBar->deleteLater();
        mTabBar = NULL;
    }
}

void OriginTabWidget::addTab( QWidget* page, const QString& label )
{
    if( mTabBar->count() >= OriginTabBar::MAX_TABS )
        return;

    mStackedWidget->addWidget(page);
    int index = 0;

    OriginTab *tab = new OriginTab(/*mTabBar*/);
    Q_ASSERT(tab);
    if( tab )
    {
        tab->setProperty("state", "active");
        tab->setStyle(QApplication::style());

        connect( tab, SIGNAL(closeRequested(OriginTab*)), this, SLOT(onTabCloseRequested(OriginTab*)));
        connect( tab, SIGNAL(enterTab()), this, SLOT(onEnterTab()) );
        connect( tab, SIGNAL(leaveTab()), this, SLOT(onLeaveTab()) );
        index = mTabBar->addTab(QString("")/*label*/); // leave label blank so that the real tab button's text does not show
        mTabBar->setTabButton(index, QTabBar::LeftSide, tab);
    }

    mTabBar->causeResize();

    // delayed emitting of signal by mTabBar->addTab()
    mTabBar->setCurrentIndex(index);
}

int OriginTabWidget::insertTab(int index, QWidget* page, const QString& label)
{
    int insertedIndex = mTabBar->insertTab(index, label);
    mStackedWidget->insertWidget(index, page);

    return insertedIndex;
}

void OriginTabWidget::closeTab(int index)
{
    if( index >= 0 && index < mTabBar->count() )
    {
        onTabCloseRequested(index);
    }
}

QWidget* OriginTabWidget::tabBar()
{
    return mTabBar;
}

void OriginTabWidget::onTabMoved(int from, int to)
{
    // copied from QTabWidgetPrivate::_q_tabMoved()
    mStackedWidget->blockSignals(true);
    QWidget *w = mStackedWidget->widget(from);
    mStackedWidget->removeWidget(w);
    mStackedWidget->insertWidget(to, w);
    mStackedWidget->blockSignals(false);
}

void OriginTabWidget::onTabCloseRequested(OriginTab* tab)
{
    int tabToClose = -1;

    for( int i = 0; i < count(); ++i )
    {
        OriginTab* t = static_cast<OriginTab*>(mTabBar->tabButton(i, QTabBar::LeftSide));
        if( t == tab )
        {
            tabToClose = i;
            //if we don't do this focus events still get sent to the tab even after its been freed
            tab->setParent(NULL);
            break;
        }
    }

    if( tabToClose >= 0 )
        onTabCloseRequested(tabToClose);
}

void OriginTabWidget::onTabCloseRequested(int index)
{
    QWidget *w = mStackedWidget->widget(index);
    
#ifdef Q_OS_MAC
    // Make sure we don't keep around references to the focus object in case that object is part of the "stacked widget" / about to get deleted!
    // (for example when used with the multi-tab browser, the OriginTabWidget would keep a reference to an edit line widget on a page
    // and try to give it back focus later on ... )
    QWidget* focusObj = focusWidget();
    if (focusObj)
    {
        if (w->isAncestorOf(focusObj))
            focusObj->clearFocus();
    }
#endif
    
    mStackedWidget->removeWidget(w);
    w->deleteLater();
    mTabBar->removeTab(index);
    mTabBar->causeResize();

    emit tabClosed(index);

    if( count() == 0 )
        emit lastTabClosed();
}

void OriginTabWidget::onCurrentChanged(int index)
{
    // change background of tab
    for( int i = 0; i < count(); ++i )
    {
        OriginTab* t = static_cast<OriginTab*>(mTabBar->tabButton(i, QTabBar::LeftSide));
        if( t )
        {
            if( i == index )
            {
                t->setProperty("state", "active");
                t->setStyle(QApplication::style());
            }
            else
            {
                t->setProperty("state", "normal");
                t->setStyle(QApplication::style());
            }
        }
    }

    emit currentChanged(index);
    mTabBar->causeResize();
}

void OriginTabWidget::onEnterTab()
{
    mTabBar->causeResize();
}

void OriginTabWidget::onLeaveTab()
{
    mTabBar->causeResize();
}

QString& OriginTabWidget::getElementStyleSheet()
{
    if(mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet += loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}


void OriginTabWidget::prepForStyleReload()
{
	mPrivateStyleSheet = QString("");
}


bool OriginTabWidget::event(QEvent* event)
{
	OriginUIBase::event(event);
    return QWidget::event(event);
}


void OriginTabWidget::paintEvent(QPaintEvent *event)
{
    updateAddTabButtonPosition();

    OriginUIBase::paintEvent(event);
    QWidget::paintEvent(event);
}

void OriginTabWidget::setMovable( bool movable ) { mTabBar->setMovable(movable); }
void OriginTabWidget::setDocumentMode( bool set ) { mTabBar->setDocumentMode(set); }
void OriginTabWidget::setTabsClosable( bool closeable ) { mTabBar->setTabsClosable(closeable); }
void OriginTabWidget::setTabBarVisible( bool visible )
{
    ui->tabs->setVisible(visible);
    ui->btnPlusTab->setVisible(visible);
}
void OriginTabWidget::setUsesScrollButtons( bool usesButtons ) { mTabBar->setUsesScrollButtons(usesButtons); }
void OriginTabWidget::setDrawBase( bool drawTheBase ) { mTabBar->setDrawBase(drawTheBase); }
void OriginTabWidget::setExpanding( bool enabled ) { mTabBar->setExpanding(enabled); }
void OriginTabWidget::setTabText( int index, const QString& text ) { mTabBar->setTabButtonText(index, text); }
QString OriginTabWidget::tabText( int index ) const { return mTabBar->tabButtonText(index); }
void OriginTabWidget::setTabIcon( int index, const QIcon& icon ) { mTabBar->setTabButtonIcon(index, icon); }
int OriginTabWidget::count() const { return mTabBar->count(); }

void OriginTabWidget::setTransparentBackground()
{
    // In the tabbed IGO Web Browser, we need to explicitly clip the tabs of the tabBar
    // since the tabBar is contained within a QGraphicsScene. By default, QGraphicsItems
    // (i.e, the tabBar and its tabs) don't get clipped. However, when clipping is set
    // on the tabBar via QGraphicsProxyWidget::createProxyForChildWidget(child), the backgrounds
    // of the 'child' and all of its parent widgets get set to be opaque... unless
    // it is set beforehand. Thus, we set the backgrounds to be transparent here... 
    mTabBar->setPalette(QPalette(QColor(0,0,0,0)));
    this->setPalette(QPalette(QColor(0,0,0,0)));
}

void OriginTabWidget::updateAddTabButtonPosition()
{
    const int TAB_MARGIN = 4; // this is to compensate for a tab bar having a built in left margin
    int index = mTabBar->count() - 1;
    int extraWidth = TAB_MARGIN;
    QSize tabBarSize = mTabBar->sizeHint();
    QRect tabRect = mTabBar->tabRect(index);
    ui->btnPlusTab->move( tabBarSize.width() + extraWidth, tabRect.bottom() - ui->btnPlusTab->height() + 1 );

    // disable if too many tabs already open
    bool enable = mTabBar->count() < OriginTabBar::MAX_TABS;
    ui->btnPlusTab->setEnabled(enable);
}

////////////////////////////////////////////////////////////////////////////////
void OriginTabBar::setTabButtonText( int index, const QString& text )
{
    OriginTab* t = static_cast<OriginTab*>(tabButton(index, QTabBar::LeftSide));
    if( t )
    {
        t->setText(text);
    }
}

void OriginTabBar::setTabButtonIcon( int index, const QIcon& icon)
{
    OriginTab* t = static_cast<OriginTab*>(tabButton(index, QTabBar::LeftSide));
    if( t )
    {
        t->setIcon(icon);
    }
}

QString OriginTabBar::tabButtonText( int index ) const
{
    OriginTab* t = static_cast<OriginTab*>(tabButton(index, QTabBar::LeftSide));
    if( t )
    {
        return t->text();
    }
    return QString("");
}

void OriginTabBar::causeResize()
{
    QSize theSize = size();
    theSize.setWidth(theSize.width()+1);
    resize(theSize);
    theSize.setWidth(theSize.width()-1);
    resize(theSize);
}

void OriginTabBar::resizeEvent( QResizeEvent *e )
{
    QTabBar::resizeEvent(e);
    adjustTabSizes();
}

int OriginTabBar::computeTabWidth() const
{
    int newWidth = 0;

    int tabCount = count();
    if( tabCount > 0 )
    {
        float availableWidth = width() - mAddButtonWidth;

        // compute new width for each tab
        newWidth = int(availableWidth / float(tabCount));

        // limit the size
        if( newWidth < MIN_TAB_WIDTH )
            newWidth = MIN_TAB_WIDTH;
        if( newWidth > MAX_TAB_WIDTH )
            newWidth = MAX_TAB_WIDTH;
    }

    return newWidth;
}

QSize OriginTabBar::tabSizeHint( int index ) const
{
    QSize s;

    int tabCount = count();
    int lastIndex = tabCount - 1;
    if( lastIndex >= 0 )
    {
        int availableWidth = width() - mAddButtonWidth;
        int newWidth = computeTabWidth();
        int remainder = availableWidth - (tabCount * newWidth);


        s.setHeight(TAB_BAR_HEIGHT);
        s.setWidth(newWidth + (index < remainder ? 1 : 0));
    }

    return s;
}

void OriginTabBar::adjustTabSizes()
{
    int tabCount = count();
    int lastIndex = tabCount - 1;
    if( lastIndex >= 0 )
    {
        int availableWidth = width() - mAddButtonWidth;
        int newWidth = computeTabWidth();
        int remainder = availableWidth - (tabCount * newWidth);

        // remove close button?
        bool removeCloseButton = false;
        if( newWidth < 100 )
            removeCloseButton = true;

        bool atMinimumSize = false;
        if( newWidth <= MIN_TAB_WIDTH )
            atMinimumSize = true;

        for( int i = 0; i < tabCount; ++i )
        {
            OriginTab* t = static_cast<OriginTab*>(tabButton(i, QTabBar::LeftSide));
            if( t )
            {
                bool isCurrentTab = false;
                if( i == currentIndex() )
                    isCurrentTab = true;

                bool showCloseButton = !removeCloseButton || isCurrentTab;
                t->setCloseButtonVisible(showCloseButton);
                bool showIcon = !atMinimumSize || (atMinimumSize && !isCurrentTab);

                // remove label
                bool showTitle = true;
                if( showCloseButton && showIcon ) // active tab
                {
                    int left = t->iconButton()->pos().x() + t->iconButton()->rect().width();
                    int right = t->closeButton()->pos().x();
                    int dx = right - left;
                    if( dx < HIDE_TITLE_DX )
                        showTitle = false;

                    if( dx < HIDE_ICON_DX)
                        showIcon = false;

                    // center the close button
                }
                else if( showCloseButton && !showIcon ) // active tab at minimum size
                {
                    showTitle = false;

                    // center the close button
                }
                else if( !showCloseButton && showIcon ) // inactive tab
                {
                    int left = t->iconButton()->pos().x() + t->iconButton()->rect().width();
                    int right = t->width() - 10 /*margin*/;
                    int dx = right - left;
                    if( dx < HIDE_TITLE_DX )
                        showTitle = false;

                    // center the icon
                }

                t->setIconVisible(showIcon);
                t->setTitleVisible(showTitle);

                remainder -= 1;
                if( remainder < 0 )
                    remainder = 0;
                t->setFixedSize(newWidth + (remainder > 0 ? 1 : 0), TAB_BAR_HEIGHT );
            }
        }
    }
}

}
}