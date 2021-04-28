/// \page da-intguide-platforms Developing for Different Platforms
/// \brief This section describes the Origin SDK's handling of development for differnent platforms, which includes
/// accessing the DIME Ecommerce API via the new Services Layer's Ecommerce API, handling multiple players on consoles,
/// and displaying the In-Game Overlay on either PCs and Macs or via the IGO API, or displaying them on either Xbox
/// 360s or PS3s via the SDK's Services Layer API.
/// 
/// The Origin SDK 1.x was designed to work only with PC platform games. The major difference in the Origin SDK 2.0
/// release is the support added for other platforms, including Apple Macintosh computers, but more significantly
/// support for the Xbox 360 and PS3 game consoles. In the 1.x releases of the SDK, Origin functionality was provided
/// from the Origin desktop application, but for consoles, the Origin desktop functionality is built into the SDK,
/// and therefore into the console versions of games that integrate Origin capabilities. This allows EA console games
/// to provide Origin's ecommerce and social network features without reliance upon the Origin application.
/// 
/// \section sectioncontents Section Contents
/// <ul>
///     <li>\ref originsdk20 &mdash; provides an overview of the Origin SDK 2.0 new features and architecture.</li>
///     <li>\subpage controllerindex &mdash; explains how to the handle multiple player inputs and outputs on consoles.</li>
///     <li>\subpage igoonconsoles &mdash; shows how to provide in-game overlay displays on consoles.</li>
/// </ul>
/// 
/// \section originsdk20 Origin SDK 2.0 New Features
///
/// <table style="width:100%;border:0px"><tr style="vertical-align:top"><td>
/// The major development in version 2.0 of the Origin SDK is the addition of the Origin SDK's Services Layer. This new
/// module of the Origin SDK provides the following new features: <ul>
///    <li>The Origin Ecommerce API now provides transparent integration with the DIME Ecommerce SDK, an abstraction
///        layer that provides access to EA's Ecommerce API, as is shown in the <b>New Features Overview</b> diagram
///        to the right, which also shows the components that make up the Origin SDK's Services Layer.</li>
///    <li>Integrating the Origin SDK into PC or Mac games uses the Origin client to provide the graphical display of
///        the In-Game Overlay windows and access to the Origin Services' data that these displays depend upon, as is
///        shown in the <b>PC &amp; Mac Implementation</b> diagram to the right.<br>&nbsp;<br> However,
///        integrating the Origin SDK into Xbox 360 and PS3 console games does not have access to the Origin client, so the
///        Origin SDK's Services Layer provides these capabilities for console games, as is shown in the <b>Console
///        Implementation</b> diagram to the right, and which is discussed in the \ref igoonconsoles page.</li>
///    <li>Also, the Origin SDK 2.0 adds a mechanism for supporting multiple players per game, which is common on
///        console platforms, and which is discussed in the \ref controllerindex page.</li>
/// </ul></td><td>
/// <img alt="Origin SDK 2.0 New Features" title="Origin SDK 2.0 New Features" src="OriginSDK-ServicesLayer.png"/>
/// </td></tr></table>
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
/// 
/// \page controllerindex Handling Multiple Players on Consoles
///
/// <table style="width:100%;border:0px"><tr style="vertical-align:top"><td>
/// Game consoles are designed to allow multiple players, while PC games typically permit only one player per computer.
/// As Origin SDK 1.0 provided support for only PC games, support for multiple player I/O from consoles is one of the
/// new features in the Origin SDK 2.0 release.
///
/// As shown in the diagram to the right, this is handled by a new parameter
/// of the OriginGetProfile() function, the userIndex parameter (0-3). Use of this parameter is also added to related functions
/// to provide full and proper handling of console multi-player inputs and outputs.
///
/// An example of handling multiple users is shown in the following example.
/// 
/// \include exampleMultiUser.cpp
/// </td><td>
/// <img alt="Handling Multiple Users" title="Handling Multiple Users" src="OriginSDK-ServicesLayer-ControllerIndex.png"/>
/// </td></tr></table>
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
/// 
/// \page igoonconsoles In-Game Overlays on Consoles with Webkit
///
/// <table style="width:100%;border:0px"><tr style="vertical-align:top"><td>
/// The display of in-game overlays must be handled very differently when integrating the Origin SDK into PC and Mac games
/// as opposed to integrating the SDK into console games.
/// 
/// When integrating into PC and Mac games, the Origin SDK leverages the features of the Origin client to display in-game
/// overlay windows, and it places them according to Origin SDK logic, as is illustrated in the top diagram to the right.
/// This approach is documented in the page \ref fa-intguide-ingameoverlay.
///
/// <p>&nbsp;</p>
///
/// <div style="position:relative;top:247px;">
/// <hr>
/// When integrating console games, the allocateSurface() function of the Webkit API is used. This function requests a
/// surface of a given size, actually a pointer to an allocation of memory, to which it returns the ARGB data for the
/// display of the requested IGO window.
/// </div>
/// </td><td>
/// <img alt="Providing In-Game Overlays" title="Providing In-Game Overlays" src="OriginSDK-ServicesLayer-IGOMethods.png"/>
/// </td></tr></table>
///
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
