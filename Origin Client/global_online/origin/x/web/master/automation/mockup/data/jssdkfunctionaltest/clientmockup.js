console.log('loading clientmockup.js');

// Wow !!!
// http://codeforthecloset.blogspot.ca/2008/12/signals-and-slots-for-javascript.html
var Signal = function (stateful) {
    var slots = [];
    /* _signal : Proxy Function
     * Acts as a multicast proxy to the functions connected to it, passing along
     * the arguments it was invoked with
     */
    var _signal = function() {
        var arglist = Array.prototype.slice.call(arguments);
        if (stateful)
            arglist.unshift(this);
        for (var j = 0; j < slots.length; j++) {
            var obj = slots[j][0];
            if (obj === undefined)
                obj = this;
            var fun = slots[j][1];
            try {
                fun.apply(obj, arglist);
            }catch(e) {
            }
        }
    }
    /* _signal.connect: Function
     * Connects a function and the scope to be called when the signal is invoked.
     * fun - The function to be invoked on signal.
     * obj - The scope
     */
    _signal.connect = function(fun, scope) {
        slots.push([scope,fun]);
    }
    /* _signal.disconnect: Function
     * Disconnects a matching function from a signal.
     * fun - The function to be removed.
     * obj - The scope
     */
    _signal.disconnect = function(fun, scope) {
        var shift = false;
        for (var i = 0; i < slots.length; i++) {
            if (shift)
                slots[i - 1] = slots[i];
            else if (scope === slots[i][0] && fun === slots[i][1])
                shift=true;
        }
        if (shift)
            slots.pop();
    }
    _signal.disconnect_all = function() {
        var slen = slots.length;
        for (var i = 0; i < slen; i++) {
            slots.pop();
        }
    }
    return _signal;
};

window.OriginGamesManager = {
    isGamePlaying : false,
    requestGamesStatus : function() {return [];},
    onRemoteGameAction : function(prodId, actionName, args) {return {};},
    changed : Signal(false),
    progressChanged : Signal(false),
    basegamesupdated : Signal(false),
    operationFailed : Signal(false),
    downloadQueueChanged : Signal(false)
};

window.OriginOnlineStatus = {
    onlineState : true,
    requestOnlineMode : function() {
        window.OriginOnlineStatus.onlineState = true;
        setTimeout(function() {
            window.OriginOnlineStatus.onlineStateChanged(true);
        }, 0);
    },
    onlineStateChanged : Signal(false)
};

window.OriginClientSettings = {
	hotkeyConflictSwap: { 'Ctrl-S': 'Save Game'},
	supportedLanguagesData: {
				'en-US': 'US English',
				'en-CA': 'Canadian English',
				'fr-CA': 'Canadian Frnech'
			},
	hotkeyInputHasFocus: function(hasFocus) {return true;},
	pinHotkeyInputHasFocus: function(hasFocus) {return true;},
	readSetting: function(settingName) {return false;},
	startLocalHostResponder: function() { return "Host Responder Started";},
	startLocalHostResponderFromOptOut: function() { return "Host Responder Started From Opt Out";},
	stopLocalHostResponder: function() { return "Host Responder Stopped";},
	writeSetting: function(settingName, payload) { return "Setting Changed";},
	updateSettings : Signal(false),
    returnFromSettingsDialog : Signal(false),
    settingsError : Signal(false),
};

window.OriginSocialManager = {
    isConnectionEstablished : function() {return true;},
    isRosterLoaded : function() {return true;},
    roster : function() {return [];},
    sendMessage : function(id, msgBody, type) {},
    setTypingState : function(state, userId) {},
    approveSubscriptionRequest : function(jid) {
        setTimeout(function() {
            window.OriginSocialManager.rosterChanged({});
        }, 0);
        return {};
    },
    denySubscriptionRequest : function(jid) {
        setTimeout(function() {
            window.OriginSocialManager.rosterChanged({});
        }, 0);
        return {};
    },
    subscriptionRequest : function(jid) {
        setTimeout(function() {
            window.OriginSocialManager.rosterChanged({});
        }, 0);
        return {};
    },
    cancelSubscriptionRequest : function(jid) {
        setTimeout(function() {
            window.OriginSocialManager.rosterChanged({});
        }, 0);
        return {};
    },
    removeFriend : function(jid) {
        setTimeout(function() {
            window.OriginSocialManager.rosterChanged({});
        }, 0);
        return {};
    },
    setInitialPresence : function(presence) {},
    requestInitialPresenceForUserAndFriends : function() {return [{}];},
    requestPresenceChange : function(presence) {},
    joinRoom : function(jid, originId) {return {};},
    leaveRoom : function(jid, originId) {return {};},
    blockUser : function(jid) {
        setTimeout(function() {
            window.OriginSocialManager.blockListChanged({});
        }, 0);
        return {};
    },
    unblockUser : function(jid) {
        setTimeout(function() {
            window.OriginSocialManager.blockListChanged({});
        }, 0);
        return {};
    },
    isBlocked : function(jid) {return true;},
    connectionChanged : Signal(false),
    messageReceived : Signal(false),
    chatStateReceived : Signal(false),
    presenceChanged : Signal(false),
    blockListChanged : Signal(false),
    rosterChanged : Signal(false),
    rosterLoaded : Signal(false)
};

