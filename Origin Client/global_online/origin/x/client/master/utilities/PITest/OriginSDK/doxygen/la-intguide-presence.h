/// \page la-intguide-presence Integrating the Presence Feature
/// \brief This page discusses the integration of the Origin Presence feature, which sets and gets the local user's
/// presence status, gets the presence status of the local user's friends, and allows the local user to subscribe and
/// unsubscribe to others' presence status.
/// 
/// The Origin client already sets a user's basic presence, which indicates whether they are offline, online, idle, or
/// playing game X. The Presence features also allows you to:
/// 
/// <ul>
/// 	<li>Set rich presence info (OriginSetPresence), which could include more context about what the player is doing
///       (such as "Winning 3-1 in CTF mode in the Jungle"). This includes the game session ID so friends can click the
///       <b>Join</b> button to jump into the same session.</li>
/// 	<li>Retrieve the current user's presence (OriginGetPresence).</li>
/// 	<li>Send a one time query for the current status for a list of players (OriginQueryPresence).</li>
/// 	<li>Subscribe (OriginSubscribePresence) and unsubscribe (OriginUnsubscribePresence) to presence changes for a list
///       of players. This is not needed for friends, who are automatically subscribed, so this could be used for clan
///       members or similar situations.</li>
/// </ul>
/// 
/// The next code example illustrates how to get and set the default user's presence information. This allows others to
/// know what this user is doing.
/// \note It is important to cast the event callback data in the template as shown in the example below.
/// 
/// \include exampleSetGetPresence.txt
/// 
/// The next code example illustrates how to get the presence information of the default user's friends, which is
/// accomplished using two techniques: <ol>
///     <li>Query a user's presence status directly using OriginQueryPresence.</li>
///     <li>Subscribe to changes in a user's presence status using OriginSubscribePresence.</li>
/// </ol>
/// 
/// \include exampleOtherUsersPresence.txt
/// 
/// The next code example illustrates how the game receives a response to querying a friend's presence, as was shown in
/// the preceding code example.
/// 
/// \include PresenceResultCallback.txt
/// 
/// The following code example shows how to register to receive a user's friend's Presence status updates. Changes to
/// the presence status of those users to which we subscribed are notified to us by the Presence Event. We must
/// register for these events, which is normally done at start up, but is shown here for clarity.
/// 
/// \include examplePresenceEvent.txt
/// 
/// This next code example shows how to encode Origin Presence event handling.
/// \note It is important to cast the data in the template as shown in the example below.
/// 
/// \include PresenceEventCallback.txt
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
