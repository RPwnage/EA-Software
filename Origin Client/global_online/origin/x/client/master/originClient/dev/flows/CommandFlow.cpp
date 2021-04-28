///////////////////////////////////////////////////////////////////////////////
// CommandFlow.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "CommandFlow.h"
#include "FlowResult.h"
#include "common/source/OriginApplication.h"
#include "utilities/CommandLine.h"
#include "PendingActionDefines.h"
#include "TelemetryAPIDLL.h"

#include "engine/login/LoginController.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"

#include <QDomDocument>

#if defined(ORIGIN_DEBUG)
#define ENABLE_FLOW_TRACE   0
#else
#define ENABLE_FLOW_TRACE   0
#endif

namespace Origin
{
    namespace Client
    {
        using namespace PendingAction;
        
        static CommandFlow* mInstance = NULL;

        void CommandFlow::create()
        {
            ORIGIN_LOG_EVENT << "create CommandFlow";
            ORIGIN_ASSERT(!mInstance);
            if (!mInstance)
            {
                mInstance = new CommandFlow();
            }
        }

        void CommandFlow::destroy()
        {
            ORIGIN_LOG_EVENT << "destroy CommandFlow";
            ORIGIN_ASSERT(mInstance);
            if (mInstance)
            {
                delete mInstance;
                mInstance = NULL;
            }
        }

        CommandFlow::CommandFlow ()
        {
            mCommandLine.clear();
            mCommandLineArgs.clear();
        }

        CommandFlow::~CommandFlow()
        {
            ORIGIN_LOG_EVENT << "destruct CommandFlow";
        }


        CommandFlow* CommandFlow::instance()
        {
            return mInstance;
        }

        void CommandFlow::start()
        {
            ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "CommandLine flow started";
        }

        void CommandFlow::setCommandLine (QString& cmdLine)
        {
            mCommandLine.clear();
            mCommandLineArgs.clear();

            //MY: IE9 mangles the url and removes the %20 and replaces with space which then confuses our parser
            //so we need to restore "%20"
            //do this only for PC since Mac handles the percent encoding properly

            //this is really kludgy but CommandLineToArgv strips out inner quotes (within the argument) as well as the outer quotes
            //so can't call it without breaking non-url protocol commandl lines
            //so first check and see if it's an origin protocol command line
            if (cmdLine.contains ("origin://") || cmdLine.contains ("origin2://"))
            {
#ifndef ORIGIN_MAC
                mCommandLineArgs = fixOriginProtocolCmdLine (cmdLine);
#else
                mCommandLineArgs = cmdLine.split(QRegExp(" "));
#endif
                for (int i = 0; i < mCommandLineArgs.count(); i++)
                {
                    mCommandLine.append (mCommandLineArgs[i]);
                    if (i < mCommandLineArgs.count() - 1)
                    {
                        mCommandLine.append(" ");
                    }
                }
            }
            else
            {
                mCommandLine = cmdLine;
                mCommandLineArgs = cmdLine.split(QRegExp(" "));
            }
        }

        void CommandFlow::setCommandLine (QStringList cmdLineArgs)
        {
            mCommandLine.clear();
            mCommandLineArgs.clear();

            for (int i = 1; i < cmdLineArgs.count(); i++)
            {
                QString argStr = cmdLineArgs[i];

                //MY: IE9 mangles the url and removes the %20 and replaces with space which then confuses our parser
                //so we need to restore "%20"
                if (cmdLineArgs[i].contains ("origin://", Qt::CaseInsensitive) ||
                    cmdLineArgs[i].contains ("origin2://", Qt::CaseInsensitive))
                {
                    argStr.replace (" ", "%20");
                }

                mCommandLine.append (argStr);
                mCommandLineArgs.append (argStr);

                if (i < cmdLineArgs.count() - 1)
                {
                    mCommandLine.append(" ");
                }
            }
        }

