//    Copyright © 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: 

#ifndef CUSTOMIZEBOXARTVIEWCONTROLLER_H
#define CUSTOMIZEBOXARTVIEWCONTROLLER_H

#include "CustomizeBoxartView.h"

#include "engine/content/CustomBoxartController.h"
#include "engine/content/Entitlement.h"

#include "services/entitlements/BoxartData.h"
#include "services/plugin/PluginAPI.h"

#include <QObject>

namespace Origin
{
namespace Client
{
    class CustomizeBoxartFlow;

    class ORIGIN_PLUGIN_API CustomizeBoxartViewController : public QObject
    {
        Q_OBJECT

    public:

        CustomizeBoxartViewController(CustomizeBoxartFlow* parent);
        ~CustomizeBoxartViewController();
            
        void initialize();
        void focus();

    signals:

        void boxartChanged(const Services::Entitlements::Parser::BoxartData&);
        void removeBoxart();

    public slots:

        void onShowCustomizeBoxartDialog(Engine::Content::EntitlementRef game);

    private slots:


    private:
        CustomizeBoxartFlow* mParent;
        CustomizeBoxartView* mCustomizeBoxartView;

    };

}
} 

#endif //CUSTOMIZEBOXARTVIEWCONTROLLER_H
