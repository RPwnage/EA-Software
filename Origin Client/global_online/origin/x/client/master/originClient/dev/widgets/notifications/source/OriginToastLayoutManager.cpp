#include "OriginToastLayoutManager.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QWidget>

#include "engine/igo/IGOController.h"
#include "ToastViewController.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"


namespace Origin
{
    namespace Client
    {
        namespace
        {
            const int TOAST_WORKSPACE_VERTICAL_MARGIN = 5;
            const int TOAST_RIGHT_SEPARATION = 20;
            const int TOAST_SEPARATION_BETWEEN_WINDOWS = -18;
            const int TOAST_MAX_WIDTH = 309;
#ifdef ORIGIN_PC
            const int TOAST_BOTTOM_SEPARATION = 10;
            const int TOAST_VOIP_HEIGHT = 35;//?????????????
#endif
        }

        OriginToastLayoutManager::OriginToastLayoutManager(const UIScope& scope, const int poolMax, const bool vertical)
            : mScope(scope)
            , mPoolMax(poolMax)
            , mIsVertical(vertical)
        {
            if (mScope == IGOScope)
            {
                initIGOLayout();
                Origin::Engine::IGOController::instance()->igowm()->registerScreenListener(this);
            }
            else
                initDesktopLayout();
        }


        OriginToastLayoutManager::~OriginToastLayoutManager()
        {
        }


        void OriginToastLayoutManager::initIGOLayout()
        {
            QSize screen = Origin::Engine::IGOController::instance()->igowm()->screenSize();
            mScreenWidth = screen.width();
            mScreenHeight = screen.height();
            // default to desktop if IGO is not available
            // This is mainly for the debug menu fakeIGO option
            // Proper IGO screen size will be set if the user starts a game
            if (mScreenWidth == 0 || mScreenHeight == 0)
                initDesktopLayout();

        }


        void OriginToastLayoutManager::initDesktopLayout()
        {
            QDesktopWidget* desktopWidget = QApplication::desktop();
            // Connect to all QDesktopWidget signals so we place notifications correctly
            ORIGIN_VERIFY_CONNECT(desktopWidget, SIGNAL(resized(int)), this, SLOT(onDesktopSizeChanged()));
            ORIGIN_VERIFY_CONNECT(desktopWidget, SIGNAL(screenCountChanged(int)), this, SLOT(onDesktopSizeChanged()));
            ORIGIN_VERIFY_CONNECT(desktopWidget, SIGNAL(workAreaResized(int)), this, SLOT(onDesktopSizeChanged()));

            QRect clientRect = desktopWidget->availableGeometry();
            
            if (mScope != IGOScope)
                mScreenOffset = QPoint(clientRect.x(), clientRect.y());
            
            mScreenWidth = clientRect.width();
            mScreenHeight = clientRect.height();
        }

        void OriginToastLayoutManager::onSizeChanged(uint32_t width, uint32_t height)
        {
            mScreenWidth = (int)width;
            mScreenHeight = (int)height;
            update();
            // default to desktop if IGO is not available
            // This is mainly for the debug menu fakeIGO option
            // Proper IGO screen size will be set if the user starts a game
            if (mScreenWidth == 0 || mScreenHeight == 0)
                initDesktopLayout();
        }

        void OriginToastLayoutManager::onDesktopSizeChanged()
        {
            QDesktopWidget* desktopWidget = QApplication::desktop();
            // for now we only show notifications on the main screen
            QRect clientRect = desktopWidget->availableGeometry();
            
            if (mScope != IGOScope)
                mScreenOffset = QPoint(clientRect.x(), clientRect.y());
            
            mScreenWidth = clientRect.width();
            mScreenHeight = clientRect.height();
        }

        void OriginToastLayoutManager::add(QWidget* toast, int id)
        {
            // Are we adding a specail id?
            if (id != GenericID)
            {
                if (id == ToastViewController::ToastType_IGOhotkey ||
                    id == ToastViewController::ToastType_VoiceIdentifierCounter)
                {
                    // We only want one instance of this specail type in our pool
                    for( int index=0; index<mPool.size(); index++) 
                    {
                        QPair<int, QPointer<QWidget>> oldPair = mPool.at(index);
                        if (oldPair.first == id)
                        {
                            remove(oldPair.second);
                            // Here we are assuming that we only had one
                            break;
                        }
                    }
                }
            }

            QPair<int, QPointer<QWidget>> newPair = qMakePair(id, QPointer<QWidget>(toast));

            int firstCallIndex = mPool.size(); // setting this to size() acts as append()
 
            if(mIsVertical)
            {
                // We want all incomming call notifications at the end of the list so that
                // they are drawn first (closest to the corner)
                if (id == ToastViewController::ToastType_IncomingCall)
                {
                    // If new toast is an incomming call add to the end of the list
                    // Need to increase the pool max so that incoming call toasts
                    // are not replaced by other toasts
                    mPoolMax += 1;
                }
                else
                {
                    // We need to look for the first incomming call toast so we can 
                    // add the new toast before it in the list.
                    for( int index=0; index<mPool.size(); index++) 
                    {
                        QPair<int, QPointer<QWidget>> oldPair = mPool.at(index);
                        if (oldPair.first == ToastViewController::ToastType_IncomingCall)
                        {
                            // If we have one save this index so that we can add the new toast here
                            firstCallIndex = index;
                            break;
                        }
                    }


                }
            }
            else
            {
                if (id == ToastViewController::ToastType_VoiceIdentifierCounter)
                {
                    firstCallIndex = 0; // we want the counter to always be last (farthest from the corner)
                }
            }
            mPool.insert(firstCallIndex,newPair);

            if(mPool.size() > mPoolMax)
            {
                mPool.removeFirst();
            }
            ORIGIN_VERIFY_CONNECT(toast, SIGNAL(finished(int)), this, SLOT(onToastClosing()));
            update();
        }

