///////////////////////////////////////////////////////////////////////////////
// IGOWindow.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGO.h"
#include "IGOWindow.h"
#define _USE_MATH_DEFINES
#include <math.h>

#include "IGOSurface.h"

#if defined(ORIGIN_PC)

#ifndef _WIN64
#include "DX8Surface.h"
#endif
#include "DX9Surface.h"
#include "DX10Surface.h"
#include "DX11Surface.h"
#include "DX12Surface.h"
#include "MantleSurface.h"

#endif

#include "OGLSurface.h"

#include "IGOIPC/IGOIPC.h"
#include "IGOTrace.h"
#include "IGOLogger.h"

namespace OriginIGO {

// Debug info helpers
static const char* AnimPropertyNames[] = { "POSX", "POSY", "WIDTH", "HEIGHT", "ALPHA", "CUSTOM" };
static_assert(_countof(AnimPropertyNames) == WindowAnimPropIndex_COUNT, "Mismatch anim property names");

//////////////////////////////////////////////////////////////////////////////////////////////////////////

    IGOWindow::IGOWindow(uint32_t id, RendererType type) :
	    mType(type),
	    mSurface(NULL),
		mAlwaysVisible(false),
        mPendingSetup(false),
        mPendingDeletion(false),
		mId(id),
        mAnimTimeInMS(ANIM_TIME_INVALID),
        mWindowState(WindowState_NONE)
    {
        memset(&mWindowBase.props, 0, sizeof(mWindowBase.props));
        memset(&mAnimCurrent.props, 0, sizeof(mAnimCurrent.props));
    }

	void IGOWindow::dealloc(void *context)
	{
		if (mSurface)	// used by Mantle API only - defered delete of actual gfx objects
		{
			mSurface->dealloc(context);
			delete mSurface;
			mSurface = NULL;

            // Reset renderer
            mType = RendererNone;
		}
	}

	IGOWindow::~IGOWindow()
    {
		DELETE_IF(mSurface);	// used by all other gfx API's - immediate delete of actual gfx objects
	}

    void IGOWindow::setRenderer(RendererType type)
    {
        if (mType != type)
        {
            if (mType != RendererNone)
            {
                IGOLogWarn("WARNING: LEAKING DUE TO MISSED RENDERER TRANSITION (%d/%d)", type, mType);
                IGO_ASSERT(0);
            }

            mType = type;
        }
    }

#ifdef ORIGIN_MAC
    bool IGOWindow::checkDirty(IGOApplication *app, bool forceRender)
    {
        if (app && !mPendingSetup && (app->isActive() || mAlwaysVisible || forceRender))
        {
            // do not render our background while we are in pinning mode
            if (app->isPinningInProgress() && mId == BACKGROUND_WINDOW_ID)
                return false;
            
            if (mSurface && mSurface->isValid())
                return true;
        }
        
        return false;
    }
    
    void IGOWindow::renderIGO(void *context, IGOApplication *app, float deltaT)
    {
        // first update our anim system
        updateAnim(app, deltaT);
        
        if (mSurface)
            mSurface->render(context);
    }
    
#else // ORIGIN_PC
    
    bool IGOWindow::checkDirty()
    {
        return false;
    }

	void IGOWindow::renderIGO(void *context, IGOApplication *app, bool forceRender, float deltaT)
    {
        if (app && !mPendingSetup && (app->isActive() || mAlwaysVisible || forceRender))
	    {
            // do not render our background while we are in pinning mode
            if (app->isPinningInProgress() && mId == BACKGROUND_WINDOW_ID)
                return;

            // first update our anim system
            updateAnim(app, deltaT);

		    if (mSurface)
				mSurface->render(context);
            
	    }
    }

	void IGOWindow::unrenderIGO(void *context, IGOApplication *app)
	{
		if (mSurface)
			mSurface->unrender(context);
	}

#endif // ORIGIN_MAC
    
    int IGOWindow::x() const
    { 
        // If we are in the middle of animating the scale, we need to adjust our
        // position depending on the reference
        int x = mAnimCurrent.props.posX;

        if (mAnimTimeInMS != ANIM_TIME_INVALID)
        {
            if (mAnimDest.propCurves.width.durationInMS > 0)
            {
                uint32_t ref = mAnimDest.valueReferences;
                int32_t refValue = mWindowBase.props.width;
                int32_t currValue = mAnimCurrent.props.width;

                // Left is default used by surfaces
                if (ref & WindowAnimOrigin_WIDTH_CENTER)
                    x += (refValue - currValue) / 2;

                else
                if (ref & WindowAnimOrigin_WIDTH_RIGHT)
                    x += refValue - currValue;
            }
        }

        return x;
    }
    
