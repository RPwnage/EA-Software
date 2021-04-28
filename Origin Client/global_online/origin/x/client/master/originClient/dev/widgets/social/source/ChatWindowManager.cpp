/////////////////////////////////////////////////////////////////////////////
// ChatWindowManager.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ChatWindowManager.h"
#include "services/debug/DebugService.h"
#include "engine/social/SocialController.h"
#include "engine/social/ConversationManager.h"
#include "engine/social/Conversation.h"
#include "engine/login/LoginController.h"
#include "engine/igo/IGOController.h"
#include "services/settings/SettingsManager.h"
#include "services/qt/QtUtil.h"
#include "flows/ClientFlow.h"
#include "views/source/ChatWindowView.h"
#include "services/voice/VoiceService.h"

#include <QApplication>
#include <QDesktopWidget>

static const int MIN_CHAT_WIDTH = 532;
static const int MIN_CHAT_HEIGHT = 303;

static const int DEFAULT_CHAT_WIDTH = 560;
static const int DEFAULT_CHAT_HEIGHT = 480;

namespace Origin
{
    namespace Client
    {
        ChatWindowManager::ChatWindowManager(QWidget *parent) :
            mWidgetParent(parent),
            mIgnoreStartingConversation(false)
        {
            initOfflineConversations();

            ORIGIN_VERIFY_CONNECT(ClientFlow::instance()->view(), SIGNAL(groupDeleted(const QString&)), this, SLOT(onGroupDeleted(const QString&)));
        }

        ChatWindowManager::~ChatWindowManager()
        {
        }

        void ChatWindowManager::initOfflineConversations()
        {
            // Spit out any offline messages

            Engine::Social::ConversationManager* conversationManager = Engine::LoginController::currentUser()->socialControllerInstance()->conversations();
            QList<QSharedPointer<Engine::Social::Conversation>> allConversations = conversationManager->allConversations();

            QList<QSharedPointer<Engine::Social::Conversation>>::iterator i;
            for (i = allConversations.begin(); i != allConversations.end(); ++i)
            {
                (*i)->setCreationReason(Origin::Engine::Social::Conversation::IncomingMessage);
                startConversation(*i, Engine::IIGOCommandController::CallOrigin_CLIENT);
            }
        }

        int ChatWindowManager::windowCount() const
        {
            return mWindowList.count();
        }

        int ChatWindowManager::igoWindowCount() const
        {
            return mIGOWindowList.count();
        }

        void ChatWindowManager::raiseMainChatWindow()
        {
            // This assumes no tear off windows or in the future at least the concept of a singular "primary" window
            ChatWindowViewController* controller = mWindowList.first();
            if (controller)
            {
                // If we are not playing a game bring the chat window up to the front
                if (!Origin::Engine::IGOController::instance()->isGameInForeground())
                {
                    // Don't call showWindow here we need  show raise and focus the 
                    // window. OriginWindows only raise and activate when Origin is in focus 
                    // Fixes https://developer.origin.com/support/browse/EBIBUGS-27036
                    controller->window()->show();
                    controller->window()->raise();
                    controller->window()->activateWindow();
                }
                // if we are playing a game just do normal showWindow()
                else
                {
                    controller->window()->showWindow();
                }
            }
        }

        void ChatWindowManager::hideMainChatWindow()
        {
            if(!mWindowList.isEmpty())
            {
                // This assumes no tear off windows or in the future at least the concept of a singular "primary" window
                ChatWindowViewController* controller = mWindowList.first();
                if (controller && controller->window())
                {
                    controller->window()->hide();
                }
            }
        }

