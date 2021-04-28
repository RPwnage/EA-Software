#include "QtUtil.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QWidget>
#include <QToolTip>
#include <QLocale>
#include <QFileInfo>
#include <QWidget>
#include <QProcess>
#include <qmetaobject.h>
// used for DrawRoundCornerForMenu
#include <QMenu>
#include <QRegion>
#include <QRect>
#include <QUrl>

#ifdef ORIGIN_PC
#define WIN32_LEAN_AND_MEAN		
#include <windows.h>
// used to get the taskbar's location
#include <winuser.h>
#include <shellapi.h>
#include <Lmcons.h>
#endif

#include "services/debug/DebugService.h"

const QString gsQTNorwegianLocale = "nb_NO";
const QString gsAccessNorwegianLocale = "no_NO";

namespace Origin
{
    namespace Services
    {
        QString FormatAsianFontToolTip(const QString & str, const int iNumAsia, const QString & wordToWatch)
        {
            QString strToolTip;
            int iStartPos = 0;
            QString newLine = "";
            while (!str.mid(iStartPos, iNumAsia).isEmpty())
            {
                if (iStartPos != 0)
                {
                    strToolTip += "\n";
                }

                newLine = str.mid(iStartPos, iNumAsia);

                if (wordToWatch.isEmpty() == false)
                {
                    // Issue being prevented: word wrap in the middle of an 
                    // english word (i.e. "Origin") in an asia character context
                    // If they tried to put word (i.e. "Origin") in the string - check where it will begin
                    const int index = newLine.indexOf(wordToWatch[0]);
                    // If the word doesn't fit into that line
                    if (index > iNumAsia - wordToWatch.length())
                    {
                        // Then we will move the word to the next line
                        const int len = iNumAsia - index;
                        newLine.remove(index, len);
                        iStartPos -= len;
                    }
                }

                strToolTip += newLine;
                iStartPos += iNumAsia;
            }
            return strToolTip;
        }

        QString FormatStandardFontToolTip(const QString & str, const int iNumNotAsia)
        {
            QString strToolTip;
            int iStartPos = 0;
            int iWrapPos = 0;
            while (!str.mid(iStartPos, iNumNotAsia).isEmpty())
            {
                iWrapPos = str.mid(iStartPos, iNumNotAsia).lastIndexOf(' ', iNumNotAsia - 1);
                const int auxPos = (iWrapPos == -1) ? iNumNotAsia : (iWrapPos + 1);
                if (iStartPos != 0)
                {
                    strToolTip += "\n";
                }
                strToolTip += str.mid(iStartPos, auxPos);
                iStartPos += auxPos;
            }
            return strToolTip;
        }

        QString FormatToolTip(const QString & str, const QString & wordToWatch, const int iNumAsia, const int iNumNotAsia)
        {
            QFontMetrics fontMetrics(QToolTip::font());
            const int stringSize = fontMetrics.width(str);
            const int standardSize = fontMetrics.averageCharWidth() * iNumAsia;

            if (stringSize >= standardSize)
            {
                if (fontMetrics.width(str.left(iNumAsia)) >= standardSize * 1.5)
                {
                    return FormatAsianFontToolTip(str, iNumAsia, wordToWatch);
                }
                else
                {
                    return FormatStandardFontToolTip(str, iNumNotAsia);
                }
            }	
            return QString(str);
        }

        QString getFormatedWidgetSize(QWidget *w)
        {
            QString ret;
            //if all 4 points are out of the available screen, not need store the size and position
            QDesktopWidget* desktopWidget=QApplication::desktop();
            if(w->isMaximized()){
                int CurrentScreenNum= desktopWidget->screenNumber(w);		
                ret = "MAXIMIZED|" + QString::number(CurrentScreenNum,10);      
            }
            else
            {
                int totoalScreens=desktopWidget->screenCount();
                QRect clientRects,appRects;
                for(int i=0; i<totoalScreens;i++){
                    clientRects=desktopWidget->availableGeometry(i);
                    appRects=w->geometry();
                    if(clientRects.intersects(appRects)){
                        ret = QString::number(w->size().width(),10)+","+QString::number(w->size().height(),10)+"|"+QString::number(w->pos().x(),10)+","+QString::number(w->pos().y(),10);		
                    }
                }
            }
            return ret;
        }

