These are the release notes for EADP SDK.

# EadpRealTimeMessaging <packageversion>
 
## Needs Action
* A clientVersion String has been added to LoginRequestV2 and integrators must now provide this parameter
 
## New Features
* [Support non-unique display name](https://eadpjira.ea.com/browse/GSNIM-2346) Support is added for user nick names. 
* [Chat channels contain timestamp in the last message](https://eadpjira.ea.com/browse/GSNIM-2536) The last text message in the chat channel contains a timestamp field.
* [Return last message with chat channel info](https://eadpjira.ea.com/browse/GSNIM-2537) Chat channels now contain the last text message sent.
* [Chat channel read indicator](https://eadpjira.ea.com/browse/GSNIM-2535) Chat channel updates to indicate a channel has been read. 
* New overloaded FetchChannelList API added to the ChatService that lets the caller create the request. 
* Typing indicator and channel update rate limits are set during the initialization of the ChatService by querying the Server. 
* The ConnectionService now waits to receive a logout response before closing the socket 
* [Message History Filter](https://eadpjira.ea.com/browse/GSNIM-2606) Integrators can now set filters on ChannelMessageType when getting message history.
* Support for multiple session login and chat

## Bug Fixes
 
## Known Issues
 
## Platform SDKs
* Windows: 10.0.14393
* MacOS: 10.8

# EadpRealTimeMessaging 1.0.1
 
## Needs Action

## New Features
* Updated logout flow. The new flow waits for a logout response before closing the socket. 

## Platform SDKs
* Windows: 10.0.14393
* MacOS: 10.8

# EadpRealTimeMessaging 1.0.0
 
## Needs Action
* [Integrate EadpRealTimeMessaging](https://developer.ea.com/display/PDE/Integrate+EadpRealTimeMessaging)

## New Features
* First official release of the RealTimeMessaging service.
* Chat, Presence, Groups and Friends Notification feature
 
## Platform SDKs
* Windows: 10.0.14393
* MacOS: 10.8
