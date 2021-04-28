/// \page ea-intguide-singlesignon Integrating Single Sign-on
/// \brief This page discusses the integration of the methods used to retrieve a Nucleus Auth Token for Single Signon purposes.
/// 
/// When launching the game through Origin, one needs to be able to get an identification token in order to log into other 
///	EA services.
/// 
/// The next code example demonstrates how to get an auth token by using the Asynchronous \ref OriginRequestTicket function.
/// In order for the callback to get called one needs to call \ref OriginUpdate in the game loop.
/// 
/// \include exampleAuthToken.cpp
/// 
/// The following code example demonstrates how to get an auth token by using the \ref OriginRequestTicketSync function.
/// 
/// \include exampleAuthTokenSync.cpp
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly

