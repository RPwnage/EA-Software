#ifndef _CHATMODEL_CONFIGURATION_H
#define _CHATMODEL_CONFIGURATION_H

namespace Origin
{
namespace Chat
{
    ///
    /// Represents configuration for an XMPP connection
    ///
    /// Once a configuration has been supplied to create a connection it is 
    /// immutable for the lifetime of the connection
    ///
    class Configuration
    {
    public:
        ///
        /// Controls TLS negotiation behavior
        ///
        enum TlsDisposition
        {
            ///
            /// TLS is never used. The connection is aborted if the server requires it.
            ///
            TlsDisabled,
            
            ///
            /// TLS is used if the server supports it
            ///
            TlsPreferSecure,

            ///
            /// TLS is required. The connection is aborted if the server does not support it.
            ///
            /// This is the default
            ///
            TlsRequireSecure
        };

        ///
        /// Creates a default configuration
        ///
        Configuration() :
            mTlsDisposition(TlsRequireSecure),
            mIgnoreInvalidCert(false)
        {
        }
 
        ///
        /// Sets the TLS disposition
        ///
        void setTlsDisposition(TlsDisposition disposition)
        {
            mTlsDisposition = disposition;
        }

        ///
        /// Returns the TLS disposition
        ///
        TlsDisposition tlsDisposition() const
        {
            //if we have the ignoreSSLCert set in the EACore.ini lets return TlsDisabled always so we can connect even when 
            //there is a bad certificate
            if(mIgnoreInvalidCert)
            {
                return TlsDisabled;
            }

            return mTlsDisposition;
        }
 
        ///
        /// Sets if we should ignore invalid server TLS certificates
        ///
        /// This should only be set to true for debugging purposes
        ///
        void setIgnoreInvalidCert(bool ignore)
        {
            mIgnoreInvalidCert = ignore;
        }

        ///
        /// Returns if we should ignore invalid server TLS certificates
        ///
        bool ignoreInvalidCert() const
        {
            return mIgnoreInvalidCert;
        }

    private:
        TlsDisposition mTlsDisposition;
        bool mIgnoreInvalidCert;
    };
}
}

#endif
