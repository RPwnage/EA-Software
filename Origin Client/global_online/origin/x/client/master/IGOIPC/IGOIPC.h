///////////////////////////////////////////////////////////////////////////////
// IGOIPC.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOIPC_H
#define IGOIPC_H

#ifdef ORIGIN_PC
#pragma warning(disable: 4996)	// 'sprintf': name was marked as #pragma deprecated
#endif

#include "IGOTrace.h"
#ifdef ORIGIN_MAC
#include "MacWindows.h"
#endif

#include "EABase/eabase.h"
#include "EASTL/vector.h"
#include "EASTL/list.h"
#include "EASTL/string.h"
#include "EASTL/shared_ptr.h"

class IGOIPCMessage;
class IGOIPCServer;
class IGOIPCClient;
class IGOIPCConnection;

#define WM_NEW_CONNECTION (WM_USER + 0x200) 
#define IGOIPC_WINDOW_CLASS L"IGOIPC Server"
#define IGOIPC_WINDOW_TITLE L"IGOIPC Server"

struct IGORECT {
	long left;
	long top;
	long right;
	long bottom;
};

class IGOIPCHandler
{
public:
	virtual void handleConnect(IGOIPCConnection *) { };
	virtual void handleDisconnect(IGOIPCConnection *, void *userData = NULL) { };
	virtual bool handleIPCMessage(IGOIPCConnection *, eastl::shared_ptr<IGOIPCMessage> msg, void *userData = NULL) = 0;
};

class IGOIPC
{
public:

	enum MessageType 
	{
		// messages Ebisu to Game
		MSG_UNKNOWN = 0
		, MSG_START
		, MSG_WINDOW_UPDATE
		, MSG_WINDOW_UPDATE_RECTS
		, MSG_SET_HOTKEY
		, MSG_SET_CURSOR
		, MSG_STOP
		, MSG_SET_IGO_MODE
		, MSG_OPEN_BROWSER
        , MSG_NEW_WINDOW
		, MSG_MOVE_WINDOW
		, MSG_CLOSE_WINDOW
		, MSG_RESET_WINDOWS
		, MSG_WINDOW_HIERARCHY
        , MSG_IGO_BROADCAST
        , MSG_IGO_BROADCAST_MIC_VOLUME
        , MSG_IGO_BROADCAST_GAME_VOLUME
        , MSG_IGO_BROADCAST_MIC_MUTED
        , MSG_IGO_BROADCAST_GAME_MUTED
		, MSG_FORCE_GAME_INTO_BACKGROUND
        , MSG_SET_IGO_ISPINNING
        , MSG_LOGGING_ENABLED
        , MSG_WINDOW_SET_ANIMATION

		// messages Game to Ebisu
		, MSG_INPUT_EVENT = 0x100
		, MSG_SCREEN_SIZE
		, MSG_RESET
		, MSG_IGO_MODE
		, MSG_IGO_FOCUS
		, MSG_IGO_KEYBOARD_LANGUAGE
#if defined(ORIGIN_MAC)
		, MSG_KEYBOARD_EVENT
        
        // IPC system internal messages
        , MSG_RELEASE_BUFFER
        
        // Telemetry
        , MSG_TELEMETRY_EVENT
#endif
        , MSG_IGO_BROADCAST_STATUS
        , MSG_IGO_BROADCAST_ERROR
        , MSG_IGO_BROADCAST_USERNAME
        , MSG_IGO_BROADCAST_DISPLAYNAME
        , MSG_IGO_BROADCAST_STREAM_INFO
        , MSG_IGO_BROADCAST_RESET_STATS
        , MSG_IGO_BROADCAST_STATS
		, MSG_IGO_BROADCAST_OPT_ENCODER_AVAILABLE
        
        , MSG_IGO_PIN_TOGGLE

        , MSG_IGO_OPEN_ATTEMPT_IGNORED

        
        // Game info exchange
        , MSG_GAME_REQUEST_INFO             // client asked for store info
        , MSG_GAME_REQUEST_INFO_COMPLETE    // game replies its done sending the info
        , MSG_GAME_PRODUCT_ID
        , MSG_GAME_ACHIEVEMENT_SET_ID
        , MSG_GAME_EXECUTABLE_PATH
        , MSG_GAME_TITLE
        , MSG_GAME_DEFAULT_URL
        , MSG_GAME_MASTER_TITLE
        , MSG_GAME_MASTER_TITLE_OVERRIDE
        , MSG_GAME_IS_TRIAL
        , MSG_GAME_TRIAL_TIME_REMAINING
        , MSG_GAME_INITIAL_INFO_COMPLETE    // client notifies it's done sending the initial info to the game (ie can be used reliably, like for example the trail info)
	};


