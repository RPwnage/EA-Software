/////////////////////////////////////////////////////////////////////////////
// SpaceInfoWidget.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef SPACEINFOWIDGET_H
#define SPACEINFOWIDGET_H

#include <QWidget>

#include "services/plugin/PluginAPI.h"

namespace Ui
{
class SpaceInfoWidget;
}

namespace Origin    
{
namespace Client
{
class ORIGIN_PLUGIN_API SpaceInfoWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qulonglong downloadSize READ downloadSize WRITE setDownloadSize)
    Q_PROPERTY(qulonglong installSize READ installSize WRITE setInstallSize)
    Q_PROPERTY(qulonglong availableSpace READ availableSpace WRITE setAvailableSpace)

public:
    SpaceInfoWidget(QWidget* parent = 0);
    ~SpaceInfoWidget();

    void setRequiredLabel(const QString& label, const QString& tooltip = "");

    qulonglong downloadSize() const {return mInstallBytes;}
    void setDownloadSize(const qulonglong& bytes);

    qulonglong installSize() const {return mInstallBytes;}
    void setInstallSize(const qulonglong& bytes);

    qulonglong availableSpace() const {return mAvailableBytes;}
    void setAvailableSpace(const qulonglong& bytes);

    bool sufficientFreeSpace() const {return mInstallBytes < mAvailableBytes;}


private:
    Ui::SpaceInfoWidget* mUI;
    qulonglong mAvailableBytes;
    qulonglong mInstallBytes;
    qulonglong mDownloadBytes;
};
}
}
#endif