    int IGOWindow::y() const 
    { 
        // If we are in the middle of animating the scale, we need to adjust our
        // position depending on the reference
        int y = mAnimCurrent.props.posY;

        if (mAnimTimeInMS != ANIM_TIME_INVALID)
        {
            if (mAnimDest.propCurves.height.durationInMS > 0)
            {
                uint32_t ref = mAnimDest.valueReferences;
                int32_t refValue = mWindowBase.props.height;
                int32_t currValue = mAnimCurrent.props.height;

                // Top is default used by surfaces
                if (ref & WindowAnimOrigin_HEIGHT_CENTER)
                    y += (refValue - currValue) / 2;

                else
                if (ref & WindowAnimOrigin_HEIGHT_BOTTOM)
                    y += refValue - currValue;
            }
        }

        return y;
    }

    void IGOWindow::resize(uint32_t w, uint32_t h) 
    { 
        stopAnim();

        mAnimCurrent.props.width = mWindowBase.props.width = w; 
        mAnimCurrent.props.height = mWindowBase.props.height = h; 
    }

    void IGOWindow::setPos(int32_t x, int32_t y) 
    { 
        stopAnim();

        mAnimCurrent.props.posX = mWindowBase.props.posX = x; 
        mAnimCurrent.props.posY = mWindowBase.props.posY = y; 
    }

    void IGOWindow::prepareForDeletion()
    {
#ifdef SHOW_ANIM_ACTIVITIES
        IGOLogWarn("Preparing for deletion, id=%d, state=%d", mId, mWindowState);
#endif
        mPendingDeletion = true;
        if (mWindowState == WindowState_OPENED || mWindowState == WindowState_OPENING)
            startCloseAnim();
    }

    bool IGOWindow::readyForDeletion()
    {
        bool result = mWindowState == WindowState_CLOSED || mWindowState == WindowState_NONE;
#ifdef SHOW_ANIM_ACTIVITIES
        if (result)
            IGOLogWarn("Ready for deletion, id=%d", mId);
#endif
        return result;
    }

    // Animation-related stuff

    void IGOWindow::setAnimProps(WindowAnimContext ctxt, const AnimPropertySet* props)
    {
        switch (ctxt)
        {
        case WindowAnimContext_OPEN:
#ifdef SHOW_ANIM_ACTIVITIES
            IGOLogWarn("Set openAnim for ID=%d", mId);
//            dumpAnimData(NULL, *props);
#endif
            mOpenAnimRef = *props;
            break;

        case WindowAnimContext_CLOSE:
#ifdef SHOW_ANIM_ACTIVITIES
            IGOLogWarn("Set closeAnim for ID=%d", mId);
//            dumpAnimData(NULL, *props);
#endif
            mCloseAnimRef = *props;
                
            if (mWindowState == WindowState_NONE)
            {
                mPendingSetup = false;
                mWindowState = WindowState_CLOSED;  // at this point, we assume the window is going to animation/have both an open and a close anim

                // Make sure we are in the stop mode (hidden) - this helps to avoid flickering when coming back from losing focus
                forceCloseAnim();
            }
            break;

        default:
            IGOLogWarn("Received animation data with no supported context (%d)", ctxt);
        }
    }

    void IGOWindow::startOpenAnim()
    {
        if (!mAlwaysVisible && mWindowState != WindowState_OPENING && mWindowState != WindowState_OPENED)
        {
            // Do we have anything to play?
            for (int idx = 0; idx < WindowAnimPropIndex_COUNT; ++idx)
            {
                if (mOpenAnimRef.curves[idx].durationInMS > 0.0f)
                {
#ifdef SHOW_ANIM_ACTIVITIES
                    IGOLogWarn("Starting openAnim for ID=%d", mId);
#endif
                    // Cancel any running animation
                    stopAnim();

                    // We are opening the window
                    mWindowState = WindowState_OPENING;
                    mAnimTimeInMS = 0.0f;

#ifdef SHOW_ANIM_ACTIVITIES
//                    dumpAnimData("mWindowBase", mWindowBase);
//                    dumpAnimData("mAnimCurrent", mAnimCurrent);
#endif
                    mAnimCurrent = mWindowBase;                                 // Restore the static actual window properties (as seen from client)

                    mAnimDest = mOpenAnimRef;                                   // We always look at the 'dest' to know about the blending curves to apply
                    mAnimDest.props = mWindowBase.props;                        // But restore the dynamic actual window properties (used during the animation)

                    resolveAnimProps(mOpenAnimRef, &mAnimSource.props);         // Compute the source properties to animate (take into account their 'origin/reference')

#ifdef SHOW_ANIM_ACTIVITIES
//                    dumpAnimData("mAnimSource", mAnimSource);
//                    dumpAnimData("mAnimDest", mAnimDest);
#endif
                    break;
                }
            }
        }
    }

