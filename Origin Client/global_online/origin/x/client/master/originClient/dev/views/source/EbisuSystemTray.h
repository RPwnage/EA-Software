#ifndef _EBISUSYSTEMTRAY_H
#define _EBISUSYSTEMTRAY_H

#include <QSystemTrayIcon>
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace UIToolkit
{
    class OriginWindow;
}
namespace Client
{
class CustomQMenu;

class ORIGIN_PLUGIN_API EbisuSystemTray
#if !defined(ORIGIN_MAC)
    : public QSystemTrayIcon
#else
    : public QObject 
#endif
{
	Q_OBJECT
public:
	~EbisuSystemTray();
    static EbisuSystemTray* instance();
    static void deleteInstance();
    void showNormalSystrayIcon();
    void showDeveloperSystrayIcon();
	void setPrimaryWindow(UIToolkit::OriginWindow* window);
	UIToolkit::OriginWindow* primaryWindow() const { return mPrimaryWindow; }
    void setTrayMenu(CustomQMenu* trayIconMenu);
    CustomQMenu* trayMenu() const { return mTrayIconMenu; }

public slots:
	void showPrimaryWindow();

signals:
    void exitApplication();
    void changedPrimaryWindow(QWidget* window);


protected slots:
	void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
	void onPrimaryWindowDestroyed();

protected:
	EbisuSystemTray(QObject* parent);

	CustomQMenu* mTrayIconMenu;
	UIToolkit::OriginWindow* mPrimaryWindow;
};
}
}
#endif