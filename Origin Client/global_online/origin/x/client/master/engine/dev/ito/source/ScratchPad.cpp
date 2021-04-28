#include "ScratchPad.h"
#include "services/log/LogService.h"

#define ENABLE_VERBOSE 0

namespace Origin
{
    namespace Engine
    {
        ScratchPad::ScratchPad() :
            m_bChanged(false)
        {

        }

        bool ScratchPad::SetValue(QString name, QVariant val)
        {
            if(m_KeyValues.contains(name))
            {
                if(m_KeyValues[name] != val)
                {
#if ENABLE_VERBOSE
                    ORIGIN_LOG_DEBUG << "Changing Variable: " << name.utf16() << ", " << val.toString().utf16();
#endif

                    m_KeyValues[name] = val;
                    m_bChanged = true;
                    return true;
                }
            }
            else
            {
#if ENABLE_VERBOSE
                ORIGIN_LOG_DEBUG << "Creating Variable: " << name.utf16() << ", " << val.toString().utf16();
#endif

                m_KeyValues[name] = val;
                m_bChanged = true;
                return true;
            }
            return false;
        }

        QVariant ScratchPad::GetValue(QString name) const
        {
            if(m_KeyValues.contains(name))
            {
                return m_KeyValues[name];
            }

            return QVariant();
        }


        bool ScratchPad::Exists(QString name) const
        {
            return m_KeyValues.contains(name);
        }

    }
}