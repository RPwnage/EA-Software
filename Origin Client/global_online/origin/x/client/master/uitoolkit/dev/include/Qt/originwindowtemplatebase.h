#ifndef ORIGINWINDOWTEMPLATEBASE_H
#define ORIGINWINDOWTEMPLATEBASE_H

#include <QWidget>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Origin {
namespace UIToolkit {

/// \brief Provides interface for window template. Used inside OriginWindow
class UITOOLKIT_PLUGIN_API OriginWindowTemplateBase : public QWidget, public OriginUIBase
{
    Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent Pointer to the QWidget that is the parent of this template
    OriginWindowTemplateBase(QWidget* parent = 0);
    /// \brief Destructor
    ~OriginWindowTemplateBase();

    // Content
    /// \brief Sets the content of the template.
    /// \param content - The widget content to be displayed in the template.
    virtual void setContent(QWidget* content) = 0;
    /// \brief Gets the content widget. Returns NULL if the message box doesn't have content.
	virtual QWidget* getContent() const = 0;


protected:
    void paintEvent(QPaintEvent* event);
    bool event(QEvent* event);
    QWidget* getSelf() {return this;}
    QString& getElementStyleSheet();
    void prepForStyleReload();
    static QString mPrivateStyleSheet;
};
}
}
#endif