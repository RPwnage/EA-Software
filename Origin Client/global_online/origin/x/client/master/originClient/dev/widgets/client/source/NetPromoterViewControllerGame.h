/////////////////////////////////////////////////////////////////////////////
// NetPromoterViewControllerGame.h
//
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef NET_PROMOTER_VIEW_CONTROLLER_GAME_H
#define NET_PROMOTER_VIEW_CONTROLLER_GAME_H

#include <QObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace UIToolkit
{
    class OriginWindow;
}
namespace Services
{
    class Setting;
}
namespace Client
{
/// \brief Controller for NetPromoter windows.
class ORIGIN_PLUGIN_API NetPromoterViewControllerGame : public QObject
{
    Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent The parent of the NetPromoterViewControllerGame - shouldn't be used.
    NetPromoterViewControllerGame(const QString& productId, QObject* parent = 0);

    /// \brief Destructor - 
    ~NetPromoterViewControllerGame();

    /// \brief Shows the net promoter window.
    void show();
    Q_INVOKABLE void closeWindow();

    static bool isSurveyVisible();

signals:
    void finish(const bool& successful);


protected slots:
    void onReject();
    void onShow();
    void onReponseError();
    void onTimeout();
    void onNetworkRequestFinished(QNetworkReply* networkReply);
    void populatePageJavaScriptWindowObject();
    void logTimeOut();
    void logResponseError();


private:
    /// \brief Checks if window should be suppressed.
    bool okToShow();
    bool isTimeToShow(const int& lastJulianDay, const int& daysToWait);

    UIToolkit::OriginWindow* mWindow;
    QTimer* mResponseTimer;
    const QString mProductId;
};
}
}

#endif
