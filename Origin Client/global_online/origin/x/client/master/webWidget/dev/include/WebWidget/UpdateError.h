#ifndef _UPDATEERROR_H
#define _UPDATEERROR_H

namespace WebWidget
{

///
/// Enumeration indicating the result of an attempted update
///
enum UpdateError
{
    ///
    /// Update completed successfully
    ///
    UpdateNoError = 0,

    ///
    /// Download of the update failed
    ///
    UpdateDownloadFailedRemote,

    ///
    /// Unable to create write to our temporary file
    ///
    UpdateDownloadFailedLocal,

    ///
    /// Unable to unpack the update package
    ///
    UpdateUnpackFailed,

    ///
    /// Signature verification of the update failed at unpack time
    ///
    UpdateInitialSignatureVerificationFailed,

    ///
    /// Update was already installed
    ///
    UpdateAlreadyInstalled
};

}

#endif