	enum InputEventType
	{
#ifdef ORIGIN_MAC
        EVENT_MOUSE_UNKNOWN,
#endif
		EVENT_MOUSE_MOVE,
		EVENT_MOUSE_LEFT_DOWN,
		EVENT_MOUSE_LEFT_UP,
		EVENT_MOUSE_RIGHT_DOWN,
		EVENT_MOUSE_RIGHT_UP,
		EVENT_MOUSE_LEFT_DOUBLE_CLICK,
		EVENT_MOUSE_RIGHT_DOUBLE_CLICK,
		EVENT_MOUSE_WHEEL,
		EVENT_KEYBOARD
	};

	enum WindowUpdateFlags
	{
		WINDOW_UPDATE_NONE = 0x0,
		WINDOW_UPDATE_X = 0x1,
		WINDOW_UPDATE_Y = 0x2,
		WINDOW_UPDATE_WIDTH = 0x4,
		WINDOW_UPDATE_HEIGHT = 0x8,
		WINDOW_UPDATE_ALPHA = 0x10,
		WINDOW_UPDATE_BITS = 0x20,
		WINDOW_UPDATE_ALWAYS_VISIBLE = 0x40,
        WINDOW_UPDATE_ALL = 0xff,
        WINDOW_UPDATE_MGPU = 0x100,             // special flag to indicate internal multi gpu texture update
        WINDOW_UPDATE_REQUIRE_ANIMS = 0x200,    // another special flag to indicate the object needs to receive its anim data before it can render properly
	};

	enum CursorType
	{
		CURSOR_UNKNOWN = 0,
		CURSOR_ARROW,
		CURSOR_CROSS,
		CURSOR_HAND,
		CURSOR_IBEAM,
		CURSOR_SIZEALL,
		CURSOR_SIZENESW,
		CURSOR_SIZENS,
		CURSOR_SIZENWSE,
		CURSOR_SIZEWE,
	};

	struct MessageHeader 
	{
		MessageType type;
		uint32_t length;  // length does not includes the header
	};

#if !defined(ORIGIN_MAC)

	struct InputEvent
	{
		InputEventType type;
		uint32_t param1;
		uint32_t param2;
		uint32_t param3;
	};

#else // MAC OSX

	struct InputEvent
	{
        InputEvent() {}
        InputEvent(InputEventType t, int32_t p1, int32_t p2, int32_t p3, int32_t p4) : type(t), param1(p1), param2(p2), param3(p3), param4(p4) {}
        
		InputEventType type;
		int32_t param1;
		int32_t param2;
		int32_t param3;
        int32_t param4;
	};

#endif

public:
	static IGOIPC *instance();
	virtual ~IGOIPC();

#if defined(ORIGIN_MAC)
    IGOIPCServer *createServer(const char* serviceName, size_t messageQueueSize);
    IGOIPCClient *createClient(const char* serviceName, size_t messageQueueSize);
#elif defined(ORIGIN_PC)
	IGOIPCServer *createServer();
    IGOIPCClient *createClient();
#endif

	
	IGOIPCMessage *createMessage(IGOIPC::MessageType type, const char *data = 0, size_t size = 0);

	// createMessages
	IGOIPCMessage *createMsgStart(uint32_t key1, uint32_t key2, uint32_t pinKey1, uint32_t pinKey2, uint32_t minWidth, uint32_t minHeight);
    IGOIPCMessage *createMsgNewWindow(uint32_t windowId, uint32_t index);
	IGOIPCMessage *createMsgWindowUpdate(uint32_t windowId, bool visible, uint8_t alpha, int16_t x, int16_t y, uint16_t w, uint16_t h, const char *data, int size, uint32_t flags);
	IGOIPCMessage *createMsgWindowUpdateRects(uint32_t windowId, const eastl::vector<IGORECT> &rects, const char *data, int size);
    IGOIPCMessage *createMsgWindowSetAnimation(uint32_t windowId, uint32_t version, uint32_t context /* WindowAnimContext */, const char *data, uint32_t size);

#if defined(ORIGIN_MAC)

	IGOIPCMessage *createMsgWindowUpdateEx(uint32_t windowId, bool visible, uint8_t alpha, int16_t x, int16_t y, uint16_t w, uint16_t h, int64_t dataOffset, int size, uint32_t flags);
	IGOIPCMessage *createMsgWindowUpdateRectsEx(uint32_t windowId, const eastl::vector<IGORECT> &rects, int64_t dataOffset, int size);
    IGOIPCMessage *createMsgKeyboardEvent(const char *data, int size);
    IGOIPCMessage *createMsgReleaseBuffer(int ownerId, int64_t offset, int64_t size);

#endif

	IGOIPCMessage *createMsgSetHotkey(uint32_t key1, uint32_t key2, uint32_t pinKey1, uint32_t pinKey2);
	IGOIPCMessage *createMsgSetCursor(CursorType type);
	IGOIPCMessage *createMsgStop();
	IGOIPCMessage *createMsgSetIGOMode(bool visible);
	IGOIPCMessage *createMsgOpenBrowser(const eastl::string &url);
	IGOIPCMessage *createMsgMoveWindow(uint32_t windowId, uint32_t index);
	IGOIPCMessage *createMsgCloseWindow(uint32_t windowId);
    IGOIPCMessage *createMsgResetWindows(const eastl::vector<uint32_t>& ids);
    IGOIPCMessage *createMsgIsPinning(bool inProgress);
    IGOIPCMessage *createMsgLoggingEnabled(bool enabled);

#ifdef ORIGIN_MAC
	IGOIPCMessage *createMsgInputEvent(InputEventType type, int32_t param1 = 0, int32_t param2 = 0, int32_t param3 = 0, int32_t param4 = 0, float param5 = 0, float param6 = 0, uint64_t param7 = 0);
#else
	IGOIPCMessage *createMsgInputEvent(InputEventType type, int32_t param1 = 0, int32_t param2 = 0, int32_t param3 = 0, int32_t param4 = 0, float param5 = 0, float param6 = 0);
#endif
	IGOIPCMessage *createMsgScreenSize(uint16_t w, uint16_t h);
	IGOIPCMessage *createMsgReset(uint16_t newWidth, uint16_t newHeight);
	IGOIPCMessage *createMsgIGOMode(bool visible, bool emitStateChange = true);
	IGOIPCMessage *createMsgIGOFocus(bool visible, uint16_t width, uint16_t height);
	IGOIPCMessage *createMsgIGOKeyboardLanguage(HKL language);

    // twitch
    IGOIPCMessage *createMsgBroadcast(bool start, int resolution = 0, int framerate = 0, int quality = 0, int bitrate = 0, bool showViewersNum = 0, bool broadcastMic = 0, bool micVolumeMuted = 0, int micVolume = 0, bool gameVolumeMuted = 0, int gameVolume = 0, bool useOptEncoder = false, const eastl::string &token ="");
    IGOIPCMessage *createMsgBroadcastMicVolume(int micVolume);
    IGOIPCMessage *createMsgBroadcastGameVolume(int gameVolume);
    IGOIPCMessage *createMsgBroadcastMicMuted(bool micVolumeMuted);
    IGOIPCMessage *createMsgBroadcastGameMuted(bool gameVolumeMuted);
    IGOIPCMessage *createMsgBroadcastStatus(bool broadcastStarted, bool archivingEnabled, const eastl::string &fixArchivingURL, const eastl::string &channelURL);
    IGOIPCMessage *createMsgBroadcastError(int errorCategory, int errorcode);
    IGOIPCMessage *createMsgBroadcastUserName(const eastl::string &userName); 
    IGOIPCMessage *createMsgBroadcastDisplayName(const eastl::string &userName); 
    IGOIPCMessage *createMsgBroadcastStreamInfo(int viewers);
    IGOIPCMessage *createMsgBroadcastResetStats();
    IGOIPCMessage *createMsgBroadcastStats(bool isBroadcasting, uint64_t streamId, uint32_t minViewers, uint32_t maxViewers, const char16_t* channel, size_t channelLength);
	IGOIPCMessage *createMsgBroadcastOptEncoderAvailable(bool available);
    
	// game focus switching
    IGOIPCMessage *createMsgGoToBackground();

    // IGO pinnable widgets
    IGOIPCMessage *createMsgTogglePins(bool state);

    // Game info
    IGOIPCMessage *createMsgGameRequestInfo();
    IGOIPCMessage *createMsgGameRequestInfoComplete();
    IGOIPCMessage *createMsgGameProductId(const char16_t* productId, size_t length);
    IGOIPCMessage *createMsgGameAchievementSetId(const char16_t* achievementSetId, size_t length);
    IGOIPCMessage *createMsgGameExecutablePath(const char16_t* executablePath, size_t length);
    IGOIPCMessage *createMsgGameTitle(const char16_t* title, size_t length);
    IGOIPCMessage *createMsgGameDefaultURL(const char16_t* url, size_t length);
    IGOIPCMessage *createMsgGameMasterTitle(const char16_t* title, size_t length);
    IGOIPCMessage *createMsgGameMasterTitleOverride(const char16_t* title, size_t length);
    IGOIPCMessage *createMsgGameIsTrial(bool isTrial);
    IGOIPCMessage *createMsgGameTrialTimeRemaining(int64_t timeRemaining);
    IGOIPCMessage *createMsgGameInitialInfoComplete();
   
////
////
	// IGO User Open Attempt Ignored
	IGOIPCMessage *createMsgUserIGOOpenAttemptIgnored();

    
	// parse messages
    bool parseMsgStart(IGOIPCMessage *msg, uint32_t &key1, uint32_t &key2, uint32_t &pinKey1, uint32_t &pinKey2, uint32_t &minWidth, uint32_t &minHeight);
    bool parseMsgNewWindow(IGOIPCMessage *msg, uint32_t &windowId, uint32_t &index);
	bool parseMsgWindowUpdate(IGOIPCMessage *msg, uint32_t &windowId, bool &visible, uint8_t &alpha, int16_t &x, int16_t &y, uint16_t &w, uint16_t &h, const char *&data, int &size, uint32_t &flags);
	bool parseMsgWindowUpdateRects(IGOIPCMessage *msg, uint32_t &windowId, eastl::vector<IGORECT> &rects, const char *&data, int &size);
    bool parseMsgWindowSetAnimation(IGOIPCMessage *msg, uint32_t &windowId, uint32_t &version, uint32_t &context /* WindowAnimContext */, const char *&data, uint32_t &size);
    
#if defined(ORIGIN_MAC)

	bool parseMsgWindowUpdateEx(IGOIPCMessage *msg, uint32_t &windowId, bool &visible, uint8_t &alpha, int16_t &x, int16_t &y, uint16_t &w, uint16_t &h, int64_t &dataOffset, int &size, uint32_t &flags);
    bool parseMsgWindowUpdateRectsEx(IGOIPCMessage *msg, uint32_t &windowId, eastl::vector<IGORECT> &rects, int64_t &dataOffset, int &size);
    bool parseMsgKeyboardEvent(IGOIPCMessage *msg, const char *&data, int &size);
    bool parseMsgReleaseBuffer(IGOIPCMessage *msg, int &ownerId, int64_t &offset, int64_t &size);

#endif

	bool parseMsgSetHotkey(IGOIPCMessage *msg, uint32_t &key1, uint32_t &key2, uint32_t &pinKey1, uint32_t &pinKey2);
	bool parseMsgSetCursor(IGOIPCMessage *msg, CursorType &type);
	bool parseMsgStop(IGOIPCMessage *msg);
    bool parseMsgIsPinning(IGOIPCMessage *msg, bool &inProgress);
    bool parseMsgSetIGOMode(IGOIPCMessage *msg, bool &visible);
	bool parseMsgOpenBrowser(IGOIPCMessage *msg, eastl::string &url);
	bool parseMsgMoveWindow(IGOIPCMessage *msg, uint32_t &windowId, uint32_t &index);
	bool parseMsgCloseWindow(IGOIPCMessage *msg, uint32_t &windowId);
	bool parseMsgResetWindows(IGOIPCMessage *msg);
    bool parseMsgResetWindows(IGOIPCMessage *msg, eastl::vector<uint32_t>& ids);

#ifdef ORIGIN_MAC
	bool parseMsgInputEvent(IGOIPCMessage *msg, InputEventType &type, int32_t &param1, int32_t &param2, int32_t &param3, int32_t &param4, float &param5, float &param6, uint64_t &param7);
#else
	bool parseMsgInputEvent(IGOIPCMessage *msg, InputEventType &type, int32_t &param1, int32_t &param2, int32_t &param3, int32_t &param4, float &param5, float &param6);
#endif
	bool parseMsgScreenSize(IGOIPCMessage *msg, uint16_t &w, uint16_t &h);
	bool parseMsgReset(IGOIPCMessage *msg, uint16_t &newWidth, uint16_t &newHeight);
	bool parseMsgIGOMode(IGOIPCMessage *msg, bool &visible, bool &emitStateChange);
	bool parseMsgIGOFocus(IGOIPCMessage *msg, bool &hasFocus, uint16_t &w, uint16_t &h);
	bool parseMsgIGOKeyboardLanguage(IGOIPCMessage *msg, HKL *language);
	
    bool parseMsgBroadcast(IGOIPCMessage *msg, bool &start, int &resolution, int &framerate, int &quality, int &bitrate, bool &showViewersNum, bool &broadcastMic, bool &micVolumeMuted, int &micVolume, bool &gameVolumeMuted, int &gameVolume, bool &parseMsgBroadcast, eastl::string &token);
    bool parseMsgBroadcastMicVolume(IGOIPCMessage *msg, int &micVolume);
    bool parseMsgBroadcastGameVolume(IGOIPCMessage *msg, int &gameVolume);
    bool parseMsgBroadcastMicMuted(IGOIPCMessage *msg, bool &micVolumeMuted);
    bool parseMsgBroadcastGameMuted(IGOIPCMessage *msg, bool &gameVolumeMuted);
    bool parseMsgBroadcastStatus(IGOIPCMessage *msg, bool &broadcastStarted, bool &archivingEnabled, eastl::string &fixArchinvingURL, eastl::string &channelURL);
    bool parseMsgBroadcastError(IGOIPCMessage *msg, int &errorCategory, int &errorcode);
	bool parseMsgBroadcastOptEncoderAvailable(IGOIPCMessage *msg, bool &available);
    bool parseMsgBroadcastUserName(IGOIPCMessage *msg, eastl::string &name);
    bool parseMsgBroadcastDisplayName(IGOIPCMessage *msg, eastl::string &name);
    bool parseMsgBroadcastStreamInfo(IGOIPCMessage *msg, int &viewers);
    bool parseMsgBroadcastResetStats(IGOIPCMessage *msg);
    bool parseMsgBroadcastStats(IGOIPCMessage *msg, bool &isBroadcasting, uint64_t &streamId, uint32_t &minViewers, uint32_t &maxViewers, const char16_t *&channel, size_t &channelLength);

	bool parseMsgGoToBackground(IGOIPCMessage *msg);

    bool parseMsgTogglePins(IGOIPCMessage *msg, bool &state);
    bool parseMsgLoggingEnabled(IGOIPCMessage *msg, bool &enabled);

    bool parseMsgGameRequestInfo(IGOIPCMessage *msg);
    bool parseMsgGameRequestInfoComplete(IGOIPCMessage *msg);
    bool parseMsgGameProductId(IGOIPCMessage *msg, const char16_t *&productId, size_t &length);
    bool parseMsgGameAchievementSetId(IGOIPCMessage *msg, const char16_t *&achievementSetId, size_t &length);
    bool parseMsgGameExecutablePath(IGOIPCMessage *msg, const char16_t *&executablePath, size_t &length);
    bool parseMsgGameTitle(IGOIPCMessage *msg, const char16_t *&title, size_t &length);
    bool parseMsgGameDefaultURL(IGOIPCMessage *msg, const char16_t *&url, size_t &length);
    bool parseMsgGameMasterTitle(IGOIPCMessage *msg, const char16_t *&title, size_t &length);
    bool parseMsgGameMasterTitleOverride(IGOIPCMessage *msg, const char16_t *&title, size_t &length);
    bool parseMsgGameIsTrial(IGOIPCMessage *msg, bool &isTrial);
    bool parseMsgGameTrialTimeRemaining(IGOIPCMessage *msg, int64_t &timeRemaining);
    bool parseMsgGameInitialInfoComplete(IGOIPCMessage *msg);
    bool parseMsgUserIGOOpenAttemptIgnored(IGOIPCMessage *msg);


private:
	typedef eastl::list<IGOIPCMessage *> IGOIPCMessageList;
	IGOIPCMessageList mMessageList;
	CRITICAL_SECTION mCS;

	IGOIPC();
};

#endif
