#include "services/settings/Variant.h"
#include "services/debug/DebugService.h"

namespace Origin
{
    namespace Services
    {
        Variant Variant::operator=(Variant const& val)
        { 
            if ( *this != val )
                m_data = val.m_data; 
            return *this; 
        }

        bool Variant::operator==(Variant const& val) const
        { 
            return m_data == val.m_data; 
        }

        bool Variant::operator!=(Variant const& val) const
        { 
            return m_data != val.m_data; 
        }
        Variant::operator int() const
        {
            if ( m_data.isNull() )
                return 0;

            if ( m_data.type() == QVariant::Int )
                return m_data.toInt();

            if ( m_data.type() == QVariant::ULongLong /*&& m_data.toULongLong() <= MAX_INT*/ )
                return m_data.toInt();

            if ( m_data.type() == QVariant::Bool )
                return m_data.toInt();

            ORIGIN_ASSERT_MESSAGE(false, "Cannot cast this setting value to int");
            return 0;
        }

        Variant::operator qulonglong() const
        {
            if ( m_data.isNull() )
                return 0;

            if ( m_data.type() == QVariant::ULongLong )
                return m_data.toULongLong();

            if ( m_data.type() == QVariant::Bool )
                return m_data.toULongLong();

            if ( m_data.type() == QVariant::Int && m_data.toInt() >= 0 )
                return m_data.toULongLong();

            ORIGIN_ASSERT_MESSAGE(false, "Cannot cast this setting value to qulonglong");
            return 0;
        }
        Variant::operator bool() const
        {
            if ( m_data.isNull() )
                return false;

            if ( m_data.type() == QVariant::Bool )
                return m_data.toBool();

            if ( m_data.type() == QVariant::Int )
                return m_data.toBool();

            if ( m_data.type() == QVariant::ULongLong )
                return m_data.toBool();

            if ( m_data.type() == QVariant::String )
                return m_data.toBool();

            ORIGIN_ASSERT_MESSAGE(false, "Cannot cast this setting value to bool");
            return false;
        }

        Variant::operator QString() const
        {
            if ( m_data.isNull() )
                return "";

            if ( m_data.type() == QVariant::String )
                return m_data.toString();

            ORIGIN_ASSERT_MESSAGE(false, "Cannot cast this setting value to QString");
            return "";
        }

        Variant::operator float() const
        {
            if ( m_data.isNull() )
                return 0.0f;

            switch ( m_data.type())
            {
            case QVariant::Double:
            case QVariant::Int:
            case QVariant::LongLong:
            case QVariant::ULongLong:
                return m_data.toFloat();
            case QMetaType::Float:
                return m_data.value<float>();
            default:
                break;
            }

            ORIGIN_ASSERT_MESSAGE(false, "Cannot cast this setting value to float");
            return 0.0f;
        }

        Variant::operator double() const
        {
            if ( m_data.isNull() )
                return 0.0;

            switch ( m_data.type())
            {
            case QVariant::Double:
            case QVariant::Int:
            case QVariant::LongLong:
            case QVariant::ULongLong:
                return m_data.toFloat();
            case QMetaType::Float:
                return m_data.value<float>();
            default:
                break;
            }

            ORIGIN_ASSERT_MESSAGE(false, "Cannot cast this setting value to double");
            return 0.0;
        }
    }
}

bool operator==(QVariant const& q, Origin::Services::Variant const& v)
{
    return v.toQVariant() == q;
}
bool operator!=(QVariant const& q, Origin::Services::Variant const& v)
{
    return v.toQVariant() != q;
}

const QString operator+(Origin::Services::Variant const& v, const char *str)
{
    return v.toString() + str;
}

const QString operator+(Origin::Services::Variant const& v, QString const &str)
{
    return v.toString() + str;
}