        QRect setFormatedWidgetSizePosition(const QString& formatedSizePos, const QSize& preferredSize, bool fromUI, bool centerAsDefault/*=true*/)
        {
            QRect ret = QRect(-1,-1,-1,-1);

            //Read back size and position and populate the app geometry
            QStringList sizePosition = formatedSizePos.split("|");
            QString size =sizePosition.first();
            QString position = sizePosition.last();
            int screen = QApplication::desktop()->primaryScreen();

            bool useDefaultLocation = false;
            bool AppMax = size.contains("MAXIMIZED",Qt::CaseInsensitive);
            if (AppMax)
            {
                screen = position.toInt();
                if (screen >= QApplication::desktop()->numScreens())
                    screen = 0;
                ret = QApplication::desktop()->availableGeometry(screen);
#if defined(ORIGIN_PC)
                // The behavior we want is for the window to go to the preferred location when the window goes to it's normal size.
                // However, there is a bug right now that the normal size goes to maximize geometry and the solution has unexplainable knock-ons.
                useDefaultLocation = true;
#endif
            }
            else if (fromUI){
                QPoint screenPosition(position.mid(0, position.indexOf(",")).toInt(), position.mid(position.indexOf(",") + 1).toInt());
                // Add a buffer so that if somehow the app starts at the far bottom or right of the screen it will load in the middle
                int pixelBuffer = 100;

                // Only use the position in the settings file if it is actually within the desktop
                // EBIBUGS-20893.  We need the available geometry for the screen on which the window is located.
                QRect virtualDesktop = QApplication::desktop()->availableGeometry(screenPosition);
                if (virtualDesktop.contains(screenPosition.x() + pixelBuffer, screenPosition.y() + pixelBuffer, true))
                {
                    if (!size.contains("0,0"))
                    {	
                        QSize screenSize(size.mid(0, size.indexOf(",")).toInt(), size.mid(size.indexOf(",") + 1).toInt());
                        ret = QRect(screenPosition, screenSize);
                    }
                    else
                    {
                        useDefaultLocation = true;
                    }
                }
                else
                {
                    useDefaultLocation = true;
                }
            }
            if (centerAsDefault && useDefaultLocation)
            {
                int width = preferredSize.width();
                int height = preferredSize.height();
                QRect availableGeom = QApplication::desktop()->availableGeometry(screen);
                if (height > availableGeom.height())
                    height = availableGeom.height() - 50;
                if (width > availableGeom.width())
                    width = availableGeom.width() - 50;
                QPoint screenCenter(availableGeom.center() - (QPoint(width, height) / 2));
                ret = QRect(screenCenter, QSize(width, height));
            }
            return ret;
        }

        /**
        This helper function locates the supported locale that is 
        closest to the system locale
        */
        QString getNearestLocale(const QStringList& langs)
        {
            QString sBestLocale = QLocale::system().name();

#ifdef WIN32
            if ( sBestLocale.compare(gsQTNorwegianLocale, Qt::CaseInsensitive) == 0 )
                sBestLocale = gsAccessNorwegianLocale;
#endif

            // Check exact match
            if ( langs.contains(sBestLocale, Qt::CaseInsensitive) )
                return sBestLocale;

            // Check partial match
            // Sometimes weird stuff like "C" can make it through so sanity check our length
            if (sBestLocale.length() >= 2)
            {
                // check for language match
                QString sLangCode = sBestLocale.left(2);
                foreach(QString sLocale, langs)
                {
                    if ( sLocale.startsWith(sLangCode,Qt::CaseInsensitive) )
                        return sLocale;
                }
                // check for region match (Norwegian did it backwards, hence this check.)
                QString sRegionCode = sBestLocale.right(2);
                foreach(QString sLocale, langs)
                {
                    if ( sLocale.endsWith(sRegionCode,Qt::CaseInsensitive) )
                        return sLocale;
                }
            }

            return "en_US";
        }

#ifdef WIN32
        bool WriteRegistryString( const HKEY PreDefKey,        // Predefined key like HKEY_LOCAL_MACHINE
            const QString& SubKey,        // The sub key string under the PreDefKey
            const QString& ValueName,
            const QString& DataString)
        {
            bool	rval = false;
            HKEY	handleKey = 0;
            DWORD	howRegCreated;
            HKEY	hRootKey = PreDefKey;

            if  ( RegCreateKeyExW( hRootKey, (LPCWSTR)SubKey.utf16(), 0, NULL, // non-remote registry key 
                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                NULL, &handleKey, &howRegCreated ) == ERROR_SUCCESS )
            {								 
                const wchar_t* buffer  = (LPCWSTR)DataString.utf16();	
                int	cbCount = (DataString.length()+1)*sizeof(wchar_t);

                if (RegSetValueExW(handleKey, (LPCWSTR)ValueName.utf16(), 0, REG_SZ, reinterpret_cast<CONST BYTE*>(buffer), cbCount )== ERROR_SUCCESS)
                {
                    rval = true;
                }

                RegCloseKey(handleKey);
            }

            return rval;
        }

