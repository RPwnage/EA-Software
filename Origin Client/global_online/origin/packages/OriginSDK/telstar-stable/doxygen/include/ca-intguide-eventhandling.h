/// \page ca-intguide-eventhandling Event Handling
/// \brief This page describes Origin's event handling, which includes demonstrating how to register for events and how
/// to respond to those events by setting the required data type for each different callback function.
/// 
/// Origin generates a number of predefined events, such as changes in friends' presence status or updates in Chat.
/// Events may occur because the game has called a function which creates an event, or because the user did
/// something in an IGO, or because Origin notified about something that caused it to create an event, such as a
/// change related to another user. Handling these events requires that we register them, which allows us to receive
/// their updates. Responding to them involves having your function called with the information received. 
/// 
/// \section contents Page Contents
/// <ul>
///     <li><a href="#registration">Event Registration</a> - Shows how to register for events.</li>
///     <li><a href="#respondingtoevents">Responding to Events</a> - Shows how to set the appropriate data type for
///         each different callback function.</li>
/// </ul>
/// 
/// \section registration Event Registration
/// 
/// This following code example demonstrates how to register for events.
/// 
/// \include RegisterCallbacks.txt
/// 
/// \section respondingtoevents Responding to Events
/// 
/// Each event has a different callback function, and each of these requires a different data type. We suggest that you
/// use the templates shown in the example below.
/// 
/// \include EventCallback.txt
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