    void IGOWindow::startCloseAnim()
    {
        if ((!mAlwaysVisible || mPendingDeletion) && mWindowState != WindowState_CLOSING && mWindowState != WindowState_CLOSED)
        {
            // Do we have anything to play?
            for (int idx = 0; idx < WindowAnimPropIndex_COUNT; ++idx)
            {
                if (mCloseAnimRef.curves[idx].durationInMS > 0.0f)
                {
#ifdef SHOW_ANIM_ACTIVITIES
                    IGOLogWarn("Starting closeAnim for ID=%d", mId);
#endif
                    // Cancel any running animation
                    stopAnim();

                    // We are closing the window
                    mWindowState = WindowState_CLOSING;
                    mAnimTimeInMS = 0.0f;

#ifdef SHOW_ANIM_ACTIVITIES
//                    dumpAnimData("mWindowBase", mWindowBase);
//                    dumpAnimData("mAnimCurrent", mAnimCurrent);
#endif

                    mAnimSource = mAnimCurrent;                         // We are starting from our current state
                    mAnimDest = mCloseAnimRef;                          // We always look at the 'dest' to know about the blending curves to apply
                    resolveAnimProps(mCloseAnimRef, &mAnimDest.props);  // Compute actual dest properties (take into account their 'origin/reference')

#ifdef SHOW_ANIM_ACTIVITIES
//                    dumpAnimData("mAnimSource", mAnimSource);
//                    dumpAnimData("mAnimDest", mAnimDest);
#endif
                    break;
                }
            }
        }
    }

    void IGOWindow::forceCloseAnim()
    {
        // Force the window to its close state if it does support animation
        if (mWindowState != WindowState_NONE && mWindowState != WindowState_CLOSED)
        {
#ifdef SHOW_ANIM_ACTIVITIES
            IGOLogWarn("Forcing closeAnim for ID=%d", mId);
#endif
            startCloseAnim();
            stopAnim();
        }
    }

    void IGOWindow::stopAnim()
    {
        if (mAnimTimeInMS != ANIM_TIME_INVALID)
        {
#ifdef SHOW_ANIM_ACTIVITIES
            IGOLogWarn("Stopping animation for ID=%d", mId);
#endif
            updateAnim(NULL, 1000000.0f);
        }
    }

    bool IGOWindow::animRunning() const
    {
        return mAnimTimeInMS != ANIM_TIME_INVALID;
    }

    void IGOWindow::updateAnim(IGOApplication* app, float deltaT)
    {
        if (mAnimTimeInMS == ANIM_TIME_INVALID)
            return;

        // Scale factor - really for debugging
        float scale = app ? app->animDurationScale() : 1.0f;

        bool animDone = true;
        mAnimTimeInMS += deltaT;
        for (int idx = 0; idx < WindowAnimPropIndex_COUNT; ++idx)
        {
            // Is it an animated layer?
            float durationInMS = mAnimDest.curves[idx].durationInMS * scale;
            if (durationInMS > 0)
            {
                // Is the animation done?
                if (mAnimTimeInMS >= durationInMS)
                {
                    mAnimDest.curves[idx].durationInMS = 0;

                    // We were supposed to get to mWindowBase, so use it directly in case it was modified
                    // during the animation 1) not a good solution 2) should really never/very rarely happen 3) would cause a snap, yes
                    if (mWindowState == WindowState_OPENING)
                        mAnimCurrent.values[idx] = mWindowBase.values[idx];

                    else
                        mAnimCurrent.values[idx] = mAnimDest.values[idx];

#ifdef SHOW_ANIM_ACTIVITIES
                    IGOLogWarn("Anim done for ID=%d, %s=%d", mId, AnimPropertyNames[idx], mAnimCurrent.values[idx]);
#endif
                }

                else
                {
                    // Well, the animation is still going..
                    animDone = false;

                    // Resolve our current advancement along the anim curve path
                    float adv = mAnimTimeInMS / durationInMS; // AnimCurve_LINEAR
                    if (mAnimDest.curves[idx].type == WindowAnimBlendFcn_EASE_IN_OUT)
                    {
                        adv = static_cast<float>(sin(adv * M_PI * 0.5f));
                    }

                    else
                    if (mAnimDest.curves[idx].type == WindowAnimBlendFcn_CUBIC_BEZIER)
                    {
                        float t = adv;
                        float t2 = adv * adv;
                        float oneMinusT = 1.0f - adv;
                        float oneMinusT2 = oneMinusT * oneMinusT;

                        adv = (mAnimDest.curves[idx].optionalA * oneMinusT * oneMinusT2)
                            + (mAnimDest.curves[idx].optionalB * 3.0f * t * oneMinusT2)
                            + (mAnimDest.curves[idx].optionalC * 3.0f * t2 * oneMinusT)
                            + (mAnimDest.curves[idx].optionalD * t * t2);
                    }

                    mAnimCurrent.values[idx] = (int32_t)((mAnimDest.values[idx] - mAnimSource.values[idx]) * adv) + mAnimSource.values[idx];
                }
            } // durationInMS > 0
        } // loop ParamIndex_COUNT
        
        // Cleanup
        if (animDone)
        {
            if (mWindowState == WindowState_OPENING)
                mWindowState = WindowState_OPENED;

            else
            if (mWindowState == WindowState_CLOSING)
                mWindowState = WindowState_CLOSED;

            mAnimTimeInMS = ANIM_TIME_INVALID;
        }
    }

