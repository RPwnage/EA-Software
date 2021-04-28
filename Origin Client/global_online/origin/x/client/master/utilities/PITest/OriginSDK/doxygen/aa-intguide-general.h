/// \page aa-intguide-general Integrating Origin
/// \brief This page discusses the basics of coding the startup of Origin, setting up event callbacks, and updating the
/// SDK so that those events can be processed, as well as shutting Origin down.
/// 
/// \section contents Page Contents
/// <ul>
///     <li><a href="#intro">Introduction to Integrating Origin</a> - A general introduction to using the Origin
///         SDK.</li>
///     <li><a href="#ebisubasics">Origin Basics</a> - The general setup for the Origin SDK.</li>
/// </ul>
/// 
/// \section intro Introduction to Integrating Origin
/// 
/// The Origin SDK is an API that provides access to the Origin client, and which provides a number of additional
/// capabilities that provide users with some social networking features, such as creating a user profile, setting an
/// avatar, maintaining a "friends" list, and chatting with friends. Through its access to the Origin client, the
/// Origin SDK also provides electronic commerce capabilities, including browsing game offerings, accessing
/// free-to-play opportunities, and accessing micro-content offerings. The Origin client includes an in-game overlay
/// (IGO), which provides users with access to web-delivered content, including the Origin SDK's social networking
/// features. The Origin SDK allows you to integrate these features into any EA game.
/// 
/// This page provides a guided introduction to the basics of integrating Origin, setting up certain basic operations,
/// and shutting Origin down. 
/// 
/// The Origin install footprint is about 30MB; however, if your game integrates with the Origin SDK the user will
/// already have installed Origin client before downloading your title. Depending on where it is installed from, Origin
/// may prompt the user for their user account credentials (User Access Control [UAC]), but, if they have already
/// logged in to their game, for example, then they would not be prompted for their credentials again to install
/// Origin.
/// 
/// If you have integrated your game with the Origin SDK, the only reason for upgrading is when you want to use the
/// features provided in a newer version of the Origin SDK. Older versions of the Origin SDK will work with newer
/// versions of the Origin client. During development, if you are using a development version of the Origin SDK, you
/// may need to upgrade when service upgrades become incompatible with that version. Only release versions of the SDK
/// guarantee backwards compatibility. 
/// 
/// \section ebisubasics Origin Basics
/// 
/// The general setup for the Origin SDK would look something like this:
/// <ul>
/// 	<li><b>OriginStartup</b> checks that the Origin desktop client is running. If the client is not running,
///         OriginStartup will start it.</li>
/// 	<li><b>OriginRegisterEventCallback</b> enables notification to events (in this case, for the
///         OriginRegisterEventCallback). This section can be expanded to register multiple event types.</li>
/// 	<li><b>OriginUpdate</b> keeps updating the SDK so that events can be processed.</li>
/// 	<li><b>OriginShutdown</b> shuts Origin down and frees up memory.</li>
/// </ul>
/// 
/// <img alt="Origin-SDK Basic Interactions" title="Origin-SDK Basic Interactions" src="OriginBasicsInteraction.png"/>
/// 
/// This basic structure is demonstrated in the following code example.
/// 
/// \include int_main.txt
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
