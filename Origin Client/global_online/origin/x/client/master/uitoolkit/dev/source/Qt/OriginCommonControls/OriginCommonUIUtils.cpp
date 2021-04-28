///////////////////////////////////////////////////////////////////////////////
// OriginCommonUIUtils.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////


#include <QtWidgets>
#include <QPainter>
#include "OriginCommonUIUtils.h"


void OriginCommonUIUtils::DrawImageSlices(QPainter& aPainter, QPixmap& aPixmap, const QRect& aDestRect, const QMargins& aSliceWidths)
{
    QRect pixmapRect = aPixmap.rect();

    if (aSliceWidths.top() == 0 && aSliceWidths.bottom() == 0 && aSliceWidths.right() == 0 && aSliceWidths.left() == 0)
    {
        // NO SLICES SPECIFIED - just draw the pixmap normally/stretched
        QRect sourceRect(0,0,pixmapRect.width(),pixmapRect.height());
        QRect targetRect(aDestRect);
        aPainter.drawPixmap( targetRect, aPixmap, sourceRect );
    }
    else if (aSliceWidths.top() == 0 && aSliceWidths.bottom() == 0 && (aSliceWidths.right() != 0 || aSliceWidths.left() != 0))
    {
        // HORIZONTAL SLICING ONLY

        bool clipping = false;
        if (aPixmap.width() > aDestRect.width())
        {
            aPainter.setClipRect(aDestRect);
            aPainter.setClipping(true);
            clipping = true;
        }

        // Draw the left slice
        {
            QRect sourceRect(0,0,aSliceWidths.left(),pixmapRect.height());
            QRect targetRect(aDestRect.x(),aDestRect.y(),aSliceWidths.left(),aDestRect.height());
            aPainter.drawPixmap( targetRect, aPixmap, sourceRect );
        }
        // Draw the right slice
        if (aDestRect.width() > aSliceWidths.left())
        {
            QRect sourceRect(pixmapRect.width()-aSliceWidths.right(),0,aSliceWidths.right(),pixmapRect.height());
            QRect targetRect(aDestRect.x() + (aDestRect.width()-aSliceWidths.right()),aDestRect.y(),aSliceWidths.right(),aDestRect.height());
            aPainter.drawPixmap( targetRect, aPixmap, sourceRect );
        }
        // Draw the center slice
        if (aDestRect.width() > aSliceWidths.left())
        {
            int leftRightSliceSize = aSliceWidths.left() + aSliceWidths.right();
            QRect sourceRect(aSliceWidths.left(),0,pixmapRect.width()-leftRightSliceSize,pixmapRect.height());
            QRect targetRect(aDestRect.x() + aSliceWidths.left(),aDestRect.y(),aDestRect.width()-leftRightSliceSize,aDestRect.height());
            aPainter.drawPixmap( targetRect, aPixmap, sourceRect );
        }

        if (clipping)
        {
            aPainter.setClipping(false);
        }
    }
    //else if (aSliceWidths.left() == 0 && aSliceWidths.right() == 0 && (aSliceWidths.top() != 0 || aSliceWidths.bottom() != 0))
    //{
        // VERTICAL SLICING ONLY
        // TODO!!
    //}
    //else
    //{
        // FULL 9-SLICE IMAGE SLICING
        // TODO!!
    //}

}

bool OriginCommonUIUtils::DrawRoundCorner(QWidget* obj, const int& r, const bool& flatTop)
{
    // use a radius value deal with the mask
    QRect rect = QRect(0, 0, obj->width(), obj->height());
    QRegion region; 

    // middle and borders 
    region += rect.adjusted(r, 0, -r, 0); 
    region += rect.adjusted(0, r, 0, -r); 

    // top left 
    QRect corner(rect.topLeft(), QSize(r*2, r*2));
    region += QRegion(corner, flatTop ? QRegion::Rectangle:QRegion::Ellipse); 

    // top right 
    corner.moveTopRight(rect.topRight()); 
    region += QRegion(corner, flatTop ? QRegion::Rectangle:QRegion::Ellipse); 

    // bottom left 
    corner.moveBottomLeft(rect.bottomLeft()); 
    region += QRegion(corner, QRegion::Ellipse); 

    // bottom right 
    corner.moveBottomRight(rect.bottomRight()); 
    region += QRegion(corner, QRegion::Ellipse); 

    obj->setMask(region);

    return true;
}