    void IGOWindow::resolveAnimProps(const AnimPropertySet& src, WindowAnimExplicitProperties* dest)
    {
        if (!dest)
            return;

        // Compute absolute values from potentially relative once
        *dest = src.props;

        int32_t width = (int32_t)SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenWidth);
        int32_t height = (int32_t)SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenHeight);

        if (src.valueReferences & WindowAnimOrigin_POS_X_RIGHT)
            dest->posX = width - src.props.posX;

        if (src.valueReferences & WindowAnimOrigin_POS_X_CENTER)
            dest->posX = width / 2 + src.props.posX;

        if (src.valueReferences & WindowAnimOrigin_POS_Y_BOTTOM)
            dest->posY = height - src.props.posY;

        if (src.valueReferences & WindowAnimOrigin_POS_Y_CENTER)
            dest->posY = height / 2 + src.props.posY;
    }

    void IGOWindow::dumpAnimData(const char* title, const AnimPropertySet& props)
    {
        IGOLogWarn("AnimPropertySet: %s", title ? title : "");
        IGOLogWarn("ValueReferences=0x%08x", props.valueReferences);
        for (int idx = 0; idx < WindowAnimPropIndex_COUNT; ++idx)
        {
            IGOLogWarn("%s=%d, curve: type=%d, durationInMS=%f, optA=%f, optB=%f, optC=%f, optD=%f"
                , AnimPropertyNames[idx], props.values[idx]
                , props.curves[idx].type, props.curves[idx].durationInMS
                , props.curves[idx].optionalA, props.curves[idx].optionalB, props.curves[idx].optionalC, props.curves[idx].optionalD);
        }
    }

    void IGOWindow::dumpAnimData(const char* title, const WindowAnimProperties& props)
    {
        IGOLogWarn("WindowAnimProperties: %s", title ? title : "");
        for (int idx = 0; idx < WindowAnimPropIndex_COUNT; ++idx)
        {
            IGOLogWarn("%s=%d", AnimPropertyNames[idx], props.values[idx]);
        }
    }

    //

	void IGOWindow::dumpBitmap(void *context, IGOApplication *app)
    {
        if (app && mSurface)
			mSurface->dumpBitmap(context);
    }
    
    void IGOWindow::dumpInfo(char* buffer, size_t bufferSize)
    {
#ifdef ORIGIN_PC
        // At a minimum dump the IGOWindow stuff - we can always add info we need from the surfaces later
        if (!buffer || bufferSize == 0)
            return;
        
        _snprintf(buffer, bufferSize, "id=%d, x=%d, y=%d, width=%d, height=%d, alwaysVisible=%d"
                    , id(), x(), y(), width(), height(), isAlwaysVisible() ? 1 : 0);
        
        buffer[bufferSize - 1] = '\0';
#else // ORIGIN_MAC
        if (mSurface)
            mSurface->dumpInfo(buffer, bufferSize);
            
        else
        {
            snprintf(buffer, bufferSize, "No surface defined");
            if (buffer && bufferSize > 0)
                buffer[0] = '\0';
        }
#endif
        }
    
    void IGOWindow::update(void *device, bool visible, uint8_t alpha, int32_t x, int32_t y, uint32_t width, uint32_t height, const char *data, int size, uint32_t updateFlags)
    {
#ifdef SHOW_ANIM_ACTIVITIES
        IGOLogWarn("Update ID=%d, visible=%d, alpha=%d, x=%d, y=%d, width=%d, height=%d, size=%d, flags=0x%08x", mId, visible, alpha, x, y, width, height, size, updateFlags);
#endif
        // Are we waiting for extra data (anim data for example) for being able to display this guy?
        if (updateFlags&IGOIPC::WINDOW_UPDATE_REQUIRE_ANIMS)
            mPendingSetup = mWindowState == WindowState_NONE;

   	    if (updateFlags&IGOIPC::WINDOW_UPDATE_X)
	    {
            mWindowBase.props.posX = x;
            if (mAnimTimeInMS == ANIM_TIME_INVALID && !mPendingSetup && mWindowState != WindowState_CLOSED)
		        mAnimCurrent.props.posX = x;
        }

	    if (updateFlags&IGOIPC::WINDOW_UPDATE_Y)
        {
            mWindowBase.props.posY = y;
            if (mAnimTimeInMS == ANIM_TIME_INVALID && !mPendingSetup && mWindowState != WindowState_CLOSED)
		        mAnimCurrent.props.posY = y;
	    }
        
	    if (updateFlags&IGOIPC::WINDOW_UPDATE_WIDTH)
        {
            mWindowBase.props.width = width;
            if (mAnimTimeInMS == ANIM_TIME_INVALID && !mPendingSetup && mWindowState != WindowState_CLOSED)
		        mAnimCurrent.props.width = width;
    }

	    if (updateFlags&IGOIPC::WINDOW_UPDATE_HEIGHT)
    {
            mWindowBase.props.height = height;
            if (mAnimTimeInMS == ANIM_TIME_INVALID && !mPendingSetup && mWindowState != WindowState_CLOSED)
		        mAnimCurrent.props.height = height;
    }

        if (updateFlags&IGOIPC::WINDOW_UPDATE_ALPHA)
    {
            mWindowBase.props.alpha = alpha;
            if (mAnimTimeInMS == ANIM_TIME_INVALID && !mPendingSetup && mWindowState != WindowState_CLOSED)
                mAnimCurrent.props.alpha = alpha;
        }

        if (updateFlags&IGOIPC::WINDOW_UPDATE_ALWAYS_VISIBLE)
		    mAlwaysVisible = visible;

	    if (!mSurface)
		    mSurface = createDeviceSpecificSurface(mType, device);

	    IGO_ASSERT(mSurface != NULL);
	    if (mSurface)
        {
            mSurface->update(device, alpha, width, height, data, size, updateFlags);

            // If IGO is open, the window is in its closed state and received data update,
            // we need to open the window
            if (mWindowState == WindowState_CLOSED 
                && data && size > 0 && (updateFlags & IGOIPC::WINDOW_UPDATE_BITS)
                && SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive))
                startOpenAnim();
        }
    }

    void IGOWindow::updateAnimValue(enum WindowAnimPropIndex propIndex, int32_t value)
    {
        if (propIndex >= WindowAnimPropIndex_POSX && propIndex < WindowAnimPropIndex_COUNT)
        {
            mWindowBase.values[propIndex] = value;
            if (mAnimTimeInMS == ANIM_TIME_INVALID && !mPendingSetup && mWindowState != WindowState_CLOSED)
                mAnimCurrent.values[propIndex] = value;
        }
    }

    void IGOWindow::updateRects(void *device, const eastl::vector<IGORECT> &rects, const char *data, int size)
    {
    #if !defined(ORIGIN_MAC)

	    if (!device && mType!=RendererOGL)
		    return;

    #endif

	    IGO_ASSERT(mSurface != NULL);
	    if (mSurface)
        {
            mSurface->updateRects(device, rects, data, size);
        }
    }

    IGOSurface *IGOWindow::createDeviceSpecificSurface(RendererType type, void *pDev)
    {
	    switch (type)
	    {
    #if defined(ORIGIN_PC)

    #ifndef _WIN64
	    case RendererDX8:
		    return new DX8Surface(pDev, this);
    #endif
	    case RendererDX9:
		    return new DX9Surface(pDev, this);

	    case RendererDX10:
		    return new DX10Surface(pDev, this);

		case RendererDX11:
			return new DX11Surface(pDev, this);

   		case RendererDX12:
			return new DX12Surface(pDev, this);

		case RendererMantle:
			return new MantleSurface(pDev, this);

    #endif

	    case RendererOGL:
		    return new OGLSurface(pDev, this);

	    default:
		    return NULL;
	    }
    }
}