        // this version of setCommandLine() checks against the 8-bit arguments passed into the OriginApplication from bootstrapper on startup
        // because the 8-bit launch parameters can have double-quotes which get stripped out by the QApplication call arguments() 
        // if double-quotes are found in the 8-bit launch parameters, they will be inserted into the QString version. (fixes EBIBUGS-27419)
        void CommandFlow::setCommandLine (QStringList cmdLineArgs, int argc, char **argv)
        {
            mCommandLine.clear();
            mCommandLineArgs.clear();

            for (int i = 1; i < cmdLineArgs.count(); i++)
            {
                QString argStr = cmdLineArgs[i];

                //MY: IE9 mangles the url and removes the %20 and replaces with space which then confuses our parser
                //so we need to restore "%20"
                if (cmdLineArgs[i].contains ("origin://", Qt::CaseInsensitive) ||
                    cmdLineArgs[i].contains ("origin2://", Qt::CaseInsensitive))
                {
                    argStr.replace (" ", "%20");

                    // verify against 8-bit arguments 
#if defined(ORIGIN_MAC)
                    if ((i < argc) && ((strncasecmp(argv[i], "origin://", 9) == 0) || (strncasecmp(argv[i], "origin2://", 10) == 0)))
#else                        
                    if ((i < argc) && ((_strnicmp(argv[i], "origin://", 9) == 0) || (_strnicmp(argv[i], "origin2://", 10) == 0)))
#endif
                    {
                        char *arg = argv[i];
                        int src_index = 0;
                        int src_length = strlen(arg);
                        int dst_index = 0;
                        QString alt = argStr;

                        bool use_alt_arg = false;

                        for (src_index = 0; src_index < src_length; src_index++,dst_index++)
                        {
                            if (dst_index == alt.size())    // end of command line arg
                            {
                                if (((src_index + 1) == src_length) && (arg[src_index] == '\"'))
                                {
                                    alt.append(QString("\""));
                                    use_alt_arg = true;
                                }
                                else
                                {   // if we are not at the end of the 8-bit source, then it doesn't match and we need to abort the repair
                                    if (use_alt_arg)
                                    {
                                        ORIGIN_LOG_WARNING << "launch parameter may need repair but end of string didn't match up after a double-quote";
                                    }
                                    use_alt_arg = false;    // no match - abort
                                    break;
                                }
                            }
                            else
                            if ((alt[dst_index] != arg[src_index]) && (arg[src_index] == '\"'))
                            {
                                if (alt[dst_index] == arg[src_index+1])
                                {
                                    use_alt_arg = true;
                                    alt.insert(dst_index, QString("\""));
                                }
                                else
                                {
                                    if (use_alt_arg)
                                    {
                                        ORIGIN_LOG_WARNING << "launch parameter may need repair but something didn't match up after a double-quote";
                                    }
                                    use_alt_arg = false;    // no match - abort
                                    break;
                                }
                            }
                        }

                        if (use_alt_arg)
                        {
                            ORIGIN_LOG_EVENT << "launch parameter repaired: " << alt;

                            argStr = alt;
                        }
                    }
                }

                mCommandLine.append (argStr);
                mCommandLineArgs.append (argStr);

                if (i < cmdLineArgs.count() - 1)
                {
                    mCommandLine.append(" ");
                }
            }
        }


        void CommandFlow::appendToCommandLine (QString cmdArg)
        {
            mCommandLine.append (" " + cmdArg);
            mCommandLineArgs.append(cmdArg);
        }

        void CommandFlow::removeParameter (QString param, const QRegularExpression exp)
        {
            if (mCommandLine.contains(param))
            {
                mCommandLine.replace(exp, "");
            }
        }

        const QString CommandFlow::urlProtocolArgument ()
        {
            if (mCommandLine.isEmpty() || mCommandLineArgs.isEmpty())
                return QString();

            for (int i = 0; i < mCommandLineArgs.size(); i++)
            {
                if (mCommandLineArgs[i].contains ("origin://", Qt::CaseInsensitive) || mCommandLineArgs[i].contains("origin2://", Qt::CaseInsensitive))
                    return mCommandLineArgs[i];
            }
            return QString();
        }

        bool CommandFlow::findArgument (const QString &arg)
        {
            for (int i = 0; i < mCommandLineArgs.size(); i++)
            {
                if (mCommandLineArgs [i].contains (arg, Qt::CaseInsensitive))
                    return true;
            }
            return false;
        }

