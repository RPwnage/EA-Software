#include <QDir>
#include "OriginCommonUI.h"


static void registerResources();


namespace OriginCommonUI
{
	void initCommonControls()
	{
		registerResources();
	}

	void changeCurrentLang(QString newLang)
	{
                Origin::UIToolkit::OriginUIBase::changeCurrentLang(newLang);
	}

    void initFonts()
    {
        Origin::UIToolkit::OriginUIBase::initFonts();
    }
}


static void registerResources()
{
	// Register the Qt resources for our static lib
	Q_INIT_RESOURCE(origincommoncontrols);
}
