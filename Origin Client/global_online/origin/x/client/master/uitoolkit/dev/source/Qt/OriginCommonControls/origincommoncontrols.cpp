#include <QtWidgets>

#include "originbannerplugin.h"
#include "origindividerplugin.h"
#include "originpushbuttonplugin.h"
#include "origincheckbuttonplugin.h"
#include "originradiobuttonplugin.h"
#include "originlabelplugin.h"
#include "originsinglelineeditplugin.h"
#include "origindropdownplugin.h"
#include "originmultilineeditplugin.h"
#include "originspinnerplugin.h"
#include "originclosebuttonplugin.h"
#include "originprogressbarplugin.h"
#include "originwebviewplugin.h"
#include "origincommoncontrols.h"
#include "../../../include/Qt/originuiBase.h"
#include "originvalidationcontainerplugin.h"
#include "origincommandlinkplugin.h"
#include "originiconbuttonplugin.h"
#include "origintooltipplugin.h"
#include "origintransferbarplugin.h"
#include "originsurveywidgetplugin.h"
#include "originsliderplugin.h"


static const char* fontResourcePaths[] =
{
    ":/OriginCommonControls/Fonts/OpenSans-Bold.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-BoldItalic.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-ExtraBold.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-ExtraBoldItalic.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-Italic.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-Light.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-LightItalic.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-Regular.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-Semibold.ttf",
    ":/OriginCommonControls/Fonts/OpenSans-SemiboldItalic.ttf",
    ":/OriginCommonControls/Fonts/ApexNew-Bold.otf",                    // "Apex New Bold"
    ":/OriginCommonControls/Fonts/ApexNew-BoldItalic.otf",              // "Apex New Bold Italic"
    ":/OriginCommonControls/Fonts/ApexNew-Medium.otf",                  // "Apex New Medium"
    ":/OriginCommonControls/Fonts/ApexNew-MediumItalic.otf",            // "Apex New Medium Italic"
    ":/OriginCommonControls/Fonts/DFG-HeiSeiGothic-W5.ttf",             // "DFGHSGothic-W5" (Japanese)
    ":/OriginCommonControls/Fonts/Kdnrm.ttf"                            // "AsiaDNR-M"      (Korean)
    ":/OriginCommonControls/Fonts/KlavikaBasic-Bold.otf",               // "Klavika Basic Bold"
    ":/OriginCommonControls/Fonts/KlavikaBasic-BoldItalic.otf",         // "Klavika Basic Bold Italic"
    ":/OriginCommonControls/Fonts/KlavikaBasic-Light.otf",              // "Klavika Basic Light"
    ":/OriginCommonControls/Fonts/KlavikaBasic-LightItalic.otf",        // "Klavika Basic Light Italic"
    ":/OriginCommonControls/Fonts/KlavikaBasic-Medium.otf",             // "Klavika Basic Medium"
    ":/OriginCommonControls/Fonts/KlavikaBasic-MediumItalic.otf",       // "Klavika Basic Medium Italic"
    ":/OriginCommonControls/Fonts/KlavikaBasic-Regular.otf",            // "Klavika Basic Regular"
    ":/OriginCommonControls/Fonts/KlavikaBasic-RegularItalic.otf",      // "Klavika Basic Regular Italic"
};

const int kFontResourceCount = sizeof(fontResourcePaths)/sizeof(fontResourcePaths[0]);

void OriginCommonControls::initFonts()
{
    for(int i = 0; i < kFontResourceCount; i++)
    {
        int result = QFontDatabase::addApplicationFont(fontResourcePaths[i]);
        Q_UNUSED(result);
    }
}


OriginCommonControls::OriginCommonControls(QObject *parent)
    : QObject(parent)
{
    initFonts();

    m_widgets.append(new OriginBannerPlugin(this));
    m_widgets.append(new OriginDividerPlugin(this));
    m_widgets.append(new OriginPushButtonPlugin(this));
    m_widgets.append(new OriginCommandLinkPlugin(this));
    m_widgets.append(new OriginIconButtonPlugin(this));
    m_widgets.append(new OriginCheckButtonPlugin(this));
    m_widgets.append(new OriginRadioButtonPlugin(this));
    m_widgets.append(new OriginLabelPlugin(this));
    m_widgets.append(new OriginSingleLineEditPlugin(this));
    m_widgets.append(new OriginDropdownPlugin(this));
    m_widgets.append(new OriginMultiLineEditPlugin(this));
    m_widgets.append(new OriginSpinnerPlugin(this));
    m_widgets.append(new OriginCloseButtonPlugin(this));
    m_widgets.append(new OriginProgressBarPlugin(this));
    m_widgets.append(new OriginTransferBarPlugin(this));
    m_widgets.append(new OriginWebViewPlugin(this));
    m_widgets.append(new OriginValidationContainerPlugin(this));
    m_widgets.append(new OriginSurveyWidgetPlugin(this));
    m_widgets.append(new OriginTooltipPlugin(this));
    m_widgets.append(new OriginSliderPlugin(this));

    Origin::UIToolkit::OriginUIBase::changeCurrentLang("en_US");
}

QList<QDesignerCustomWidgetInterface*> OriginCommonControls::customWidgets() const
{
    return m_widgets;
}
// Thomas: Not sure if the file name is correct for Qt5, I used this as a referecne to convert it to Qt5 http://qt.gitorious.org/qt/qtbase/commit/963b4c1647299fd023ddbe7c4a25ac404e303c5d
Q_PLUGIN_METADATA(IID QDesignerCustomWidgetCollectionInterface_iid FILE "OriginCommonControls.cpp")
