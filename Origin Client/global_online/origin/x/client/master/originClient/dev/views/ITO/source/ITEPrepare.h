#ifndef ITEPREPARE_H
#define ITEPREPARE_H

#include "engine/downloader/ContentInstallFlowContainers.h"
#include "engine/downloader/IContentInstallFlow.h"
#include "engine/downloader/ContentInstallFlowState.h"
#include "services/plugin/PluginAPI.h"

#include <QtWidgets/QWidget>

#include "eula.h"

namespace Ui 
{
    class ITEPrepare;
}

namespace Origin
{
namespace Client
{
namespace JsInterface
{
    class InstallDirectoryManager;
}

class InstallSpaceInfo;

class ORIGIN_PLUGIN_API ITEPrepareWidget : public QWidget
{
    Q_OBJECT
public:
    ITEPrepareWidget();
    ~ITEPrepareWidget();
    void setOptionalEulaList(const Downloader::EulaList& optionalEulaList);
    void init();
    InstallSpaceInfo* installSpaceInfo() const;

public slots:
    void updateDiskSpaceText();

protected slots:
    void updateData();
    void changeClicked();
    void onDesktopShortcutStateChange(int state);
    void onStartMenuShortcutStateChange(int state);
    void updateUI();
    void onDiskpaceRetry();

    void setInsufficientDiskState(bool insufficient);

protected:
    void retranslate();

private:
    //for communicating with core
    Ui::ITEPrepare* ui;
    JsInterface::InstallDirectoryManager* mInstallDirectoryManager;
    QTimer *mDiskspaceTimer;
};
}
}

#endif  //ITEPREPARE_H
