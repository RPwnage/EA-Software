/// \page fa-intguide-ingameoverlay Integrating the In-Game Overlay
/// \brief This page discusses the integration of the In-Game Overlay, which allows Origin displays to be overlayed on
/// top of the game display.
/// 
/// If you are integrating using the SDK, the SDK will load the IGO; otherwise, it will be injected when the game is 
/// launched from Origin. Also, if you are integrating with the SDK, you will receive notifications when the IGO is up;
/// otherwise, you will receive no indications. When the IGO is running, the game will continue running. As the SDK 
/// provides notification when the IGO is running, you can choose to suspend it if you want.
/// 
/// System memory, which is the memory for Origin running in the background, is the player's resolution times 12 bytes, 
/// for example, 1024x768x12 bytes. There is no additional network traffic, except for chat, which is hundreds of bytes 
/// per minute. DX8 and openGL 1.1 are supported. If you have the ability to detect when the frame rate is low and can 
/// reduce the rendering load, doing so would ensure that the Origin experience is a positive one. 
/// 
/// The IGO uses (Shift + ~) as its default activation key, although the user can change that. You might choose to 
/// avoid using that key combination, but when the IGO is up, the game will not receive keyboard input. As keyboard and 
/// mouse support are handled automatically, no work has to be done to hand off control, if you are using direct input 
/// 1, 2, 7, or 8, or Windows messages.
///  
/// Note that IGO windows can be moved by the user. There is no API capability for this at the moment.
/// 
/// The game can display various In-Game Overlays for each of the SDK's various features (which is discussed in the
/// following pages of the <i>Origin SDK Integrator's Guide</i>). The game can also receive an event to show when the
/// IGO is displayed or hidden, as is shown in the following code example.
/// 
/// \include exampleIGOEvent.txt
/// 
/// The following code example demonstrates how to respond to In-Game Overlay events.
/// \note It is important to cast the data in the template as shown in the example below.
/// 
/// \include IgoEventCallback.txt
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
