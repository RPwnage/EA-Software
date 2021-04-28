/// \page ia-intguide-invites Integrating the Invite Feature
/// \brief This page discusses the integration of the Origin Invite feature, which allows the game to invite other
/// players to join in the default user's game in progress if they are playing on a PC or Mac.
/// 
/// A user's friends can also see the user as "in game" and can come join the game if they already own it,
/// or they can purchase the game through the Origin store if they do not. The feature is available through the
/// Origin SDK on PC and Mac. Accepting an invite passes a parameter to the game to tell it to connect to
/// a given session. Games also use the SDK to manage whether or not users are in a joinable session.
/// 
/// Game invites can be sent either in-fiction or using the Origin game invites UI and have been usability tested.
/// In-fiction invites have shipped with FIFA 12 and 13; the game invites UI has shipped with Mass Effect 3.
/// 
/// <img alt="Origin In-Game Overlay Invites UI" title="Origin In-Game Overlay Invites UI" src="ME3Invite.png"/>
/// <h6>The Origin In-Game Overlay Invites UI</h6>
/// 
/// Users can either join a friend after receiving an invitation, or they can click on the friend to establish a
/// chat session and then click on the "join game" call to action.
/// 
/// Origin 9.3 (due out in July) adds group chat and in-line messaging for game presence events. This makes
/// it both easy for groups of friends to see that someone in their group is in-game and to join their session.
/// 
/// <img alt="Chat friends' game play notifications" title="Chat friends' game play notifications" src="ChatFriendsGamePlayNotifications.png"/>
/// <h6>Chat friends' game play notifications</h6>
/// 
/// \section responsibilities Delineation of Responsibilities between Origin and Game Teams
/// 
/// Game teams are responsible for:
/// <ul>
///     <li>Managing a user's presence, marking whether users are in game or joinable.</li>
///     <li>Building a user interface for in-fiction chat, if desired.</li>
///     <li>Calling OriginShowGameInvites, if using Origin's UI is desired.</li>
///     <li>Handling Origin passing a parameter to tell the game what server to connect
///        to (the contents of the session that users are joining are opaque to Origin).</li>
///     <li>Providing messages to users if a session no longer exists, etc.</li>
///     <li>Providing host or session migration if users need to join a different session.</li>
///     <li>Providing messages to users that they need to enable Origin In-Game if they have it off and try to invoke the
///         OriginShowGameInvites SDK call.</li>
/// </ul>
/// 
/// Origin is responsible for:
/// <ul>
///     <li>Drawing the UI to send game invites if the SDK call is invoked and the user has the IGO turned on.</li>
///     <li>Selling the user the game if they don't own it.</li>
///     <li>Allowing users to install the game if they own it but don't have it installed.</li>
///     <li>Launching the game with a parameter if users accept an invite.</li>
///     <li>Managing chat windows for the user.</li>
///     <li>Showing the call to action to join a game.</li>
/// </ul>
/// 
/// \section sendinvite Sending an Invite
/// 
/// A game can send invites using the SDK or you can choose to use the IGO. The following options are available.
/// An invite can be sent to any UserId, but the samples below use friends.
///																				
/// <ul>
/// 	<li>Send an invite directly from the game (OriginSendInvite) and get an invite that the user has accepted (QueryFriends).</li>
/// 	<li>Show the invite via IGO (OriginShowInviteUI), which lets the user type a custom message with their invite.</li>
/// </ul>
/// 
/// The first code example demonstrates how to send an invite directly from the game.
/// 
/// Invites are sent to individuals or to list of friends. In practice the friends list would be maintained separately.
/// The user would then select a friend to whom to send an invite. For the purpose of this example, a list of friends
/// is generated and an invite is sent to each friend. Two similar flows are possible: <ol>
/// 	<li>Generate a list of friends using the OriginQueryFriends function (in this example,
///         <code> \ref OriginQueryFriendsSync</code>).</li>
/// 	<li>Send invites to the selected friends.</li>
/// 	<li>Then either: <ol>
/// 	    <li>Call QueryFriends again until <code>OriginFriendT.FriendState == MUTUAL</code>, or</li>
/// 	    <li>When a FriendsEvent is received, check if <code>OriginFriendT.FriendState == MUTUAL</code>.</li>
///     </ol>
/// </ol>
/// Using method 3.2 involves the least processing. In this case, we register to receive updates about our friends so
/// that we will know when the FriendState changes.
/// \include FriendsEventCallback2.txt
/// 
/// This next code example demonstrates how to send an invite via the In-Game Overlay.
/// 
/// Again, invites are sent to individuals or list of friends, and in practice, the friends list would be maintained
/// separately. The user would then select a friend to whom to send an invite. For the purpose of this example a list
/// of friends is generated and an invite In-Game Overlay dialog is invoked with a list of friends. The user can then
/// select who to send invites to. The state of the friend accepting the invite can be monitored using the call back
/// discussed for case 3.2 in the example above.
/// \include exampleInviteIGO.txt
/// 
/// \section receiveinvite Receiving an Invite
/// 
/// This next code example demonstrates how to allow a user to receive an invite.
/// 
/// \include InviteEventCallback.txt
/// 
/// \section knownissue Known Issue
/// 
/// <TABLE CLASS="right"><TR><TH>State</TH><TH>Implication</TH></TR>
/// <TR><TD>ONLINE</TD><TD>User is on-line.</TD></TR>
/// <TR><TD>INGAME</TD><TD>User is on-line and in-game.</TD></TR>
/// <TR><TD>JOINABLE</TD><TD>User is on-line, in-game, and joinable.</TD></TR></TABLE>
/// There is a known issue that creates an apparent loss of the "Join Game" button in an invite dialog box.
/// The issue is that a user can send an invite when their Presence status has been set to INGAME, rather than
/// set to JOINABLE. In the screen captures below, the left image shows an instance of this issue. The
/// right image shows the proper appearance of this dialog box, when the Presence status has been set to JOINABLE,
/// resulting in the "Join Game" button being displayed.
/// 
/// If you encounter this problem, ensure that you are properly setting your users' Presence status to JOINABLE,
/// rather than simply as INGAME when they are playing an Origin-launched game. Relevant Presence status setting are:
/// 
/// <img alt="Invites Known Issue" title="Invites Known Issue" src="InvitesKnownIssue.png"/>
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