        // Parses a .rtp file to find the contentIds and contentTitle
        // rtpPath [in] is the path to the .rtp file
        // contentIds [out] is the list of content Ids
        // contentTitle [out] is the title of the content
        bool CommandFlow::parseRtPFile(QString rtpPath, QString &contentIds, QString &contentTitle)
        {
            if (rtpPath.isEmpty())
            {
		        ORIGIN_LOG_ERROR << "RtP launch aborted -- no rtp path found ";
                return false;
            }

            QFile rtpFile(rtpPath);

	        if (!rtpFile.open(QIODevice::ReadOnly|QIODevice::Text))
	        {
		        ORIGIN_LOG_ERROR << "RtP launch aborted -- unable to open rtp file: " << rtpPath;
		        return false;
	        }

	        QDomDocument doc("rtpdoc");
            QString errorMsg;
            int errorLine = 0;
            int errorCol = 0;
	        if (!doc.setContent(&rtpFile, false, &errorMsg, &errorLine, &errorCol))
	        {
		        ORIGIN_LOG_ERROR << "RtP launch aborted -- Unable to parse XML for rtp file: " << rtpPath;
                ORIGIN_LOG_ERROR << "parse error: " << errorMsg << " line: " << errorLine << " col: " << errorCol;
		        return false;
	        }

	        QDomElement rtpElement(doc.documentElement());
    
	        if ((rtpElement.tagName() != "rtp"))
	        {
		        ORIGIN_LOG_ERROR << "RtP launch aborted -- Unexpected rtp configuration XML structure for file: " << rtpPath;
		        // Unexpected document element and namespace
		        return false;
	        }

            const QString& originLocale = OriginApplication::instance().locale();

            bool bSuccess = false;
            QString expectedHash;
            QString defaultTitle;
            bool bTitleFound = false;

	        // We should only parse the first content element if it's valid or not
	        for(QDomElement element = rtpElement.firstChildElement();
		        !element.isNull();
		        element = element.nextSiblingElement())
	        {

                // Check for standardized tags
		        if (element.tagName() == "gameHash")
                {
                    expectedHash = element.text();
                    expectedHash = expectedHash.simplified();
                    if (expectedHash.length() != 32)
                    {
                        ORIGIN_LOG_ERROR << "RtP launch aborted -- Invalid hash length.";
                        return false;
                    }
                }
		        else if (element.tagName() == "gameIDs")
		        {
                    contentIds = element.text();
                    contentIds.remove(' '); // Spaces inside the string need to be removed as well.
                    bSuccess = true;
		        }
		        else if (element.tagName() == "gameName")
		        {
                    if (element.attribute("locale", "") == originLocale)
                    {
                        contentTitle = element.text();
                        contentTitle = contentTitle.simplified();
                        bTitleFound = true;
                    }
            
                    if (defaultTitle.isEmpty() || element.attribute("locale", "") == "en_US")
                    {
                        defaultTitle = contentTitle.simplified();
                    }
		        }
	        }

            // If we didn't find a title that matched our Origin locale, use the default
            if (!bTitleFound)
            {
                if (defaultTitle.isEmpty())
                    contentTitle = "Unknown";
                else
                    contentTitle = defaultTitle;
            }

            // Calculate the hash of the .rtp file and compare it against the expected hash in the .rtp file
            QString hash = calculateRtPHash(rtpFile, expectedHash);
            if (expectedHash.toLower() != hash.toLower())
            {
                ORIGIN_LOG_ERROR << "RtP launch aborted -- Hash doesn't match.";
                return false;
            }

            return bSuccess;
        }

        bool CommandFlow::isLaunchedFromBrowser()
        {
            return findArgument ("-Origin_LaunchedFromBrowser");
        }

        bool CommandFlow::isOpenAutomateParam()
        {
            return findArgument (OpenAutomateId);
        }

