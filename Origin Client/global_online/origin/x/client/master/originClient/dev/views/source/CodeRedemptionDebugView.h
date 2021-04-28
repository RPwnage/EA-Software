/////////////////////////////////////////////////////////////////////////////
// CodeRedemptionDebugView.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CODEREDEMPTIONDEBUGVIEW_H
#define CODEREDEMPTIONDEBUGVIEW_H

#include <QWidget>
#include <QUrl>
#include <QNetworkReply>
#include <QListWidget>
#include <QList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

#include "services/plugin/PluginAPI.h"

namespace Ui
{
    class CodeRedemptionDebugView;
}

namespace Origin
{
namespace Client
{
    enum Lockbox{
        Get,
        Put
    };
/// \brief Controller for 
class ORIGIN_PLUGIN_API CodeRedemptionDebugView : public QWidget
{
    Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent - The parent of the View Controller.
    explicit CodeRedemptionDebugView(QWidget* parent = NULL);

    /// \brief Destructor
    ~CodeRedemptionDebugView();

private:
    void init();
    void clearVariables();
    QString parseJsonForOfferId(const QByteArray& response);
    void createGetRequest();
    void createPutRequest();
    QString buildResponseString(QNetworkReply* reply, const QString rawResponse, const int httpResponse, Lockbox type) const;

    Ui::CodeRedemptionDebugView* mUi;
    QList<QString> mResponseInformation;
    QString mCdKey;
    QString mOfferId;
    bool mGet;
    QString mGetString;
    QString mPutString;


private slots:
    void onRedeemClicked();
    void onClear();
    void onLockboxNetworkReceived();
    void onItemClicked(QListWidgetItem *item);
};

}
}
#endif