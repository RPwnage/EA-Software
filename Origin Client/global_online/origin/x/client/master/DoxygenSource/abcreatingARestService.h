/// \page abcreatingARestService Creating a REST Service
///
/// To create a new REST service, you much develop it in two parts:
/// <UL>
/// <LI>The Service client (the request).</LI>
/// <LI>The Response client (the response).</LI>
/// </UL>
///
/// Most of the basic, generic functions of both the Service and the response are implemented in
/// their base classes. Use the following instructions when implementing new REST services.
///
/// \section serviceclient The Service Client
///
/// <b>Creation</b>
/// <OL>
/// <LI>Create your service client by deriving public from <KBD>OriginServiceClient</KBD>.</LI>
/// <LI>The Service client needs to be named <KBD>XXXServiceClient</KBD> where <KBD>XXX</KBD> is
/// the service this client is to provide, for example: <KBD>AvatarServiceClient</KBD>.</LI>
/// <LI>Make the CTOR explicit and private.</LI>
/// </OL>
///
/// <b>Obtaining Responses (<KBD>XXXServiceResponse</KBD>) from the <KBD>XXXServiceclient</KBD></b>
/// <OL>
///     <LI>Any function that returns a response has to be public and static.</LI>
///     <LI>These static functions:
///     <OL TYPE="a">
///         <LI>Create a <KBD>XXXServiceClient</KBD> on the heap via a (smart pointer)
///             <i>singleton</i>.<BR>
///         <b>Note:</b> We do this to avoid instantiating a new client each time we need access
///             to any REST services.</LI>
///         <LI>Call the relevant member function on the newly created singleton instance of the
///             <KBD>XXXServiceClient</KBD>.</LI>
///         <LI>Return a <KBD>XXXServiceResponse</KBD> pointer to the caller.</LI>
///     </OL> </LI>
///     <LI>The user will then call the <KBD>XXXSeviceClient</KBD> static function and store the
///         return value in a <KBD>XXXServiceResponse</KBD> pointer.</LI>
///     <LI>This object will contain the required interfaces to allow the user access to the
///         retrieved data.</LI>
/// </OL>
///
/// \section responseclient The Response Client
/// <OL>
///     <LI>Create your response client, derive public from <KBD>OriginServiceResponse</KBD>.</LI>
///     <LI>The Service needs to be named <KBD>XXXServiceResponse</KBD> (same rules as the
///         client).</LI>
///     <LI>Make the CTOR explicit and public, taking a <KBD>QNetworkReply*</KBD> as a
///         parameter.</LI>
///     <LI>Implement <KBD>bool parseSuccessBody(QIODevice*)</KBD>.</LI>
///     <LI>Implement the required interfaces to access the retrieved data from the response. 
///         <OL TYPE="a">
///             <LI>This data is parsed and interpreted within
///                 <KBD>parseSuccessBody(QIODevice*)</KBD>.</LI>
///         </OL> </LI>
/// </OL>
/// 
/// There are many examples of REST clients, start with ChatService* and go from there.
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
/// 