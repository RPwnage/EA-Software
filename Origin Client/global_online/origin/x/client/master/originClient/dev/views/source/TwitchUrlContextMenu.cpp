// *********************************************************************
//  TwitchUrlContextMenu.cpp
//  Copyright (c) 2013 Electronic Arts Inc. All rights reserved.
// **********************************************************************

#include "TwitchUrlContextMenu.h"
#include <QApplication>
#include <QClipboard>
#include <QLabel>

#include "engine/igo/IGOController.h"
#include "services/debug/DebugService.h"
#include "CustomQMenu.h"
#include "UIScope.h"


TwitchUrlContextMenu::TwitchUrlContextMenu(QWidget* window, QObject *parent)
    : QObject(parent)
    , mNativeMenu(NULL)
    , mCurrentLabel(NULL)
    , mWindow(window)
    , mActionCopy(NULL)
{
    bool inIGO = Origin::Engine::IGOController::instance()->isActive();
    mNativeMenu = new Origin::Client::CustomQMenu(inIGO ? Origin::Client::IGOScope : Origin::Client::ClientScope);

    if (inIGO)
    {
        if (Origin::Engine::IGOController::instance()->igowm()->createContextMenu(window, mNativeMenu, this, false))
        {
            initMenuActions();
            ORIGIN_VERIFY_CONNECT(mNativeMenu, SIGNAL(aboutToHide()), this, SLOT(hiding()));
        }
    }
}

void TwitchUrlContextMenu::initMenuActions()
{
    const char copyTextId[] = "ebisu_client_menu_copy";

    mActionCopy = new QAction(tr(copyTextId), this);
    ORIGIN_VERIFY_CONNECT(mActionCopy, SIGNAL(triggered()), this, SLOT(onCopy()));
}

void TwitchUrlContextMenu::onCopy()
{
    if(mCurrentLabel)
    {
        if(mCurrentLabel->hasSelectedText())
        {
            QApplication::clipboard()->setText(mCurrentLabel->selectedText());
        }
    }
}

TwitchUrlContextMenu::~TwitchUrlContextMenu()
{
    if( mActionCopy )
    {
        delete mActionCopy;
        mActionCopy = NULL;
    }

    if( mNativeMenu )
    {
        delete mNativeMenu;
        mNativeMenu = NULL;
    }
}

void TwitchUrlContextMenu::popup(QLabel *label)
{
    if (Origin::Engine::IGOController::instance()->isActive())
    {
        bool actionsExist = false;
        mNativeMenu->clear();

        mCurrentLabel = NULL;
        if(label && label->hasSelectedText())
        {
            mCurrentLabel = label;
            mNativeMenu->addAction(mActionCopy);
            actionsExist = true;
        }

        if(actionsExist)
        {
            // get active highlight colors
            const QPalette& palette = mCurrentLabel->palette();
            const QBrush& brush_highlight = palette.brush(QPalette::Active, QPalette::Highlight);
            const QBrush& brush_highlightedtext = palette.brush(QPalette::Active, QPalette::HighlightedText);
            const QBrush& brush_highlight_inactive = palette.brush(QPalette::Inactive, QPalette::Highlight);
            const QBrush& brush_highlightedtext_inactive = palette.brush(QPalette::Inactive, QPalette::HighlightedText);

            // save inactive highlight colors
            mInactiveHighlightColor.setRgba(brush_highlight_inactive.color().rgba());
            mInactiveHighlightedTextColor.setRgba(brush_highlightedtext_inactive.color().rgba());

            // set inactive highlight colors using active highlight colors
            QPalette inactivePalette;
            inactivePalette.setBrush(QPalette::Inactive, QPalette::Highlight, brush_highlight);
            inactivePalette.setBrush(QPalette::Inactive, QPalette::HighlightedText, brush_highlightedtext);
            mCurrentLabel->setPalette(inactivePalette);

            Origin::Engine::IIGOWindowManager::WindowProperties cmProperties;
            if (Origin::Engine::IGOController::instance()->igowm()->showContextMenu(mWindow, mNativeMenu, &cmProperties))
                mNativeMenu->popup(cmProperties.position());
        }
    }
}

void TwitchUrlContextMenu::onFocusChanged(QWidget* window, bool hasFocus)
{
    Q_UNUSED(window);
    if (Origin::Engine::IGOController::instance()->isActive())
    {
        mNativeMenu->hide();
    }
}

void TwitchUrlContextMenu::onDeactivate()
{
    if (Origin::Engine::IGOController::instance()->isActive())
    {
        mNativeMenu->hide();

        // reset palette
        if( mCurrentLabel )
        {
            // set inactive highlight colors back to normal
            QPalette inactivePalette;
            inactivePalette.setBrush(QPalette::Inactive, QPalette::Highlight, QBrush(mInactiveHighlightColor));
            inactivePalette.setBrush(QPalette::Inactive, QPalette::HighlightedText, QBrush(mInactiveHighlightedTextColor));
            mCurrentLabel->setPalette(inactivePalette);
            mCurrentLabel->setFocus();
        }
    }
}

void TwitchUrlContextMenu::hiding()
{
    if (Origin::Engine::IGOController::instance()->isActive())
    {
        Origin::Engine::IGOController::instance()->igowm()->hideContextMenu(mWindow);
    }
}
