#ifndef FEATURECALLOUTCONTROLLER_H
#define FEATURECALLOUTCONTROLLER_H

#include <QObject>
#include <QMap>

namespace Origin
{
namespace UIToolkit
{
    class OriginCallout;
    class OriginCalloutIconTemplateContentLine;
}

namespace Client
{

struct CalloutSetting
{
    enum Release
    {
        UNDEFINED,
        RELEASE_9_5
    };

    Release mFeatureVersion;
    QString mSettingName;
    QString mBindedObjectName;
    QString mSeriesName;
    
    CalloutSetting(const Release& featureVersion = UNDEFINED, const QString& settingName = "", const QString& parentName = "", const QString& seriesName = "");
};


/// \brief Creates, formats the callout. It logs information about the callouts and sends the telemetry.
/// Serves as a helper for tracks current release callouts. Makes it easy to remember to remove callouts for release iterations.
class FeatureCalloutController : public QObject
{
	Q_OBJECT
	
public:
    FeatureCalloutController();
    ~FeatureCalloutController();
    UIToolkit::OriginCallout* showCallout(const QString& settingName, const QString& blueTitle, const QString& title, const QString& message);
    UIToolkit::OriginCallout* showCallout(const QString& settingName, const QString& blueTitle, const QString& title, QWidget* content = NULL);
    UIToolkit::OriginCallout* showCallout(const QString& settingName, const QString& blueTitle, const QString& title, QList<UIToolkit::OriginCalloutIconTemplateContentLine*> contentLines);
    UIToolkit::OriginCallout* showCallout(const QString& settingName, UIToolkit::OriginCallout* callout);
    UIToolkit::OriginCallout* callout(const QString& settingName);
    void revertSettingsForRelease(const CalloutSetting::Release& number);
    CalloutSetting calloutSettingBySettingName(const QString& settingName);
    CalloutSetting calloutSettingByBindName(const QString& bindObjName);


private slots:
    void onCalloutDone(const int& result = 1);
    void onCalloutDyingFromParent();

private:
    void writeCalloutSetting(const QString& settingName, const bool& val);

    QList<CalloutSetting> mCalloutSettings;
    QMap<QString, UIToolkit::OriginCallout*> mCalloutMap;
};
}
}
#endif // FEATURECALLOUTCONTROLLER_H