///////////////////////////////////////////////////////////////////////////////
// OriginCommonUIUtils.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef ORIGINCOMMONUIUTILS_H
#define ORIGINCOMMONUIUTILS_H

#include "uitoolkitpluginapi.h"

class QPainter;
class QPixmap;
class QRect;
class QMargins;

namespace OriginCommonUIUtils
{
    UITOOLKIT_PLUGIN_API void DrawImageSlices(QPainter& aPainter, QPixmap& aPixmap, const QRect& aDestRect, const QMargins& aSliceWidths);
    UITOOLKIT_PLUGIN_API bool DrawRoundCorner(QWidget* obj, const int& r, const bool& flatTop = false);
};


#endif // ORIGINCOMMONUIUTILS_H