        ChatWindowViewController* ChatWindowManager::newWindow(QSharedPointer<Engine::Social::Conversation> conv, UIScope scope, Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            ChatWindowViewController* controller = new ChatWindowViewController(mWidgetParent, conv, scope);

            controller->window()->setMinimumWidth(MIN_CHAT_WIDTH);
            controller->window()->setMinimumHeight(MIN_CHAT_HEIGHT);

            if (scope == ClientScope)
            {
                restoreViewSizeAndPosition(controller->window()); // TODO - check to see if this is the master tabbed window or not
                ORIGIN_VERIFY_CONNECT(controller, SIGNAL(windowClosed(ChatWindowViewController*)), this, SLOT(onWindowClosed(ChatWindowViewController*)));
                mWindowList.append(controller);
                // We need to wait a small amount of time before we attempt to set focus/activate
                // the chat window otherwise a user double-clicking on the friends list will steal
                // focus away from the chat window
#ifdef ORIGIN_PC
                QTimer::singleShot(1, controller, SLOT(onActivateWindow()));
#endif
            }
            else
            {
                Origin::Engine::IIGOWindowManager::WindowProperties properties;
                properties.setCallOrigin(callOrigin);
                properties.setWId(Origin::Engine::IViewController::WI_CHAT);
                properties.setPosition(controller->defaultPosition());
                properties.setRestorable(true);
                properties.setOpenAnimProps(Engine::IGOController::instance()->defaultOpenAnimation());
                properties.setCloseAnimProps(Engine::IGOController::instance()->defaultCloseAnimation());
                
                Origin::Engine::IIGOWindowManager::WindowBehavior behavior;
                behavior.nsBorderSize = 10;
                behavior.ewBorderSize = 10;
                behavior.supportsContextMenu = true;
                behavior.pinnable = true;
                behavior.screenListener = controller;

                Engine::IGOController::instance()->igowm()->addWindow(controller->window(), properties, behavior);
                
                // We need to capture the cursor change for resizing the window when multiple chats are visible
                ORIGIN_VERIFY_CONNECT(controller->view(), SIGNAL(cursorChanged(int)), this, SLOT(setCursor(int)));

                ORIGIN_VERIFY_CONNECT(controller, SIGNAL(windowClosed(ChatWindowViewController*)), this, SLOT(onIGOWindowClosed(ChatWindowViewController*)));
                mIGOWindowList.append(controller);
                // Need to delay the OIG window on Mac since we delay the desktop window
                // This helps fix https://developer.origin.com/support/browse/EBIBUGS-24588
#ifdef ORIGIN_MAC
                QTimer::singleShot(500, this, SLOT(onActivateWindow()));
#else
                Engine::IGOController::instance()->igowm()->setFocusWindow(controller->window());
#endif
                if (Origin::Engine::IGOController::instance()->showWebInspectors())
                    Origin::Engine::IGOController::instance()->showWebInspector(controller->view()->page());
            }

            ORIGIN_VERIFY_CONNECT(controller, SIGNAL(tabClosed(QSharedPointer<Engine::Social::Conversation>)), this, SLOT(onTabClosed(QSharedPointer<Engine::Social::Conversation>)));

            return controller;
        }

        void ChatWindowManager::setCursor(int cursorShape)
        {
            Origin::Engine::IGOController::instance()->igowm()->setCursor(cursorShape);
        }
        
        void ChatWindowManager::onActivateWindow()
        {
            if(Engine::IGOController::instance()->isActive())
            {
                ChatWindowViewController* controller = mIGOWindowList.first();
                Engine::IGOController::instance()->igowm()->activateWindow(controller->window());
            }
        }
                                        
        void ChatWindowManager::startConversation(Chat::Conversable *conversable, Engine::Social::Conversation::CreationReason reason, Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            // Grabbing the conversation below might create it; ignore that so we don't call startConversation() twice
            mIgnoreStartingConversation = true;
            
            Engine::Social::ConversationManager* conversationManager = Engine::LoginController::currentUser()->socialControllerInstance()->conversations();
            QSharedPointer<Engine::Social::Conversation> conversation = conversationManager->conversationForConversable(conversable, reason);
            conversation->setCreationReason(reason);

            mIgnoreStartingConversation = false;
            startConversation(conversation, callOrigin);
        }

        void ChatWindowManager::startConversation(QSharedPointer<Origin::Engine::Social::Conversation> conv)
        {
            startConversation(conv, Engine::IIGOCommandController::CallOrigin_CLIENT);
        }

        void ChatWindowManager::startConversation(QSharedPointer<Origin::Engine::Social::Conversation> conv, Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            //if (mIgnoreStartingConversation)
            {
                return;
            }

            //Check for IGO first so that we can ensure we give the correct focus if IGO is active
            bool IGOActive = false;

            // IGO scope
            if (Engine::IGOController::instance())
            {
                if (Engine::IGOController::instance()->igowm()->visible())
                {
                    IGOActive = true;
                }
                ChatWindowViewController* controller = NULL;
                if (igoWindowCount())
                {
                    // Just open a new tab in the existing window
                    controller = mIGOWindowList.first();

                    switch(conv->creationReason())
                    {
                    case Origin::Engine::Social::Conversation::IncomingMessage:
                    case Origin::Engine::Social::Conversation::SDK:
                        controller->insertConversation(conv);
                        break;
                    case Origin::Engine::Social::Conversation::InternalRequest:
                    case Origin::Engine::Social::Conversation::StartVoice:
                        controller->raiseConversation(conv);
                    default:
                        break;
                    }
                }
                else
                {
                    // No windows available - create one
                    controller = newWindow(conv, IGOScope, callOrigin);
                }
                
                Origin::Engine::IIGOWindowManager* igowm = Origin::Engine::IGOController::instance()->igowm();
                igowm->showWindow(controller->window());
                
#ifdef ORIGIN_MAC
                if(IGOActive)
#endif
                    igowm->activateWindow(controller->window());
            }
            
            // Client scope
            if (windowCount())
            {
                // Just open a new tab in the existing window
                ChatWindowViewController* controller = mWindowList.first();
                switch(conv->creationReason())
                {
                case Origin::Engine::Social::Conversation::IncomingMessage:
                case Origin::Engine::Social::Conversation::SDK:
                    controller->insertConversation(conv);
                    break;
                case Origin::Engine::Social::Conversation::InternalRequest:
                case Origin::Engine::Social::Conversation::StartVoice:
                    controller->raiseConversation(conv);
                    if (!IGOActive)
                    {
                        // Don't call showWindow here we need  show raise and focus the 
                        // window. OriginWindows only raise and activate when Origin is in focus 
                        // Fixes https://developer.origin.com/support/browse/EBIBUGS-26822
                        controller->window()->show();
                        controller->window()->raise();
                        controller->window()->activateWindow();
                    }
                default:
                    break;
                }
            }
            else
            {
                // No windows available - create one
                newWindow(conv, ClientScope, Engine::IIGOCommandController::CallOrigin_CLIENT);
            }
        }

        void ChatWindowManager::onWindowClosed(ChatWindowViewController* window)
        {
            saveViewSizeAndPosition(window->window()); // TODO - check to see if this is the master tabbed window or not
            mWindowList.removeOne(window);

#if ENABLE_VOICE_CHAT
            bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
            if( isVoiceEnabled )
            {
                // Make sure to leave voice to prevent an assert (in Qt's HashTableConstIterator::checkValidity()) when closing the Chat Window
                // without first leaving voice or closing the individual chat tabs
                Engine::Social::ConversationManager* conversationManager = Engine::LoginController::currentUser()->socialControllerInstance()->conversations();
                QList<QSharedPointer<Engine::Social::Conversation>> allConversations = conversationManager->allConversations();

                QList<QSharedPointer<Engine::Social::Conversation>>::iterator i;
                for (i = allConversations.begin(); i != allConversations.end(); ++i)
                {
                    QSharedPointer<Engine::Social::Conversation> conversation = (*i);
                    if( !conversation->isVoiceCallDisconnected() )
                    {
                        conversation->leaveVoice();
                        break; // only one VoiceClient to disconnect
                    }
                }
            }
#endif

            // Close the IGO window also if we have one
            if (igoWindowCount())
            {
                ChatWindowViewController* controller = mIGOWindowList.first();
                if (controller)
                {
                    controller->window()->reject();
                }
            }
            window->deleteLater();
        }

        void ChatWindowManager::onIGOWindowClosed(ChatWindowViewController* window)
        {
            mIGOWindowList.removeOne(window);

            // Close the desktop window also if we have one
            if (windowCount())
            {
                ChatWindowViewController* controller = mWindowList.first();
                if (controller)
                {
                    controller->window()->reject();
                }
            }

            if (igoWindowCount() == 0)
            {
                Engine::IGOController::instance()->igowm()->removeWindow(window->window(), true);
            }

            delete window;
        }

        void ChatWindowManager::onTabClosed(QSharedPointer<Engine::Social::Conversation> conv)
        {
            if (conv)
            {
                // We need to make sure that if a tab is closed in one window it is also closed in the other

                // IGO scope
                ChatWindowViewController* controller = mIGOWindowList.isEmpty() ? NULL : mIGOWindowList.first();
                if (controller)
                {
                    controller->closeConversation(conv);
                }

                // Client scope
                controller = mWindowList.isEmpty() ? NULL : mWindowList.first();
                if (controller)
                {
                    controller->closeConversation(conv);
                }
            }
        }