        QUrl CommandFlow::getActionUrl()
        {
            QString actionUrlStr = urlProtocolArgument ();
            if (actionUrlStr.isEmpty())
                return QUrl();

            //TELEMETRY
            QString logLine;
            QStringList actionUrlParsed = actionUrlStr.split(QRegExp("[ =?&]"));
            //	Iterate through the string list
            for (int i = 0; i < actionUrlParsed.size(); i++)
            {
                //	Filter out login token / auth token
                if ((actionUrlParsed[i].contains(QString("logintoken"), Qt::CaseInsensitive) == true))
                {
                    actionUrlParsed[i] = "logintoken";
                }
                else if (actionUrlParsed[i].contains(QString("authtoken"), Qt::CaseInsensitive) == true)
                {
                    if (actionUrlParsed.length() > i+1)
                        actionUrlParsed[i+1] = "XXX";
                }

                //	append back to the QString
                logLine += actionUrlParsed[i] + "_";
            }
            // Only send the first 200 characters of the command line to telemetry
            logLine.truncate(200);
            GetTelemetryInterface()->Metric_APP_CMD(logLine.toUtf8().data());

            QUrl actionUrl;
            if (actionUrlStr.contains ("origin://", Qt::CaseInsensitive))    //need to translate it to origin2:// first
            {
                actionUrl = translateToOrigin2 (actionUrlStr);
            }
            else
            {
                //strip any leading/trailing "" 
                int pos = actionUrlStr.indexOf ("origin2://", 0, Qt::CaseInsensitive);
                if (pos > 0)
                {
                    actionUrlStr.remove(0, pos);
                    actionUrlStr = actionUrlStr.trimmed();  //trim any trailing spaces
                    pos = actionUrlStr.lastIndexOf ("\"");
    
                    if (pos == actionUrlStr.length()-1)
                        actionUrlStr.remove (pos,1); 
                }
                actionUrl =  QUrl::fromEncoded(actionUrlStr.toLatin1());
            }

            return actionUrl;
        }


        //****************************************************************************************************/
        //************************************ PRIVATE FUNCTIONS *********************************************/
        //****************************************************************************************************/

        //crazy hack needed for IE9 -- it replaces %20 in origin:// with " " so we need to restore it to %20
        QStringList CommandFlow::fixOriginProtocolCmdLine (QString& cmdLine)
        {
            QString modCmdLine = cmdLine;
            QStringList argsList;
            QString argStr;
            QString originprotocol = "origin://";

            int pos = cmdLine.indexOf ("origin://", 0, Qt::CaseInsensitive);
            if (pos == -1)
            {
                pos = cmdLine.indexOf ("origin2://", 0, Qt::CaseInsensitive);
                originprotocol = "origin2://";
            }

            if (pos != -1)  //shouldn't be but...
            {
                //ok, start at the beginning and keep parsing until we hit origin://
                int nextpos;

                while (!(modCmdLine.startsWith ("\"" + originprotocol)) && !(modCmdLine.startsWith (originprotocol)))
                {
                    nextpos = modCmdLine.indexOf (" ", 0);
                    if (nextpos != -1)  //shouldn't happen but...
                    {
                        argStr = modCmdLine.left (nextpos);
                        argsList.append (argStr);

                        QString rightStr = modCmdLine.right (modCmdLine.length() - (nextpos+1));

                        //get rid of any leading or trailing space
                        modCmdLine = rightStr.trimmed();
                    }
                    else
                        break;
                }

                //ok found origin://
                //need to find the end of the origin:// param
                //make a wild assumption that it will either be terminated with " or not at all
                //but we need to also make sure that the next " we find isn't an embedded one

                QString protocolArg;
                int endPos = modCmdLine.indexOf ("\"", 1); //if " doesn't exist endPos will be -1

                if (endPos == -1)   //no terminating "
                {
                    protocolArg = modCmdLine;   //just grab the entire thing
                    modCmdLine.clear();
                }
                else
                {
                    //see if we can find " followed by space
                    int endPos2 = modCmdLine.indexOf ("\" ", 1); 

                    //if it doesn't exist, then there is no more arguments past the "
                    if (endPos2 == -1)  //terminating one
                    {
                        endPos2 = modCmdLine.lastIndexOf ("\""); //then use the position of just " 
                    }

                    protocolArg = modCmdLine.left (endPos2 + 1);

                    int modlen = modCmdLine.length();
                    int protocollen = protocolArg.length();

                    if (modlen > (protocollen+1))   //there's more past the origin://
                    {
                        QString rightStr = modCmdLine.right (modCmdLine.length() - (protocolArg.length()+1));
                        modCmdLine = rightStr.trimmed();
                    }
                    else
                    {
                        modCmdLine.clear();
                    }
                }

                //now substitue " " with %20 in this string
                protocolArg.replace (" ", "%20");
                argsList.append (protocolArg);

                if (!modCmdLine.isEmpty())
                {
                    //continue with the rest
                    QStringList restArgsList = modCmdLine.split(QRegExp(" "));
                    for (int i = 0; i < restArgsList.count(); i++)
                    {
                        argsList.append (restArgsList [i]);
                    }
                }
            }
            else
            {
                argsList = cmdLine.split(QRegExp(" "));
            }
            return (argsList);
        }

