
/*************************************************************************************************/
/*!
    \class TdfEncoder

    This is the base class of all encoders

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <EATDF/codec/tdfencoder.h>
#include <EAStdC/EASprintf.h>

namespace EA
{
namespace TDF
{
/*************************************************************************************************/
/*!
    \brief convertMemberToElement

    Utility method to take a member name and convert it into an element name by doing the 
    following:  strip the leading 'm' or 'm_' member tag, and convert everything to lowercase

    \param[in] memberName - Unmodified member name
    \param[in] buf - Where to write the modified member
    \param[in] buflen - Length in bytes of buf

    \return - true if conversion fits in the provided buffer, false if there isn't enough room or parameters are invalid
*/
/*************************************************************************************************/
bool TdfEncoder::convertMemberToElement(const char8_t *memberName, char8_t* buf, const size_t buflen) const
{
    if (memberName == nullptr || *memberName == '\0' || buf == nullptr || buflen == 0)
    {
        return false;       // Bad parameters
    }

    // Skip the m or m_ member tag.  A single lowercase 'm' may not indicate a member tag.  But if
    // it's followed by an underscore or a capital letter, it is a tag.  If the tag precedes a number,
    // leave it as numbers cannot be the first characters of XML elements.
    //
    // Examples:
    //   "m_SomeName" becomes "somename" ("m_" tag removed)
    //   "mSomeName" becomes "somename" ("m" tag removed)
    //   "mask" becomes "mask" (no tag, "m" not followed by underscore or an uppercase letter)
    //   "Mask" becomes "mask" (no tag, same reason as above)
    //   "m_2Dim" becomes "m_2dim" (can't strip tag, "2dim" is invalid)
    const char8_t* tmpMemberName = memberName;
    if (tmpMemberName[0] == 'm')
    {
        if (tmpMemberName[1] >= 'A' && tmpMemberName[1] <= 'Z')
        {
            ++tmpMemberName;
        }
        else if (tmpMemberName[1] == '_')
        {
            tmpMemberName += 2;
        }
    }

    // Now copy the member into our new storage and make it lowercase as well.
    uint32_t len = 0;
    for ( ; len < buflen-1 ; ++len)
    {
        if (len == 0)
            buf[len] = static_cast<char8_t>(tolower(tmpMemberName[len]));
        else
            buf[len] = tmpMemberName[len];

        if (buf[len] == '\0')
        {
            break;          // All done
        }
    }

    if (len == 0)
    {
        // After tag stripping, there must be nothing left.  Go back and use the member name in its entirety, which
        // we know is either "m" or "m_".  Such is life.
        EA::StdC::Snprintf( buf, buflen, memberName);
    }
    else if (len == buflen && tmpMemberName[len] != '\0')
    {
        // Passed in buffer was too small
        return false;
    }

    return true;
}

} //namespace TDF
} //namespace EA
