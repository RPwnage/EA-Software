class Sonar
    constructor: (options) ->
        @CAPTURE_MODE_PUSH_TO_TALK = 1
        @CAPTURE_MODE_VOICE_ACTIVATION = 2
        @handlers = {}
        @_parseOptions options
        throw new Error 'Plugin not installed' unless @_isPluginInstalled()
        @_embedClient()
        throw new Error 'Plugin version is too old' unless @_hasMinRequiredVersion()

    bind: (name, handler) ->
        @handlers[name] = @handlers[name] || []
        @handlers[name].push handler
    
    unbind: (name, handler) ->
        if not name?
            @handlers = {}
            return

        h = @handlers[name]
        for i in [0...h.length]
            h.splice i, 1 if handler is h[i]
        
    trigger: (type, data) ->
        if @handlers['*']?
            for handler in @handlers['*']
                handler {type: type, data: data}

        return unless @handlers[type]?
        for handler in @handlers[type]
            handler {type: type, data: data}

    connect: ->
        if not @client.setupInstance @options.instanceName, @options.token, @options.themeXmlUrl
            throw new Error "Couldn't set up Sonar instance"

        @setVariable 'whisper.capture.onaudio.skiprate', 0.3
        capMode = @client.getVariable "whisper.capture.mode"

        # TODO: Why do we do this?
        if capMode != @CAPTURE_MODE_PUSH_TO_TALK && capMode != @CAPTURE_MODE_VOICE_ACTIVATION
            @setCaptureMode @CAPTURE_MODE_VOICE_ACTIVATION

        @_registerEventHandlers()
        # TODO: Do the ensure connected to server? Why?

    getVariable: (name) -> @client.getVariable name
    setVariable: (name, value) -> @client.setVariable name, value

    #addAudioCallback: -> @client.registerEventHandler "whisper.capture.onaudio", @_onAudio

    getUserId: -> @getVariable 'whisper.userId'
    getUserDescription: -> @getVariable 'whisper.username'
    getCurrentChannel: ->
        return null unless @client
        {id: @getVariable('whisper.channelid'), description: @getVariable('whisper.channeldesc')}

    getUsersInCurrentChannel: ->
        return [] unless @getCurrentChannel()
        rawUsers = @client.syncExecCommand "voice.channel.listclients", new Array()
        return [] unless users
        rawUsers = eval '(' + rawUsers + ')'

        users = []
        for i in [0...rawUsers.length] by 3
            users.push {userId: rawUsers[i], username: rawUsers[i + 1], channelId: rawUsers[i + 2]}

        return users

    # PTT html functions

    setPushToTalkKey: (key) ->
        key = [key]

        # Set nonblocking as default, change to true if we don't want this feature
        blockingkey = false
        key.push blockingkey

        if @client.syncExecCommand("whisper.capture.ptt.sethotkey", key) == "[]"
            throw new Error "Couldn't set push-to-talk hot key"

        # TODO: Trigger this ESN.Sonar.trigger("whisper.capture.ptt.sethotkey", {"key":key[0], "message": "Push to talk key is set"});

    getPushToTalkKey: -> @getVariable 'whisper.capture.ptt.hotkey'

    setCaptureDevice: (deviceId) ->
        @setVariable 'audio.capture.deviceid', deviceId
        @client.syncExecCommand 'audio.stop', []
        @client.syncExecCommand 'audio.start', []

    setPlaybackDevice: (deviceId) ->
        @setVariable 'audio.playback.deviceid', deviceId
        @client.syncExecCommand 'audio.stop', []
        @client.syncExecCommand 'audio.start', []

    getCaptureDevices: ->
        rawDevices = @client.syncExecCommand "audio.capture.listdevices", []
        rawDevices = eval '(' + rawDevices + ')'

        devices = [];
        for i in [0...rawDevices.length] by 2
            devices.push {deviceId: deviceId[i], deviceName: deviceName[i + 1]}

        return devices

    getPlaybackDevice: -> @client.getVariable 'audio.playback.deviceid'
    getVoiceSensitivity: -> @client.getVariable 'audio.capture.voicesensitivity'
    setVoiceSensitivity: (value) ->
        @setVariable 'audio.capture.voicesensitivity', parseInt(value)
        # TODO: Trigger ESN.Sonar.trigger("audio.capture.voicesensitivity", {"value": parseInt(value)});

    getCaptureVolume: -> @getVariable 'audio.capture.volume'
    setCaptureVolume: (value) ->
        value = parseFloat value
        @setVariable 'audio.capture.volume', value
        # TODO: Trigger ESN.Sonar.trigger("audio.capture.volume", {"value": value});

    getCaptureDevice: -> @getVariable 'audio.capture.deviceid'
    isAudioLoopbackEnabled: -> @getVariable 'audio.loopbackenabled'
    isVoiceActivationEnabled: -> @getVariable('whisper.capture.mode') == @CAPTURE_MODE_VOICE_ACTIVATION
    getCaptureMode: -> @getVariable 'whisper.capture.mode'
    getPlaybackDevices: ->
        rawDevices = @client.syncExecCommand "audio.playback.listdevices", []
        rawDevices = eval '(' + rawDevices + ')'

        devices = [];
        for i in [0...rawDevices.length] by 2
            devices.push {deviceId: deviceId[i], deviceName: deviceName[i + 1]}

        return devices

    getPlaybackVolume: -> @getVariable 'audio.playback.volume'
    setPlaybackVolume: (volume) ->
        volume /= 10; # TODO: Set this in a logarithmic scale instead, 0-8 is the min, max in VOIPlay.
        @setVariable 'audio.playback.volume', volume
        # TODO: Trigger ESN.Sonar.trigger("audio.playback.volume", {"volume": volume});

    isCaptureMuted: -> @getVariable 'whisper.capture.mute'
    isPlaybackMuted: -> @getVariable 'whisper.playback.mute'
    setPlaybackMuted: (muted) -> @setVariable 'whisper.playback.mute', muted
    setCaptureMuted: (muted) -> @setVariable 'whisper.capture.mute', muted
    toggleCapture: -> @setCaptureMuted !@getVariable("whisper.capture.mute")
    togglePlayback: -> @setPlaybackMuted !@getVariable("whisper.playback.mute")

    toggleAudioLoopback: ->
        loopback = !@isAudioLoopbackEnabled()
        @client.syncExecCommand "audio.setloopback", loopback
        # TODO: Trigger ESN.Sonar.trigger("audio.setloopback", {"state": loopback});

    toggleVoiceActivation: ->
        state = if @isVoiceActivationEnabled() then @CAPTURE_MODE_PUSH_TO_TALK else @CAPTURE_MODE_VOICE_ACTIVATION
        @client.syncExecCommand 'whisper.capture.setmode', state
        # TODO: TriggerESN.Sonar.trigger("whisper.capture.setmode", {"state": state});




    setCaptureMode: (mode) ->
        if mode != @CAPTURE_MODE_PUSH_TO_TALK && mode != @CAPTURE_MODE_VOICE_ACTIVATION
            throw new Error 'Trying to set unknown capture mode: ' + mode

        @client.syncExecCommand "whisper.capture.setmode", mode
        #@trigger("whisper.capture.setmode", {"state": mode});


    _onConnect: (event, args) =>
        args = eval(args);
        console.log 'onConnect'
        console.log 'this', this
        @trigger("voice.onconnect", args) # TODO: Fix trigger

    _onDisconnect: (event, args) =>
        myargs = @_formatCallbackArguments(["reason", "voiceserverReason", "description"], args)
        myargs.voiceserverDescription = @_formatVoiceServerReason(myargs.voiceserverReason)
        @trigger("voice.ondisconnect", myargs); # TODO: Fix trigger

    _voiceChannelOnParted: (event, args) =>
        myargs = @_formatCallbackArguments( ["reason", "voiceserverReason", "description"], args)
        myargs.voiceserverDescription = @_formatVoiceServerReason(myargs.voiceserverReason)
        @trigger("voice.channel.onparted", myargs) # TODO: Fix trigger


    _voiceChannelOnClientJoin: (event, args) =>
        myargs = @_formatCallbackArguments(["userId", "userInfo", "clientChid", "clientSubChannelState", "silent"], args)
        @trigger("voice.channel.onclientjoin", myargs) # TODO: Fix trigger

    _voiceChannelOnJoined: (event, args) =>
        myargs = @_formatCallbackArguments(["channelId", "channelDesc"], args)
        @trigger("voice.channel.onjoined", myargs) # TODO: Fix trigger

    _voiceChannelOnClientPart: (event, args) =>
        myargs = @_formatCallbackArguments(["clientChid", "clientName", "reasonString"], args)
        @trigger("voice.channel.onclientpart", myargs) # TODO: Fix trigger

    _onError: (event, args) =>
        errors = [
            "WHSR_SUCCESS",
            "WHSR_PROTOCOL_ERROR",
            "WHSR_NETWORK_ERROR",
            "WHSR_ACCESS_ERROR",
            "WHSR_PARAMETER_ERROR",
            "WHSR_CAPTURE_DEVICE_ERROR",
            "WHSR_PLAYBACK_DEVICE_ERROR",
            "WHSR_TOKEN_MISSING_ARGUMENTS",
            "WHSR_TOKEN_EMPTY_ARGUMENTS",
            "WHSR_TOKEN_INVALID_TIMESTAMP",
            "WHSR_TOKEN_EXPIRED",
            "WHSR_TOKEN_UNKNOWN_OPERATOR",
            "WHSR_TOKEN_INVALID_DIGEST",
            "WHSR_TOKEN_ACCESS_DENIED"]

        args = eval(args)
        myargs = {"error": errors[args[0]]}
        @trigger("whisper.onerror", myargs)

    _onAudio: (event, args) =>
        try
            myargs = @_formatCallbackArguments(["clip", "avgAmp", "peakAmp", "vaState", "xmit", "audioFrame"], args)
            @trigger("audio.capture.onaudio", myargs)
        catch error
            a = 1

    _onVoiceTalkers: (event, args) =>
        args = eval(args)
        talkers = []
        for i in [0...args.length] by 2
            talkers.push args[i]

        try
            @trigger("whisper.voice.talkers", {talkers: talkers});
        catch error
            a = 1 # TODO: Fix?

    _onMenuExit: (event, args) => @trigger "whisper.menu.exit", {}
    _onPlaybackMute: (event, args) =>
        args = eval args
        try
            @trigger "whisper.playback.mute", {muted: args[0]}
        catch error
            a = 1 # TODO: Fix? Silent ignore

    _onCaptureMute: (event, args) =>
        args = eval args
        try
            @trigger "whisper.capture.mute", {"muted": args[0]}
        catch error
            a = 1 # TODO: Fix? Silent ignore

    _parseOptions: (options) ->
        throw new Error('Options not set') unless options?
        @options = options
        throw new Error('Token not set') unless @options.token?
        @options.token = options.token
        @options.instanceName ?= window.location.hostname
        @options.themeXmlUrl ?= ''
        @options.pluginObjectId ?= 'sonarClient'
        @options.pluginContainer ?= null # Default to body
        @options.minVersion ?= '0.18.0' # Minimum required version of plugin

    _isPluginInstalled: ->
        return true if navigator.appName isnt "Opera" and navigator.appName isnt "Netscape"
        navigator.plugins.refresh();
        for plugin in navigator.plugins
            return yes if plugin.filename?.indexOf("npesnsonar") isnt -1
        no

    _embedClient: ->
        pluginHtml = '<embed width="0" height="0" type="application/mozilla-plugin-esn-sonar" id="' + @options.pluginObjectId + '"></embed>'
        if navigator.appName is "Microsoft Internet Explorer"
            pluginHtml = '<object width="0" height="0" id="' + @options.pluginObjectId + '" classid="CLSID:eba7a1e6-e69d-4ba5-b291-95782aadf24a" type="application/x-oleobject"></object>'

        e = document.createElement 'div'
        e.id = @options.pluginObjectId + "_container"
        e.innerHTML = pluginHtml

        container = document.body
        if @pluginContainer
            container = document.getElementById @options.pluginContainer
            throw new Error "Couldn't find container" unless container
            
        container.appendChild e
        @client = document.getElementById @options.pluginObjectId

    _hasMinRequiredVersion: ->
        [major, minor, patch] = parseInt v for v in @client.getVersion().split "."
        [minMajor, minMinor, minPatch] = parseInt v for v in @options.minVersion.split "."
        return major >= minMajor && minor >= minMinor

    _registerEventHandlers: ->
        @client.registerEventHandler "whisper.voice.talkers", @_onVoiceTalkers
        @client.registerEventHandler "voice.onconnect", @_onConnect
        @client.registerEventHandler "voice.ondisconnect", @_onDisconnect
        @client.registerEventHandler "voice.channel.onparted", @_voiceChannelOnParted
        @client.registerEventHandler "voice.channel.onclientjoin", @_voiceChannelOnClientJoin
        @client.registerEventHandler "voice.channel.onclientpart", @_voiceChannelOnClientPart
        @client.registerEventHandler "voice.channel.onJoined", @_voiceChannelOnJoined
        @client.registerEventHandler "whisper.onerror", @_onError
        @client.registerEventHandler "whisper.menu.exit", @_onMenuExit
        @client.registerEventHandler "whisper.capture.mute", @_onCaptureMute
        @client.registerEventHandler "whisper.playback.mute", @_onPlaybackMute

    _formatCallbackArguments: (argNames, args) ->
        args = eval(args);
        myargs = {};
        for i in [0...argNames.length]
            myargs[argNames[i]] = args[i]

        return myargs

    _formatVoiceServerReason: (reason) ->
        reasons = [
            "VSDR_DISCONNECT_REQUESTED",
            "VSDR_WRONG_VERSION",
            "VSDR_WRONG_SUBPROTOCOL_VERSION",
            "VSDR_AUTHENTICATION_ERROR",
            "VSDR_USERID_IN_USE",
            "VSDR_INVALID_TICKET",
            "VSDR_CONNECTION_LOST",
            "VSDR_REGISTER_TIMEDOUT",
            "VSDR_LOGGEDIN_ELSEWHERE",
            "VSDR_ADMIN_KICK",
            "VSDR_ADMIN_BAN",
            "VSDR_ACCESS_DENIED",
            "VSDR_CHANNEL_DESTROYED",
            "VSDR_CHANNEL_EXPIRED",
            "VSDR_ROAMING",
            "VSDR_LEAVING",
            "VSDR_DESTROYED_BY_OWNER",
            "VSDR_TOKEN_MISSING_ARGUMENTS",
            "VSDR_TOKEN_EMPTY_ARGUMENTS",
            "VSDR_TOKEN_INVALID_TIMESTAMP",
            "VSDR_TOKEN_EXPIRED",
            "VSDR_TOKEN_UNKNOWN_OPERATOR",
            "VSDR_TOKEN_INVALID_DIGEST",
            "VSDR_TOKEN_ACCESS_DENIED"
        ]

        try
            return reasons[reason];
        catch error
            return reason;

console.log 'sonar.coffee declared'


username = "charlie"
$.getJSON "/api/default/users/" + username + "/control-token", (token) ->
    console.log "Token is " + token
    sonar = new Sonar token: token
    sonar.connect()
    sonar.bind '*', (event) -> console.log 'Event', event
    sonar.setVariable "voice.subchannel.mask", 1

    $.ajax({
        url: "/api/default/users/" + username + "/channel",
        type: 'PUT',
        dataType: 'json',
        data: "testchannel",
        success: (result) ->
            console.log("join channel result: " + result)
            sonar.setVariable "voice.subchannel.mask", 1
    })