        void ChatWindowManager::saveViewSizeAndPosition(UIToolkit::OriginWindow* window)
        {
            Origin::Services::writeSetting(Origin::Services::SETTING_CHATWINDOWSSIZEPOSITION, Origin::Services::getFormatedWidgetSize(window));
        }
            
        void ChatWindowManager::restoreViewSizeAndPosition(UIToolkit::OriginWindow* window)
        {
            // The Friends List should open to the right side of the client upon
            // initial spawn. The chat window should open to the right of the
            // friends list, provided it has space if not, place it to the left.
            // If the friends list is not currently open, the conversation
            // should appear in the center of the screen on initial spawn.

            // Check the existence and location of the chat window
            const QString formatedSizePos = Services::readSetting(Services::SETTING_CHATWINDOWSSIZEPOSITION);
            if (Services::isDefault(Services::SETTING_CHATWINDOWSSIZEPOSITION) && ClientFlow::instance())
            {
                /*
                TODO RS: Do we need to handle this for popped chat windows?  
                FriendsListViewController* friendsListViewController = ClientFlow::instance()->friendsListViewController();
                if (friendsListViewController)
                {
        	        QSize defaultSize(DEFAULT_CHAT_WIDTH,DEFAULT_CHAT_HEIGHT);
        	        QRect screenRec = QApplication::desktop()->screenGeometry(friendsListViewController->window());
        	        QRect availableRec = QApplication::desktop()->availableGeometry(friendsListViewController->window());
        	        QPoint pos((screenRec.width() - defaultSize.width()) / 2, (screenRec.height() - defaultSize.height()) / 2);

                    QRect rosterGeom = friendsListViewController->window()->geometry();

                    // See if there is space to put the chat window on the right side of the friends list
                    const int gutter = 10;

                    if ((availableRec.right() - rosterGeom.right()) > (defaultSize.width() + (gutter * 2)))
                    {
                        // Move the chat window to the right of the friends list
                        pos.setX(rosterGeom.right() + gutter);
                    }
                    else if (rosterGeom.x() > (defaultSize.width() + (gutter * 2)))
                    {
                        // Move the chat window to the left of the friends list
                        pos.setX(rosterGeom.x() - (defaultSize.width() + gutter));
                    }
                    // else leave it centered horizontally

                    // Center it vertically against the friends list
                    pos.setY((rosterGeom.y() + (rosterGeom.height() / 2)) - (defaultSize.height() / 2));

                    // Set the default geometry
                    window->setGeometry(pos.x(),pos.y(),defaultSize.width(),defaultSize.height());
                }*/
            }
            else
            {
                // Apply the saved size and position if present
                window->setGeometry(Services::setFormatedWidgetSizePosition(formatedSizePos, QSize(DEFAULT_CHAT_WIDTH, DEFAULT_CHAT_HEIGHT), true, true));
#if defined(ORIGIN_PC)
                if(QString(formatedSizePos.split("|").first()).contains("MAXIMIZED",Qt::CaseInsensitive))
                {
                    window->showMaximized();
                }
#endif
            }
        }

        bool ChatWindowManager::isChatWindowActive() const
        {
            if (windowCount())
            {
                ChatWindowViewController* controller = mWindowList.first();
                // Need to make sure that Origin is the active application before we declare that the chat window is Active
                // Fixes https://developer.origin.com/support/browse/EBIBUGS-26535
                if (controller && controller->window()->isActiveWindow() && QApplication::focusWidget())
                {
                    return true;
                }
            }
            return false;
        }

        bool ChatWindowManager::isIGOChatWindowActive() const
        {
            if (igoWindowCount())
            {
                ChatWindowViewController* controller = mIGOWindowList.first();
                if (controller && Origin::Engine::IGOController::instance()->igowm()->isActiveWindow(controller->window()))
                {
                    return true;
                }
            }
            return false;
        }

        void ChatWindowManager::onGroupDeleted(const QString& groupGuid)
        {
            // IGO scope
            ChatWindowViewController* controller = mIGOWindowList.isEmpty() ? NULL : mIGOWindowList.first();
            if (controller)
            {
                controller->closeConversation(groupGuid);
            }

            // Client scope
            controller = mWindowList.isEmpty() ? NULL : mWindowList.first();
            if (controller)
            {
                controller->closeConversation(groupGuid);
            }

        }

    }
}