        bool HideAppFromRecentMenu(const QString& fileName)
        {
            QString sAppFileName = fileName;

            if ( sAppFileName.isEmpty() )
            {
                QFileInfo fi(QCoreApplication::applicationFilePath());
                sAppFileName = fi.fileName();
            }

            bool result = WriteRegistryString(HKEY_CLASSES_ROOT, QString("Applications\\%1").arg(sAppFileName), "NoStartPage", "" );
            result |= WriteRegistryString(HKEY_CURRENT_USER, QString("Software\\Classes\\Applications\\%1").arg(sAppFileName), "NoStartPage", "" );

            return result;
        }

        int GetEdge(RECT rc)
        {
            int uEdge = -1;

            if (rc.top == rc.left && rc.bottom > rc.right)
            {
                uEdge = ABE_LEFT;
            }
            else if (rc.top == rc.left && rc.bottom < rc.right)
            {
                uEdge = ABE_TOP;
            }
            else if (rc.top > rc.left )
            {
                uEdge = ABE_BOTTOM;
            }
            else
            {
                uEdge = ABE_RIGHT;
            }

            return uEdge;
        }

        TaskbarPosition CalculateTaskbarPosition()
        {
            HWND  pwnd = FindWindow(L"Shell_TrayWnd", NULL);
            TaskbarPosition retPosition = NoTaskbar;

            if (pwnd != NULL)
            {
                APPBARDATA abd;

                abd.cbSize = sizeof(APPBARDATA);
                abd.hWnd = pwnd;

                SHAppBarMessage(ABM_GETTASKBARPOS, &abd);

                int uEdge = GetEdge(abd.rc);

                switch(uEdge)
                {
                case ABE_LEFT:
                    retPosition = TaskbarOnLeft;
                    break;
                case ABE_RIGHT:
                    retPosition = TaskbarOnRight;
                    break;
                case ABE_TOP:
                    retPosition = TaskbarOnTop;
                    break;
                case ABE_BOTTOM:
                    retPosition = TaskbarOnBottom;
                    break;
                default:
                    break;
                }
            }

            return retPosition;
        }

#endif


        unsigned int memory()
        {
            unsigned int  result(0);
#if defined(ORIGIN_PC)
            MEMORYSTATUSEX memory_status;
            ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
            memory_status.dwLength = sizeof(MEMORYSTATUSEX);
            if (GlobalMemoryStatusEx(&memory_status)) 
            {
                result = memory_status.ullAvailPhys / (1024 * 1024);
            }
#elif defined(ORIGIN_MAC)
            QProcess p;
            p.start("sysctl", QStringList() << "kern.version" << "hw.physmem");
            p.waitForFinished();
            result = QString(p.readAllStandardOutput()).toDouble() ;
            p.close();
#else
#error "Required OS-specific specialization"
#endif
            return result;
        }

#ifdef WIN32
        BOOL CALLBACK BringWindowToTopProc( HWND hwnd, LPARAM lParam ) 
        {   
            DWORD dwPID;    
            GetWindowThreadProcessId( hwnd, &dwPID );    
            if( dwPID == (DWORD) lParam ) 
            {     
                SetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );      
                // Or just SetFocus( hwnd );     
                return FALSE;   
            }    
            return TRUE; 
        } 
#endif

        void OpenAddOrRemovePrograms()
        {
#ifdef WIN32
            STARTUPINFOW startupInfo;
            memset(&startupInfo,0,sizeof(startupInfo));
            PROCESS_INFORMATION procInfo;
            memset(&procInfo,0,sizeof(procInfo));
            startupInfo.cb = sizeof(startupInfo);

            const QString command("rundll32.exe shell32.dll Control_RunDLL appwiz.cpl");
            bool result = ::CreateProcess(NULL, (LPWSTR) command.data(), NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &procInfo);
            if (result == true)
            {
                //  Passing the add/remove window process id to bring it's window to topmost layer
                EnumWindows(BringWindowToTopProc, ( LPARAM )( procInfo.dwProcessId)); 
            }
            else
            {
                //  If create process doesn't work, we fallback to old style of bringing
                //  up the add/remove program dialog from pre-client 9.1 which might
                //  show up under the client window.
                if (procInfo.hProcess)
                {
                    ::CloseHandle(procInfo.hProcess);
                }

                if (procInfo.hThread)
                {
                    ::CloseHandle(procInfo.hThread);
                }

                WinExec(command.toLatin1().data(), SW_SHOWNORMAL);
            }
#endif
        }

