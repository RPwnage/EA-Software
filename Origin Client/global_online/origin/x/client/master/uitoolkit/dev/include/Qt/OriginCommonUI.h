
#include "uitoolkitpluginapi.h"
#include "origincallout.h"
#include "originbanner.h"
#include "origindivider.h"
#include "originpushbutton.h"
#include "origincheckbutton.h"
#include "originradiobutton.h"
#include "originsinglelineedit.h"
#include "originmsgbox.h"
#include "originmultilineedit.h"
#include "originlabel.h"
#include "origindropdown.h"
#include "originspinner.h"
#include "originslider.h"
#include "originclosebutton.h"
#include "originprogressbar.h"
#include "originwebview.h"
#include "origindialog.h"
#include "originwindow.h"
#include "originwindowmanager.h"
#include "originchromebase.h"
#include "origintooltip.h"
#include "origintransferbar.h"
#include "originiconbutton.h"
#include "originsurveywidget.h"
#include "originscrollablemsgbox.h"
namespace OriginCommonUI
{
	UITOOLKIT_PLUGIN_API void initCommonControls();
	UITOOLKIT_PLUGIN_API void changeCurrentLang(QString newLang);
    UITOOLKIT_PLUGIN_API void initFonts();
}
