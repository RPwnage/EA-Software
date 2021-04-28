/// \page ja-intguide-ablockeduser Integrating the BlockList
/// \brief This page discusses the integration of the methods used to retrieve the Origin block list.
/// 
/// The next code example demonstrates how to get the blocklist through the asynchronous \ref OriginQueryBlockedUsers function.
/// In order for the callback to get called one needs to call \ref OriginUpdate in the game loop.
/// 
/// \include exampleBlocklist.cpp
/// 
/// The following code example demonstrates how to get the blocklist by using the synchronous \ref OriginQueryBlockedUsersSync function.
/// 
/// \include exampleBlocklistSync.cpp
/// 
/// The following code example demonstrates how to get blocklist event updates.
/// 
/// \include exampleBlocklistEvent.cpp
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
