/// \page ka-intguide-recentplayers Integrating the Recent Players Feature
/// \brief This page discusses the integration of the Origin Recent Players feature, which allows the game to provide a
/// list of users that the local user has recently played with to Origin.
/// 
/// Origin has no implicit tie to Blaze or other matchmaking technology, so the game must manually call
/// OriginAddRecentPlayers as needed in order to populate this list (for example, as a round starts or as players join
/// sessions already in progress).
/// 
/// The following code example illustrates how to get the recent players list for internal use.
/// 
/// \include exampleRecentPlayers.txt
/// 
/// The following code example illustrates how to display the recent players list in the in-game overlay.
///  
/// \include exampleRecentPlayersIGO.txt
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
