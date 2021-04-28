///////////////////////////////////////////////////////////////////////////////
// IGOWindow.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef IGOWINDOW_H
#define IGOWINDOW_H

#include "IGOApplication.h"

namespace OriginIGO {

    #include "IGOAnimationTypes.h"

    class IGOSurface;

    class IGOWindow 
    {
    public:
	    IGOWindow(uint32_t id, RendererType type);
	    virtual ~IGOWindow();
		void dealloc(void *context);
        void setRenderer(RendererType type);

		void dumpInfo(char* buffer, size_t bufferSize);
		void dumpBitmap(void *context, IGOApplication *app);
#ifdef ORIGIN_MAC
        bool checkDirty(IGOApplication *app, bool forceRender);
        void renderIGO(void *context, IGOApplication *app, float deltaT);
#else
        bool checkDirty();
        void renderIGO(void *context, IGOApplication *app, bool forceRender, float deltaT);
        void unrenderIGO(void *context, IGOApplication *app);
#endif
		void update(void *context, bool visible, uint8_t alpha, int32_t x, int32_t y, uint32_t width, uint32_t height, const char *data, int size, uint32_t updateFlags);
        void updateAnimValue(enum WindowAnimPropIndex propIndex, int32_t value);
		void updateRects(void *context, const eastl::vector<IGORECT> &rects, const char *data, int size);

        int width() const { return mAnimCurrent.props.width; }
        int height() const { return mAnimCurrent.props.height; }
        int x() const;
        int y() const;
        int alpha() const { return mAnimCurrent.props.alpha; }
        int customValue() const { return mAnimCurrent.props.customValue; }
        uint32_t id() { return mId; }
        bool isAlwaysVisible() const { return mAlwaysVisible; }
        void resize(uint32_t w, uint32_t h);
        void setPos(int32_t x, int32_t y);
        void prepareForDeletion();
        bool readyForDeletion();

        // Anim public interface
        void setAnimProps(WindowAnimContext ctxt, const AnimPropertySet* props);
        void startOpenAnim();
        void startCloseAnim();
        void forceCloseAnim();
        void stopAnim();
        bool animRunning() const;
        enum WindowState
        {
            WindowState_NONE = 0, // no animation to worry about
            WindowState_CLOSED,
            WindowState_OPENING,
            WindowState_OPENED,
            WindowState_CLOSING
        };
        WindowState state() const { return mWindowState; }

    private:
        void updateAnim(IGOApplication* app, float deltaT);
        void resolveAnimProps(const AnimPropertySet& src, WindowAnimExplicitProperties* dest);
        void dumpAnimData(const char* title, const AnimPropertySet& props);
        void dumpAnimData(const char* title, const WindowAnimProperties& props);

        RendererType mType;
        IGOSurface *mSurface;
        bool mAlwaysVisible;    // always visible even when IGO is turned off
        bool mPendingSetup;     // true when waiting on extra data (anim data for example) before showing the window
        bool mPendingDeletion;  // we are removing this window entirely
        uint32_t mId;


        #define ANIM_TIME_INVALID -1.0f     // Value used to show there is no animation going on

        float mAnimTimeInMS;                // Current time used for the anim
        WindowAnimProperties mWindowBase;   // Client window properties (outside of animation context)
        WindowAnimProperties mAnimCurrent;  // Current properties to use for rendering
        WindowAnimProperties mAnimSource;   // Source values to use during animation
        AnimPropertySet mAnimDest;          // Destination values to use during animation
        AnimPropertySet mOpenAnimRef;       // Reference animation data for opening the window
        AnimPropertySet mCloseAnimRef;      // Reference animation data for closing the window

        WindowState mWindowState;           // Current window state

		IGOSurface *createDeviceSpecificSurface(RendererType type, void *context);
    };
}
#endif 
