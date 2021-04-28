
#include "BoxartData.h"

namespace Origin
{

namespace Services
{

namespace Entitlements
{

namespace Parser
{

        BoxartData::BoxartData() :
            mUsesDefaultBoxart(true)
        {

        }

        BoxartData::BoxartData(const BoxartData& other)
        {
            if (this != &other)
            {
                this->mProductId = other.mProductId;
                this->mBoxartSource = other.mBoxartSource;
				this->mBoxartBase = other.mBoxartBase;
                this->mBoxartDisplayArea = other.mBoxartDisplayArea;
                this->mUsesDefaultBoxart = other.mUsesDefaultBoxart;
            }
        }

        BoxartData& BoxartData::operator=(const BoxartData& other)
        {
            if (this != &other)
            {
                this->mProductId = other.mProductId;
                this->mBoxartSource = other.mBoxartSource;
				this->mBoxartBase = other.mBoxartBase;
                this->mBoxartDisplayArea = other.mBoxartDisplayArea;
                this->mUsesDefaultBoxart = other.mUsesDefaultBoxart;
            }

            return *this;
        }
    
        void BoxartData::setProductId(const QString& productId)
        {
            mProductId = productId;
        }

        void BoxartData::setBoxartSource(const QString& boxartSource)
        {
            mBoxartSource = boxartSource;
        }

		void BoxartData::setBoxartBase(const QString& boxartBase)
        {
            mBoxartBase = boxartBase;
        }

        void BoxartData::setBoxartDisplayArea(const QRect& displayArea)
        {
            mBoxartDisplayArea = displayArea;
        }

        void BoxartData::setBoxartDisplayArea(int left, int top, int width, int height)
        {
            mBoxartDisplayArea = QRect(left, top, width, height);
        }

        void BoxartData::setUsesDefaultBoxart(bool usesDefaultBoxart)
        {
            mUsesDefaultBoxart = usesDefaultBoxart;
        }

        QString BoxartData::getProductId() const
        {
            return mProductId;
        }

        QString BoxartData::getBoxartSource() const
        {
            return mBoxartSource;
        }

		QString BoxartData::getBoxartBase() const
        {
            return mBoxartBase;
        }

        QRect BoxartData::getBoxartDisplayArea() const
        {
            return mBoxartDisplayArea;
        }

        bool BoxartData::usesDefaultBoxart() const
        {
            return mUsesDefaultBoxart;
        }
}

}

}

}