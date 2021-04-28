/// \page ga-intguide-profile Integrating the User Profile
/// \brief This page discusses the integration of the methods used to retrieve the local user's profile.
/// 
/// When player lists are shown in the In-Game Overlay (IGO), the user is able to pick one of those users to see their 
/// profile data. From the profile data IGO (e.g., OriginShowProfileUI), a player can send a friend request if they are 
/// not a friend already and send feedback or report abusive behaviour about another player.
/// 
/// \note The User Profile integrates with the Customer Service Integration Layer (CSIL), allowing EA Customer Support 
/// reps to handle problem reports.
/// 
/// The following code example demonstrates how to get the default user's profile information for internal processing.
/// 
/// \include exampleProfile.txt
/// 
/// This next code example shows how to display the default user's profile information in the In-Game Overlay, as well
/// as the user profile information for a friend of the default user.
/// 
/// \include exampleProfileIGO.txt
/// 
/// The following code example shows how to register to receive the default user's profile updates.
/// 
/// \include exampleProfileEvent.txt
/// 
/// This next code example shows how to encode Origin Profile event handling.
/// \note It is important to cast the data in the template as shown in the example below.
/// 
/// \include ProfileEventCallback.txt
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly

