#include <QObject>
#include <QList>
#include <QString>
#include <QHash>

#include "WebWidget/UpdateError.h"
#include "WebWidget/WidgetLink.h"

#include "engine/login/User.h"
#include "services/plugin/PluginAPI.h"

namespace WebWidget
{
    class WidgetRepository;
    class InstalledWidget;
    class UpdateIdentifier;
    class WidgetPage;
}

namespace Origin
{
namespace Client
{

///
/// High level controller for supporting Web Widgets
///
/// This has two major functions:
/// -# Registering and unregstering our native interfaces from jsinterface
/// -# Handling WebKit's native storage
///
class ORIGIN_PLUGIN_API WebWidgetController : public QObject
{
    Q_OBJECT
public:
    ///
    /// Creates a WebWidgetController singleton
    ///
    static void create();

    static WebWidgetController *instance();

    ///
    /// Loads a web widget with a user-specific authority
    ///
    /// This means local storage, offline storage and offline cache will be scoped to the user and inaccessible
    /// by other instances of the widget for different users
    ///
    bool loadUserSpecificWidget(WebWidget::WidgetPage *page, const QString &name, Engine::UserRef user, const WebWidget::WidgetLink &link = WebWidget::WidgetLink()) const;

    ///
    /// Loads a web widget with a constant authority
    ///
    /// This means local storage, offline storage and offline cache will be shared by all instances of the 
    /// widget
    ///
    bool loadSharedWidget(WebWidget::WidgetPage *page, const QString &name, const WebWidget::WidgetLink &link = WebWidget::WidgetLink()) const;

    ///
    /// Destroys the WebWidgetController singleton
    ///
    static void destroy();

signals:
    void allProxiesLoaded();

private slots:
    void userLoggedIn(Origin::Engine::UserRef user);
    void userLoggedOut(Origin::Engine::UserRef user);

    void updateStarted(const WebWidget::UpdateIdentifier &);
    void updateFinished(const WebWidget::UpdateIdentifier &, WebWidget::UpdateError error);

private:
    bool tracedWidgetLoad(WebWidget::WidgetPage *page, const QString &name, const WebWidget::InstalledWidget &, const WebWidget::WidgetLink &) const;
    void enablePersistentStorage();

    WebWidgetController();
    ~WebWidgetController();

    QList<QString> mUserSpecificInterfaces;
    WebWidget::WidgetRepository *mWidgetRepository;

    QHash<QByteArray, qint64> mUpdateStartMSecsSinceEpoch;
};

}
}
