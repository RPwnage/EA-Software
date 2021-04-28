#ifndef BOXARTDATA_H
#define BOXARTDATA_H

#include <QMetaType>
#include <QRect>
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Services
{

namespace Entitlements
{

namespace Parser
{

class ORIGIN_PLUGIN_API BoxartData
{
    public:

        BoxartData();
        BoxartData(const BoxartData& other);

        BoxartData& operator=(const BoxartData& other);

        void setProductId(const QString& productId);
        void setBoxartSource(const QString& boxartSource);
		void setBoxartBase(const QString& boxartBase);
        void setBoxartDisplayArea(int left, int top, int width, int height);
        void setBoxartDisplayArea(const QRect& displayArea);
        void setUsesDefaultBoxart(bool usesDefaultBoxart);

        QString getProductId() const;
        QString getBoxartSource() const;
        QRect getBoxartDisplayArea() const;
        bool usesDefaultBoxart() const;
		QString getBoxartBase() const;

    private:
    
        QString mProductId;

        QString mBoxartSource;
        QString mBoxartBase;
        QRect mBoxartDisplayArea;
        bool mUsesDefaultBoxart;
};

}

}

}

}

Q_DECLARE_METATYPE(Origin::Services::Entitlements::Parser::BoxartData);

#endif // BOXARTDATA_H