        void OriginToastLayoutManager::onToastClosing()
        {
            QWidget* toast = static_cast<QWidget*>(sender());
            remove(toast);
        }


        void OriginToastLayoutManager::remove(QWidget* toast)
        {
            for( int index=0; index<mPool.size(); index++) 
            {
                QPair<int, QPointer<QWidget>> oldPair = mPool.at(index);
                if (oldPair.second == toast)
                {
                    // If we are removing an incoming call toast drop our pool max down
                    if (oldPair.first == ToastViewController::ToastType_IncomingCall)
                    {
                        mPoolMax -= 1;
                    }
                    mPool.remove(index);
                    // Here we are assuming that we only had one
                    break;

                }
            }
        }


        void OriginToastLayoutManager::update()
        {
            if(mIsVertical)
            {
                verticalUpdate();
            }
            else
            {
                horizontalUpdate();
            }
        }

        void OriginToastLayoutManager::verticalUpdate()
        {
            if (mScreenWidth == 0 || mScreenHeight == 0)
                initIGOLayout();
            if(mPool.size() ==0)
                return;
            for( int index=mPool.size()-1; index >= 0; index--) 
            {
                QPointer<QWidget> widget = mPool.at(index).second;
                if (widget.isNull())                    
                    mPool.remove(index);
            }

   	        int xPos = 0;
            int yPos = 0;
            int x = 0;
            int y = 0;
            bool first = true;
#ifdef ORIGIN_MAC
            xPos += (TOAST_MAX_WIDTH + TOAST_SEPARATION_BETWEEN_WINDOWS - 4);
#else
            //this part it temporary and will probably change once we set up widgets with dynamic width
            xPos += (TOAST_MAX_WIDTH + TOAST_RIGHT_SEPARATION) + TOAST_SEPARATION_BETWEEN_WINDOWS;
#endif
            x = mScreenWidth - xPos;
            for( int index=mPool.size()-1; index>= 0; index--) 
            {
                QPointer<QWidget> toast = mPool.at(index).second;
                if (toast)
                {
#ifdef ORIGIN_MAC
                    yPos += TOAST_WORKSPACE_VERTICAL_MARGIN;
                    if(first && mScope != IGOScope)
                    {
                       // Need to space the first Mac notification down a little bit
                       yPos += mScreenOffset.y();
                       first = false;
                   }
                   y = yPos; // Mac notifications begin in the upper right
                   // Add this here so the next notification is spaced correctly
                   yPos += (toast->height());
#else
                    if(first && mScope == IGOScope)
                    {
                        yPos += (TOAST_VOIP_HEIGHT + TOAST_BOTTOM_SEPARATION);
                        first = false;
                    }
                    yPos += TOAST_WORKSPACE_VERTICAL_MARGIN;
                    yPos += (toast->height());
                    y = mScreenHeight - yPos; // Windows notifications begin in the lower right
#endif
                    QPoint position(x, y);
                    if (mScope == IGOScope)
                    {
                        repositionIGOToast(toast, position);
                    }
                    else
                        repositionDesktopToast(toast, position);
                }
            }

        }

        void OriginToastLayoutManager::horizontalUpdate()
        {
            if (mScreenWidth == 0 || mScreenHeight == 0)
                initIGOLayout();
            if(mPool.size() == 0)
                return;
            for( int index=mPool.size()-1; index >= 0; index--) 
            {
                QPointer<QWidget> widget = mPool.at(index).second;
                if (widget.isNull())                    
                    mPool.remove(index);
            }

   	        int xPos = 0;
            int yPos = 0;
            int x = 0;
            int y = 0;

#ifdef ORIGIN_MAC
            yPos += mScreenOffset.y() + TOAST_WORKSPACE_VERTICAL_MARGIN;
            y = yPos; // Mac notifications begin in the upper right
            // Add this here so the next notification is spaced correctly
            //yPos += (TOAST_VOIP_HEIGHT + TOAST_BOTTOM_SEPARATION);
#else
            yPos += TOAST_WORKSPACE_VERTICAL_MARGIN;
            yPos += (TOAST_VOIP_HEIGHT + TOAST_BOTTOM_SEPARATION);
            y = mScreenHeight - yPos; // Windows notifications begin in the lower right
#endif
            for( int index=mPool.size()-1; index>= 0; index--) 
            {
                QPointer<QWidget> toast = mPool.at(index).second;
                if (toast)
                {
                    //this part it temporary and will probably change once we set up widgets with dynamic width
                    xPos += (TOAST_RIGHT_SEPARATION) + TOAST_SEPARATION_BETWEEN_WINDOWS;
                    xPos += toast->width();
                    x = mScreenWidth - xPos;
                    QPoint position(x, y);
                    if (mScope == IGOScope)
                    {
                        repositionIGOToast(toast, position);
                    }
                    else
                        repositionDesktopToast(toast, position);
                }
            }
        }

        void OriginToastLayoutManager::repositionDesktopToast(QWidget* toast, QPoint position)
        {                             
            toast->move(position);
        }


        void OriginToastLayoutManager::repositionIGOToast(QWidget* toast, QPoint position)
        {
            toast->adjustSize();

			ORIGIN_LOG_DEBUG << "Position Toast=" << toast << " at X=" << position.x() << ", Y=" << position.y();

            Origin::Engine::IIGOWindowManager::WindowProperties properties;
            properties.setPosition(position);
            properties.setSize(QSize(toast->width(), toast->height()));
            Origin::Engine::IGOController::instance()->igowm()->setWindowProperties(toast, properties);
        }

    } // namespace Client
} // namespace Origin
