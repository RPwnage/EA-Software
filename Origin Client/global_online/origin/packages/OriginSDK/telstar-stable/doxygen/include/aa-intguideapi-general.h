/// \page aa-intguideapi-general Integrating Origin IGO API
/// \brief This page discusses the basics of using the IGO API. Please refer to the \ref aa-intguide-general for details on how to use the Origin SDK before you continue.
/// 
/// \section contents Page Contents
/// <ul>
///     <li><a href="#introapi">Introduction to Integrating Origin IGO API</a> - A general introduction to using the Origin IGO API.</li>
///     <li><a href="#ebisuprerequisitsapi">Origin IGO API Prerequisites for development builds</a> - The general development setup for using the Origin IGO API in development configuration.</li>
///     <li><a href="#ebisuprerequisitsapiretail">Origin IGO API Prerequisites for release builds</a> - The general development setup for using the Origin IGO API in release configuration.</li>
///     <li><a href="#ebisubasicsapi">Origin IGO API Basics</a> - The general setup for the Origin IGO API.</li>
///     <li><a href="#ebisuadvancedapi">Origin IGO API Advanced Rendering Support</a> - Advanced support for custom rendering.</li>
/// </ul>
/// 
/// \section benefitsoigapi Benefits of using the OIG API compared to the legacy OIG
/// 
///	OIG API means that OIG is no longer depending on API hooking and DLL injection. This has a few important benfits:
/// <ul>
///	<li>100% compatible with 3rd party applications like OBS (Open Broadcaster Software)</li>
///	<li>easy to integrate into existing game engines</li>
///	<li>supports DirectX 9, DirectX 10, DirectX 11, DirectX 12 and OpenGL out of the box</li>
///	<li>100% graphics API independent (<a href="#ebisuadvancedapi">Origin IGO API Advanced Rendering Support</a>)</li>
///	<li>rendering and screen capturing code can be tuned and debugged by game engine engineers (<a href="#ebisuadvancedapi">Origin IGO API Advanced Rendering Support</a>)</li>
///	<li>fully compatible with advanced rendering techniques like HDR (<a href="#ebisuadvancedapi">Origin IGO API Advanced Rendering Support</a>)</li>
/// </ul>
///
///
/// \section introapi Introduction to Integrating Origin IGO API
/// 
/// The Origin IGO API is an API that provides direct integration of the In-Game Overlay without the
/// need of any API hooking or DLL injection into the game process - in contrast to the legacy implementation of the In-Game Overlay.
/// 
/// This page provides a guided introduction to the basics of integrating the API into your game. There are also example implementations available
/// for different graphics API under the OIGAPITool\dev\TestAppXXX folder. 
/// 
/// 
/// \section ebisuprerequisitsapi Origin IGO API Prerequisites for development builds
/// 
/// In order to make use of the Origin IGO API for your development executables you have to setup a few things:
/// <ul>
///     <li>for non RTP wrapped development builds</li><br>
/// 	 -set this environment variable "EAUseIGOAPI=1" for your game process or globally and launch your game process after Origin has been started
///	
///		<li>for RTP wrapped development builds without proper <b>EAInstaller</b> feature flag <b>enableOriginInGameAPI</b>:<br>
///         -add your game executable as a NOG(Non Origin Game) to the Origin game library<br>
///         -open ODT and enable "OIG API Override" for your title<br>
/// </ul>
/// 
/// \section ebisuprerequisitsapiretail Origin IGO API Prerequisites for release builds
/// 
/// In order to make use of the Origin IGO API for your release executables you have to setup this:
/// <ul>
///		<li>set the <b>EAInstaller</b> feature flag <b>enableOriginInGameAPI=1</b></li>
/// </ul>
/// 
/// 
/// \section ebisubasicsapi Origin IGO API Basics
/// <br>
/// <b>Benefits</b>
/// <ul>
///	<li>easy to integrate into existing game engines</li>
///	<li>supports DirectX 9, DirectX 10, DirectX 11, DirectX 12 and OpenGL out of the box</li>
/// </ul>
/// <br>
/// The general setup for the Origin IGO API would look something like this (The basic usage is demonstrated in the following code example from the well know DirectX SDK BasicHLSL sample application.)
/// The following example code uses a DirectX 9 based sample application, that's why you have to include the proper headers <b>OriginSDK/OriginInGameDX9.h</b> and structures <b>OriginIGORenderingContext_DX9_T</b> for the DX9 version of the Origin IGO API.
/// <ul>
/// 	<li><b>OriginSDKErrorCallback and OriginIGOVisibilityCallback</b> are required to get additional error messages and warnings from the Origin IGO API and to get the visibility state of the IGO, so that that game process can hide and show the mouse cursor.
///     <br>
/// 	\include int_api_main_1.txt
///     <br>
///     <br>
///		</li>
/// 	<li><b>OriginIGOInit</b> will initialize the IGO via a OriginIGOContextInitT structure. This structure has to be setup, so that the IGO know, if it should use different memory allocators, the initial screen dimensions and what version of the Origin IGO API to use.
///     <br>
/// 	\include int_api_main_2.txt
///     <br>
///     <br>
/// 	</li>
/// 	<li><b>OriginIGOUpdate</b> runs the IGO IPC message & rendering(optionally runs video capturing & encoding when streaming) loop and keeps updating the IGO. It is also responsible for letting the IGO know, which renderer it should use and additional rendering parameters, like render target and screen capture source.
///     <br>
/// 	\include int_api_main_3.txt
///     <br>
///     <br>
/// 	</li>
/// 	<li><b>OriginIGOResize</b> uses a OriginIGORenderingResizeT structure to informs the IGO about any screen dimension changes. Once those values have been changed, you must call <b>OriginIGOReset</b> to properly apply them!
///     <br>
/// 	\include int_api_main_4.txt
///     <br>
///     <br>
/// 	</li>
/// 	<li><b>OriginIGOReset</b> will release IGO's internal surfaces whenever a resize or render device reset has to be done by the game process. If you are about to do a <b>IDirect3DDevice9::Reset</b> call in DX9, you should call <b>OriginIGOReset</b> before that call.
///     <br>
/// 	\include int_api_main_5.txt
///     <br>
///     <br>
/// 	</li>
/// 	<li><b>OriginIGOProcessEvents</b> let the IGO process WM_XXX messages like keyboard and mouse events. Whenever the IGO ingest a WM_XXX message, that message must be hidden from the game process. The game has to block all input API's (mouse/keyboard/gamepad etc.) while the IGO is visible on screen! 
///     <br>
/// 	\include int_api_main_6.txt
///     <br>
///     <br>
/// 	</li>
/// 	<li><b>OriginIGOShutdown</b> shuts down the OIG and releases all memory.
///     <br>
/// 	\include int_api_main_7.txt
///     <br>
///     <br>
/// </ul>
/// 
/// 
/// \section ebisuadvancedapi Origin IGO API Advanced Rendering Support
/// <br>
/// <b>Benefits</b>
/// <ul>
///	<li>rendering and screen capturing code can be tuned and debugged by game engine engineers</li>
///	<li>fully compatible with advanced rendering techniques like HDR</li>
///	<li>100% graphics API independent</li>
/// </ul>
/// <br>
/// The advanced support requires the same preparation steps like the basic setup and additionally, you have to write your own OIG renderer and frame buffer capture code.
/// You have to include the proper headers <b>OriginSDK/OriginInGameCALLBACK.h</b> for the @ref IOriginIGORenderer "IOriginIGORenderer" and structures, along with <b>OriginSDK/OriginError.h</b> for OIG API errors.
/// <ul>
/// 	<li>To utilize a custom OIG API renderer, you have to provide to the OIG via the <b>OriginIGORenderingContext_CALLBACK_T</b> structure like this:
///     <br>
/// 	\include oig_api_renderer_update_context_1.txt
///     <br>
///     <br>
///		</li>
/// 	<li>Here is a pseudo implementation of <b>IOriginIGORenderer</b>. It has to manage all textures that OIG uses for rendering and it has to provide a call for retrieving a game frame without the OIG on it!
///     <br>
/// 	\include oig_api_renderer_1.txt
///     <br>
///     <br>
///		</li>
/// 	<li><b>beginFrame</b> will be called before the rendering of an the OIG frame.
///     <br>
///		\include oig_api_renderer_beginframe.txt
///		<br>
///		<br>
/// 	</li>
/// 	<li><b>endFrame</b> will be called, once rendering of a OIG frame has been finished.
///     <br>
/// 	\include oig_api_renderer_endframe.txt
///     <br>
///     <br>
/// 	</li>
/// 	<li><b>createTexture2D</b> is used to create an OIG texture via the <b>OriginIGORenderingTexture2D_T</b> structure. It returns the internal texID.
///     <br>
/// 	\include oig_api_renderer_createtexture.txt
///     <br>
///     <br>
/// 	</li>
/// 	<li><b>updateTexture2D</b> is used to update the content or the properties of an existing texture via <b>OriginIGORenderingTexture2D_T</b> structure, it requires a valid texID!
///     <br>
/// 	\include oig_api_renderer_updatetexture.txt
///     <br>
///     <br>
/// 	</li>
/// 	<li><b>deleteTexture2D</b> removes a given texture resource.
///     <br>
/// 	\include oig_api_renderer_deletetexture.txt
///     <br>
///     <br>
/// 	</li>
/// 	<li><b>renderQuad</b> is used to perform the rendering of a single OIG surface. You must use the same blending as in the sample implementation! It uses the <b>OriginIGORenderingQuad_T</b> structure.
///     <br>
/// 	\include oig_api_renderer_renderQuad.txt
///     <br>
///     <br>
/// 	<li><b>streamingSetup</b> is used to setup the video streaming properties via the <b>OriginIGOStreamingInfo_T</b> structure.
///     <br>
/// 	\include oig_api_renderer_streamsetup.txt
///     <br>
///     <br>
/// 	</li>
/// 	<li><b>transferFrame</b> fills a <b>OriginIGOStreamingInfo_T</b> structure with a captured frame for the OIG streaming.
///     <br>
/// 	\include oig_api_renderer_transferFrame.txt
///     <br>
///     <br>
/// 	</li>
/// 	\htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly</li>
/// </ul>