        QUrl CommandFlow::translateToOrigin2 (QString& originalProtocolStr)
        {
            QUrl origin2formatUrl;

            QUrl commandUrl = getUrlFromCommandLine(originalProtocolStr);
            QString urlHost = commandUrl.host();

            if ((urlHost == "store") || (urlHost == "storejump"))
            {
                origin2formatUrl = translateHostStore(commandUrl, (urlHost == "storejump"));
            }
            else if ((urlHost == "library") || (urlHost == "libraryjump"))
            {
                origin2formatUrl = translateHostLibrary(commandUrl, (urlHost == "libraryjump"));
            }
            else if ((urlHost == "launchgame") || (urlHost == "launchgamejump"))
            {
                origin2formatUrl = translateHostLaunchGame(commandUrl, false, (urlHost == "launchgamejump"));
            }
            else if (urlHost == "launchgamertp")
            {
                origin2formatUrl = translateHostLaunchGame(commandUrl, true, false);
            }
            else if ((urlHost == "friends")
                || (urlHost == "settings")
                || (urlHost == "redeem")
                || (urlHost == "help")
                || (urlHost == "addgame")
                || (urlHost == "addfriend")
                || (urlHost == "profile")
                || (urlHost == "logout")
                || (urlHost == "quit"))
            {
                origin2formatUrl = translateToCurrentTab (urlHost);
            }
            return origin2formatUrl;
        }

        QUrl CommandFlow::translateHostStore (QUrl& commandUrl, bool fromJumpList)
        {
            //supported formats:
            //origin://store
            //origin://store/<product id> - open the store tab to the product id
            QUrl origin2Url ("origin2://store/open");
            QUrlQuery urlQuery(origin2Url);

            if (!commandUrl.path().isEmpty() && commandUrl.path() != "/")
            {
                QString urlStoreProductBarePath = commandUrl.path().remove(0, 1);
                urlQuery.addQueryItem(ParamContextTypeId, ContextTypeOfferId);
                urlQuery.addQueryItem(ParamIdId, urlStoreProductBarePath);
            }

            if (fromJumpList)
            {
                urlQuery.addQueryItem (ParamInternalJumpId, "true");
            }

            if (!urlQuery.isEmpty())
                origin2Url.setQuery(urlQuery);

            return origin2Url;
        }

        QUrl CommandFlow::translateHostLibrary (QUrl& commandUrl, bool fromJumpList)
        {
            //supported formats
            //origin://library
            //origin://library/<contentid>
            QUrl origin2Url ("origin2://library/open");
            QUrlQuery urlQuery(origin2Url);

            // Should we refresh the games page?
            QUrlQuery commandUrlQuery (commandUrl);
            QString refreshQuery = commandUrlQuery.queryItemValue(ParamRefreshId).trimmed();
            if (!commandUrl.path().isEmpty() && commandUrl.path() != "/")
            {
                QString urlGameIds = commandUrl.path().remove(0, 1);
                urlQuery.addQueryItem(ParamClientPageId, ClientPageGameDetailsByUnknownTypeId);  //id passed in could be contentID or offerID
                urlQuery.addQueryItem(ParamIdId, urlGameIds);
            }

            if (!refreshQuery.isNull() && 
                (refreshQuery != "false") &&
                (refreshQuery != "0"))
            {
                urlQuery.addQueryItem (ParamRefreshId, "true");
            }

            if (fromJumpList)
            {
                urlQuery.addQueryItem (ParamInternalJumpId, "true");
            }

            if (!urlQuery.isEmpty())
                origin2Url.setQuery(urlQuery);

            return origin2Url;
        }

