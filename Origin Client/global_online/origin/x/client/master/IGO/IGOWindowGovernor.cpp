//
//  IGOWindowGovernor.cpp
//  IGO
//
//  Created by Gamesim on 7/13/15.
//  Copyright (c) 2015 Electronic Arts. All rights reserved.
//
#include "IGOWindowGovernor.h"

#include "IGOWindow.h"
#include "IGOLogger.h"

#ifdef ORIGIN_MAC
#include "OGLHook.h"
#endif

namespace OriginIGO
{
// detect concurrencies - assert if our mutex is already locked
#ifdef _DEBUG
    #define DEBUG_MUTEX_TEST(x){\
	    bool s=x.TryLock();\
	    if(s)\
		    x.Unlock();\
        else \
            IGOLogWarn("IGO threading lock");\
	    }
#else
    #define DEBUG_MUTEX_TEST(x) void(0)
#endif

IGOWindowGovernor* IGOWindowGovernor::instance()
{
    static IGOWindowGovernor governor;
    return &governor;
}

IGOWindowGovernor::IGOWindowGovernor()
{
    mCompleteCleanupRequired = false;
}

IGOWindowGovernor::~IGOWindowGovernor()
{
}

IGOWindow* IGOWindowGovernor::create(uint32_t windowId, RendererType renderer, bool* created)
{
    DEBUG_MUTEX_TEST(mWindowMutex);
	EA::Thread::AutoFutex m(mWindowMutex);
	WindowMap::const_iterator iter = mWindows.find(windowId);
	if (iter != mWindows.end())
    {
        if (created)
            *created = false;

        // Make sure we have the proper render type in case there was a switch
        IGOWindow* window = iter->second;
        window->setRenderer(renderer);

		return window;
    }
	
	IGOLogInfo("Create window 0x%x", windowId);
	IGOWindow *window = new IGOWindow(windowId, renderer); 
	mWindows[windowId] = window;
    if (windowId == BACKGROUND_WINDOW_ID)
        mWindowList.insert(mWindowList.begin(), window);

    else
    if (windowId == DISPLAY_WINDOW_ID)
        mWindowList.push_back(window);

    else
    {
        if (mWindowList.size() > 0)
        {
            if (mWindowList.back()->id() == DISPLAY_WINDOW_ID)
            {
                WindowList::iterator position = mWindowList.begin();
                advance(position, mWindowList.size() - 1);
                mWindowList.insert(position, window);
            }

            else
                mWindowList.push_back(window);
        }

        else
            mWindowList.push_back(window);
    }
        
    if (created)
        *created = true;
        
	return window;
}

void IGOWindowGovernor::move(uint32_t windowId, uint32_t index)
{
    DEBUG_MUTEX_TEST(mWindowMutex);
	EA::Thread::AutoFutex m(mWindowMutex);
	WindowMap::const_iterator iter = mWindows.find(windowId);
	if (iter != mWindows.end())
	{
        IGOWindow* window = (*iter).second;
		WindowList::iterator iterList = find(mWindowList.begin(), mWindowList.end(), window);
		IGO_ASSERT(iterList != mWindowList.end());
		
		if (iterList != mWindowList.end())
		{	
			mWindowList.erase(iterList);

            // Really, the background should always be first and the info window last, but 1) to be safe 2) in case we start
            // adding more IGO-only windows
            uint32_t currentIdx = 0;
            iterList = mWindowList.begin();
            for (; iterList != mWindowList.end(); ++iterList)
            {
                IGOWindow* iterWnd = *iterList;
                if (windowId != BACKGROUND_WINDOW_ID && windowId != DISPLAY_WINDOW_ID)
                {
                        if (iterWnd->id() == BACKGROUND_WINDOW_ID)
                            continue;

						if (iterWnd->id() == DISPLAY_WINDOW_ID)
							break;
                }

                if (currentIdx == index)
                    break;
                    
                ++currentIdx;
            }
                
            mWindowList.insert(iterList, window);
                
			IGOLogInfo("Move window = %d, %d, size = %dx%d", window->id(), index, window->width(), window->height());
		}
		else
			IGOLogInfo("Tried to move a dead window (%d).", windowId);
	}
        
#ifdef _DEBUG
    char displayIDs[128] = { 0 };
    if (getWindowIDs(displayIDs, sizeof(displayIDs)))
        IGOLogInfo("Window IDs: %s", displayIDs);
#endif
}

void IGOWindowGovernor::close(uint32_t windowId)
{
    DEBUG_MUTEX_TEST(mWindowMutex);
	EA::Thread::AutoFutex m(mWindowMutex);
	WindowMap::iterator iter = mWindows.find(windowId);
	if (iter != mWindows.end())
	{
		WindowList::iterator iterList = find(mWindowList.begin(), mWindowList.end(), (*iter).second);

		IGOWindow *window = (*iter).second;
		IGO_ASSERT(iterList != mWindowList.end());
		mWindowList.erase(iterList);
		mWindows.erase(iter);

		IGOLogInfo("Close window = %d, size = %dx%d", window->id(), window->width(), window->height());

        window->prepareForDeletion();
        mDeletableWindows.push_back(window);
	}
}

