/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MOCKJOB_H
#define BLAZE_MOCKJOB_H

/*** Include files *******************************************************************************/
#include <string>
#include "framework/system/job.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class MockJob : public Job
{
public:
    MockJob(std::string name) 
        {
            mValue = name; 
        };

    virtual void execute()
    {
        std::ostringstream oss;
        oss << "[processing] >>> " << toString() << " <<<" << std::endl; 
        std::cout << oss.str();
        oss.str("");
    };

    std::string toString() const { return mValue; };

protected:

private:
    std::string mValue;
};

} // Blaze

#endif // BLAZE_MOCKJOB_H

