//
//  MacNativeSupport.h
//  OriginCrashReporter
//
//  Created by Frederic Meraud on 4/3/13.
//  Copyright (c) 2013 Electronic Arts. All rights reserved.
//

#ifndef __OriginCrashReporter__MacNativeSupport__
#define __OriginCrashReporter__MacNativeSupport__

#ifdef ORIGIN_MAC

class QWidget;

void setOpacity(QWidget* window, float alpha);
void hideTitlebarDecorations(QWidget* window);
void forceFocusOnWindow(QWidget* window);

#endif

#endif /* defined(__OriginCrashReporter__MacNativeSupport__) */