void IGOWindowGovernor::prepareForCompleteCleanup()
{
    mCompleteCleanupRequired = true;
}

void IGOWindowGovernor::clear(void* renderCtxt)
{
    DEBUG_MUTEX_TEST(mWindowMutex);
	EA::Thread::AutoFutex m(mWindowMutex);
	IGOLogInfo("Clear all IGO windows");

	for (WindowMap::const_iterator iter = mWindows.begin(); iter != mWindows.end(); ++iter)
	{
        IGOWindow* window = iter->second;
		IGO_ASSERT(window != NULL);

        // Clean up the underlying surface, but keep the window around = keep the window setup (like anims and all)
        // EXCEPT if we lost the connection = Origin died
		window->dealloc(renderCtxt);

        if (mCompleteCleanupRequired)
            delete window;
	}

    for (WindowList::const_iterator iter = mDeletableWindows.begin(); iter != mDeletableWindows.end(); ++iter)
    {
        IGOWindow* window = *iter;
        window->dealloc(renderCtxt);
        delete window;
    }

    if (mCompleteCleanupRequired)
    {
	    mWindowList.clear();
	    mWindows.clear();
    }
    mDeletableWindows.clear();

    mCompleteCleanupRequired = false;
}

void IGOWindowGovernor::refresh(RendererType renderer, eastl::vector<uint32_t>& windowIDs)
{
    DEBUG_MUTEX_TEST(mWindowMutex);
	EA::Thread::AutoFutex m(mWindowMutex);
	IGOLogInfo("Refresh IGO windows");

    // Add our local window IDs: the background the info display.
    windowIDs.insert(windowIDs.begin(), BACKGROUND_WINDOW_ID);
    windowIDs.push_back(DISPLAY_WINDOW_ID);

#ifdef _DEBUG
    char displayIDs[128] = { 0 };
    if (getWindowIDs(displayIDs, sizeof(displayIDs)))
        IGOLogInfo("Window IDs: %s", displayIDs);
#endif
        
    // Recreate our lists
    WindowMap newWindows;
    WindowList newWindowsList;
    for (eastl::vector<uint32_t>::const_iterator iter = windowIDs.begin(); iter != windowIDs.end(); ++iter)
    {
        uint32_t windowID = *iter;
        WindowMap::const_iterator existingRef = mWindows.find(windowID);
        IGOWindow* window = NULL;
        if (existingRef != mWindows.end())
            window = (*existingRef).second;
            
        else
            window = new IGOWindow(windowID, renderer);
                
        newWindows[windowID] = window;
        newWindowsList.push_back(window);
    }

    // Time to delete the windows that are not in use anymore
    for (WindowList::const_iterator iter = mWindowList.begin(); iter != mWindowList.end(); ++iter)
    {
        WindowList::const_iterator keptIter = eastl::find(newWindowsList.begin(), newWindowsList.end(), *iter);
        if (keptIter == newWindowsList.end())
		{
            (*iter)->prepareForDeletion();
            mDeletableWindows.push_back(*iter);
        }
    }
        
	mWindowList = newWindowsList;
	mWindows = newWindows;
}

