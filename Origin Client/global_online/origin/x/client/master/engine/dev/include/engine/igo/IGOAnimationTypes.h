///////////////////////////////////////////////////////////////////////////////
// IGOAnimationTypes.h
// 
// Created by Frederic Meraud
// Copyright (c) 2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGO_ANIMATION_TYPES_DEFINED
#define IGO_ANIMATION_TYPES_DEFINED

// This file is to be included under the proper namespace on the engine and IGO sides.

#pragma pack(push)
#pragma pack(1)

// Defines the reference to use when applying an animation (from the right side, scale from the center instead of the top left, etc...)
enum WindowAnimOrigin
{
    WindowAnimOrigin_DEFAULT       = 0x0000,

    WindowAnimOrigin_POS_X_RIGHT   = 0x0001,
    WindowAnimOrigin_POS_X_CENTER  = 0x0002,

    WindowAnimOrigin_POS_Y_BOTTOM  = 0x0010,
    WindowAnimOrigin_POS_Y_CENTER  = 0x0020,

    WindowAnimOrigin_WIDTH_RIGHT   = 0x0100,
    WindowAnimOrigin_WIDTH_CENTER  = 0x0200,

    WindowAnimOrigin_HEIGHT_BOTTOM = 0x1000,
    WindowAnimOrigin_HEIGHT_CENTER = 0x2000,

    WindowAnimOrigin_POS_X_MASK    = 0x000f,
    WindowAnimOrigin_POS_Y_MASK    = 0x00f0,
    WindowAnimOrigin_WIDTH_MASK    = 0x0f00,
    WindowAnimOrigin_HEIGHT_MASK   = 0xf000
};

// Supported blend functions
enum WindowAnimBlendFcn
{
    WindowAnimBlendFcn_LINEAR = 0,
    WindowAnimBlendFcn_EASE_IN_OUT,
    WindowAnimBlendFcn_CUBIC_BEZIER
};

// Animation purpose - at the moment, we only support open/close animations for windows (including the desktop static windows)
enum WindowAnimContext
{
    WindowAnimContext_OPEN = 0,
    WindowAnimContext_CLOSE
};

// Src -> Dest blend parameters for one specific value (posX, posY, width, ...)
struct WindowAnimBlendParams
{
    WindowAnimBlendFcn type;
    float durationInMS;     // we use this to detect whether there is an animation to play!
    float optionalA;        // 4 potential params because of Cubic bezier blending
    float optionalB;
    float optionalC;
    float optionalD;
};

// Supported properties for animation
enum WindowAnimPropIndex
{
    WindowAnimPropIndex_POSX = 0,
    WindowAnimPropIndex_POSY,
    WindowAnimPropIndex_WIDTH,
    WindowAnimPropIndex_HEIGHT,
    WindowAnimPropIndex_ALPHA,
    WindowAnimPropIndex_CUSTOM_VALUE,

    WindowAnimPropIndex_COUNT
};

// Set of properties we can animate
struct WindowAnimExplicitProperties
{
    int32_t posX;
    int32_t posY;
    int32_t width;
    int32_t height;
    int32_t alpha;
    int32_t customValue;
};

// Helper to allow indexes or direct access
union WindowAnimProperties
{
    int32_t values[WindowAnimPropIndex_COUNT];
    WindowAnimExplicitProperties props;
};

// Set of corresponding blend parameters
struct WindowAnimBlendExplicitProperties
{
    WindowAnimBlendParams posX;
    WindowAnimBlendParams posY;
    WindowAnimBlendParams width;
    WindowAnimBlendParams height;
    WindowAnimBlendParams alpha;
    WindowAnimBlendParams customValue;
};

// Helper to allow indexing or direct access
union WindowAnimBlendProperties
{
    WindowAnimBlendParams curves[WindowAnimPropIndex_COUNT]; 
    WindowAnimBlendExplicitProperties propCurves;
};

// Defines the animation for all the supported properties in one set (easier to track/manage)
struct AnimPropertySet
{
    static const uint32_t Version = 1; // please increase when modifying those types

    AnimPropertySet()
    {
        memset(this, 0, sizeof(*this));
    }

    bool empty() const
    {
        for (int idx = 0; idx < WindowAnimPropIndex_COUNT; ++idx)
        {
            if (curves[idx].durationInMS > 0)
                return false;
        }

        return true;
    }


    WindowAnimOrigin valueReferences;

    union                                           // our dest values (animating from current)
    {
        int32_t values[WindowAnimPropIndex_COUNT];
        WindowAnimExplicitProperties props;
    };

    union                                           // associated blend definitions
    {
        WindowAnimBlendParams curves[WindowAnimPropIndex_COUNT]; 
        WindowAnimBlendExplicitProperties propCurves;
    };
};

#pragma pack(pop)

#endif // IGO_ANIMATION_TYPES_DEFINED