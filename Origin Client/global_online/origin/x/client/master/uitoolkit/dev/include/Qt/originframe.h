#ifndef ORIGINFRAME_H
#define ORIGINFRAME_H

#include <QtWidgets/QFrame>
#include "originmultilineedit.h"
#include "uitoolkitpluginapi.h"

namespace Origin {
    namespace UIToolkit {
class UITOOLKIT_PLUGIN_API OriginFrame: public QFrame
{
    Q_OBJECT
    Q_PROPERTY(bool background READ isBackgroundEnabled WRITE setBackgroundEnabled)
    Q_PROPERTY(QString html READ getHTML WRITE setHTML)

public:
    OriginFrame(QWidget *parent = 0);

    bool isBackgroundEnabled() const;
    void setBackgroundEnabled(bool enabled);

    QString getHTML() const;
    void setHTML(QString htmlText);

protected:
     void paintEvent(QPaintEvent* event);
     bool event(QEvent* event);



protected slots:
     void onChildFocusChanged(bool focusIn);

private:
     static QString mPrivateStyleSheet;
     static QString& getPrivateStyleSheet();

     bool m_backgroundEnabled;
     OriginMultiLineEdit * m_text;
};

}
}

#endif