void IGOWindowGovernor::render(void* renderCtxt, IGOApplication* app, const AppContext& appCtxt)
{
    DEBUG_MUTEX_TEST(mWindowMutex);
	EA::Thread::AutoFutex m(mWindowMutex);

    // Clean up any closing windows
    for (WindowList::iterator iter = mDeletableWindows.begin(); iter != mDeletableWindows.end();)
    {
        IGOWindow* window = *iter;
        if (window->readyForDeletion())
        {
            window->dealloc(renderCtxt);
            delete window;

            iter = mDeletableWindows.erase(iter);
        }

        else
            ++iter;
    }

    // Don't render anything if the game doesn't have focus
    if (!appCtxt.hasFocus)
    {
        // Complete any running anim
        for (WindowList::const_iterator iter = mWindowList.begin(); iter != mWindowList.end(); ++iter)
			(*iter)->forceCloseAnim();

        return;
    }

	// never call checkHotKey && toggleIGO from this call, because it usually lives in a different thread (not the main win32 message thread!!!)

    // Did we just open/close OIG? ie need to trigger the appropriate animation!
    static bool prevActiveState = false;
    bool active = appCtxt.isActive;
    if (prevActiveState != active)
    {
#ifdef SHOW_ANIM_ACTIVITIES
        IGOLogWarn("Active state change: prevActiveState=%d, active=%d", prevActiveState, active);
        app->dumpActiveState();
#endif
        prevActiveState = active;

        if (active)
        {
#ifdef SHOW_ANIM_ACTIVITIES
            IGOLogWarn("Render -> startOpenAnim");
#endif
            for (WindowList::const_iterator iter = mWindowList.begin(); iter != mWindowList.end(); ++iter)
				(*iter)->startOpenAnim();
        }

        else
        {
#ifdef SHOW_ANIM_ACTIVITIES
            IGOLogWarn("Render -> startCloseAnim");
#endif
            for (WindowList::const_iterator iter = mWindowList.begin(); iter != mWindowList.end(); ++iter)
				(*iter)->startCloseAnim();
        }
    }

    // If we are closing OIG but still animating, keep on rendering
    bool forceRender = false;
    for (WindowList::const_iterator iter = mWindowList.begin(); iter != mWindowList.end(); ++iter)
    {
        // We are not very precise right now because we are only using animations when opening/closing windows/OIG,
        // but we may need to be at some point
        if ((*iter)->animRunning())
        {
            forceRender = true;
            break;
        }
    }

	// render each window
#ifdef ORIGIN_PC
    if (appCtxt.screenWidth >= appCtxt.minScreenWidth && appCtxt.screenHeight >= appCtxt.minScreenHeight) //only if we have a valid resolution!!!
    {
        WindowList::const_iterator iter;
        for (iter = mWindowList.begin(); iter != mWindowList.end(); ++iter)
        {
			(*iter)->renderIGO(renderCtxt, app, forceRender, appCtxt.deltaT);
        }

        for (iter = mDeletableWindows.begin(); iter != mDeletableWindows.end(); ++iter)
        {
			(*iter)->renderIGO(renderCtxt, app, forceRender, appCtxt.deltaT);
        }
    }
#else
    OGLHook::DevContext* oglCtxt = (OGLHook::DevContext*)renderCtxt;
    
    WindowList::const_iterator iter;
    for (iter = mWindowList.begin(); iter != mWindowList.end(); ++iter)
    {
        if (app->dumpWindowBitmaps())
        {
            oglCtxt->SetRequiresRendering();
            (*iter)->dumpBitmap(renderCtxt, app);
        }
            
        if ((*iter)->checkDirty(app, forceRender))
        {
            oglCtxt->SetRequiresRendering();
            (*iter)->renderIGO(renderCtxt, app, appCtxt.deltaT);
        }
    }
#endif
}

void IGOWindowGovernor::unrender(void* context, IGOApplication* app)
{
#ifdef ORIGIN_PC
	EA::Thread::AutoFutex m(mWindowMutex);
	WindowList::const_iterator iter;
	for (iter = mWindowList.begin(); iter != mWindowList.end(); ++iter)
	{
		(*iter)->unrenderIGO(context, app);
	}
#endif
}

// Dump a few stats to the system console
void IGOWindowGovernor::dumpStats()
{
    DEBUG_MUTEX_TEST(mWindowMutex);
    EA::Thread::AutoFutex m(mWindowMutex);

    IGOLogWarn("OIG Stats: %d windows\n", mWindowList.size());
        
    uint32_t idx = 1;
    char info[MAX_PATH];
	WindowMap::const_iterator iter;
	for (iter = mWindows.begin(); iter != mWindows.end(); ++iter)
	{
        IGOWindow* window = (*iter).second;
        window->dumpInfo(info, sizeof(info));
        IGOLogWarn("  - %d. %s\n", idx, info);
            
        ++idx;
	}
}

uint32_t IGOWindowGovernor::textureSize()
{
    DEBUG_MUTEX_TEST(mWindowMutex);
	EA::Thread::AutoFutex m(mWindowMutex);
	uint32_t count = 0;
	    
	for (WindowMap::const_iterator iter = mWindows.begin(); iter != mWindows.end(); ++iter)
	{
		IGOWindow *win = (*iter).second;
		count += win->width()*win->height()*4;
	}

   	for (WindowList::const_iterator iter = mDeletableWindows.begin(); iter != mDeletableWindows.end(); ++iter)
	{
		IGOWindow *win = *iter;
		count += win->width()*win->height()*4;
	}

	return count;
}

bool IGOWindowGovernor::getWindowIDs(char* buffer, size_t bufferSize)
{
    char wID[16];
    uint32_t currSize = 0;
    for (WindowList::iterator debugIter = mWindowList.begin(); debugIter != mWindowList.end(); ++debugIter)
    {
        IGOWindow* wnd = *debugIter;
#ifdef ORIGIN_MAC
        uint32_t interSize = snprintf(wID, sizeof(wID), "%d ", wnd->id());
#else
        uint32_t interSize = _snprintf(wID, sizeof(wID), "%d ", wnd->id());
#endif
        if ((currSize + interSize + 1) >= bufferSize)
            break;

        strncat(buffer, wID, bufferSize - (currSize + 1));
        currSize += interSize;
    }

    return true;
}

} // namespace OriginIGO