        QUrl CommandFlow::translateHostLaunchGame (QUrl& commandUrl, bool bParseRTP, bool fromJumpList)
        {
            //supported formats
            //origin://launchgame/<contentid>
            //origin://launchgamertp/<path to .rtp file>
            //parameters
            // AutoDownload=1
            // Title=<game title>
            // Restart=1
            // CommandParams
            // AuthCode
            // AuthRedirectUrl
            // AuthToken
            QUrlQuery commandUrlQuery(commandUrl.query());
            QString urlGameIds = "";
            QString urlGameTitle = QObject::tr("ebisu_client_r2p_unknown_game_title");
            if (bParseRTP)
            {
                QString parPath = commandUrl.path().remove(0, 1);
                parPath = QUrl::fromPercentEncoding(parPath.toUtf8());

                // If we got a .rtp file instead of content IDs, parse the file
                if (!parPath.endsWith(".rtp", Qt::CaseInsensitive))
                {
                    ORIGIN_LOG_ERROR << "RtP launch aborted -- couldn't find .rtp file name.";
                    return QUrl();
                }
    
                if (!parseRtPFile(parPath, urlGameIds, urlGameTitle)) // urlGameIds and urlGameTitle are out parameters
                    // Do we want an error dialog here?
                    return QUrl(); // errors will already have been logged by parseRtPFile()
            }
            else
            {
                urlGameIds = commandUrl.path().remove(0, 1);
            }

            if (commandUrlQuery.hasQueryItem("Title"))
            {
#ifdef ORIGIN_PC
                // don't want to chagne the wasy we handle the PC display title
                 urlGameTitle = commandUrlQuery.queryItemValue("Title");
#endif
#ifdef ORIGIN_MAC
                // for the Mac we need to decode the display title with using Base64
                // thie title is encoded this way to prevent the QUrl API from trying to
                // handle special characters like '#' and '&' that could be in the title.
                
                // first we grab the encoded title as a using UTF8 representation
                QByteArray encodedArray = commandUrlQuery.queryItemValue("Title").toUtf8();
                
                // now decode the array using Base64
                QByteArray tempArray = QByteArray::fromBase64(encodedArray);
                
                // now save the array to the expected variable
                urlGameTitle = QString::fromUtf8(tempArray, tempArray.size());
#endif
            }
            QString urlSSOtoken = ( commandUrlQuery.hasQueryItem("AuthToken") ? commandUrlQuery.queryItemValue("AuthToken") : "" ) ;
            QString urlSSOauthCode = ( commandUrlQuery.hasQueryItem("AuthCode") ? commandUrlQuery.queryItemValue("AuthCode") : "" ) ;
            QString urlSSOauthRedirectUrl = ( commandUrlQuery.hasQueryItem("AuthRedirectUrl") ? commandUrlQuery.queryItemValue("AuthRedirectUrl") : "" ) ;
            QString urlItemSubType = ( commandUrlQuery.hasQueryItem("ItemSubType") ? commandUrlQuery.queryItemValue("ItemSubType") : "" );

            bool urlAutoDownload = false;
            bool urlRestart = false;

            // Optional game exe command line parameters (Added for games like BF3 that relaunch the exe from a browser with a command line)
            //[Qt5] Qt5 double encodes from this in parseCommandLineforUrl:     return QUrl::fromEncoded(line.toLatin1());
            //passing in FullyDecoded to queryItemValue will remove the double encoding, converting it back to %20
            QString urlCmdParams = commandUrlQuery.queryItemValue("CommandParams", QUrl::FullyDecoded);
            //this will change %20 to " " as needed for parsing below
            urlCmdParams = QUrl::fromPercentEncoding (urlCmdParams.toLatin1());

            // Also need to check for command line parameters that might have an AuthToken
            if (urlSSOtoken.isEmpty())
            {
                if (urlCmdParams.contains("-AuthToken"))
                {
                    QStringList paramsList = urlCmdParams.split(" ");
                    for (int i = 0; i < paramsList.size(); ++i)
                    {
                        if (paramsList[i] == "-AuthToken" && (i+1 != paramsList.size()))
                        {
                            urlSSOtoken = paramsList[i+1];
                            break;
                        }
                    }
                }
            }

            if (urlSSOauthCode.isEmpty())
            {
                if (urlCmdParams.contains("-AuthCode"))
                {
                    QStringList paramsList = urlCmdParams.split(" ");
                    for (int i = 0; i < paramsList.size(); ++i)
                    {
                        if (paramsList[i] == "-AuthCode" && (i+1 != paramsList.size()))
                        {
                            urlSSOauthCode = paramsList[i+1];
                            break;
                        }
                    }
                }
            }

            if (urlSSOauthRedirectUrl.isEmpty())
            {
                if (urlCmdParams.contains("-AuthRedirectUrl"))
                {
                    QStringList paramsList = urlCmdParams.split(" ");
                    for (int i = 0; i < paramsList.size(); ++i)
                    {
                        if (paramsList[i] == "-AuthRedirectUrl" && (i+1 != paramsList.size()))
                        {
                            urlSSOauthRedirectUrl = paramsList[i+1];
                            break;
                        }
                    }
                }
            }
            Origin::Client::CommandLine::UnicodeUnescapeString(urlGameTitle);

            if (commandUrlQuery.hasQueryItem("AutoDownload") && commandUrlQuery.queryItemValue("AutoDownload").toInt() )
            {
                urlAutoDownload = true;
            }

            // Quick Fix for Crysis 2 restart, which did not have the new Restart=1 parameter option
            // available for their build
            //if (urlHost == "launchgame" && (line.contains("71645") || line.contains("71656")))
            //{
            //    ORIGIN_LOG_EVENT << "Adding Restart Query Parameter.";
            //    commandUrlQuery.addQueryItem("Restart", "1");
            //    commandUrl.setQuery(commandUrlQuery);
            //}
            if (!bParseRTP && !fromJumpList)    //just launchgame, not launchgamertp or launchgamejump
            {
                ORIGIN_LOG_EVENT << "Adding Restart Query Parameter.";
                if (urlGameIds.contains ("71645") || urlGameIds.contains ("71656"))
                    urlRestart = true;
            }

            if (commandUrlQuery.hasQueryItem("Restart") && (commandUrlQuery.queryItemValue("Restart").toInt() || commandUrlQuery.queryItemValue("Restart").compare("true", Qt::CaseInsensitive) == 0))
            {
                urlRestart = true;
            }

            // We need the content ID because we can't find the entitlement without that.
            if (!urlGameIds.isEmpty())
            {
                QUrl origin2Url ("origin2://game/launch");
                QUrlQuery urlQuery(origin2Url);

                //required
                urlQuery.addQueryItem (ParamOfferIdsId, urlGameIds);

                //optional
                if (!urlGameTitle.isEmpty())
                    urlQuery.addQueryItem (ParamTitleId, urlGameTitle);
                if (!urlSSOtoken.isEmpty())
                    urlQuery.addQueryItem (ParamAuthTokenId, urlSSOtoken);
                if (!urlSSOauthCode.isEmpty())
                    urlQuery.addQueryItem (ParamAuthCodeId, urlSSOauthCode);
                if (!urlSSOauthRedirectUrl.isEmpty())
                    urlQuery.addQueryItem (ParamAuthRedirectUrlId, urlSSOauthRedirectUrl);
                if (!urlCmdParams.isEmpty())
                    urlQuery.addQueryItem (ParamCmdParamsId, urlCmdParams);
                if (urlAutoDownload)
                    urlQuery.addQueryItem (ParamAutoDownloadId, "true");
                if (urlRestart)
                    urlQuery.addQueryItem (ParamRestartId, "true");
                if (urlItemSubType.isEmpty())
                    urlQuery.addQueryItem (ParamItemSubTypeId, urlItemSubType);

                if (!urlQuery.isEmpty())
                    origin2Url.setQuery(urlQuery);

                return origin2Url;
            }
            else
                return QUrl ();
        }

