#ifndef ORIGINNOTIFICATIONDIALOG_H_
#define ORIGINNOTIFICATIONDIALOG_H_

#include "originchromebase.h"
#include "uitoolkitpluginapi.h"
#include <QTime>

class QLabel;

namespace Origin
{
namespace UIToolkit
{

class UITOOLKIT_PLUGIN_API OriginNotificationDialog : public Origin::UIToolkit::OriginChromeBase
{
    Q_OBJECT

public:
    OriginNotificationDialog(QWidget* content = 0, QWidget* parent = 0);
    ~OriginNotificationDialog();
    const QMargins borderWidths() const {return QMargins(9,6,8,9);}
    void setBannerVisible(const bool&) {}
    void setMsgBannerVisible(const bool&) {}
    void showBubble(bool doShow);
    void setInitalOpacity(qreal opacity);
    void setInitalIGOMode(bool igoMode);
    void setMaxOpacity(float max){mMaxOpacity = max;};

    void setFadeInTime(const int& fadeInTime);
    int  getLifeSpan(){return mLifeTimer->elapsed();};
    void resetLifeSpan(){mLifeTimer->restart();};
    void setIGOFlags(bool isIGO);
    void activated();
    void shouldAcceptClick(bool shouldAccept){mShouldAcceptClick = shouldAccept;};
    void setCloseTimer(int time);
    const qreal opacity() const {return mOpacity;};

signals:
    // the clicked signal should be connected with a Qt::QueuedConnection so that we can emit the signal and close the
    // notification dialog right away. For example:
    // ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), this, SLOT(doClicked()), Qt::QueuedConnection);
    void clicked();
    void opacityChanged(qreal opacity);
    void changePos(const QPoint& pos);
    void igoActivated();
    void actionAccepted();
    void actionRejected();

protected:
    QString& getElementStyleSheet();
    void prepForStyleReload();
    void ensureCreatedMsgBanner() {}
    void enterEvent(QEvent*);
    void leaveEvent(QEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void showEvent(QShowEvent*);
    void resizeEvent(QResizeEvent*);

private slots:
    void fadeInTick();
    void fadeOutTick();

public slots:
    void setIGOMode(bool igoMode);
    void windowClicked();
    void windowClosed();
    void beginFadeOut();
    void onActionAccepted();
    void onActionRejected();

private:
    void setOpacity(qreal);

    qreal mOpacity;
    qreal mMaxOpacity;
    QTimer* mTimer;
    QTime* mLifeTimer;
    static QString mPrivateStyleSheet;
    QLabel* mBubbleLabel;
    bool mIGOMode;
    bool mShouldAcceptClick;
    bool mClosing;
    int mCloseTime;

};

}
}
#endif
