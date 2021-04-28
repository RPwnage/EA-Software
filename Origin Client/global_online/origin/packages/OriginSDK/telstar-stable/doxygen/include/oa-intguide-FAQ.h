/// \page oa-intguide-FAQ Origin Integration FAQ
/// \brief This page provides answers to some of the most Frequently Asked Questions about integrating the Origin SDK.
/// 
/// <hr>
/// 
/// <b>Q:</b> Throughout the Origin SDK documentation, there are references to the "local user" or "default user"; what is the
/// the significance of these terms?
/// 
/// <b>A:</b> Most Origin functions require a user identifier as the first parameter. This allows future support for multiple
/// users on the same device, but multiple users are not supported at this time. The "local" or "default" user,
/// therefore, is the user on the local PC or console.
/// 
/// <hr>
/// 
/// <b>Q:</b> OriginGetDefaultUser() always returns 0
/// 
/// <b>A:</b> The OriginGetDefaultUser() value gets updated after a call to OriginGetProfile(). If you were not logged in at the 
/// time when you called OriginStartup, the value for OriginGetDefaultUser() will remain 0.<br>
/// To fix this you need to intercept the ORIGIN_EVENT_LOGIN event, and call OriginGetProfile as a response.
/// 
/// 
/// <hr>
/// <b>Q:</b> What user should I use to fill in the argument of functions that require a user?
/// 
/// <b>A:</b> Use \ref OriginGetDefaultUser to get the user ID.
/// 
/// <hr>
/// <b>Q:</b> How to get an AuthCode to use in your OAuth2.0 flow?
/// 
/// <b>A:</b> Use \ref OriginRequestAuthCode or \ref OriginRequestAuthCodeSync to get an authcode. 
/// Use this authcode on the server to authenticate against Nucleus https://accounts.ea.com/connect/token API.
/// 
/// <hr>
/// 
/// <b>Q:</b> Why are there synchronous and asynchronous version for many of the SDK functions?
/// 
/// <b>A:</b> The various queries and requests that you make can be returned immediately or the information may not be returned
/// instantly. In situations where the information is not available immediately, two approaches to handling queries and
/// requests are provided: synchronous and asynchronous methods. Synchronous methods wait for an immediate return of
/// the information; asynchronous methods request or query for information and then perform a callback to get the
/// information later. Asynchronous calls have the advantage that thet do not block the execution thread from which
/// they are called. Also, they are ideally suited to UI design, in which asynchronous events, such as button presses
/// and external messages, are being received. See the \ref syncvsasync section in the Design Patterns page for a code
/// example of this.
/// 
/// <hr>
/// 
/// <b>Q:</b> What do the functions named OriginQuery&lt;Something&gt; (such as OriginQueryFriends and OriginQueryPresence) do?
/// 
/// <b>A:</b> These functions enumerate lists of objects. They are all encoded in the same general way:
/// <ol>
/// 	<li>Call the basic OriginQuery function to get an enumeration handle and determine the return buffer size.</li>
/// 	<li>Allocate a buffer of appropriate size based on the *pBufLen value returned from OriginQuery.</li>
/// 	<li>Call either OriginEnumerateSync or OriginEnumerateAsync depending on whether blocking or non-blocking is
///       desired.</li>
/// 	<li>Wait for the data to transfer.</li>
/// 	<li>Call OriginDestroyHandle to release the handle.</li>
/// </ol>
/// See the \ref enumeratorpattern section in the Design Patterns page for a code example of this.
/// 
/// <hr>
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
