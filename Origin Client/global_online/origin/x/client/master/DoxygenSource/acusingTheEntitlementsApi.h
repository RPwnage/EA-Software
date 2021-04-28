/// \page acusingTheEntitlementsApi Using the Entitlements API
///
/// The Entitlment REST API implements its own persistent caching mechanism, allowing users to load their
/// entitlements while offline. The entitlement cache stores the server replies exactly as received from
/// the server. While not currently used, the design anticipates the implementation of support for
/// delta-requests, where the server will return two kinds of entitlement data, the entire data set
/// (called consolidated), and the delta that includes only the changes to the data since a given time
/// point. The offline entitlement cache will therefore contain one consolidated entitlement set and
/// zero or more delta entitlement sets.
/// 
/// \section howtocall How to call
/// 
/// When retrieving entitlement data from the network or cache, use the EntitlementServiceClient and
/// EntitlementServiceResponse classes the way you would use any other of our REST APIs, i.e. call the
/// static response factory methods on the EntitlementServiceClient, connect slots to the signals on the
/// EntitlementServiceResponse coming back, and handle success and failure in the slots.
/// 
/// \section lowlevel Low level
/// 
/// This system consists of two levels, a lower level dealing with the network only, and a higher level
/// that deals with the network and the entitlement cache.
/// 
/// The lower level consists of EntitlementXMLServiceClient and EntitlementXMLServiceResponse, both
/// in the Origin::Services::Entitlements::Raw namespace. These classes implement the non-cached network
/// access (other than HTTP caching, which is ignored in this discussion) needed to retrieve entitlement
/// data from the servers. These classes follow the design of all the other REST services, using the base
/// classes OriginServiceClient and OriginServiceResponse, respectively. EntitlementXMLServiceClient
/// builds the network requests and talks to the network through its base class, returning
/// EntitlementXMLServiceResponse instances. EntitlementXMLServiceResponse retrieves the entitlement
/// data from the network reply, but does not parse or process it in any way.
/// 
/// \section highlevel High level
/// 
/// The higher level consists of EntitlementServiceClient and EntitlementServiceResponse in the
/// Origin::Services::Entitlements namespace. These classes implement the cached access needed to
/// retrieve the entitlement data from local disk entitlement cache or from the network. These do <i>not</i>
/// derive from the REST base classes, but their APIs are designed in the same way, with the service client
/// having static, asynchronous request functions that return response instances from which the result may
/// be retrieved. The request functions take a cache control argument (QNetworkRequest::CacheLoadControl,
/// http://doc.qt.nokia.com/4.7-snapshot/qnetworkrequest.html#CacheLoadControl-enum) which may be used to
/// control whether the entitlement data is pulled from the cache or the network. The
/// EntitlementServiceClient class is simple, creating response instances with the arguments as passed to
/// the request, and asynchronously invoking request() on the response instance. EntitlementServiceResponse
/// is pure abstract with two derived classes for consolidated and delta replies. However, the derived
/// classes are very simple and most of the logic is in the base class and shared between both.
/// 
/// EntitlementServiceResponse::request() uses the cache control flag passed to the constructor to
/// determine whether to initially go to the cache or the network. If the initial request fails, the
/// flag also specifies whether to fall back on the network or cache, respectively. If requesting
/// from the network, the entitlement data is pulled from the lower level
/// EntitlementXMLServiceClient, otherwise it is pulled from the cache. A successful network request
/// causes the received data to be stored in the cache.
/// 
/// \section entitlementcache Entitlement cache
/// 
/// The cache is implemented by EntitlementCache, located in the
/// Origin::Services::Entitlements::Cache namespace. The cache class handles the entitlement data as
/// opaque QByteArray blocks, which it reads and writes to disk using symmetric encryption. A \#define
/// may be used to disable encryption for development work. When the data is read from disk or updated
/// from received network replies, it is also kept in memory, stored within the session using the
/// AbstractSession::ServiceData generic storage. When the user logs in, a slot in the cache reads the
/// data from disk using a path which consists of a platform dependent root directory, hard-coded
/// application specific directories, an environment-dependent directory (omitted if the environment
/// is production), and a per-user directory containing the actual cache files.
/// 
/// The entitlement cache also supports devSupport files. These are clear-text entitlement XML files
/// that are loaded from a directory location one level up from the network cache files, i.e in a
/// environment-dependent but user-independent location. These may be used to support development of
/// content that is not yet present on the server. Support for this features should <i>not</i> be compiled
/// in production builds, at this moment this is ensured by \#ifdef guards around calls to
/// parseDevSupportData() in EntitlementServiceResponse.
/// 
/// \section parsing Parsing
/// 
/// Whether the entitlement data is coming from the server or the disk cache, the further handling of
/// the data is identical. An instance of EntitlementParser is created, this class is located in the
/// Origin::Services::Entitlements::Parser namespace. The parser is given the entitlement data and the
/// content type as received from the server. The content type may be used in the future to support
/// different entitlement formats, e.g. different formats of the entitlement signature. Most of the
/// logic in the parser class has to do with error handling, much of which is environment specific, so
/// that features related to the entitlement format may be rolled out in some environments and not
/// others. This design has been driven primarily by the needs of developing entitlement signatures.
/// The actual parsing is carried out by the EntitlementXmlImport class. It returns a vector of
/// EntitlementServerData holding the entitlement data and an instance of EntitlementSecurityData
/// holding the signature.
/// 
/// \section signatureverification Signature verification
/// 
/// If the parsing is successful, the signature is verified against the entitlement data. The
/// signature is verified by the class SignatureVerifier which is located in the
/// Origin::Services::Entitlements::Parser namespace. Signature verification consists of the
/// following steps:
/// <OL>
///     <LI>Retrieve the signature from the EntitlementSecurityData</LI>
///     <LI>Decrypt the signature using a public key that is compiled into the %Origin client</LI>
///     <LI>Using the data from the EntitlementServerData instances, compute the signing string</LI>
///     <LI>Compute the Sha1 hash of the signing string</LI>
///     <LI>Compare the hash to the decrypted signature. If they are identical, the signature is valid</LI>
/// </OL>
/// 
/// \section duplicatehandling Duplicate handling
/// 
/// If signature verification succeeds, duplicate entitlements are removed, and the result passed to the 
/// caller through a signal. Note that if duplicates are discarded before signature verification, 
/// verification will fail, since the duplicates were part of the data as it was signed by the server.
/// 
/// There are several reasons why we may have duplicates in the set of entitlements:
/// <UL>
///     <LI>In order to support games under development, special entitlements with originPermissions 
///         attribute values of 1 and/or 2 may be issued. These were previously known as 1102 and 1103 
///         test codes, respectively. Normal entitlements have zero for this attribute. When using 
///         originPermissions, the same account may be entitled to up to three entitlements with the 
///         same product id but different originPermissions values. In such cases, the entitlement with 
///         the largest value of the originPermissions is kept, and the others are discarded. </LI>
///     <LI>There may be entitlements that appears as both CMS and Nucleus entitlements, in which case 
///         the CMS entitlement is kept. When migrated away from Nucleus, this issue will disappear.</LI>
///     <LI>When we support delta entitlement data, the same entitlement may appear both in the 
///         consolidated entitlement data and in one or more of the delta data sets. In this case, 
///         the one in the most recent delta should be kept. Note that this has not yet been implemented.</LI>
/// </UL>
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
///