window.InstallDirectoryManager = {
	browseInstallerCache: function() {return {};},
	chooseInstallerCacheLocation: function() {return {};},
	chooseDownloadInPlaceLocation: function() {return {};},
	deleteInstallers: function() {return {};},
	resetDownloadInPlaceLocation: function() {return {};},
	resetInstallerCacheLocation: function() {return {};}
	
};
window.OriginVoice = {
	supported: true,
	audioInputDevices: {'devices': 0},
	audioOutputDevices: {'devices': 1},
	channelId: 0,
	networkQuality: 0,
	selectedAudioInputDevice: 'device',
	selectedAudioOutputDevice: 'device',
	isInVoice: true,
	isSupportedBy: function(friendNucleusId) {
		return true;
	},
	joinVoice: function(id, participants){
		setTimeout(function() {
            window.OriginVoice.voiceConnected({});
        }, 0);
		return {
			'id': id, 
			'participants': participants
		};
	},
	leaveVoice: function(id){
		setTimeout(function() {
            window.OriginVoice.voiceDisconnected({});
        }, 0);
		return {
			'id': id, 
		};
	},
	showSurvey: function(channelId) {
		return {
			'channelId': channelId
		};
	},
	showToast: function(event, originId, conversationId){
		return {
			'event': event,
			'originId': originId,
			'conversationId': conversationId
		};
	},
	changeInputDevice: function(device){
		setTimeout(function() {
			window.OriginVoice.deviceChanged([]);
		}, 0);
		
		return {
			'device': device
		};
	},
	changeOutputDevice: function(device){
		setTimeout(function() {
			window.OriginVoice.deviceChanged([]);
		}, 0);
		
		return {
			'device': device
		};
	},
	testMicrophoneStart: function() {
		setTimeout(function() {
			window.OriginVoice.enableTestMicrophone([]);
		}, 0);
		
		return true;
	},
	testMicrophoneStop: function() {
		setTimeout(function() {
			window.OriginVoice.disableTestMicrophone([]);
		}, 0);
		
		return true;
	},
	stopVoiceChannel: function() {return true;},
	playIncomingRing: function() {return {};},
	playOutgoingRing: function() {return {};},
	stopIncomingRing: function() {return {};},
	stopOutgoingRing: function() {return {};},
	setInVoiceSettings: function() {return {};},
	startVoiceChannel: function() {return {};},
	muteSelf: function() {return {};},
	unmuteSelf: function() {return {};},
	deviceAdded: Signal(false),
	deviceRemoved: Signal(false),
    defaultDeviceChanged: Signal(false),
    deviceChanged: Signal(false),
    voiceLevel: Signal(false),
    underThreshold: Signal(false),
    overThreshold: Signal(false),
    voiceConnected: Signal(false),
    voiceDisconnected: Signal(false),
    enableTestMicrophone: Signal(false),
    disableTestMicrophone: Signal(false),
    clearLevelIndicator: Signal(false),
    voiceCallEvent: Signal(false),
	
};

window.OriginUser = {
	requestSidRefresh : function() { 
        setTimeout(function() {
            window.OriginUser.sidRenewalResponseProxy(0, 200);
        }, 0);
		return {};
	},
	offlineUserInfo: function() {
		return {
			'nucleusId': 'jssdkfunctionaltest',
			'personaId': 1000101795502,
			'originId': 'jssdkmockupdata',
			'dob': '1980-01-01',
		};
	},
	requestLogout: function() {return {};},
	sidRenewalResponseProxy : Signal(false)
};

window.OriginIGO = {
	setCreateWindowRequest: function(requestUrl){
		return {
			'url': requestUrl
		}
	},
	moveWindowToFront: function() {return {};},
	openIGOConversation: function() {return {};},
	openIGOProfile: function() {return {};}
};

window.DesktopServices = {
	asyncOpenUrl: function(url){
		return {
			'url': url
		};
	},
	launchExternalBrowserWithEADPSSO: function(url){
		return {
			'url': url
		};
	},
	flashIcon: function(int) {		
		return {
			'int': int
		};
	},
	dockIconClicked: Signal(false)
};
//window.OriginUser = {
//    requestLogout : function() {},
//    offlineUserInfo : function() {}
//};