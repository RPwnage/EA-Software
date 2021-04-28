///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Server.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_SERVER_H
#define EAPATCHCLIENT_SERVER_H


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Base.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
    #pragma warning(disable: 4275)  // non dll-interface class used as base for dll-interface class.
#endif


namespace EA
{
    namespace Patch
    {
        /// Server
        ///
        /// Represents a patch server and the information needed to contact it.
        /// This class is not intended to be a singleton, nor is it intended to 
        /// be used by only client at a time. Rather this class merely holds information
        /// about how to contact a server, and this information is typically constant
        /// over the course of an application's lifetime.
        /// This class does not have Error state, as it is merely a container which doesn't 
        /// execute fallable actions.
        /// This class supports C++ copy semantics. So you can copy construct and assign instances
        /// of Server with each other.
        ///
        class EAPATCHCLIENT_API Server
        {
        public:
            Server();
            Server(const Server& other);
            virtual ~Server();

            Server& operator=(const Server& other);
  
            // Deprecated. Do not use.
            const char8_t* GetURLBase() const;
            void           SetURLBase(const char8_t* pURLBase);

            // Deprecated. Use PatchDirectoryRetriever::Get/SetPatchDirectoryURL instead.
            const char8_t* GetPatchDirectoryURL() const;
            void           SetPatchDirectoryURL(const char8_t* pURL);

        protected:
            String msURLBase;       // e.g. https://patch.ea.com:1234/
            String msDirectoryURL;  // e.g. https://patch.ea.com:1234/FIFA/2014/PS3/PatchDirectory.eaPatchDir
        };


        /// Gets and sets the global Server singleton. The Server class isn't required to be a
        /// singleton, though there is one global instance that's typically used.
        /// To consider: rename this to GetDefaultServer, as other similar global functions use  
        /// the term 'default' in their names.
        EAPATCHCLIENT_API Server* GetServer();
        EAPATCHCLIENT_API void    SetServer(Server* pServer);


    } // namespace Patch

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard



 




