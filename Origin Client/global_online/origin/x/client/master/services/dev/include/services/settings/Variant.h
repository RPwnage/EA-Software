#ifndef SETTINGSMANAGER_VARIANT_H
#define SETTINGSMANAGER_VARIANT_H

#include <limits>
#include <QDateTime>
#include <QVariant>
#include <QString>
#include <QStringList>
#include <QDate>
#include <QTime>
#include <QUrl>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        /// This variant class is more restrictive than QVariant in terms
        /// of the type conversions it allows (QVariant allows conversion
        /// between strings and numbers in both directions, this does not).
        /// This class is less restrictive than QVariant in how it handles 
        /// conversions through conversion operators. These operators trigger 
        /// asserts on invalid conversions, no error reporting is done in 
        /// production. In order to avoid creating a loop-holes, this class 
        /// is not derived from QVariant, and it does not contain a conversion 
        /// operator to QVariant. However, it does contain an explicit 
        /// toQVariant() getter as well as a type() function returning the 
        /// QVariant type. To further make the two types distinct, the
        /// constructor from QVariant is explicit. 
        class ORIGIN_PLUGIN_API Variant
        {
        public:
            enum Type {
                Invalid     = QVariant::Invalid,
                Bool        = QVariant::Bool,
                Int         = QVariant::Int,
                ULongLong   = QVariant::ULongLong,
                String      = QVariant::String,
                StringList  = QVariant::StringList,
                LongLong    = QVariant::LongLong,
                Float       = QVariant::Double
            };
            Variant()                          : m_data()     {}
            Variant( const Variant& val )      : m_data(val.m_data)  {}
            Variant operator=(Variant const& val);

            bool operator==(Variant const& val) const;
            bool operator!=(Variant const& val) const;

            explicit Variant( const QVariant& val ) : m_data(val)  {}
            Variant::Type type() const { return (Variant::Type) m_data.type(); }
            QVariant const& toQVariant() const { return m_data; }
            bool isNull() const { return m_data.isNull(); }
            QString toString() const { return m_data.toString(); }
            QStringList toStringList() const { return m_data.toStringList(); }
            
            // If we're treated like a string implicitly convert to a string
            QString arg(const QString &a) { return m_data.toString().arg(a); }

            Variant( int val )                  : m_data(val)  {}
            Variant( qulonglong val )           : m_data(val)  {}
            Variant( qlonglong val )            : m_data(val)  {}
            Variant( bool val )                 : m_data(val)  {}
            Variant( const QString & val )      : m_data(val)  {}
            Variant( const char * val )         : m_data(val)  {}
            Variant( const QStringList & val )  : m_data(val)  {}
            Variant( float val )                : m_data(val)  {}
            Variant( double val )               : m_data(val)  {}

            operator int() const;
            operator qulonglong() const;
            operator qlonglong() const;
            operator bool() const;
            operator QString() const;
            operator float() const;
            operator double() const;

        private:
            QVariant m_data;
        };
    }
}

ORIGIN_PLUGIN_API bool operator==(QVariant const& q, Origin::Services::Variant const& v);
ORIGIN_PLUGIN_API bool operator!=(QVariant const& q, Origin::Services::Variant const& v);

ORIGIN_PLUGIN_API const QString operator+(Origin::Services::Variant const& v, const char *str);
ORIGIN_PLUGIN_API const QString operator+(Origin::Services::Variant const& v, QString const &str);

#endif // SETTINGSMANAGER_VARIANT_H
