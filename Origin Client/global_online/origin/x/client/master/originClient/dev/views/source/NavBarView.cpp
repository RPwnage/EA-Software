/////////////////////////////////////////////////////////////////////////////
// NavBarView.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "NavBarView.h"
#include "Services/debug/DebugService.h"
#include "NavigationController.h"
#include "engine/login/LoginController.h"
#include "ui_NavBarView.h"
#include <QMouseEvent>

namespace Origin
{
	namespace Client
	{
		NavBarView::NavBarView(QWidget *parent)	: 
            QWidget(parent),
            ui(new Ui::NavBarView()),
            mMyGamesPressed(false),
            mStorePressed(false)
		{
			ui->setupUi(parent);
            parent->installEventFilter(this);

			init();
		}

		NavBarView::~NavBarView()
		{

		}

		void NavBarView::init()
		{
            clearTabs();

#if defined(ORIGIN_MAC)
			// Extra spacing needed on Mac to keep the buttons from overlapping
            ui->navButtonSpacer->changeSize(12, 1);
            parentWidget()->setProperty("platform", QVariant("OSX"));
            parentWidget()->setStyle(QApplication::style());
#endif

/*
			// Surface these signals to our controller
			ORIGIN_VERIFY_CONNECT(ui->userArea, SIGNAL(showSettings()), this, SIGNAL(showSettings()));
			ORIGIN_VERIFY_CONNECT(ui->userArea, SIGNAL(logout()), this, SIGNAL(logout()));
			ORIGIN_VERIFY_CONNECT(ui->userArea, SIGNAL(exit()), this, SIGNAL(exit()));
			ORIGIN_VERIFY_CONNECT(ui->userArea, SIGNAL(addFriend()), this, SIGNAL(addFriend()));
*/
			ORIGIN_VERIFY_CONNECT(ui->btnMyGames, SIGNAL(clicked()), this, SLOT(onMyGamesClicked()));
			ORIGIN_VERIFY_CONNECT(ui->btnStore, SIGNAL(clicked()), this, SLOT(onStoreClicked()));

			onMyGamesClicked();

            ORIGIN_VERIFY_CONNECT(ui->btnBackward, SIGNAL(clicked()), NavigationController::instance(), SLOT(navigateBackward()));
            ORIGIN_VERIFY_CONNECT(ui->btnForward, SIGNAL(clicked()), NavigationController::instance(), SLOT(navigateForward()));
            ORIGIN_VERIFY_CONNECT(NavigationController::instance(), SIGNAL(canGoBackChange(const bool)), ui->btnBackward, SLOT(setEnabled(bool)));
            ORIGIN_VERIFY_CONNECT(NavigationController::instance(), SIGNAL(canGoForwardChange(const bool)), ui->btnForward, SLOT(setEnabled(bool)));
			
            ui->btnBackward->setDisabled(true);
            ui->btnForward->setDisabled(true);

            ui->btnMyOrigin->setVisible(false);
            ui->divMiddleLeft->setVisible(false);
        }

        void NavBarView::handleUnderAgeMode(bool isUnderAge)
        {
            ui->btnStore->setVisible(!isUnderAge);
            ui->curTabStore->setVisible(!isUnderAge);
            ui->divMiddleRight->setVisible(!isUnderAge);
            updateMyGamesTab();
        }

        bool NavBarView::eventFilter(QObject* object, QEvent* e)
        {
            switch(e->type())
            {
            case QEvent::MouseButtonPress:
                if (object == parentWidget()) 
                {
                    // see if the click was on one/near the my store or my games buttons, if so, set the right value
                    checkToolbarMouseDown(((QMouseEvent*)e)->pos(), mMyGamesPressed, mStorePressed);
                }
                break;

            case QEvent::MouseButtonRelease:
                if (object == parentWidget()) 
                {
                    // do the click of the toolbar button
                    doToolbarButtonClick(((QMouseEvent*)e)->pos());
                }
                mMyGamesPressed = false;
                mStorePressed = false;
                break;

            default:
                break;
            }

            return QWidget::eventFilter(object, e);
        }

        QWidget* NavBarView::getSocialUserArea() 
        { 
            return ui->socialStatusArea; 
        }

        QLayout* NavBarView::getSocialUserAreaLayout() 
        { 
            return ui->socialStatusAreaLayout; 
        }

        void NavBarView::doToolbarButtonClick(const QPoint& pt)
        {
            // don't even bother if there wasn't a down on one of the buttons
            if (mMyGamesPressed || mStorePressed) 
            {
                bool myGamesPressed = false;
                bool storePressed = false;
                checkToolbarMouseDown(pt, myGamesPressed, storePressed);

                if (mMyGamesPressed && myGamesPressed) 
                {
                    ui->btnMyGames->click();
                }
                else if (mStorePressed && storePressed) 
                {
                    ui->btnStore->click();
                }
                mMyGamesPressed = false;
                mStorePressed = false;
            }
        }

        void NavBarView::checkToolbarMouseDown(const QPoint& pt, bool& myGamesPressed, bool& storePressed)
        {
            // don't really care about the height. Care only that it is to the right of the divider and to the left of the next
            // divider
            const bool isUnderage = Origin::Engine::LoginController::currentUser()->isUnderAge();
            const int lastDivX = isUnderage ? ui->divLast->pos().x() : ui->divMiddleRight->pos().x();
            // Todo: This can be taken out when My Origin is always around
            const int divBeforeMyGamesX = ui->divFirst->pos().x();

            // My Games
            // Todo: When My Origin is always around divBeforeMyGamesX can 
            // change to ui->divMiddleLeft->pos().x()
            if ((pt.x() > divBeforeMyGamesX) && (pt.x() < lastDivX)) 
            {
                myGamesPressed = true;
                storePressed = false;
            }
            // Store
            else 
            {
                if ((isUnderage == false) &&
                    (pt.x() > lastDivX) && (pt.x() < ui->divLast->pos().x())) 
                {
                    myGamesPressed = false;
                    storePressed = true;
                }
            }
        }

        void NavBarView::onMyGamesClicked()
        {
            updateMyGamesTab();
            emit (showMyGames());
        }

        void NavBarView::onStoreClicked()
        {
            updateStoreTab();
            emit (showStore());
		}

        void NavBarView::updateMyGamesTab()
        {   
            clearTabs(false);
            if(parentWidget())
            {
                parentWidget()->setProperty("style", QVariant::Invalid);
                parentWidget()->setStyle(QApplication::style());
                ui->separator->setStyle(QApplication::style());
            }

            ui->curTabGames->show();
            highlightTab(ui->btnMyGames);
        }

        void NavBarView::updateStoreTab()
        {
            clearTabs(false);
            if(parentWidget())
            {
                parentWidget()->setProperty("style", QVariant::Invalid);
                parentWidget()->setStyle(QApplication::style());
                ui->separator->setStyle(QApplication::style());
            }

            ui->curTabStore->show();
            highlightTab(ui->btnStore);
        }

        void NavBarView::clearTabs(bool setStyleProperty)
        {
			ui->curTabMyOrigin->hide();
            ui->curTabGames->hide();
            ui->curTabStore->hide();
            highlightTab(ui->btnStore, false);
            highlightTab(ui->btnMyGames, false);
			highlightTab(ui->btnMyOrigin, false);
        }

        void NavBarView::highlightTab(QPushButton* selectedTab, const bool isSelected)
        {
            selectedTab->setProperty("style", isSelected ? QVariant("selected") : QVariant::Invalid);
            selectedTab->setStyle(QApplication::style());
        }
	}
}
