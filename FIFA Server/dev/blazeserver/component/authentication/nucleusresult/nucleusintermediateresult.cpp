/*************************************************************************************************/
/*!
    \file nucleusintermediateresult.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class NucleusIntermediateResult
        Holds the results for the DIRECT GET of Personas from Nucleus.

    \notes

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/nucleusresult/nucleusintermediateresult.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief <MethodName>

    <Description of the method>

    \param[in]  <varname> - <Description of variable>
    \param[out] <varname> - <Description of variable>

    \return - <Description of return value - Delete this line if returning void>

    \notes
        <Any additional detail including references.  Delete this section if
        there are no notes.>
*/
/*************************************************************************************************/
void NucleusIntermediateResult::clearAll()
{
    // Clears and resets stuff.
    clearError();
    mURI[0] = 0;

} // clearAll

void NucleusIntermediateResult::setAttributes(const char8_t* fullname,
        const Blaze::XmlAttribute* attributes, size_t attributeCount)
{
    // Do nothing.
} /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

void NucleusIntermediateResult::setValue(const char8_t* fullname, const char8_t* data, size_t dataLen)
{
    size_t copySize = 0;

    // Check buffer size
    copySize = (uint32_t)sizeof(mURI);

    if (dataLen + 1 <= copySize)
    {
        blaze_strsubzcpy(mURI, copySize, data, dataLen);
    }
    else
    {
        WARN_LOG("[NucleusIntermediateResult] uri was larger than buffer!");
        // Should I crash or not???
    } // if
    return;

} // setValue

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

} // Authentication
} // Blaze
