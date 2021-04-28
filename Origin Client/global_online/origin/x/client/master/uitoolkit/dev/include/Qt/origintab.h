#ifndef ORIGINTAB_H
#define ORIGINTAB_H

#include "originuiBase.h"
#include "uitoolkitpluginapi.h"
#include <QPushButton>
#include <QPoint>

class Ui_OriginTab;

namespace Origin {
    namespace UIToolkit {

class UITOOLKIT_PLUGIN_API OriginTabIcon: public QPushButton
{
    Q_OBJECT

public:
    OriginTabIcon(QWidget* parent = 0);
    ~OriginTabIcon();

protected:
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
};

class UITOOLKIT_PLUGIN_API OriginTabCloseButton : public QPushButton
{
    Q_OBJECT

public:
    OriginTabCloseButton(QWidget* parent = 0);
    ~OriginTabCloseButton();

protected:
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);

private:
    bool mIsMouseDown;
    bool mIsDragging;
    QPoint mMouseDownPos;
    int mStartDragDistance;
};

class UITOOLKIT_PLUGIN_API OriginTab : public QWidget, public OriginUIBase
{
    Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent Pointer to the parent widget, defaults to NULL
    OriginTab(QWidget* parent = 0);

    /// \brief Destructor
    ~OriginTab();

    void setText(const QString& text);
    QString text() const;
    void setIcon(const QIcon& icon);

    void setCloseButtonVisible(bool visible);
    void setIconVisible(bool visible);
    void setTitleVisible(bool visible);

    OriginTabCloseButton *closeButton();
    OriginTabIcon* iconButton();
    int labelWidth();
    QFont labelFont();

signals:
    void closeRequested(OriginTab*);
    void enterTab();
    void leaveTab();

protected slots:
    void onCloseButtonClicked();

protected:
    QString& getElementStyleSheet();
    void prepForStyleReload();
    bool event(QEvent* event);
    void paintEvent(QPaintEvent* event);
    void enterEvent(QEvent* event);
    void leaveEvent(QEvent* event);
    QWidget* getSelf() {return this;}

private:
    static QString mPrivateStyleSheet;
    Ui_OriginTab* ui;
};

}
}
#endif