        QString GetWindowsAccount()
        {
            QString userName;
#ifdef WIN32
            LPTSTR lpszSystemInfo;
            DWORD cchBuff = 256;
            TCHAR tchBuffer[UNLEN + 1];
            lpszSystemInfo = tchBuffer;
            GetUserName(lpszSystemInfo, &cchBuff);
            userName = QString::fromUtf16(lpszSystemInfo);
#else
            userName = "";
#endif
            return  userName;
        }

        void EnableFontAA(QWidget* pWidget, bool bRecursive)
        {
            if(bRecursive)
            {
                // Set it for our children recursively...
                const QObjectList& objectList = pWidget->children();

                for(QObjectList::const_iterator it = objectList.begin(); it != objectList.end(); ++it)
                {
                    QObject* pObject      = *it;
                    QWidget* pWidgetChild = dynamic_cast<QWidget*>(pObject); // Returns NULL if not a QWidget.

                    if(pWidgetChild)
                        EnableFontAA(pWidgetChild, bRecursive);
                }
            }

            // Set it for ourself.
            const QFont& font = pWidget->font();                // Get the current font

            QFont::StyleStrategy styleStrategy = font.styleStrategy();

            if(styleStrategy != QFont::PreferAntialias)
            {
                // Make a copy that of the existing font that only differs on antialiasing
                QFont aaFont(font);
                aaFont.setStyleStrategy(QFont::PreferAntialias);

                pWidget->setFont(aaFont);
            }
        }

        bool DrawRoundCornerForMenu(QMenu *menu, int r,bool flatTop)
        {
            // use a image to deal with the mask
#if USEROUNDIMAGE
            // creat a round corner pixmap 
            QPixmap pixmap(sStrRoundCornerImage);
            // no round corner image, then return
            if(pixmap.isNull())
            {
                return false;
            }

            // get menu's size
            QSize size = menu->sizeHint();
            QSizeF sizeF (size);

            // get pixmap's size
            QSize sizeMap = pixmap.size();
            QSizeF sizeMapF (sizeMap);

            // scale pixmap
            QTransform transform;
            qreal x = sizeF.width()/sizeMapF.width();
            qreal y = sizeF.height()/sizeMapF.height();

            transform.scale(x, y);
            QBitmap newMap = pixmap.mask().transformed(transform);

            // reset the mask
            menu->setMask(newMap);

            return true;
#else
            // use a radius value deal with the mask
            QSize size = menu->sizeHint();
            menu->setMask(roundedRect(QRect(0,0,size.width(),size.height()), r,flatTop));
            return true;
#endif
        }

        QRegion roundedRect(const QRect& rect, int r,bool flatTop) 
        { 
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

            return region; 
        }

        //  Need to percent encode all symbols in the URL for telemetry because we need to 
        //  accurately reconstruct the url for reporting.  This function was added here
        //  because the telemetry system does not have Qt support.
        //
        QByteArray EncodeUrlForTelemetry(const QUrl& url)
        {
            return QUrl::toPercentEncoding(url.toString(), "", "-._~:/?#[]@!$&'()*+,;=");
        }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ThreadObjectReaper::ThreadObjectReaper(QObject* obj, const QString& deleteMethodName)
            : mDeleteMethodName(deleteMethodName)
        {
            mThreadObjects.append(obj);
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(start()), this, SLOT(onStart()));
        }

        ThreadObjectReaper::ThreadObjectReaper(const QVector<QObject*>& objs, const QString& deleteMethodName)
            : mDeleteMethodName(deleteMethodName)
        {
            foreach (QObject* obj, objs)
                mThreadObjects.append(obj);

            ORIGIN_VERIFY_CONNECT(this, SIGNAL(start()), this, SLOT(onStart()));
        }

        void ThreadObjectReaper::onStart()
        {
            foreach (QObject* obj, mThreadObjects)
            {
                ORIGIN_VERIFY_CONNECT(obj, SIGNAL(destroyed(QObject*)), this, SLOT(onDelete(QObject*)));
            }

            if (mDeleteMethodName.isEmpty())
            {
                foreach (QObject* obj, mThreadObjects)
                    obj->deleteLater();
            }

            else
            {
                foreach (QObject* obj, mThreadObjects)
                    QMetaObject::invokeMethod(obj, mDeleteMethodName.toLatin1(), Qt::DirectConnection);
            }
        }

        void ThreadObjectReaper::onDelete(QObject* obj)
        {
            int idx = mThreadObjects.indexOf(obj);
            if (idx >= 0)
            {
                mThreadObjects.remove(idx);
                if (mThreadObjects.count() == 0)
                {
                    emit objectsDeleted();
                    deleteLater();
                }
            }
        }

} // Services
} // Origin