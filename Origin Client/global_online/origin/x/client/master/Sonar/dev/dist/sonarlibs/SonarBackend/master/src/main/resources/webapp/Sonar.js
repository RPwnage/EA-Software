/**
 * @fileOverview ESN Sonar JavaScript Client
 * @author ESN Social Software AB
 * @version 1
 * Copyright (c) 2010-2011 ESN Social Software AB
 */

var SonarVoice;

function SonarError(message) { this.name = "SonarError"; this.message = (message || ""); }
SonarError.prototype = new Error();
SonarError.prototype.name = 'SonarError';

function SonarOutdatedVersion(message) { this.name = "SonarOutdatedVersion"; this.message = (message || ""); }
SonarOutdatedVersion.prototype = new Error();
SonarOutdatedVersion.prototype.name = 'SonarOutdatedVersion';

function SonarPluginError(message) { this.name = "SonarPluginError"; this.message = (message || ""); }
SonarPluginError.prototype = new Error();
SonarPluginError.prototype.name = 'SonarPluginError';

function SonarInvalidArgument(message) { this.name = "SonarInvalidArgument"; this.message = (message || ""); }
SonarInvalidArgument.prototype = new Error();
SonarInvalidArgument.prototype.name = 'SonarInvalidArgument';

(function() {

var EXPECTED_VERSION = '0.70.3';
var CAPTURE_MODE_CONTINUOUS = 0;
var CAPTURE_MODE_PUSH_TO_TALK = 1;
var CAPTURE_MODE_VOICE_ACTIVATION = 2;
var JOIN_CHANNEL_TIMEOUT = 5000; // ms
var VOICE_SERVER_REASONS = ["DISCONNECT_REQUESTED", "WRONG_VERSION", "WRONG_SUBPROTOCOL_VERSION", "AUTHENTICATION_ERROR", "USERID_IN_USE", "INVALID_TICKET", "CONNECTION_LOST", "REGISTER_TIMEDOUT", "LOGGEDIN_ELSEWHERE", "ADMIN_KICK", "ADMIN_BAN", "ACCESS_DENIED", "CHANNEL_DESTROYED", "CHANNEL_EXPIRED", "ROAMING", "LEAVING", "DESTROYED_BY_OWNER", "TOKEN_MISSING_ARGUMENTS", "TOKEN_EMPTY_ARGUMENTS", "TOKEN_INVALID_TIMESTAMP", "TOKEN_EXPIRED", "TOKEN_UNKNOWN_OPERATOR", "TOKEN_INVALID_DIGEST", "TOKEN_ACCESS_DENIED"];
var ERRORS = ["SUCCESS", "PROTOCOL_ERROR", "NETWORK_ERROR", "ACCESS_ERROR", "PARAMETER_ERROR", "CAPTURE_DEVICE_ERROR", "PLAYBACK_DEVICE_ERROR", "TOKEN_MISSING_ARGUMENTS", "TOKEN_EMPTY_ARGUMENTS", "TOKEN_INVALID_TIMESTAMP", "TOKEN_EXPIRED", "TOKEN_UNKNOWN_OPERATOR", "TOKEN_INVALID_DIGEST", "TOKEN_ACCESS_DENIED"];
var TOKEN_PREFIX = "SONAR2|";

/**
 * Client gets connected to a voice server.
 * @name SonarVoice#connect
 * @event
 * @param {String} channelId Channel ID of voice channel connected to.
 * @param {String} channelDescription Description of voice channel connected to.
 */

/**
 * Client gets disconnected from a voice server.
 * @name SonarVoice#disconnect
 * @event
 */

/**
 * Client gets connected to voice channel.
 * @name SonarVoice#channeljoin
 * @event
 * @param {String} channelId Channel ID of voice channel connected to.
 * @param {String} channelDescription Description of voice channel connected to.
 */

/**
 * Client gets disconnected from a voice channel.
 * @name SonarVoice#channelpart
 * @event
 * @param {String} reasonType Reason type for being disconnected.
 * @param {String} reasonDescription Reason description.
 */

/**
 * A user joins current channel.
 * @name SonarVoice#channeluserjoin
 * @event
 * @param {String} userId User ID of joined user
 * @param {String} userId User description of joined user
 */

/**
 * A user parts current channel.
 * @name SonarVoice#channeluserpart
 * @event
 * @param {String} userId User ID of user parted
 * @param {String} userDescription User description of user parted
 */

/**
 * A user started or stopped talking.
 * @name SonarVoice#channelusertalk
 * @event
 * @param {String} userId User ID
 * @param {String} userDescription User description
 * @param {String} talking True if user is talking. If muted, the user will not be heard locally. Other users in channel may still hear him/her though.
 * @param {String} muted True if user is muted.
 */

/**
 * A user was muted in local client.
 * @name SonarVoice#channelusermute
 * @event
 * @param {String} userId User ID
 * @param {String} userDescription User description
 * @param {String} talking True if user is talking. If muted, the user will not be heard locally. Other users in channel may still hear him/her though.
 * @param {String} muted True if user is muted.
 */

/**
 * An error occurred.
 * @name SonarVoice#error
 * @event
 * @param {Number} code Numeric error code
 * @param {String} Description Message describing the error
 */

/**
 * The Sonar host process exited.
 * @name SonarVoice#exit
 * @event
 */

/**
 * Playback volume changed.
 * @name SonarVoice#playbackvolume
 * @event
 * @param {Number} volume New playback volume between 0.0 and 1.0.
 */

/**
 * Playback mute changed.
 * @name SonarVoice#playbackmute
 * @event
 * @param {Boolean} muted True if playback became muted
 */

/**
 * Capture volume changed.
 * @name SonarVoice#capturevolume
 * @event
 * @param {Number} volume New capture volume between 0.0 and 1.0.
 */

/**
 * Capture mute changed.
 * @name SonarVoice#capturemute
 * @event
 * @param {Boolean} muted True if capture became muted
 */

/**
 * Hot key changed.
 * @name SonarVoice#hotkey
 * @event
 * @param {String} name Name of hot key (can only be 'ptt', more will follow)
 * @param {String} key New keystroke for hotkey
 */

/**
 * Voice sensitivity changed.
 * @name SonarVoice#voicesensitivity
 * @event
 * @param {Number} sensitivity New sensitivity between 0.0 and 1.0
 */

/**
 * Audio loopback became enabled/disabled.
 * @name SonarVoice#audioloopback
 * @event
 * @param {Boolean} enabled True if audio loopback became enabled
 */

/**
 * Capture mode changed.
 * @name SonarVoice#capturemode
 * @event
 * @param {String} mode New capture mode, can be either 'pushtotalk', 'voiceactivation' or 'continuous'.
 */

/**
 * Audio data changed.
 * This event will not get triggered for wildcard event handlers (such as bind('*', myHandler)
 * @name SonarVoice#audio
 * @event
 * @param {Boolean} clip Not used.
 * @param {Number} avgAmp Average amplitude of data.
 * @param {Number} peakAmp Peak amplitude of data.
 * @param {Boolean} vaState Not used.
 * @param {Boolean} xmit Not used.
 *
 */
    
/**
 * Creates a client and tries initialize the Sonar Host process, which in turn will connect
 * to backend server. If an existing Sonar Host process with matching instance name is running,
 * it will try to re-use it.
 *
 * If the Host is already connected to backend server, nothing will happen.
 *
 * Will verify options passed, test if the plugin is installed then load the browser plugin.
 * @class Sonar JavaScript client
 *
 * @param {String} token Authentication token used to connect to the backend server.
 * @param {Object} [options] Options object parameters to pass. Populate the options object with the parameters below.
 * @param {String} [options.instanceName=window.location.hostname] Instance name for this client. This must be equal to domain of current web page.
 * @param {String} [options.themeXmlUrl] URL to an XML containing theme configuration.
 * @param {String} [options.pluginObjectId=sonarClient] DOM element ID of browser plugin when embedded.
 * @param {String} [options.pluginContainer] What HTML tag to inject the browser plugin under. Set to null to inject in 'body' tag.
 * @param {String} [options.joinChannelTimeout=5000] Timeout in ms to wait for a user-initiated join channel request to complete.
 *
 * @throws {SonarInvalidArgument} If token is missing/invalid or if passed options are invalid.
 * @throws {SonarOutdatedVersion} If Sonar Host or browser plugin is too old and incompatible.
 * @throws {SonarPluginError} If there was a problem initializing/embedding the browser plugin used to communicate with Sonar Host.
 * @throws {SonarError} If there was a problem initializing the Sonar Host.
 */
SonarVoice = function (token, options) {
    var parseOptions = function(options) {
        if (options === null || options === undefined) { options = {}; }

        self.instanceName = options.instanceName !== null && options.instanceName !== undefined ? options.instanceName : defaultInstanceName();
        self.themeXmlUrl = options.themeXmlUrl !== null && options.themeXmlUrl !== undefined ? options.themeXmlUrl : '';
        self.pluginObjectId = options.pluginObjectId !== null && options.pluginObjectId !== undefined ? options.pluginObjectId : 'sonarClient';
        // When null, default to body
        self.pluginContainer = options.pluginContainer !== null && options.pluginContainer !== undefined ? options.pluginContainer : null;
        // Minimum required version of plugin
        self.expectedVersion = EXPECTED_VERSION;
        self.joinChannelTimeout = options.joinChannelTimeout !== null && options.joinChannelTimeout !== undefined ? options.joinChannelTimeout : JOIN_CHANNEL_TIMEOUT;
        return options;
    };

    var isPluginInstalled = function() {

        if (navigator.appName === "Microsoft Internet Explorer") {
            try {
                var control = new ActiveXObject('WHISPERAX.WhisperAxCtrl.' + self.expectedVersion);
                return true;
            } catch (e) {
                return false;
            }
        }

        navigator.plugins.refresh();
        for(var i in navigator.plugins) {
            var plugin = navigator.plugins[i];
            if (!plugin) continue;
            if (plugin.filename &&
                (plugin.filename.indexOf("npesnsonar") != -1 && plugin.description == self.expectedVersion))
                return true;
        }
        return false;
    };

    var hasExpectedVersion = function() {
        return self.client.getVersion() >= self.expectedVersion;
    };

    var validateToken = function(token) {
        return token.substr(0, 7) === TOKEN_PREFIX;
    };

    var versionToHex = function(version) {
        var values = version.split(".");
        var ret = "";

        for (var i = 0; i < values.length; i++) {
            var n = parseInt(values[i], 10);
            n &= 0xff;
            h = n.toString(16);
            if (h.length == 1) {
                h = "0" + h;
            }

            ret += h;
        }
        return ret;
    }

    var embedClient = function() {

        var versionHex = versionToHex(self.expectedVersion);

        var pluginHtml = '<embed width="0" height="0" type="application/mozilla-plugin-esn-sonar-' + self.expectedVersion + '" id="' + self.pluginObjectId + '"></embed>';
        if (navigator.appName === "Microsoft Internet Explorer")
            pluginHtml = '<object width="0" height="0" id="' + self.pluginObjectId + '" classid="CLSID:eba7a1e6-e69d-4ba5-b291-95782a' + versionHex + '" type="application/x-oleobject"></object>';

        var e = document.createElement('div');
        e.id = self.pluginObjectId + "_container";
        e.innerHTML = pluginHtml;

        var container = document.body;
        if (self.pluginContainer) {
            container = document.getElementById(self.pluginContainer);
            if (!container) throw new SonarPluginError("Couldn't find container");
        }

        container.appendChild(e);
        var pe = document.getElementById(self.pluginObjectId);
        if (pe == null)
            throw new SonarPluginError("Couldn't find plugin element '" + self.pluginObjectId + "'");

        self.client = pe;
        return self.client;
    };

    var defaultInstanceName = function() {
        var hostname = window.location.hostname;
        var validIp = /^(([1-9][0-9]{0,2})|0)\.(([1-9][0-9]{0,2})|0)\.(([1-9][0-9]{0,2})|0)\.(([1-9][0-9]{0,2})|0)$/;
        if (validIp.test(hostname))
            return hostname;

        var s = hostname.split('.');
        hostname = s[s.length - 2] + "." + s[s.length - 1];
        return hostname;
    };

    var command = function() {
        var result = client.syncExecCommand(arguments[0], Array.prototype.slice.call(arguments, 1));
        return eval('(' + result + ')');
    };

    var asyncCommand = function() {
        var result = client.asyncExecCommand(arguments[0], Array.prototype.slice.call(arguments, 1));
        return eval('(' + result + ')');
    };

    var onError = function(event, data) {
        self.trigger('error', {code: data.errorCode, description: formatError(data.errorCode, ERRORS)});
    };

    var onDisconnect = function(event, args) {
        args.reasonType = formatError(args.reasonType, VOICE_SERVER_REASONS);
        self.trigger('disconnect', args);
    };

    var onChannelJoin = function(event, args) {
        if (self.joinChannelCallback != null) {
            clearTimeout(self.joinChannelTimeout);
            self.joinChannelCallback(true, "Channel joined");
            self.joinChannelTimeout = null;
            self.joinChannelCallback = null;
        }
        self.trigger('channeljoin', args);
    };
   
    var parseEvent = function(callback) { return function(event, args) { callback(event, eval(args)); }; };
    var parseNamedEvent = function(names, callback) {
        return function (event, args) {
            var arguments = eval(args), namedArgs = {};
            for (var i in names) {
                if (names[i] === null) continue;
                namedArgs[names[i]] = arguments[i];
            }
            callback(event, namedArgs);
        };
    };
    var triggerNamedEvent = function(name, argNames) {
        return parseNamedEvent(argNames, function (internalName, event) {
            self.trigger(name, event);
        });
    };

    var registerEventHandlers = function() {
        client.registerEventHandler("whisper.onconnected", triggerNamedEvent('connect', [null, 'channelId', 'channelDescription']));
        client.registerEventHandler("whisper.ondisconnected", parseNamedEvent(['reasonType', 'reasonDescription'], onDisconnect));
        client.registerEventHandler("whisper.channel.onclientjoined", triggerNamedEvent('channeluserjoin', [null, 'userId', 'userDescription']));
        client.registerEventHandler("whisper.channel.onclientparted", triggerNamedEvent('channeluserpart', [null, 'userId', 'userDescription', 'reason']));
        client.registerEventHandler("whisper.channel.onclientmuted", triggerNamedEvent('channelusermute', [null, 'userId', 'userDescription', 'talking', 'muted']));
        client.registerEventHandler("whisper.channel.onclienttalking", triggerNamedEvent('channelusertalk', [null, 'userId', 'userDescription', 'talking', 'muted']));
        client.registerEventHandler("whisper.onerror", parseNamedEvent(['errorCode'], onError));
        client.registerEventHandler("whisper.menu.exit", triggerNamedEvent('exit', []));
        client.registerEventHandler("whisper.playback.mute", triggerNamedEvent('playbackmute', ['muted']));
        client.registerEventHandler("whisper.capture.mute", triggerNamedEvent('capturemute', ['muted']));
        client.registerEventHandler("whisper.bind.pttkey.stroke", parseEvent(function (event, args) {
            var key = args[0];
            if (key == "") key = null;
            self.trigger("hotkey", {type: 'ptt', key: key});
        }));
    };

    var getUsersInCurrentChannel = function() {
        if (!self.channel()) return [];
        var rawUsers = command("whisper.channel.listclients");
        if (!rawUsers) return [];

        var result = {userIds: [], channelUsers: {}, channelUserIds: {}, users: []};
        for(var i = 0; i < rawUsers.length; i += 5) {
            //var userId = rawUsers[i], userDescription = rawUsers[i + 1], channelUserId = rawUsers[i + 2];
            var channelUserId = rawUsers[i], userId = rawUsers[i + 1], userDescription = rawUsers[i + 2], talking = rawUsers[i + 3], muted = rawUsers[i + 4];
            result.userIds.push(userId);
            result.channelUsers[channelUserId] = {userId: userId, userDescription: userDescription};
            result.channelUserIds[channelUserId] = userId;
            result.users.push({userId: userId, userDescription: userDescription, talking: talking, muted: muted});
        }

        return result;
    };

    var getUser = function (userId) {
        var users = getUsersInCurrentChannel().users;
        for (var i in users) {
            if (users[i].userId == userId)
                return users[i];
        }
        return null;
    };

    var addAudioCallback = function() {
        if (audioCallbackEnabled) return;
        client.registerEventHandler("whisper.capture.onaudio", triggerNamedEvent('audio', ['clip', 'avgAmp', 'peakAmp', 'vaState', 'xmit']));
    };

    var formatError = function(errorCode, errorList) {
        try { return errorList[errorCode]; } catch (e) { return errorCode; }
    };

    var getDevices = function(type) {
        var devices = command(type);
        var returnDevices = [];
        for (var i = 0; i < devices.length; i += 2)
            returnDevices.push({deviceId: devices[i], deviceName: devices[i + 1]});
        return returnDevices;
    };

    var verifyVolume = function(volume) {
        if ((volume - 0) != volume || volume.length <= 0 || volume < 0.0 || volume > 1.0)
            throw new SonarInvalidArgument('Invalid volume ' + volume + ' supplied. Must be between 0.0 and 1.0');
    };

    var toWhisperVolume = function(volume) { return roundNumber(volume * 10, 2); };
    var fromWhisperVolume = function(volume) { return roundNumber(volume / 10, 2); };
    var roundNumber = function(number, digits) {
        var multiple = Math.pow(10, digits);
        return Math.round(number * multiple) / multiple;
    };

    // Public functions

    /**
     * Bind an event to a given callback function.
     * See the Event section a for a complete list of all available events. 
     * @param {String} event Event name. Passing '*' will bind the callback to all events fired.
     * @param {Function} callback Callback to bind event to
     */
    this.bind = function(event, callback) {
        var calls = this._callbacks || (this._callbacks = {});
        var list  = this._callbacks[event] || (this._callbacks[event] = []);
        list.push(callback);
        if (event == 'audio') {
           addAudioCallback();
        }
        return this;
    };

    /**
     * Remove one or many callbacks.
     * @param {String} event Event which callbacks should be removed. If null, all callbacks will be removed.
     * @param {Function} callback Callback to remove. If null, removes all callbacks for that event.
     */
    this.unbind = function(event, callback) {
        var calls;
        if (!event) {
            this._callbacks = {};
        } else if (calls = this._callbacks) {
            if (!callback) {
               calls[event] = [];
            } else {
                var list = calls[event];
                if (!list) return this;
                for (var i = 0, l = list.length; i < l; i++) {
                    if (callback === list[i]) {
                        list.splice(i, 1);
                        break;
                    }
                }
            }
        }
        return this;
    };

    // Trigger an event, firing all bound callbacks. Callbacks are passed the
    // same arguments as `trigger` is, apart from the event name.
    // Listening for `"*"` passes the true event name as the first argument.
    this.trigger = function(event) {
        var list, calls, i, l;
        if (!(calls = this._callbacks)) return this;

        arguments[1].name = event;
        if (list = calls[event])
            for (i = 0, l = list.length; i < l; i++)
                list[i].apply(this, Array.prototype.slice.call(arguments, 1));

        if ((list = calls['*']) && event !== 'audio')
            for (i = 0, l = list.length; i < l; i++)
                list[i].apply(this, Array.prototype.slice.call(arguments, 1));

        return this;
    };

    /**
     * Get the user ID of the local user currently connected to Sonar.
     * @returns {String} User ID
     */
    this.userId = function() {
        return client.getVariable('whisper.userId');
    };

    /**
     * Get the user description of the local user currently connected to Sonar.
     * @returns {String} User ID
     */
    this.userDescription = function() {
        return client.getVariable('whisper.userDesc');
    };

    /**
     * Get current channel for user.
     * @returns {Object} Current channel with property 'id' and 'description'.
     */
    this.channel = function() {
        var channelId = client.getVariable('whisper.channelid');
        if (channelId == '')
            return null;
        return {id: channelId, description: client.getVariable('whisper.channeldesc')};
    };

    /**
     * Get users in current channel.
     * @returns {Array} List of users containing objects with properties userId, userDescription, talking and muted.
     */
    this.usersInChannel = function() {
        if (self.channel() == null)
            return [];
        
        return getUsersInCurrentChannel().users;
    };

    /**
     * Get the push-to-talk hot key.
     * Returns key stroke in Windows virtual-key code format.
     * Example: <code>LCONTROL</code> or <code>NUMPAD4</code>
     * For a list of all available key codes on Windows, please see the
     * <a href="http://msdn.microsoft.com/en-us/library/dd375731(VS.85).aspx">MSDN documentation</a>.
     * @returns {String} Hot key
     */
    this.pushToTalkKey = function() {
        return client.getVariable("whisper.bind.pttkey.stroke");
    };

    /**
     * Records a keystroke.
     * Will asynchronously record a keystroke for the push-to-talk hot key (Esc button will cancel).
     * When the keystroke has been recorded by the Sonar client, an event will be triggered.
     * @see SonarVoice#pushToTalkKey for more info on the key code format.
     * @see SonarVoice#hotkey for more details on the triggered event
     */
    this.recordPushToTalkKey = function() {
        asyncCommand("whisper.bindpttkey");
        return false;
    };

    /**
     * Get all available capture devices.
     * @returns {Array} A list of capture devices
     * @see SonarVoice#captureDevice for more information on capture devices
     */
    this.captureDevices = function() {
        return getDevices("audio.capture.listdevices");
    };

    /**
     * Get/set the capture device.
     * @param {String} [deviceId] ID of new capture device. If not set, function will just return current value.
     * @returns {Object} Current capture device
     */
    this.captureDevice = function(deviceId) {
        if (deviceId !== undefined) {
            client.setVariable("audio.capture.deviceid", deviceId);
            command("audio.stop");
            command("audio.start");
            return deviceId;
        }
        return client.getVariable("audio.capture.deviceid");
    };

    /**
     * Get all available playback devices.
     * @returns {Array} A list of playback devices
     * @see SonarVoice#playbackDevice for more information on playback devices
     */
    this.playbackDevices = function() {
        return getDevices("audio.playback.listdevices");
    };

    /**
     * Get/set the playback device.
     * @param {String} [deviceId] ID of new playback device. If not set, function will just return current value.
     * @returns {Object} Current playback device
     */
    this.playbackDevice = function(deviceId) {
        if (deviceId !== undefined) {
            client.setVariable("audio.playback.deviceid", deviceId);
            command("audio.stop");
            command("audio.start");
        }
        return client.getVariable("audio.playback.deviceid");
    };

    /**
     * Get/set voice sensitivity.
     * Voice sensitivity is used for triggering voice activation.
     * Sensitivity is specified between 0.0 and 1.0, where 1.0 is the most sensitive.
     * @param {Number} [sensitivity] Sensitivity to be set. If not set, function will just return current value.
     * @returns {Number} Current sensitivity.
     * @throws {SonarInvalidArgument} If sensitivity is not valid range of 0.0 and 1.0.
     */
    this.voiceSensitivity = function(sensitivity) {
        if (sensitivity !== undefined) {
            verifyVolume(sensitivity);
            client.setVariable("audio.capture.voicesensitivity", parseInt(sensitivity * 100));
            self.trigger("voicesensitivity", {sensitivity: sensitivity});
            return sensitivity;
        }
        return roundNumber(client.getVariable("audio.capture.voicesensitivity") / 100, 2);
    };

    /**
     * Get/set current playback volume.
     * Playback volume determines how loud other users and sound effects will be.
     * Volume is specified between 0.0 and 1.0, where 1.0 is the loudest.
     * @param {Number} [volume] Volume to be set. If not set, function will just return current value.
     * @returns {Number} Current volume.
     * @throws {SonarInvalidArgument} If volume is not valid range of 0.0 and 1.0.
     */
    this.playbackVolume = function(volume) {
        if (volume !== undefined) {
            verifyVolume(volume);
            client.setVariable("audio.playback.volume", toWhisperVolume(volume));
            self.trigger("playbackvolume", {volume: volume});
            return volume;
        }
        return fromWhisperVolume(client.getVariable("audio.playback.volume"));
    };

    /**
     * Set capture volume.
     * Capture volume determines how much sound to pick up from the user's microphone.
     * Volume is specified between 0.0 and 1.0, where 1.0 is the loudest.
     * @param {Number} [volume] Volume to be set. If not set, function will just return current value.
     * @returns {Number} Current volume.
     * @throws {SonarInvalidArgument} If volume is not valid range of 0.0 and 1.0.
     */
    this.captureVolume = function(volume) {
        if (volume !== undefined) {
            verifyVolume(volume);
            client.setVariable("audio.capture.volume", toWhisperVolume(volume));
            self.trigger("capturevolume", {volume: volume});
            return volume;
        }
        return fromWhisperVolume(client.getVariable("audio.capture.volume"));
    };

    /**
     * Get/set playback to be muted/unmuted.
     * @param {Boolean} [muted] True if playback should be muted. If not set, function will just return current value.
     * @returns {Boolean} Current mute state, true if muted.
     */
    this.playbackMuted = function(muted) {
        if (muted !== undefined) {
            client.setVariable("whisper.playback.mute", muted);
            return muted;
        }
        return client.getVariable("whisper.playback.mute");
    };

    /**
     * Toggle playback to be muted/unmuted.
     * @returns {Boolean} True if playback was muted
     */
    this.togglePlaybackMuted = function() {
        return this.playbackMuted(!client.getVariable("whisper.playback.mute"));
    };

    /**
     * Get/set capture to be muted/unmuted.
     * @param {Boolean} muted True if capture should be muted. If not set, function will just return current value.
     * @returns {Boolean} Current mute state, true if muted.
     */
    this.captureMuted = function(muted) {
        if (muted !== undefined) {
            client.setVariable("whisper.capture.mute", muted);
            return muted;
        }
        return client.getVariable("whisper.capture.mute");
    };

    /**
     * Toggle capture to be muted/unmuted.
     * @returns {Boolean} True if capture was muted
     */
    this.toggleCaptureMuted = function() {
        return this.captureMuted(!client.getVariable("whisper.capture.mute"));
    };

    /**
     * Set audio loopback to be enabled/disabled.
     * With audio loopback enabled, user will hear her own voice looping back through the speakers.
     * Useful for helping users tune their capture settings.
     * @params {Boolean} [enabled] True if audio loopback should be enabled. If not set, function will just return current value.
     * @returns {Boolean} Current audio loopback state, true if enabled.
     */
    this.audioLoopback = function(enabled) {
        if (enabled !== undefined) {
            command("audio.setloopback", enabled);
            self.trigger("audioloopback", {enabled: enabled});
            return enabled;
        }
        return client.getVariable("audio.loopbackenabled");
    };

    /**
     * Toggle audio loopback to be enabled/disabled.
     * @see SonarVoice#audioLoopback for more information.
     * @returns {Boolean} Current audio loopback state, true if enabled.
     */
    this.toggleAudioLoopback = function() {
        return self.audioLoopback(!self.audioLoopback());
    };

    /**
     * Check if voice activation is enabled.
     * This is merely a helper function for captureMode() providing
     * an easier way to check if voice activation is enabled.
     * @see SonarVoice#captureMode for more information.
     * @returns {Boolean} True if voice activation is enabled.
     */
    this.voiceActivation = function() {
        return self.captureMode() == 'voiceactivation';
    };

    /**
     * Get/set capture mode.
     * Sonar can operate in two different capture modes.
     * One is push-to-talk, where the user has to press a key to start transmitting.
     * The other is voice activation where the user sets a certain threshold in capture
     * volume to start transmitting instead.
     * Valid mode names are 'pushtotalk', 'voiceactivation' or 'continuous'.
     * @param {String} [mode] Capture mode to be set. If not set, function will just return current value.
     * @returns {String} Current capture mode.
     * @throws {SonarInvalidArgument} If suppliled mode parameter is not a valid capture mode.
     */
    this.captureMode = function(mode) {
        if (mode !== undefined) {
            if (mode !== 'pushtotalk' && mode !== 'voiceactivation' && mode !== 'continuous')
               throw new SonarInvalidArgument('Trying to set invalid capture mode: ' + mode);
            var newMode = null;
            if (mode === 'pushtotalk') newMode = CAPTURE_MODE_PUSH_TO_TALK;
            if (mode === 'voiceactivation') newMode = CAPTURE_MODE_VOICE_ACTIVATION;
            if (mode === 'continuous') newMode = CAPTURE_MODE_CONTINUOUS;

            command("whisper.capture.setmode", newMode);
            self.trigger("capturemode", {mode: mode});
            return mode;
        }

        var m = client.getVariable("whisper.capture.mode");
        if (m == CAPTURE_MODE_PUSH_TO_TALK) return 'pushtotalk';
        if (m == CAPTURE_MODE_VOICE_ACTIVATION) return 'voiceactivation';

        return 'continuous';
    };

    /**
     * Leave any channel currently connected to.
     * Can also be done through the Sonar Host process by using the tray icon.
     * @param {String} [reasonDescription="JavaScript client parted from channel"] Reason for leaving channel in human-readble form.
     * @param {String} [reasonType="JS_CLIENT_PARTED"] Reason type for leaving channel.
     * @throws {SonarError} If part channel for some reason failed to complete.
     */
    this.partChannel = function(reasonType, reasonDescription) {
        if (reasonDescription === undefined) { reasonDescription = "JavaScript client parted from channel"; }
        if (reasonType === undefined) { reasonType = "JS_CLIENT_PARTED"; }

        if (!command('whisper.partchannel', reasonType, reasonDescription)) {
            throw new SonarError("Error trying to leave channel");
        }
    };

    /**
     * Mute a user locally.
     * @param {String} userId User ID to mute/unmute.
     * @param {Boolean} [mute] True to mute the user. If not set, function will just return current value.
     * @throws {SonarInvalidArgument} If requested user ID is not found.
     * @returns {String} True if user is muted.
     */
    this.userMuted = function(userId, mute) {
        if (mute !== undefined) {
            command('whisper.channel.muteclient', userId, mute);
            return mute;
        }

        var user = getUser(userId);
        if (user == null)
            throw new SonarInvalidArgument("User with user ID " + userId + " not in channel.");

        return user.muted;
    };

    /**
     * Toggle user to be muted/unmuted.
     * @see SonarVoice#userMuted for more information.
     * @throws {SonarInvalidArgument} If requested user ID is not found
     * @returns {Boolean} True if user is muted.
     */
    this.toggleUserMuted = function(userId) {
        return self.userMuted(userId, !self.userMuted(userId));
    };

    /**
     * Enable/disable echo cancellation.
     * Echo cancellation can be used reduce the acoustic echo that sometimes occur when voice chatting.
     * @param {Boolean} [enable] True to enabled echo cancellation. If not set, function will just return current value.
     * @returns {String} True if echo cancellation is enabled.
     */
    this.echoCancellation = function(enable) {
        if (enable !== undefined) {
            client.setVariable("audio.capture.echo", enable);
            command("audio.stop");
            command("audio.start");
            return enable;
        }

        return client.getVariable("audio.capture.echo");
    };

    /**
     * Toggle echo cancellation to be enabled/disabled.
     * @see SonarVoice#echoCancellation for more information.
     * @returns {Boolean} True if echo cancellation is enabled.
     */
    this.toggleEchoCancellation = function() {
        return self.echoCancellation(!self.echoCancellation());
    };

    var self = this;
    var audioCallbackEnabled = false;
    this.options = parseOptions(options);
    this.token = token;
    self.joinChannelCallback = null;
    self._command = command;

    if (token === null || token === undefined)
        throw new SonarInvalidArgument('No token supplied');

    if (!validateToken(token))
        throw new SonarInvalidArgument('Invalid token supplied. Must start with ' + TOKEN_PREFIX);

    if (!isPluginInstalled())
        throw new SonarPluginError('Plugin not installed');

    var client = embedClient();

    if (!hasExpectedVersion())
        throw new SonarOutdatedVersion('Plugin version is too old');

    if (!client.setupInstance(this.instanceName, token, this.themeXmlUrl))
        throw new SonarError("Couldn't initialize the Sonar host instance");

    client.setVariable('whisper.capture.onaudio.skiprate', 0.3);
    var capMode = client.getVariable("whisper.capture.mode");
    if (capMode !== CAPTURE_MODE_PUSH_TO_TALK && capMode !== CAPTURE_MODE_VOICE_ACTIVATION && capMode !== CAPTURE_MODE_CONTINUOUS)
        this.captureMode('pushtotalk');

    registerEventHandlers();
    return this;
}

})();
