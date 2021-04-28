/////////////////////////////////////////////////////////////////////////////
// InstallerView.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "InstallerView.h"
#include "origindropdown.h"
#include <QJsonObject>
#include "engine/content/ContentOperationQueueController.h"
#ifdef ORIGIN_PC
#include "rtf2html/rtf2html.h"
#endif
#include "services/downloader/FilePath.h"
#include "services/downloader/StringHelpers.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include "DialogController.h"

namespace Origin
{
namespace Client
{
const QString InstallerView::DownloadInfoContent::KEY_DESKTOPSHORTCUT = "desktopshortcut";
const QString InstallerView::DownloadInfoContent::KEY_STARTMENUSHORTCUT = "startmenushortcut";

InstallerView::InstallerView(QObject* parent)
: QObject(parent)
{

}


InstallerView::~InstallerView()
{

}


QWidget* InstallerView::languageSelectionContent(const Origin::Downloader::InstallLanguageRequest& input, const QString& initLocale)
{
    UIToolkit::OriginDropdown* langSelector = new UIToolkit::OriginDropdown();
    int englishLangIdx = -1;
    bool clientLangFound = false;
    QString currentLocale = QLocale().name();

    if (currentLocale == "nb_NO")
        currentLocale = "no_NO";

    for(int i = 0; i < input.availableLanguages.count(); i++)
    {    
        // We default the combobox language to English by default if client locale not found
        // so track the index of an available English language
        if (input.availableLanguages[i].startsWith("en_") == true)
        {
            englishLangIdx = i;
        }

        QString itemText = tr(QString("ebisu_client_language_"+input.availableLanguages[i].toLower()).toStdString().c_str());
        // The additionalDownloadRequiredLanguages is only be populated by the install flow
        // if we are installing via ITO so this is basically a no-op for other install mechanisms.
        if(input.additionalDownloadRequiredLanguages.contains(input.availableLanguages[i]))
        {
            itemText.append(" - ");
            itemText.append(tr("ebisu_client_additional_download_required"));
        }

        langSelector->addItem(itemText, input.availableLanguages[i]);
        if(initLocale.count())
        {
            if(initLocale.compare(input.availableLanguages[i], Qt::CaseInsensitive) == 0)
            {
                langSelector->setCurrentIndex(i);
                clientLangFound = true;
            }
        }
        else if(currentLocale.compare(input.availableLanguages[i], Qt::CaseInsensitive) == 0)
        {
            langSelector->setCurrentIndex(i);
            clientLangFound = true;
        }
    }

    if(!clientLangFound && englishLangIdx != -1)
    {
        langSelector->setCurrentIndex(englishLangIdx);
    }

    return langSelector;
}


QJsonObject localeItem(const QString& id, const QString& text)
{
    QJsonObject obj;
    obj["name"] = text;
    obj["id"] = id;
    return obj;
}


QJsonObject InstallerView::formatLanguageSelection(const Origin::Downloader::InstallLanguageRequest& input, const QString& initLocale)
{
    const QString KEY_CURRENTINDEX = "currentindex";
    const QString KEY_LOCALES = "locales";
    QJsonArray locales;
    QJsonObject returnObj;
    int currentIndex = 0;
    int englishLangIdx = -1;
    bool clientLangFound = false;

    for(int i = 0; i < input.availableLanguages.count(); i++)
    {    
        // We default the combobox language to English by default if client locale not found
        // so track the index of an available English language
        if (input.availableLanguages[i].startsWith("en_") == true)
        {
            englishLangIdx = i;
        }

        QString itemText = tr(QString("ebisu_client_language_"+input.availableLanguages[i].toLower()).toStdString().c_str());
        // The additionalDownloadRequiredLanguages is only be populated by the install flow
        // if we are installing via ITO so this is basically a no-op for other install mechanisms.
        if(input.additionalDownloadRequiredLanguages.contains(input.availableLanguages[i]))
        {
            itemText.append(" - ");
            itemText.append(tr("ebisu_client_additional_download_required"));
        }

        locales.append(localeItem(input.availableLanguages[i], itemText));
        if(initLocale.count())
        {
            if(initLocale.compare(input.availableLanguages[i], Qt::CaseInsensitive) == 0)
            {
                currentIndex = i;
                clientLangFound = true;
            }
        }
        else if(QLocale().name().compare(input.availableLanguages[i], Qt::CaseInsensitive) == 0)
        {
            currentIndex = i;
            clientLangFound = true;
        }
    }

    if(!clientLangFound && englishLangIdx != -1)
    {
        currentIndex = englishLangIdx;
    }

    returnObj[KEY_CURRENTINDEX] = currentIndex;
    returnObj[KEY_LOCALES] = locales;

    return returnObj;
}


QJsonObject InstallerView::checkObject(const QString& id, const QString& text, const QString& tooltip, const bool& checked)
{
    QJsonObject obj;
    obj["name"] = text;
    obj["tooltip"] = tooltip;
    obj["checked"] = checked;
    obj["id"] = id;
    return obj;
}


InstallerView::DownloadInfoContent::DownloadInfoContent(const Origin::Downloader::InstallArgumentRequest& request)
{
    checkBoxContent.append(InstallerView::checkObject(KEY_DESKTOPSHORTCUT, tr("ebisu_client_ite_create_desktop_shortcut"), "", Services::readSetting(Services::SETTING_CREATEDESKTOPSHORTCUT, Services::Session::SessionService::currentSession())));
#ifdef ORIGIN_PC
    checkBoxContent.append(InstallerView::checkObject(KEY_STARTMENUSHORTCUT, tr("ebisu_client_ite_create_start_menu_shortcut"), "", Services::readSetting(Services::SETTING_CREATESTARTMENUSHORTCUT, Services::Session::SessionService::currentSession())));
#endif

    bool extraCheckBoxesChecked = true;
    // OFM-9537: If they are Canadian, checkboxes should be unchecked
    if(Services::readSetting(Services::SETTING_GeoCountry).toString().compare("CA", Qt::CaseInsensitive) == 0)
    {
        extraCheckBoxesChecked = false;
    }

    for(int i = 0; i < request.optionalEulas.listOfEulas.size(); i++)
    {
        checkBoxContent.append(checkObject("chkb"+QString::number(i), request.optionalEulas.listOfEulas.at(i).InstallName, request.optionalEulas.listOfEulas.at(i).ToolTip, extraCheckBoxesChecked));
    }

    // must protect against user logging out which results in a null queueController
    Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
    if(queueController && queueController->entitlementsQueued().length())
    {
        title = tr("ebisu_client_add_to_download_queue_uppercase");
#ifdef ORIGIN_PC
        text = tr("ebisu_client_preparing_to_add_to_download_queue").arg(request.contentDisplayName).arg(Downloader::CFilePath(request.installPath).GetVolume() + ":");
#else
        text = tr("ebisu_client_preparing_to_add_to_download_queue_mac_no_drive").arg(request.contentDisplayName);
#endif
    }
    else if(request.isPreload)
    {
        title = tr("ebisu_client_preload_uppercase");
#ifdef ORIGIN_PC
        text = tr("ebisu_client_preparing_preload").arg(request.contentDisplayName).arg(Downloader::CFilePath(request.installPath).GetVolume() + ":");
#else
        text = tr("ebisu_client_preparing_preload_mac_no_drive").arg(request.contentDisplayName);
#endif
    }
    else
    {
        title = tr("ebisu_client_download_uppercase");
#ifdef ORIGIN_PC
        text = tr("ebisu_client_preparing_download").arg(request.contentDisplayName).arg(Downloader::CFilePath(request.installPath).GetVolume() + ":");
#else
        text = tr("ebisu_client_preparing_download_mac_no_drive").arg(request.contentDisplayName);
#endif
    }
}


QJsonObject InstallerView::eulaObject(Downloader::Eula& eula, const int& index, const int& count)
{
    QString title = "";

    // If the eula name is only 1 word - most likely not valid
    if (eula.Description.split(" ").count() < 2)
    {
        if (eula.IsRtfFile)
        {
            // Read in our temp title in case the eula.Description isn't valid.
            title = convertRtfFileToHtml(eula.FileName);
        }
        else
        {
            title = readTextFile(eula.FileName);
        }
        const int firstLineBreak = title.indexOf("<br>");
        if (firstLineBreak != -1)
        {
            title = title.left(firstLineBreak);
            title.replace("</p>", " ");
            title.remove(QRegExp("<[^>]*>"));
        }
        title = DialogController::MessageTemplate::toAttributeFriendlyText(title);
    }

    QJsonObject obj;
    obj["name"] = title.count() ? title : eula.Description;
    obj["index"] = QString::number(index);
    return obj;
}


void formatEula(QString& text)
{
    // Let's try to do some assumed formatting... Keeping it to a minimum.
    // Remove the \n to <br> so it formats properly in the web
    text.replace("\n", "<br>");
    // Make it so that the title (first line) is bold and has it's own spacing
    text = text.prepend("<p><b>");
    const int firstLineEnd = text.indexOf("<br>");
    // Remove <br> with </p>
    text = text.remove(firstLineEnd, QString("<br>").count());
    text = text.insert(firstLineEnd, "</b></p>");
}


QString InstallerView::readTextFile(const QString& fileName)
{
    QString text;

    QFile inputFile(fileName);
    if (inputFile.open(QIODevice::ReadOnly))
    {
        const QByteArray blob = inputFile.readAll();
        QTextStream in(blob);

        // Try detecting the codec (ie html or .txt file with http-equiv defined) or BOM) - otherwise use the default (System)
        QTextCodec* defaultCodec = in.codec();
        QTextCodec* codecToUse = QTextCodec::codecForHtml(blob, defaultCodec);
        in.setCodec(codecToUse);

        text = in.readAll();

        // If this isn't an html file, reformat it
        if(Downloader::StringHelpers::containsHtmlTag(text) == false)
        {
            formatEula(text);
        }
    }

    return text;
}


void removeBlock(QString& org, const QString& sectionStart, const QString& sectionEnd)
{
    int startindex;
    int endindex;
    QString str;
    while(org.contains(sectionStart))
    {
        startindex = org.indexOf(sectionStart);
        endindex = org.indexOf(sectionEnd, startindex);
        str = "";
        //for (int i = startindex; i <= endindex; ++i) {
        //    str += org[i];
        //}
        //ORIGIN_LOG_ACTION << "fixing http in eula: " << str;
        org.remove(endindex, 1);
        org.remove(startindex, 1);
    }
}


QString InstallerView::convertRtfFileToHtml(const QString& fileName)
{
    QString text;
#ifdef ORIGIN_PC
    RTF2HTML::Rtf2Html* rtfConverter = new RTF2HTML::Rtf2Html(true /*ignoreBGcolor*/);
    text = rtfConverter->ConvertRtf(fileName);
    delete rtfConverter;
    // Remove the title. It's the file name plus spacing. We don't want it to show up in the eula.
    const int titleStart = text.indexOf("<title>");
    const int titleEnd = text.indexOf("</title>") + QString("</title>").count();
    text = text.remove(titleStart, titleEnd - titleStart);

    QXmlStreamReader xml(text);
    text = "";
    while (!xml.atEnd())
    {
        if (xml.readNext() == QXmlStreamReader::Characters)
        {
            text += xml.text();
        }
    }
    text = text.trimmed();
    removeBlock(text, "<http", ">");
    removeBlock(text, "<mailto", ">");
    formatEula(text);
#endif

    return text;
}

}
}