/// \page ha-intguide-friends Integrating the Friends Feature
/// \brief This page discusses the integration of the Origin Friends feature, which includes functions that identify a
/// user's friends and that query for information on those friends.
/// 
/// You can use the Friends API to get the friends list for processing or to display the information in the in-game overlay:
/// 
/// <ul>
/// 	<li>You can query the friends list for use by the game (OriginQueryFriends).</li>
///     <li>You can use the friends list to display the information in the in-game overlay, including: <ul>
/// 	    <li>Showing the friends list In-Game Overlay (IGO) (OriginShowFriendsUI).</li>
/// 	    <li>Reporting player feedback about another user by directly showing the feedback IGO (OriginShowFeedbackUI).</li>
/// 	    <li>Sending a friend request to a user by directly showing the friend request IGO (OriginRequestFriendUI). Players
///          can add a custom message to their request from this screen. The user must explicity use the IGO to send a 
///          request.</li>
///     </ul></li>
/// </ul>
/// 
/// The following code example illustrates how a list of users is checked to see which are friends of the current user.
/// 
/// \include exampleAreFriends.txt
/// 
/// The next code example illustrates the use of the Friends API to get a complete list of a user's friends for use by the game.
/// 
/// \include exampleFriends.txt
/// 
/// The next code example illustrates how to display a user's friends information in the in-game overlay.
/// 
/// \include exampleFriendsIGO.txt
/// 
/// The next code example illustrates how to display a dialog box in the in-game overlay that allows a user to provide
/// feedback about a friend.
/// 
/// \include exampleFeedbackIGO.txt
/// 
/// The following code example shows how to register to receive the friend's updates.
/// 
/// \include exampleFriendsEvent.txt
/// 
/// This next code example shows how to encode Origin Friends event handling.
/// \note It is important to cast the data in the template as shown in the example below.
/// 
/// \include FriendsEventCallback.txt
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
