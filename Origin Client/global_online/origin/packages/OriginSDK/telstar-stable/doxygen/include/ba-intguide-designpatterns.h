/// \page ba-intguide-designpatterns Design Patterns
/// \brief This page discusses Origin design patterns:
/// <ul>
///     <li><a href="#syncvsasync">Using Synchronous vs. Asynchronous Queries</a> - An explanation of synchronous and
///         asynchronous methods.</li>
///     <li><a href="#enumeratorpattern">Enumerator Pattern</a> - An introduction to Origin enumeration.</li>
/// </ul>
/// 
/// \section syncvsasync Using Synchronous vs. Asynchronous Queries
/// 
/// The various queries and requests that you make may or may not be returned immediately. In situations where the
/// information is not available immediately, two approaches to handling queries and requests are provided: synchronous
/// and asynchronous methods. Synchronous methods wait for the information to be returned before doing any other
/// processing. Asynchronous methods query for the information and provide a callback function and a context pointer.
/// There is no return, so the calling thread can proceed with other processing. When the asynchronously called
/// function has the requested result, it sets the contents of the context pointer and calls the callback function.
/// The requested information can then be retrieved and processed. Asynchronous calls have the advantage that thet do
/// not block the execution thread from which they are called. Also, they are ideally suited to UI design, in which
/// asynchronous events, such as button presses and external messages, are being received.
/// 
/// <img alt="Synchronous vs. Asynchronous Calls" title="Synchronous vs. Asynchronous Calls" src="IntGuide2DesignPatternsSyncVAsyncInteractions.png"/>
/// 
/// The following code example illustrates a sychronous approach.
/// 
/// \include exampleSynchronousCall.txt
/// 
/// This next code example illustrates an asychronous approach.
/// 
/// When calling an asynchronous function, you must pass it a callback function (ticketCallback) to get the data when
/// it is available, and pass it a context pointer (g_TicketReceived) to update via the callback function. This can be
/// used to indicate the callback function was called. The application continues execution following the asynchronous
/// call. When the data is available: <ol>
/// 	<li>The callback function ticketCallback is called.</li>
/// 	<li>The callback function sets the contents of the context pointer (g_TicketReceived).</li>
/// </ol>
/// \include exampleAsynchronousCall.txt
/// 
/// \section enumeratorpattern Enumerator Pattern
/// 
/// There are functions named OriginQuery&lt;Something&gt; (such as OriginQueryFriends, OriginQueryPresence, etc.), which
/// enumerate lists of objects. They are all encoded in the same general way:
/// <ol>
/// 	<li>Call the basic OriginQuery function to get an enumeration handle and determine the return buffer size.</li>
/// 	<li>Allocate a buffer of appropriate size based on the *pBufLen value returned from OriginQuery.</li>
/// 	<li>Call either OriginEnumerateSync or OriginEnumerateAsync depending on whether blocking or non-blocking is
///       desired.</li>
/// 	<li>Wait for the data to transfer.</li>
/// 	<li>Call OriginDestroyHandle to release the handle.</li>
/// </ol>
/// 
/// All functions return an integer result. A negative value is an error (see ORIGIN_ERROR_xxx for definitions), while
/// non-negative numbers indicate success. A few functions (where explicitly noted), return a positive value to reflect
/// a function status value.
/// 
/// The following example shows the encoding for the OriginQueryFriends feature.
/// 
/// \include exampleEnumerator.txt
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