        QUrl CommandFlow::translateToCurrentTab (QString &urlHost)
        {
            QUrl origin2Url;
            QUrlQuery urlQuery(origin2Url);

            if (urlHost == "friends")
            {
                origin2Url.setUrl ("origin2://currentTab");
                urlQuery.addQueryItem (ParamPopupWindowId, PopupWindowFriendListId);
            }
            else if (urlHost == "settings")
            {
                origin2Url.setUrl ("origin2://library/open");   //this translates to library/open since it is a full-page load and not a popup window 
                urlQuery.addQueryItem (ParamClientPageId, ClientPageSettingsGeneralId);
            }
            else if (urlHost == "redeem")
            {
                origin2Url.setUrl ("origin2://currentTab");
                urlQuery.addQueryItem (ParamPopupWindowId, PopupWindowRedeemGameId);
                urlQuery.addQueryItem (ParamRequestorId, RequestorIdClient);
            }
            else if (urlHost == "help")
            {
                origin2Url.setUrl ("origin2://currentTab");
                urlQuery.addQueryItem (ParamPopupWindowId, PopupWindowHelpId);
            }
            else if (urlHost == "addgame")
            {
                origin2Url.setUrl ("origin2://currentTab");
                urlQuery.addQueryItem (ParamPopupWindowId, PopupWindowAddGameId);
            }
            else if (urlHost == "addfriend")
            {
                origin2Url.setUrl ("origin2://currentTab");
                urlQuery.addQueryItem (ParamPopupWindowId, PopupWindowAddFriendId);
            }
            else if (urlHost == "profile")
            {
                origin2Url.setUrl ("origin2://library/open");   //this translates to library/open since it is a full-page load and not a popup window 
                urlQuery.addQueryItem (ParamClientPageId, ClientPageProfileId);
            }
            else if (urlHost == "logout")
            {
                if (MainFlow::instance() && Engine::LoginController::isUserLoggedIn())
                {
                    ORIGIN_LOG_EVENT << "User requested application logout via command line, kicking off logout flow.";
                    // If we are logged in, kick off the exit flow.  Skip the confirmation step.
                    MainFlow::instance()->exit(Origin::Client::ClientLogoutExitReason_Logout_Silent);
                }
            }
            else if (urlHost == "quit")
            {
                if (MainFlow::instance() && Engine::LoginController::isUserLoggedIn())
                {
                    ORIGIN_LOG_EVENT << "User requested application exit via command line, kicking off exit flow.";
                    // If we are logged in, kick off the exit flow.  Skip the confirmation step.
                    MainFlow::instance()->exit(ClientLogoutExitReason_Exit_NoConfirmation);
                }
                else
                {
                    ORIGIN_LOG_EVENT << "User requested application exit via command line, but no user has logged in yet so simply exiting.";
                    OriginApplication::instance().cancelTaskAndExit(ReasonSystrayExit);
                }
            }

            if (!urlQuery.isEmpty())
                origin2Url.setQuery(urlQuery);

            return origin2Url;
        }

