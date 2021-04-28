/// \page ma-intguide-chat Integrating the Chat Feature
/// \brief This page discusses the integration of the Origin Chat feature, which provides instant messaging capabilities
/// between the local user and his or her friends.
/// 
/// Users are able to import friends from Facebook, X-Box Live, and the PlayStation Network at launch. Users' friends
/// lists are not auto-populated; rather, galleries of their friends from their friends lists on these other social
/// networking sites are displayes, and the user can send requests to join them as friends on the Origin Network.
/// Users must use the In-Game Overlay (IGO) to actually send messages. The game can display the following screens:
/// <ul>
/// 	<li>The messages IGO (OriginShowMessagesUI), which lets the user read their messages and send new messages.
///       out.</li>
/// 	<li>The message composition IGO (OriginComposeMessageUI), which is a more direct way of letting a user send
///       a message. You can pre-populate the message with a string and a list of users to send it to.</li>
/// </ul>
/// 
/// \note Social functionality is disabled for underage users.
/// 
/// This following code example demonstrates how to encode the callback for SDK Chat Event handling.
/// \note It is important to cast the data in the template as shown in the example below.
/// 
/// \include ChatEventCallback.txt
/// 
/// This following code example demonstrates how to send a Chat message initiated from within the game. This is done in
/// three steps: <ol>
///     <li>Set which user the chat message will be sent to.</li>
///     <li>Register for a callback to receive chat events from the other user.</li>
///     <li>Initiate the chat session, in this case from within the game.</li>
/// </ol>
/// 
/// \include exampleChat.txt
/// 
/// Alternatively, the user can initiate a Chat session from within the In-Game Overlay, as is demonstrated in the
/// following code example.
/// 
/// \include exampleChatIGO.txt
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly