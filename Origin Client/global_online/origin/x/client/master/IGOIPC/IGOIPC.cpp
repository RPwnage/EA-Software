///////////////////////////////////////////////////////////////////////////////
// IGOIPC.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGOIPC.h"

#include "IGOIPCMessage.h"
#include "IGOIPCServer.h"
#include "IGOIPCClient.h"

IGOIPC *IGOIPC::instance() 
{
	static IGOIPC ipc;
	return &ipc;
}

IGOIPC::IGOIPC()
{
	InitializeCriticalSection(&mCS);
}

IGOIPC::~IGOIPC()
{
	EnterCriticalSection(&mCS);

	IGOIPCMessageList::const_iterator iter;
	for (iter = mMessageList.begin(); iter != mMessageList.end(); ++iter)
	{
		delete (*iter);
	}
	mMessageList.clear();

	LeaveCriticalSection(&mCS);

	DeleteCriticalSection(&mCS);
}

#if defined(ORIGIN_MAC)
IGOIPCServer *IGOIPC::createServer(const char* serviceName, size_t messageQueueSize)
{
    return new IGOIPCServer(serviceName, messageQueueSize);
}

IGOIPCClient *IGOIPC::createClient(const char* serviceName, size_t messageQueueSize)
{
    return new IGOIPCClient(serviceName, messageQueueSize);
}
#elif defined(ORIGIN_PC)
IGOIPCServer *IGOIPC::createServer()
{
	return new IGOIPCServer();
}

IGOIPCClient *IGOIPC::createClient()
{
	return new IGOIPCClient();
}
#endif

IGOIPCMessage *IGOIPC::createMessage(IGOIPC::MessageType type, const char *data, size_t size)
{
	IGOIPCMessage *retval = NULL;
	EnterCriticalSection(&mCS);

	if (mMessageList.empty())
	{
		retval = new IGOIPCMessage(type, data, size);
	}
	else
	{
		retval = mMessageList.front();

		retval->setType(type);
		retval->setData(data, size);
		retval->reset();

		mMessageList.pop_front();
	}

	LeaveCriticalSection(&mCS);

	return retval;
}

// createMessages
IGOIPCMessage *IGOIPC::createMsgStart(uint32_t key1, uint32_t key2, uint32_t pinKey1, uint32_t pinKey2, uint32_t minWidth, uint32_t minHeight)
{
	IGOIPCMessage *msg = createMessage(MSG_START);
    msg->add(key1);
    msg->add(key2);
    msg->add(pinKey1);
    msg->add(pinKey2);
    msg->add(minWidth);
    msg->add(minHeight);

	return msg;
}

IGOIPCMessage *IGOIPC::createMsgNewWindow(uint32_t windowId, uint32_t index)
{
	IGOIPCMessage *msg = createMessage(MSG_NEW_WINDOW);
    msg->add(windowId);
    msg->add(index);

	return msg;
}

IGOIPCMessage *IGOIPC::createMsgWindowUpdate(uint32_t windowId, bool visible, uint8_t alpha, int16_t x, int16_t y, uint16_t w, uint16_t h, const char *data, int size, uint32_t flags)
{
	IGOIPCMessage *msg = createMessage(MSG_WINDOW_UPDATE);
	msg->add(windowId);
	msg->add(visible);
	msg->add(alpha);
	msg->add(x);
	msg->add(y);
	msg->add(w);
	msg->add(h);
	msg->add(flags);

	msg->add(data, size);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgWindowUpdateRects(uint32_t windowId, const eastl::vector<IGORECT> &rects, const char *data, int size)
{
	IGOIPCMessage *msg = createMessage(MSG_WINDOW_UPDATE_RECTS);
	msg->add(windowId);
	msg->add((uint32_t)rects.size());
	msg->add((const char *)&rects[0], sizeof(IGORECT)*rects.size());
	msg->add(data, size);
	return msg;	
}

IGOIPCMessage *IGOIPC::createMsgWindowSetAnimation(uint32_t windowId, uint32_t version, uint32_t context /* WindowAnimContext */, const char *data, uint32_t size)
{
    IGOIPCMessage *msg = createMessage(MSG_WINDOW_SET_ANIMATION);
    msg->add(windowId);
    msg->add(version);
    msg->add(context);
    msg->add(data, size);

    return msg;
}

#if defined(ORIGIN_MAC)

IGOIPCMessage *IGOIPC::createMsgWindowUpdateEx(uint32_t windowId, bool visible, uint8_t alpha, int16_t x, int16_t y, uint16_t w, uint16_t h, int64_t dataOffset, int size, uint32_t flags)
{
	IGOIPCMessage *msg = createMessage(MSG_WINDOW_UPDATE);
	msg->add(windowId);
	msg->add(visible);
	msg->add(alpha);
	msg->add(x);
	msg->add(y);
	msg->add(w);
	msg->add(h);
    msg->add(dataOffset);
    msg->add(size);
	msg->add(flags);

	return msg;
}

IGOIPCMessage *IGOIPC::createMsgWindowUpdateRectsEx(uint32_t windowId, const eastl::vector<IGORECT> &rects, int64_t dataOffset, int size)
{
	IGOIPCMessage *msg = createMessage(MSG_WINDOW_UPDATE_RECTS);
	msg->add(windowId);
	msg->add((uint32_t)rects.size());
	msg->add((const char *)&rects[0], sizeof(IGORECT)*rects.size());
    msg->add(dataOffset);
    msg->add(size);
    
	return msg;	
}

IGOIPCMessage *IGOIPC::createMsgKeyboardEvent(const char *data, int size)
{
	IGOIPCMessage *msg = createMessage(MSG_KEYBOARD_EVENT, data, size);    
	return msg;	    
}

IGOIPCMessage *IGOIPC::createMsgReleaseBuffer(int ownerId, int64_t offset, int64_t size)
{
    IGOIPCMessage *msg = createMessage(MSG_RELEASE_BUFFER);
    msg->add(ownerId);
    msg->add(offset);
    msg->add(size);
    
    return msg;
}

#endif // ORIGIN_MAC

IGOIPCMessage *IGOIPC::createMsgSetHotkey(uint32_t key1, uint32_t key2, uint32_t pinKey1, uint32_t pinKey2)
{
	IGOIPCMessage *msg = createMessage(MSG_SET_HOTKEY);
	msg->add(key1);
	msg->add(key2);
	msg->add(pinKey1);
	msg->add(pinKey2);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgSetCursor(CursorType type)
{
	IGOIPCMessage *msg = createMessage(MSG_SET_CURSOR);
	msg->add((uint32_t)type);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgGoToBackground()
{
	IGOIPCMessage *msg = createMessage(MSG_FORCE_GAME_INTO_BACKGROUND);
    return msg;
}

IGOIPCMessage *IGOIPC::createMsgUserIGOOpenAttemptIgnored()
{
	IGOIPCMessage *msg = createMessage(MSG_IGO_OPEN_ATTEMPT_IGNORED);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgTogglePins(bool state)
{
	IGOIPCMessage *msg = createMessage(MSG_IGO_PIN_TOGGLE);
	msg->add(state);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgIsPinning(bool inProgress)
{
    IGOIPCMessage *msg = createMessage(MSG_SET_IGO_ISPINNING);
    msg->add(inProgress);
    return msg;
}

IGOIPCMessage *IGOIPC::createMsgLoggingEnabled(bool enabled)
{
    IGOIPCMessage *msg = createMessage(MSG_LOGGING_ENABLED);
    msg->add(enabled);
    return msg;
}

IGOIPCMessage *IGOIPC::createMsgStop()
{
	IGOIPCMessage *msg = createMessage(MSG_STOP);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgSetIGOMode(bool visible)
{
	IGOIPCMessage *msg = createMessage(MSG_SET_IGO_MODE);
	msg->add(visible);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgOpenBrowser(const eastl::string &url)
{
	IGOIPCMessage *msg = createMessage(MSG_OPEN_BROWSER);
	msg->add(static_cast<uint32_t>(url.size()));
	msg->add(url.c_str(), url.size());
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgMoveWindow(uint32_t windowId, uint32_t index)
{
	IGOIPCMessage *msg = createMessage(MSG_MOVE_WINDOW);
	msg->add(windowId);
    msg->add(index);

	return msg;
}

IGOIPCMessage *IGOIPC::createMsgCloseWindow(uint32_t id)
{
	IGOIPCMessage *msg = createMessage(MSG_CLOSE_WINDOW);
	msg->add(id);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgResetWindows(const eastl::vector<uint32_t>& ids)
{
	IGOIPCMessage *msg = createMessage(MSG_RESET_WINDOWS);
    uint32_t windowCnt = static_cast<uint32_t>(ids.size());
    
#ifdef ORIGIN_MAC
    // Make sure we have enough room to send the window ids, otherwise send a full reset
    size_t messageSize = sizeof(IGOIPC::MessageHeader) + sizeof(uint32_t) * (windowCnt + 1);
    if (messageSize > Origin::Mac::MachPort::MAX_MSG_SIZE)
    {
        IGO_TRACEF("Too many window ids to be sent with MSG_RESET_WINDOWS message (%d)", windowCnt);
        windowCnt = 0;
    }
#endif
    
    msg->add(windowCnt);
    for (size_t idx = 0; idx < windowCnt; ++idx)
        msg->add(ids[idx]);
    
	return msg;
}

#ifdef ORIGIN_MAC
IGOIPCMessage *IGOIPC::createMsgInputEvent(IGOIPC::InputEventType type, int32_t param1, int32_t param2, int32_t param3, int32_t param4, float param5, float param6, uint64_t param7)
#else
IGOIPCMessage *IGOIPC::createMsgInputEvent(IGOIPC::InputEventType type, int32_t param1, int32_t param2, int32_t param3, int32_t param4, float param5, float param6)
#endif
{
	IGOIPCMessage *msg = createMessage(MSG_INPUT_EVENT);
	msg->add((uint32_t)type);
	msg->add(param1);
	msg->add(param2);
	msg->add(param3);
	msg->add(param4);
	msg->add(param5);
	msg->add(param6);
#ifdef ORIGIN_MAC
    msg->add(param7);
#endif
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgScreenSize(uint16_t w, uint16_t h)
{
	IGOIPCMessage *msg = createMessage(MSG_SCREEN_SIZE);
	msg->add(w);
	msg->add(h);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgReset(uint16_t newWidth, uint16_t newHeight)
{
	IGOIPCMessage *msg = createMessage(MSG_RESET);
	msg->add(newWidth);
	msg->add(newHeight);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgIGOMode(bool visible, bool emitStateChange)
{
	IGOIPCMessage *msg = createMessage(MSG_IGO_MODE);
	msg->add(visible);
    msg->add(emitStateChange);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgIGOFocus(bool hasFocus, uint16_t width, uint16_t height)
{
	IGOIPCMessage *msg = createMessage(MSG_IGO_FOCUS);
	msg->add(hasFocus);
	msg->add(width);
	msg->add(height);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgIGOKeyboardLanguage(HKL language)
{
	IGOIPCMessage *msg = createMessage(MSG_IGO_KEYBOARD_LANGUAGE);

#if defined(ORIGIN_PC)
	msg->add((const char*)&language, sizeof(HKL));
#elif defined(ORIGIN_MAC)
    msg->add(language);
#endif

	return msg;
}

IGOIPCMessage *IGOIPC::createMsgBroadcast(bool start, int resolution, int framerate, int quality, int bitrate, bool showViewersNum, bool broadcastMic, bool micVolumeMuted, int micVolume, bool gameVolumeMuted, int gameVolume, bool useOptEncoder, const eastl::string &token)
{
    IGO_TRACEF("createMsgBroadcast %d", start);
	IGOIPCMessage *msg = createMessage(MSG_IGO_BROADCAST);
	msg->add(start);
	msg->add(resolution);
	msg->add(framerate);
	msg->add(quality);
	msg->add(bitrate);
	msg->add(showViewersNum);
	msg->add(broadcastMic);
    msg->add(micVolumeMuted);
	msg->add(micVolume);
	msg->add(gameVolumeMuted);
	msg->add(gameVolume);
	msg->add(useOptEncoder);

	msg->add(static_cast<uint32_t>(token.size()));
	msg->add(token.c_str(), token.size());

	return msg;
}

IGOIPCMessage *IGOIPC::createMsgBroadcastMicVolume(int micVolume)
{
	IGOIPCMessage *msg = createMessage(MSG_IGO_BROADCAST_MIC_VOLUME);
	msg->add(micVolume);
    return msg;
}

IGOIPCMessage *IGOIPC::createMsgBroadcastGameVolume(int gameVolume)
{
	IGOIPCMessage *msg = createMessage(MSG_IGO_BROADCAST_GAME_VOLUME);
	msg->add(gameVolume);
    return msg;
}

IGOIPCMessage *IGOIPC::createMsgBroadcastMicMuted(bool micVolumeMuted)
{
	IGOIPCMessage *msg = createMessage(MSG_IGO_BROADCAST_MIC_MUTED);
	msg->add(micVolumeMuted);
    return msg;
}

IGOIPCMessage *IGOIPC::createMsgBroadcastGameMuted(bool gameVolumeMuted)
{
	IGOIPCMessage *msg = createMessage(MSG_IGO_BROADCAST_GAME_MUTED);
	msg->add(gameVolumeMuted);
    return msg;
}

IGOIPCMessage *IGOIPC::createMsgBroadcastStreamInfo(int viewers)
{
	IGOIPCMessage *msg = createMessage(MSG_IGO_BROADCAST_STREAM_INFO);
	msg->add(viewers);
    return msg;
}

IGOIPCMessage *IGOIPC::createMsgBroadcastStatus(bool broadcastStarted, bool archivingEnabled, const eastl::string &fixArchivingURL, const eastl::string &channelURL)
{
	IGOIPCMessage *msg = createMessage(MSG_IGO_BROADCAST_STATUS);
	msg->add(broadcastStarted);
	msg->add(archivingEnabled);
	
    if (fixArchivingURL.size() == 0)
    {
        // dummy
        msg->add(static_cast<uint32_t>(1));
        msg->add("\0", 1);
    }
    else
    {
        msg->add(static_cast<uint32_t>(fixArchivingURL.size()));
        msg->add(fixArchivingURL.c_str(), fixArchivingURL.size());
    }

    if (channelURL.size() == 0)
    {
        // dummy
        msg->add(static_cast<uint32_t>(1));
        msg->add("\0", 1);
    }
    else
    {
        msg->add(static_cast<uint32_t>(channelURL.size()));
        msg->add(channelURL.c_str(), channelURL.size());
    }

    return msg;
}

IGOIPCMessage *IGOIPC::createMsgBroadcastError(int errorCategory, int errorcode)
{
	IGOIPCMessage *msg = createMessage(MSG_IGO_BROADCAST_ERROR);
	msg->add(errorCategory);
	msg->add(errorcode);
    return msg;
}

IGOIPCMessage *IGOIPC::createMsgBroadcastOptEncoderAvailable(bool available)
{
	IGOIPCMessage *msg = createMessage(MSG_IGO_BROADCAST_OPT_ENCODER_AVAILABLE);
	msg->add(available);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgBroadcastUserName(const eastl::string &userName)
{
	IGOIPCMessage *msg = createMessage(MSG_IGO_BROADCAST_USERNAME);
	msg->add(static_cast<uint32_t>(userName.size()));
	msg->add(userName.c_str(), userName.size());
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgBroadcastDisplayName(const eastl::string &userName)
{
	IGOIPCMessage *msg = createMessage(MSG_IGO_BROADCAST_DISPLAYNAME);
	msg->add(static_cast<uint32_t>(userName.size()));
	msg->add(userName.c_str(), userName.size());
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgBroadcastResetStats()
{
	IGOIPCMessage *msg = createMessage(MSG_IGO_BROADCAST_RESET_STATS);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgBroadcastStats(bool isBroadcasting, uint64_t streamId, uint32_t minViewers, uint32_t maxViewers, const char16_t* channel, size_t channelLength)
{
    IGOIPCMessage *msg = createMessage(MSG_IGO_BROADCAST_STATS);
    msg->add(isBroadcasting);
    msg->add(streamId);
    msg->add(minViewers);
    msg->add(maxViewers);
    msg->add(reinterpret_cast<const char*>(channel), channelLength);

	return msg;
}

IGOIPCMessage *IGOIPC::createMsgGameRequestInfo()
{
	IGOIPCMessage *msg = createMessage(MSG_GAME_REQUEST_INFO);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgGameRequestInfoComplete()
{
	IGOIPCMessage *msg = createMessage(MSG_GAME_REQUEST_INFO_COMPLETE);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgGameProductId(const char16_t* productId, size_t length)
{
	IGOIPCMessage *msg = createMessage(MSG_GAME_PRODUCT_ID, reinterpret_cast<const char*>(productId), length);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgGameAchievementSetId(const char16_t* achievementSetId, size_t length)
{
	IGOIPCMessage *msg = createMessage(MSG_GAME_ACHIEVEMENT_SET_ID, reinterpret_cast<const char*>(achievementSetId), length);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgGameExecutablePath(const char16_t* executablePath, size_t length)
{
	IGOIPCMessage *msg = createMessage(MSG_GAME_EXECUTABLE_PATH, reinterpret_cast<const char*>(executablePath), length);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgGameTitle(const char16_t* title, size_t length)
{
	IGOIPCMessage *msg = createMessage(MSG_GAME_TITLE, reinterpret_cast<const char*>(title), length);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgGameDefaultURL(const char16_t* url, size_t length)
{
	IGOIPCMessage *msg = createMessage(MSG_GAME_DEFAULT_URL, reinterpret_cast<const char*>(url), length);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgGameMasterTitle(const char16_t* title, size_t length)
{
	IGOIPCMessage *msg = createMessage(MSG_GAME_MASTER_TITLE, reinterpret_cast<const char*>(title), length);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgGameMasterTitleOverride(const char16_t* title, size_t length)
{
	IGOIPCMessage *msg = createMessage(MSG_GAME_MASTER_TITLE_OVERRIDE, reinterpret_cast<const char*>(title), length);
	return msg;
}

IGOIPCMessage *IGOIPC::createMsgGameIsTrial(bool isTrial)
{
    IGOIPCMessage *msg = createMessage(MSG_GAME_IS_TRIAL);
    msg->add(isTrial);
    
    return msg;
}

IGOIPCMessage *IGOIPC::createMsgGameTrialTimeRemaining(int64_t timeRemaining)
{
    IGOIPCMessage *msg = createMessage(MSG_GAME_TRIAL_TIME_REMAINING);
    msg->add(timeRemaining);
    
    return msg;
}

IGOIPCMessage *IGOIPC::createMsgGameInitialInfoComplete()
{
    IGOIPCMessage *msg = createMessage(MSG_GAME_INITIAL_INFO_COMPLETE);
    return msg;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// parse messages
bool IGOIPC::parseMsgStart(IGOIPCMessage *msg, uint32_t &key1, uint32_t &key2, uint32_t &pinKey1, uint32_t &pinKey2, uint32_t &minWidth, uint32_t &minHeight)
{
	if (msg->type() != MSG_START)
		return false;

    msg->reset();
    msg->read(key1);
    msg->read(key2);
    msg->read(pinKey1);
    msg->read(pinKey2);
    msg->read(minWidth);
    msg->read(minHeight);

	return true;
}

bool IGOIPC::parseMsgNewWindow(IGOIPCMessage *msg, uint32_t &windowId, uint32_t &index)
{
	if (msg->type() != MSG_NEW_WINDOW)
		return false;

	msg->reset();
	msg->read(windowId);
    msg->read(index);

	return true;
}

// WARNING: data bytes returned are from the IGOIPCMessage object memory
// When the IGOIPCMessage object is deleted, the returned memory is invalid
bool IGOIPC::parseMsgWindowUpdate(IGOIPCMessage *msg, uint32_t &windowId, bool &visible, uint8_t &alpha, int16_t &x, int16_t &y, uint16_t &w, uint16_t &h, const char *&data, int &size, uint32_t &flags)
{
	if (msg->type() != MSG_WINDOW_UPDATE)
		return false;

	msg->reset();
	msg->read(windowId);
	msg->read(visible);
	msg->read(alpha);
	msg->read(x);
	msg->read(y);
	msg->read(w);
	msg->read(h);
	msg->read(flags);

	data = msg->readPtr();
	size = msg->remainingSize();

	return true;
}

// WARNING: data bytes returned are from the IGOIPCMessage object memory
// When the IGOIPCMessage object is deleted, the returned memory is invalid
bool IGOIPC::parseMsgWindowUpdateRects(IGOIPCMessage *msg, uint32_t &windowId, eastl::vector<IGORECT> &rects, const char *&data, int &size)
{
	if (msg->type() != MSG_WINDOW_UPDATE_RECTS)
		return false;

	uint32_t numRects = 0;
	IGORECT rect;

	msg->reset();
	msg->read(windowId);
	msg->read(numRects);

	rects.reserve(numRects);
	rects.clear();
	for (uint32_t i = 0; i < numRects; i++)
	{
		msg->read((char *)&rect, sizeof(rect));
		rects.push_back(rect);
	}

	data = msg->readPtr();
	size = msg->remainingSize();

	return true;
}

bool IGOIPC::parseMsgWindowSetAnimation(IGOIPCMessage *msg, uint32_t &windowId, uint32_t &version, uint32_t &context /* WindowAnimContext */, const char *&data, uint32_t &size)
{
    if (msg->type() != MSG_WINDOW_SET_ANIMATION)
		return false;

	msg->reset();
	msg->read(windowId);
    msg->read(version);
	msg->read(context);

	data = msg->readPtr();
	size = msg->remainingSize();

	return true;
}

#if defined(ORIGIN_MAC)

bool IGOIPC::parseMsgWindowUpdateEx(IGOIPCMessage *msg, uint32_t &windowId, bool &visible, uint8_t &alpha, int16_t &x, int16_t &y, uint16_t &w, uint16_t &h, int64_t &dataOffset, int &size, uint32_t &flags)
{
	if (msg->type() != MSG_WINDOW_UPDATE)
		return false;
    
	msg->reset();
	msg->read(windowId);
	msg->read(visible);
	msg->read(alpha);
	msg->read(x);
	msg->read(y);
	msg->read(w);
	msg->read(h);
    msg->read(dataOffset);
    msg->read(size);
	msg->read(flags);

	return true;
}

// WARNING: data bytes returned are from the IGOIPCMessage object memory
// When the IGOIPCMessage object is deleted, the returned memory is invalid
bool IGOIPC::parseMsgWindowUpdateRectsEx(IGOIPCMessage *msg, uint32_t &windowId, eastl::vector<IGORECT> &rects, int64_t &dataOffset, int &size)
{
	if (msg->type() != MSG_WINDOW_UPDATE_RECTS)
		return false;
    
	uint32_t numRects = 0;
	IGORECT rect;
    
	msg->reset();
	msg->read(windowId);
	msg->read(numRects);
    
	rects.reserve(numRects);
	rects.clear();
	for (uint32_t i = 0; i < numRects; i++)
	{
		msg->read((char *)&rect, sizeof(rect));
		rects.push_back(rect);
	}
    
    msg->read(dataOffset);
    msg->read(size);
    
	return true;
}

bool IGOIPC::parseMsgKeyboardEvent(IGOIPCMessage *msg, const char *&data, int &size)
{
    if (msg->type() != MSG_KEYBOARD_EVENT)
        return false;
    
    msg->reset();
    
    data = msg->readPtr();
    size = msg->remainingSize();
    
    return true;
}

bool IGOIPC::parseMsgReleaseBuffer(IGOIPCMessage *msg, int &ownerId, int64_t &offset, int64_t &size)
{
    if (msg->type() != MSG_RELEASE_BUFFER)
        return false;
    
    msg->reset();
    msg->read(ownerId);
    msg->read(offset);
    msg->read(size);
    
    return true;
}

#endif // MAC OSX

bool IGOIPC::parseMsgSetHotkey(IGOIPCMessage *msg, uint32_t &key1, uint32_t &key2, uint32_t &pinKey1, uint32_t &pinKey2)
{
	if (msg->type() == MSG_SET_HOTKEY)
    {
    	msg->reset();
	    msg->read(key1);
	    msg->read(key2);
	    msg->read(pinKey1);
	    msg->read(pinKey2);
        return true;
    }
    else
        return false;
}

bool IGOIPC::parseMsgSetCursor(IGOIPCMessage *msg, IGOIPC::CursorType &type)
{
	if (msg->type() != MSG_SET_CURSOR)
		return false;

	uint32_t t;
	msg->reset();
	msg->read(t);
	type = (IGOIPC::CursorType)t;

	return true;
}

bool IGOIPC::parseMsgUserIGOOpenAttemptIgnored(IGOIPCMessage *msg)
{
	if (msg->type() != MSG_IGO_OPEN_ATTEMPT_IGNORED)
		return false;
	return true;
}

bool IGOIPC::parseMsgTogglePins(IGOIPCMessage *msg, bool &state)
{
    if (msg->type() != MSG_IGO_PIN_TOGGLE)
		return false;

	bool t;
	msg->reset();
	msg->read(t);
	state = t;

    return true;
}


bool IGOIPC::parseMsgGoToBackground(IGOIPCMessage *msg)
{
	if (msg->type() != MSG_FORCE_GAME_INTO_BACKGROUND)
		return false;

	return true;
}

bool IGOIPC::parseMsgStop(IGOIPCMessage *msg)
{
	if (msg->type() != MSG_STOP)
		return false;

	return true;
}

bool IGOIPC::parseMsgIsPinning(IGOIPCMessage *msg, bool &inProgress)
{
    if (msg->type() != MSG_SET_IGO_ISPINNING)
        return false;

    msg->reset();
    msg->read(inProgress);

    return true;
}

bool IGOIPC::parseMsgSetIGOMode(IGOIPCMessage *msg, bool &visible)
{
	if (msg->type() != MSG_SET_IGO_MODE)
		return false;

	msg->reset();
	msg->read(visible);

	return true;
}

bool IGOIPC::parseMsgOpenBrowser(IGOIPCMessage *msg, eastl::string &url)
{
	url.clear();

	if (msg->type() != MSG_OPEN_BROWSER)
		return false;

	uint32_t size;
	msg->reset();
	msg->read(size);
	url.reserve(size);
	url.resize(size);
	msg->read(&url[0], size);

	return true;
}

bool IGOIPC::parseMsgMoveWindow(IGOIPCMessage *msg, uint32_t &windowId, uint32_t &index)
{
	if (msg->type() != MSG_MOVE_WINDOW)
		return false;

	msg->reset();
	msg->read(windowId);
    msg->read(index);

	return true;
}

bool IGOIPC::parseMsgCloseWindow(IGOIPCMessage *msg, uint32_t &windowId)
{
	if (msg->type() != MSG_CLOSE_WINDOW)
		return false;

	msg->reset();
	msg->read(windowId);

	return true;
}

bool IGOIPC::parseMsgResetWindows(IGOIPCMessage *msg)
{
	if (msg->type() != MSG_RESET_WINDOWS)
		return false;

	msg->reset();

	return true;
}

bool IGOIPC::parseMsgResetWindows(IGOIPCMessage *msg, eastl::vector<uint32_t>& ids)
{
	if (msg->type() != MSG_RESET_WINDOWS)
		return false;
    
	msg->reset();
    
    uint32_t cnt = 0;
    msg->read(cnt);
    
    uint32_t windowID = 0;
    for (size_t idx = 0; idx < cnt; ++idx)
    {
        msg->read(windowID);
        ids.push_back(windowID);
    }
    
	return true;
}

#ifdef ORIGIN_MAC
bool IGOIPC::parseMsgInputEvent(IGOIPCMessage *msg, IGOIPC::InputEventType &type, int32_t &param1, int32_t &param2, int32_t &param3, int32_t &param4, float &param5, float &param6, uint64_t &param7)
#else
bool IGOIPC::parseMsgInputEvent(IGOIPCMessage *msg, IGOIPC::InputEventType &type, int32_t &param1, int32_t &param2, int32_t &param3, int32_t &param4, float &param5, float &param6)
#endif
{
	if (msg->type() != MSG_INPUT_EVENT)
		return false;

	uint32_t newType;
	msg->reset();
	msg->read(newType);
	type = (InputEventType)newType;
	msg->read(param1);
	msg->read(param2);
	msg->read(param3);
	msg->read(param4);
	msg->read(param5);
	msg->read(param6);
#ifdef ORIGIN_MAC
    msg->read(param7);
#endif
    
	return true;
}

bool IGOIPC::parseMsgScreenSize(IGOIPCMessage *msg, uint16_t &w, uint16_t &h)
{
	if (msg->type() != MSG_SCREEN_SIZE)
		return false;

	msg->reset();
	msg->read(w);
	msg->read(h);
	
	return true;
}

bool IGOIPC::parseMsgReset(IGOIPCMessage *msg, uint16_t &newWidth, uint16_t &newHeight)
{
	if (msg->type() != MSG_RESET)
		return false;

	msg->reset();
	msg->read(newWidth);
	msg->read(newHeight);

	return true;
}

bool IGOIPC::parseMsgIGOMode(IGOIPCMessage *msg, bool &visible, bool &emitStateChange)
{
	if (msg->type() != MSG_IGO_MODE)
		return false;

	msg->reset();
	msg->read(visible);
    msg->read(emitStateChange);

	return true;
}

bool IGOIPC::parseMsgIGOFocus(IGOIPCMessage *msg, bool &hasFocus, uint16_t &width, uint16_t &height)
{
	if (msg->type() != MSG_IGO_FOCUS)
		return false;

	msg->reset();
	msg->read(hasFocus);
    msg->read(width);
	msg->read(height);

	return true;
}

bool IGOIPC::parseMsgLoggingEnabled(IGOIPCMessage *msg, bool &enabled)
{
	if (msg->type() != MSG_LOGGING_ENABLED)
		return false;

	msg->reset();
	msg->read(enabled);

	return true;
}

bool IGOIPC::parseMsgIGOKeyboardLanguage(IGOIPCMessage *msg, HKL *language)
{
	if (msg->type() != MSG_IGO_KEYBOARD_LANGUAGE)
		return false;

	msg->reset();

#if defined(ORIGIN_PC)
	memcpy((char*)language, msg->readPtr(), msg->remainingSize());
#elif defined(ORIGIN_MAC)
    msg->read(*language);
#endif

	return true;
}

bool IGOIPC::parseMsgBroadcast(IGOIPCMessage *msg, bool &start, int &resolution, int &framerate, int &quality, int &bitrate, bool &showViewersNum, bool &broadcastMic, bool &micVolumeMuted, int &micVolume, bool &gameVolumeMuted, int &gameVolume, bool &useOptEncoder, eastl::string &token)
{
	if (msg->type() != MSG_IGO_BROADCAST)
		return false;

	msg->reset();

	msg->read(start);
	msg->read(resolution);
	msg->read(framerate);
	msg->read(quality);
	msg->read(bitrate);
	msg->read(showViewersNum);
	msg->read(broadcastMic);
    msg->read(micVolumeMuted);
	msg->read(micVolume);
	msg->read(gameVolumeMuted);
	msg->read(gameVolume);
	msg->read(useOptEncoder);

	uint32_t size;
	msg->read(size);
	token.reserve(size);
	token.resize(size);
	msg->read(&token[0], size);

    return true;
}

bool IGOIPC::parseMsgBroadcastMicVolume(IGOIPCMessage *msg, int &micVolume)
{
	if (msg->type() != MSG_IGO_BROADCAST_MIC_VOLUME)
		return false;

	msg->reset();
	msg->read(micVolume);

	return true;
}

bool IGOIPC::parseMsgBroadcastGameVolume(IGOIPCMessage *msg, int &gameVolume)
{
	if (msg->type() != MSG_IGO_BROADCAST_GAME_VOLUME)
		return false;

	msg->reset();
	msg->read(gameVolume);

	return true;
}

bool IGOIPC::parseMsgBroadcastMicMuted(IGOIPCMessage *msg, bool &micVolumeMuted)
{
	if (msg->type() != MSG_IGO_BROADCAST_MIC_MUTED)
		return false;

	msg->reset();
	msg->read(micVolumeMuted);

	return true;
}

bool IGOIPC::parseMsgBroadcastGameMuted(IGOIPCMessage *msg, bool &gameVolumeMuted)
{
	if (msg->type() != MSG_IGO_BROADCAST_GAME_MUTED)
		return false;

	msg->reset();
	msg->read(gameVolumeMuted);

	return true;
}

bool IGOIPC::parseMsgBroadcastStatus(IGOIPCMessage *msg, bool &broadcastStarted, bool &archivingEnabled, eastl::string &fixArchivingURL, eastl::string &channelURL)
{
	if (msg->type() != MSG_IGO_BROADCAST_STATUS)
		return false;

	uint32_t size;
    fixArchivingURL.clear();
    channelURL.clear();
	msg->reset();
	msg->read(broadcastStarted);
	msg->read(archivingEnabled);
	msg->read(size);
	fixArchivingURL.reserve(size);
	fixArchivingURL.resize(size);
	msg->read(&fixArchivingURL[0], size);
	msg->read(size);
	channelURL.reserve(size);
	channelURL.resize(size);
	msg->read(&channelURL[0], size);

	return true;
}

bool IGOIPC::parseMsgBroadcastError(IGOIPCMessage *msg, int &errorCategory, int &errorcode)
{
	if (msg->type() != MSG_IGO_BROADCAST_ERROR)
		return false;

	msg->reset();
	msg->read(errorCategory);
	msg->read(errorcode);

	return true;
}


bool IGOIPC::parseMsgBroadcastOptEncoderAvailable(IGOIPCMessage *msg, bool &available)
{
	if (msg->type() != MSG_IGO_BROADCAST_OPT_ENCODER_AVAILABLE)
		return false;

	msg->reset();
	msg->read(available);

	return true;
}

bool IGOIPC::parseMsgBroadcastUserName(IGOIPCMessage *msg, eastl::string &name)
{
	name.clear();

	if (msg->type() != MSG_IGO_BROADCAST_USERNAME)
		return false;

	uint32_t size;
	msg->reset();
	msg->read(size);
	name.reserve(size);
	name.resize(size);
	msg->read(&name[0], size);

	return true;
}

bool IGOIPC::parseMsgBroadcastDisplayName(IGOIPCMessage *msg, eastl::string &name)
{
	name.clear();

	if (msg->type() != MSG_IGO_BROADCAST_DISPLAYNAME)
		return false;

	uint32_t size;
	msg->reset();
	msg->read(size);
	name.reserve(size);
	name.resize(size);
	msg->read(&name[0], size);

	return true;
}

bool IGOIPC::parseMsgBroadcastStreamInfo(IGOIPCMessage *msg, int &viewers)
{
	if (msg->type() != MSG_IGO_BROADCAST_STREAM_INFO)
		return false;

	msg->reset();
	msg->read(viewers);

	return true;
}

bool IGOIPC::parseMsgBroadcastResetStats(IGOIPCMessage *msg)
{
	if (msg->type() != MSG_IGO_BROADCAST_RESET_STATS)
		return false;
    
	return true;
}

bool IGOIPC::parseMsgBroadcastStats(IGOIPCMessage *msg, bool &isBroadcasting, uint64_t &streamId, uint32_t &minViewers, uint32_t &maxViewers, const char16_t *&channel, size_t &channelLength)
{
	if (msg->type() != MSG_IGO_BROADCAST_STATS)
		return false;
    
    msg->reset();
    msg->read(isBroadcasting);
    msg->read(streamId);
    msg->read(minViewers);
    msg->read(maxViewers);
    
    channelLength = msg->remainingSize();
    channel = (channelLength > 0) ? reinterpret_cast<const char16_t*>(msg->readPtr()) : NULL;

	return true;
    
}

bool IGOIPC::parseMsgGameRequestInfo(IGOIPCMessage *msg)
{
	if (msg->type() != MSG_GAME_REQUEST_INFO)
		return false;
    
	return true;
}

bool IGOIPC::parseMsgGameRequestInfoComplete(IGOIPCMessage *msg)
{
	if (msg->type() != MSG_GAME_REQUEST_INFO_COMPLETE)
		return false;
    
	return true;
}

bool IGOIPC::parseMsgGameProductId(IGOIPCMessage *msg, const char16_t *&productId, size_t &length)
{
    if (msg->type() != MSG_GAME_PRODUCT_ID)
        return false;
    
    msg->reset();
    
    length = msg->remainingSize();
    productId = (length > 0) ? reinterpret_cast<const char16_t*>(msg->readPtr()) : NULL;

    return true;
}

bool IGOIPC::parseMsgGameAchievementSetId(IGOIPCMessage *msg, const char16_t *&achievementSetId, size_t &length)
{
    if (msg->type() != MSG_GAME_ACHIEVEMENT_SET_ID)
        return false;
    
    msg->reset();
    
    length = msg->remainingSize();
    achievementSetId = (length > 0) ? reinterpret_cast<const char16_t*>(msg->readPtr()) : NULL;
    
    return true;
}

bool IGOIPC::parseMsgGameExecutablePath(IGOIPCMessage *msg, const char16_t *&executablePath, size_t &length)
{
    if (msg->type() != MSG_GAME_EXECUTABLE_PATH)
        return false;
    
    msg->reset();
    
    length = msg->remainingSize();
    executablePath = (length > 0) ? reinterpret_cast<const char16_t*>(msg->readPtr()) : NULL;
    
    return true;
}

bool IGOIPC::parseMsgGameTitle(IGOIPCMessage *msg, const char16_t *&title, size_t &length)
{
    if (msg->type() != MSG_GAME_TITLE)
        return false;
    
    msg->reset();
    
    length = msg->remainingSize();
    title = (length > 0) ? reinterpret_cast<const char16_t*>(msg->readPtr()) : NULL;
    
    return true;
}

bool IGOIPC::parseMsgGameDefaultURL(IGOIPCMessage *msg, const char16_t *&url, size_t &length)
{
    if (msg->type() != MSG_GAME_DEFAULT_URL)
        return false;
    
    msg->reset();

    length = msg->remainingSize();
    url = (length > 0) ? reinterpret_cast<const char16_t*>(msg->readPtr()) : NULL;
    
    return true;
}

bool IGOIPC::parseMsgGameMasterTitle(IGOIPCMessage *msg, const char16_t *&title, size_t &length)
{
    if (msg->type() != MSG_GAME_MASTER_TITLE)
        return false;
    
    msg->reset();

    length = msg->remainingSize();
    title = (length > 0) ? reinterpret_cast<const char16_t*>(msg->readPtr()) : NULL;
    
    return true;
}

bool IGOIPC::parseMsgGameMasterTitleOverride(IGOIPCMessage *msg, const char16_t *&title, size_t &length)
{
    if (msg->type() != MSG_GAME_MASTER_TITLE_OVERRIDE)
        return false;
    
    msg->reset();

    length = msg->remainingSize();
    title = (length > 0) ? reinterpret_cast<const char16_t*>(msg->readPtr()) : NULL;
    
    return true;
}

bool IGOIPC::parseMsgGameIsTrial(IGOIPCMessage *msg, bool &isTrial)
{
	if (msg->type() != MSG_GAME_IS_TRIAL)
		return false;
    
	msg->reset();
	msg->read(isTrial);
    
	return true;
    
}

bool IGOIPC::parseMsgGameTrialTimeRemaining(IGOIPCMessage *msg, int64_t &timeRemaining)
{
	if (msg->type() != MSG_GAME_TRIAL_TIME_REMAINING)
		return false;
    
	msg->reset();
	msg->read(timeRemaining);
    
	return true;
}

bool IGOIPC::parseMsgGameInitialInfoComplete(IGOIPCMessage *msg)
{
    if (msg->type() != MSG_GAME_INITIAL_INFO_COMPLETE)
		return false;
    
	msg->reset();    
	return true;
}