        // Returns the hash of xmlFile with the value of hash replaced by the salt value
        // xmlFile [in] is an open file handle to the .rtp xml file.  File position and text mode will be modified.
        // hash [in] is the expected hash value to replace with the salt
        QString CommandFlow::calculateRtPHash(QFile &rtpFile, const QString &hash)
        {
            const char* RTP_SALT = "OriginAndTonic";

            // Read the file contents
            rtpFile.seek(0);
            rtpFile.setTextModeEnabled(false);
            QByteArray bytes = rtpFile.readAll();

            // Replace the hash with the salt value so we can calculate the actual md5 without the expected md5 contaminating the results
            bytes.replace(hash, RTP_SALT);

            // Calculate the md5 and return it
 	        QCryptographicHash md5Hash(QCryptographicHash::Md5);
            md5Hash.addData(bytes);

        //ORIGIN_LOG_ERROR << "Got: " << md5Hash.result().toHex();
        //ORIGIN_LOG_ERROR << "Expected: " << hash;

            return md5Hash.result().toHex();
        }

        QUrl CommandFlow::getUrlFromCommandLine(QString const& ln)
        {
            QString line(ln);
            QString temp = line.toLower();
            //check for origin scheme 
            int urlBeginIndex = temp.indexOf("origin://");
            if(urlBeginIndex == -1)
            {
                //if can't find origin check for eadm
                urlBeginIndex = temp.indexOf("eadm://");
            }

            //if the url is not first in the list, remove the initial part of the command line so that
            //the url is first
            if(urlBeginIndex > 0)
                line.remove(0, urlBeginIndex);

            // URLs don't contain spaces......so if a URL was found, strip everything after any found space
            int spaceIndex = line.indexOf(" ");
            if (spaceIndex > 0)
                line.remove(spaceIndex, line.length() - spaceIndex);

            if((line.count('"') % 2) != 0)
            {
                line.remove(line.lastIndexOf('"'), 1);
            }

#ifdef ORIGIN_MAC
            line = QUrl::fromPercentEncoding(line.toUtf8());
#endif

            //	Output the URL with tokens filtered out to logs
            Origin::Services::LoggerFilter::DumpCommandLineToLog("URL Param", line);

            //when ":" is passed in percent encoded, simply passing it into the constructor for QUrl isn't converting it
            //so for the PC, we need to use fromEncoded; but for the Mac, since they are already calling fromPercentEncoding
            //(because they were double percent encoding because they were trying to pass something unencoded from bootstrap)
            //we don't want to call fromEncoded again
#ifdef ORIGIN_PC
            return QUrl::fromEncoded(line.toLatin1());
#else
            return QUrl(line);
#endif
        }

    }
}
