//
//  IGOWindowManager.h
//  IGO
//
//  Created by Gamesim on 7/13/15.
//  Copyright (c) 2015 Electronic Arts. All rights reserved.
//
#ifndef IGOWINDOWMANAGER_H
#define IGOWINDOWMANAGER_H

#include "EASTL/hash_map.h"
#include "EASTL/list.h"
#include "EASTL/vector.h"
#include "eathread/eathread_futex.h"

#include "IGOSharedStructs.h"


namespace OriginIGO
{
// Fwd decls
class IGOWindow;
class IGOApplication;

class IGOWindowGovernor
{
public:
    static IGOWindowGovernor* instance();

    // How many windows do we have right now? (with or without render backend surface)
    uint32_t count() { return static_cast<uint32_t>(mWindows.size()); }

    // Lookup a window by id / create a new window in top of the stack if necessary
    IGOWindow* create(uint32_t windowId, RendererType renderer, bool* created);

    // Move window layer (larer index = above)
    void move(uint32_t windowId, uint32_t index);

    // Close a window by id
    void close(uint32_t windowId);

    // Notify that we want to actually clean up all windows data next time we call 'clear' (because we lost the connection with Origin = Origin was closed/crashed)
    // Otherwise even if we re-create the main IGOApplication/re-hook/do any crazy stuff, we want to keep the windows data (for example anim data) around
    void prepareForCompleteCleanup();

    // Clean up the windows render backend surfaces BUT keep the window data around - EXCEPT if we previously call 'prepareForCompleteCleanup'
    void clear(void* renderCtxt);

    // Encapsulates the reset of our current window layout - we remove windows that don't appear in the list anymore, we re-organize the layering, etc...
    void refresh(RendererType renderer, eastl::vector<uint32_t>& windowIDs);

    struct AppContext
    {
        uint32_t screenWidth;
        uint32_t screenHeight;
        uint32_t minScreenWidth;
        uint32_t minScreenHeight;
        float deltaT;
        bool isActive;
        bool hasFocus;
    };

    // Render the set of windows based on the current application context (ie screen size, whether active/with focus, ...)
    void render(void* renderCtxt, IGOApplication* app, const AppContext& appCtxt);

    // Magic necessary for Mantle
    void unrender(void* context, IGOApplication* app);

    // Debug helpers
    void dumpStats();
    uint32_t textureSize();
    bool getWindowIDs(char* buffer, size_t bufferSize);

private:
    IGOWindowGovernor();
    ~IGOWindowGovernor();


    EA::Thread::Futex mWindowMutex;

    typedef eastl::hash_map<uint32_t, IGOWindow *> WindowMap;
    WindowMap mWindows;

    typedef eastl::list<IGOWindow *> WindowList;
    WindowList mWindowList;
    WindowList mDeletableWindows;
    
    bool mCompleteCleanupRequired;
};

} // namespace OriginIGO

#endif // IGOWINDOWMANAGER_H