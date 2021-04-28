#ifndef __SCRATCH_PAD_H__
#define __SCRATCH_PAD_H__

#include <QMap.h>
#include <QVariant.h>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Engine
    {
            /// \brief A class that contains a map of key value pairs.
            class ORIGIN_PLUGIN_API ScratchPad
            {
            public:
	            /// \brief Constructor
	            ScratchPad();

	            /// \brief Get the value associated with the name
	            /// \param name The name of the variable you want the value of.
	            /// \return The value of the variable. It will return an empty QVariant if the variable is not found.
	            QVariant GetValue(QString name) const;
	
	            /// \brief Set/Create a variable with name, and value 
	            /// \param name The name of the variable
	            /// \param val The value of the variable
	            /// \return Returns true if the variable is created, or the value of the existing variable changed.
	            bool SetValue(QString name, QVariant val);

	            /// \brief Checks whether a variable with name already exists.
	            /// \param name The name of the variable to check
	            bool Exists(QString name) const;

	            /// \brief Indicator whether a value in the scratchpad has changed.
	            /// \return Changed value.
	            bool HasChanged(){return m_bChanged;}

	            /// \brief Clear the changed value
	            void ClearChanged(){m_bChanged = false;}

            private:
	            bool m_bChanged;
	            QMap<QString, QVariant> m_KeyValues;
            };
    }
}

#endif // __SCRATCH_PAD_H__