(function() {
    'use strict';


    function RosterDataFactory(UserDataFactory, AuthFactory, EventsHelperFactory, ComponentsLogFactory) {

        var roster = {};
        var filteredRosters = {};

        var rosterRetrievalInProgress = false;
        var rosterLoaded = false;
        var cancelRosterLoad = false;
        var requestRosterOnXmppConnect = false;

        // For performance tuning
        var rosterLoadStartTime = 0;

        var myEvents = new Origin.utils.Communicator();
        var playingOfferId = {}; //list of users playing by offerId
        var playedOfferId = {}; // list of users that have played the offer in the session
        var currentUserPresence = {
            show: 'UNAVAILABLE'
        };
        var isInvisible = false;
        var rosterViewportHeight = 0;
        var friendPanelHeight = 0;
        var viewportFriendCount = 10;
        var viewportFriendCountUsingDefault = true;
        var initialized = false;
        var handles = [];
        
        var friendRequestAcceptInProgress = false;


        // set up window handler so the events are disconnected upon the window closing
        function destroy() {
            if (!!EventsHelperFactory) {
                EventsHelperFactory.detachAll(handles);
            }
        }
        window.addEventListener('unload', destroy);

        /**
         * updates the flag that determines whether a given friend has voice chat capability
         * @return {string} nucleusId
         * @method
         */
        function updateFriendVoiceSupported(nucleusId) {

            if (!Origin.voice.supported()) {
                return;
            }

            Origin.voice.isSupportedBy(nucleusId).then(function(result) {
                roster[nucleusId].isVoiceSupported = result;
            });
        }

        /**
         * get the nucleusid from the jabberId ie: 12297642087@integration.chat.dm.origin.com
         * @param {string} jid jabberId
         * @return {string} nucleusId
         * @method
         */
        function getNucleusIdFromJid(jid) {
            if (jid.indexOf('@') <= 0) {
                return '';
            }
            return (jid.split('@'))[0];
        }

        function handleNameUpdate(friend) {
            //console.log('handleNameUpdate: ' + JSON.stringify(friend));

            var id = friend.userId;
            if (typeof roster[id] !== 'undefined') {
                roster[id].firstName = (friend.firstName ? friend.firstName : roster[id].firstName);
                roster[id].lastName = (friend.lastName ? friend.lastName : roster[id].lastName);
                roster[id].originId = (friend.EAID ? friend.EAID : roster[id].originId);

                // add or update filtered rosters
                addItemToFilteredRosters(id, roster[id]);
            }
        }

        function removeItemFromFilteredRosters(nucleusId, deferSignalHandler) {
            for (var name in filteredRosters) {
                if (!!filteredRosters[name].roster[nucleusId]) {
                    delete filteredRosters[name].roster[nucleusId];
                    myEvents.fire('socialRosterDelete' + name, nucleusId, (deferSignalHandler || false));
                }
            }
        }

        function addItemToFilteredRosters(nucleusId, friend) {
            for (var name in filteredRosters) {
                if (typeof filteredRosters[name].filter === 'function' && filteredRosters[name].filter(friend) && !filteredRosters[name].roster[nucleusId]) {
                    if (friend.originId) {
                        filteredRosters[name].roster[nucleusId] = friend;
                        myEvents.fire('socialRosterUpdate' + name, friend);
                    }
                }
            }
        }

        function addItemToRoster(nucleusId, originId, jabberId, subState, subReqSent, presence) {
            //console.log('addItemToRoster: ' + nucleusId + ' originId: ' + originId);

            // check if this friend has already been added to roster by presence update
            var rosterEntry = roster[nucleusId];
            if (!rosterEntry) {
                roster[nucleusId] = {
                    originId: originId,
                    jabberId: jabberId,
                    nucleusId: nucleusId,
                    firstName: '',
                    lastName: '',
                    subState: subState,
                    subReqSent: subReqSent,
                    presence: presence,
                    isVoiceSupported: false
                };

                //console.log('retrieveRoster: ' + JSON.stringify(roster[nucleusId]));

            } else {
                roster[nucleusId].originId = originId;
                roster[nucleusId].subState = subState;
                roster[nucleusId].subReqSent = subReqSent;

                //console.log('retrieveRoster existing roster item: ' + JSON.stringify(roster[nucleusId]));
            }

            addItemToFilteredRosters(nucleusId, roster[nucleusId]);

            // check to see if the added friend supports voice chat
            updateFriendVoiceSupported(nucleusId);

            // Once we have a single item in the roster signal that the results have started coming in
            if (Object.keys(roster).length === 1) {
                myEvents.fire('socialRosterResults');
            }

            // get user's originId if not known
            if ((typeof roster[nucleusId].originId === 'undefined' || roster[nucleusId].originId.length === 0)) {
                //console.log('getUserRealName: ' + nucleusId + ' originId: ' + originId);

                UserDataFactory.getUserRealName(nucleusId);
                handles[handles.length] = UserDataFactory.events.on('socialFriendsFullNameUpdated:' + nucleusId, handleNameUpdate);
            }

        }

        function updateRosterPresence(friendNucleusId, newPresence) {
            removeItemFromFilteredRosters(friendNucleusId);

            roster[friendNucleusId].presence = newPresence;

            addItemToFilteredRosters(friendNucleusId, roster[friendNucleusId]);

            if (newPresence.show !== 'UNAVAILABLE') {
                // check to see if the added friend supports voice chat
                updateFriendVoiceSupported(friendNucleusId);
            }
        }

        /**
         * retrieves the roster if request not already in progress
         * @return {Object} roster
         * @method
         */
        function requestRoster() {
            return new Promise(function(resolve, reject) {

                if (!Origin.xmpp.isConnected()) {
                    reject(-1);
                    return;
                }

                rosterLoadStartTime = Date.now();
                Origin.xmpp.requestRoster().then(function(response) {
                    resolve(response);
                }, function(error) {
                    console.log('requestRoster error', error);
                    rosterRetrievalInProgress = false;
                    reject(error);
                }).catch(function(error) {
                    rosterRetrievalInProgress = false;
                    ComponentsLogFactory.error('RosterDataFactory - requestRoster', error);
                });

            });
        }

        /**
         * @method
         */
        function retrieveRoster() {
            if (!rosterLoaded && !rosterRetrievalInProgress) {
                rosterRetrievalInProgress = true;
                requestRoster().then(function(data) {

                    //console.log('TR rosterDataFactory retrieveRoster: ' + data.length);
                    rosterRetrievalInProgress = false;
                    rosterLoaded = true;

                    var previousTime = +new Date();

                    var processRosterResponse = function() {

                        var timeNow = +new Date();

                        // This checks for 'no friends' case.
                        // If no data.length, roster object assignment will cause 'ERROR'
                        if (data.length) {
                            var friend = data.pop();
                            var nucleusId = getNucleusIdFromJid(friend.jid);

                            //console.log('TR rosterDataFactory retrieveRoster addItemToRoster: ' + friend.originId);

                            // Add all roster friends as offline, until presence is determined
                            addItemToRoster(nucleusId, friend.originId, friend.jid, friend.subState, friend.subReqSent, {
                                jid: nucleusId,
                                presenceType: '',
                                show: 'UNAVAILABLE',
                                gameActivity: {}
                            });

                            if (!cancelRosterLoad) {

                                if ((timeNow - previousTime) >= 500) { // every 500 milliseconds
                                    previousTime = timeNow;
                                    setTimeout(processRosterResponse, 0);
                                } else {
                                    processRosterResponse();
                                }

                            } else {
                                data = [];
                            }
                        } else {

                            // Fire roster loaded event
                            myEvents.fire('socialRosterLoaded');
                        }
                    };

                    //now listen for presence and roster changes
                    Origin.events.on(Origin.events.XMPP_PRESENCECHANGED, onXmppPresenceChanged);
                    Origin.events.on(Origin.events.XMPP_ROSTERCHANGED, onXmppRosterChanged);

                    // update our own presence so we can receive presence of our friends
                    Origin.xmpp.updatePresence();

                    processRosterResponse();

                }, function(error) {
                    rosterLoaded = true;
                    console.log('retrieveRoster error:', error);
                    myEvents.fire('socialRosterLoadError');
                }).catch(function(error) {
                    rosterLoaded = true;
                    ComponentsLogFactory.error('RosterDataFactory - retrieveRoster', error);
                    myEvents.fire('socialRosterLoadError');
                });
            }
        }

        /**
         * listens for presence change notifications from xmpp
         * @param {Object} newPresence
         * @method
         */
        function onXmppPresenceChanged(newPresence) {
            //console.log('TR RosterDataFactory::onXmppPresenceChanged ' + JSON.stringify(newPresence));

            if (typeof newPresence.jid !== 'undefined') {
				
				// Ignore all presence events from "strophe" resource
				if (newPresence.jid.indexOf('strophe') > 0) {
					return;
				}
								
                var friendNucleusId = getNucleusIdFromJid(newPresence.jid);

                // handle current user's presence changing
                if (Number(friendNucleusId) === Number(Origin.user.userPid())) {
                    currentUserPresence = newPresence;
                    myEvents.fire('socialPresenceChanged:' + friendNucleusId, newPresence);
                    return; //TR 06-03-2015
                }

                //NOTE:  we may get presence change for things other than existing friends, e.g. when
                // adding a friend or receiving a friend request...
                if (!roster[friendNucleusId]) {
                    //console.log('RosterDataFactory: onXmppPresenceChanged: receiving presence for friend not in roster:', newPresence.jid);

                    if (newPresence.presenceType === 'SUBSCRIBE') {
                        // Incoming friend request - add it to the roster
                        addItemToRoster(friendNucleusId, '', newPresence.jid, 'NONE', false,  {
                                jid: friendNucleusId,
                                presenceType: newPresence.presenceType,
                                show: 'UNAVAILABLE',
                                gameActivity: newPresence.gameActivity
                            });
                        myEvents.fire('socialPresenceChanged:' + friendNucleusId, newPresence);
                    } else if (newPresence.presenceType === 'UNSUBSCRIBE') {
                        // Cancel incoming friend request - remove it from the roster
                        delete roster[friendNucleusId];
                        removeItemFromFilteredRosters(friendNucleusId);
                    } else if (!rosterLoaded) {
                        // friend presence update before loaded into roster - add it to the roster

                        // the presence is not "offline". Offline friends will be added after roster load
                        if (newPresence.show !== 'UNAVAILABLE') {
                            addItemToRoster(friendNucleusId, '', newPresence.jid, '', false, {
                                    jid: friendNucleusId,
                                    presenceType: '',
                                    show: newPresence.show,
                                    gameActivity: newPresence.gameActivity
                                });
                        }

                    }
                    return;
                }

                // Check if the presence has really changed.
                if (!angular.equals(roster[friendNucleusId].presence, newPresence)) {

                    // friend request accept error
                    if (roster[friendNucleusId].subState === 'NONE' && newPresence.presenceType === 'ERROR') {
                        myEvents.fire('friendRequestError');
                        return;
                    }

                    //check to see if they were playing a game, and now they're not
                    var wasPlayingOfferId = '';

                    if (typeof roster[friendNucleusId].presence.gameActivity !== 'undefined') {
                        if (typeof roster[friendNucleusId].presence.gameActivity.productId !== 'undefined') {
                            wasPlayingOfferId = roster[friendNucleusId].presence.gameActivity.productId;
                        }
                    }

                    updateRosterPresence(friendNucleusId, newPresence);

                    if (wasPlayingOfferId !== '') {
                        // remove the user from the list of users who were playing
                        // and add them to the list of users who have played the
                        // game with the last time that they played it
                        addToPlayedGamesList(wasPlayingOfferId, friendNucleusId);
                        removeFromPlayingGamesList(wasPlayingOfferId, friendNucleusId);
                    }

                    if (typeof newPresence.gameActivity !== 'undefined' && typeof newPresence.gameActivity.productId !== 'undefined') {
                        addToPlayingGamesList(newPresence.gameActivity.productId, friendNucleusId);
                    }

                    //now call the callback
                    myEvents.fire('socialPresenceChanged:' + friendNucleusId, newPresence);
                }
            } else {
                console.log('received presence with no jid', newPresence);
            }
        }

        /**
         * listens for roster change notifications from xmpp
         * @param {Object} newPresence
         * @method
         */
        function onXmppRosterChanged(rosterChangeObject) {
            //console.log('TR RosterDataFactory::onXmppRosterChanged ' + JSON.stringify(rosterChangeObject));
            var id = getNucleusIdFromJid(rosterChangeObject.jid);

            // Jingle sends a roster update event on presence change events
            // Ignore the roster change event if there is no actual change to the roster
            if ( (typeof roster[id] === 'undefined') || (roster[id].subState !== rosterChangeObject.subState) ) {

                if (rosterChangeObject.subState === 'REMOVE') {
                    // change the presence on the user object even though we
                    // are removing it from the roster as some of the directives
                    // could still have a reference to it
                    myEvents.fire('socialPresenceChanged:' + id, { show: 'UNAVAILABLE' });

                    myEvents.fire('removedFriend', id);
                    delete roster[id];
                    removeItemFromFilteredRosters(id);
                } else if (typeof roster[id] === 'undefined') {
                    // Do not re-add users to roster if this is a "block" operation
                    // A "block" roster change notice looks like subState=="NONE" and subReqSent is false

                    // Do add users to roster if this is "send friend request" operation.
                    // A "send friend request" looks like subState=="NONE" and subReqSent is true.
                    if (rosterChangeObject.subState !== 'NONE' || rosterChangeObject.subReqSent) {
                        removeItemFromFilteredRosters(id);
                        addItemToRoster(id, '', rosterChangeObject.jid, rosterChangeObject.subState, rosterChangeObject.subReqSent, {
                                jid: id,
                                presenceType: '',
                                show: 'UNAVAILABLE',
                                gameActivity: {}
                            });
                    }
                } else {
                    var friendRequestAccepted = false;
                    if (friendRequestAcceptInProgress && roster[id].subState === 'NONE' && !roster[id].subReqSent && rosterChangeObject.subState === 'BOTH') {
                        myEvents.fire('friendRequestAccepted');
                        friendRequestAccepted = true;
                    }

                    removeItemFromFilteredRosters(id, friendRequestAccepted);

                    roster[id].subState = rosterChangeObject.subState;
                    addItemToFilteredRosters(id, roster[id]);
                }

            }
        }

        /**
         * listens for xmpp connection to be complete, and initiates roster retrieval
         * @return {void}
         * @method
         */
        function onXmppConnected() {
            //console.log('RosterDataFactory: onXmppConnected');
            cancelRosterLoad = false;
            if (!rosterLoaded && !rosterRetrievalInProgress) {
                if (requestRosterOnXmppConnect) {
                    requestRosterOnXmppConnect = false;
                    retrieveRoster();
                }
            } else {
                console.log('RosterDataFactory - roster retrieval in progress');
            }
            myEvents.fire('xmppConnected');
            Origin.xmpp.loadBlockList();
        }

        function onXmppDisconnected() {
            rosterLoaded = false;
            rosterRetrievalInProgress = false;
            requestRosterOnXmppConnect = true;
            myEvents.fire('xmppDisconnected');
        }

        function onXmppBlockListChanged() {
            //console.log('RosterDataFactory: firing xmppBlockListChanged');
            myEvents.fire('xmppBlockListChanged');
        }

        function initializeFilteredRosters() {
            filteredRosters.ALL = {
                'filter': filters.ALL,
                'roster': {}
            };
            filteredRosters.FAVORITES = {
                'filter': filters.FAVORITES,
                'roster': {}
            };
            filteredRosters.FRIENDS = {
                'filter': filters.FRIENDS,
                'roster': {}
            };
            filteredRosters.INCOMING = {
                'filter': filters.INCOMING,
                'roster': {}
            };
            filteredRosters.OUTGOING = {
                'filter': filters.OUTGOING,
                'roster': {}
            };
            filteredRosters.ONLINE = {
                'filter': filters.ONLINE,
                'roster': {}
            };
        }

        function clearFilteredRosters() {
            for (var name in filteredRosters) {
                if (Object.keys(filteredRosters[name].roster).length > 0) {
                    myEvents.fire('socialRosterClear' + name);
                    filteredRosters[name].roster = {};
                }
            }
        }

        function onClientOnlineStateChanging(onlineObj) {
            if (onlineObj && !onlineObj.isOnline) {
                onJSSDKlogout();
            }
        }

        function onClientOnlineStateChanged(onlineObj) {
            if (onlineObj && onlineObj.isOnline) {
                onJSSDKlogin();
            }
        }

        /**
         * listens for JSSDK login to be complete, initiates xmpp connection
         * @return {void}
         * @method
         */
        function onJSSDKlogin() {
            //now that we're logged in, initiate xmpp startup
            //console.log('onJSSDKlogin: ' + Origin.xmpp.isConnected());
            cancelRosterLoad = false;
            if (Origin.xmpp.isConnected()) {
                requestRosterOnXmppConnect = true;
                onXmppConnected();
            }
            myEvents.fire('jssdkLogin');
        }

        /**
         * listens for JSSDK logout, clears roster
         * @return {void}
         * @method
         */
        function onJSSDKlogout() {
            //console.log('onJSSDKlogout: ' + Origin.xmpp.isConnected());
            if (!rosterLoaded) {
                cancelRosterLoad = true;
            }

            Origin.events.off(Origin.events.XMPP_PRESENCECHANGED, onXmppPresenceChanged);
            Origin.events.off(Origin.events.XMPP_ROSTERCHANGED, onXmppRosterChanged);

            rosterLoaded = false;
            rosterRetrievalInProgress = false;
            //clear the roster and playing list
            roster = {};
            clearFilteredRosters();
            myEvents.fire('rosterCleared');

            for (var offer in playingOfferId) {
                if (playingOfferId.hasOwnProperty(offer)) {
                    myEvents.fire('socialFriendsPlayingListUpdated:' + offer, []);
                }
            }
            playingOfferId = {}; //list of users playing by offerId

            currentUserPresence = {
                show: 'UNAVAILABLE'
            };
            myEvents.fire('socialPresenceChanged:' + Origin.user.userPid(), currentUserPresence);
            initialized = false;
        }

        function getFilteredRoster(name) {
            return filteredRosters[name].roster;
        }

        function resolveRoster(name) {
            return function(resolve, reject) {
                if (rosterLoaded) {
                    resolve(getFilteredRoster(name));
                } else {
                    myEvents.on('socialRosterLoaded', function() {
                        resolve(getFilteredRoster(name));
                    });

                    myEvents.on('socialRosterLoadError', function() {
                        reject(getFilteredRoster(name));
                    });
                }
            };
        }


        /**
         * returns a promise to return the roster data
         * @return {roster data}
         * @method getRoster
         */
        function getRoster(name, filter) {
            if (typeof filteredRosters[name] === 'undefined') {
                filteredRosters[name] = {
                    'filter': filter,
                    'roster': {}
                };
            }

            if (!rosterLoaded && !rosterRetrievalInProgress) {
                 if (Origin.xmpp.isConnected()) {
                    retrieveRoster();
                 } else {
                    requestRosterOnXmppConnect = true;
                 }
            }

            return new Promise(resolveRoster(name));
        }

        /**
         * returns the list of friends playing a game
         * @param {string} offerId
         * @return {void}
         * @method
         */
        function getFriendsWhoArePlaying(offerId) {
            var playingList = [];

            if (playingOfferId[offerId]) {
                playingList = playingOfferId[offerId].playingFriends;
            }

            return playingList;
        }

        /**
         * given an offerId and friendsNucleusId, adds friendNucleusId to playingOfferId[] to keep track of friends playing a particular offerId
         * and calls emitFriendsWoArePlayingUpdate to emit that the playingOfferId has changed
         * @param {string} offerId
         * @param {string} friendNucleusId
         * @return {void}
         * @method
         */
        function addToPlayingGamesList(offerId, friendNucleusId) {
            if (playingOfferId[offerId]) {
                playingOfferId[offerId].playingFriends.push(friendNucleusId);
            } else {
                playingOfferId[offerId] = {
                    playingFriends: [friendNucleusId]
                };
            }
            myEvents.fire(
                'socialFriendsPlayingListUpdated:' + offerId,
                getFriendsWhoArePlaying(offerId)
            );
        }

        /**
         * Track when the user last played the game
         * @param {string} offerId
         * @param {string} friendNucleusId
         * @return {void}
         * @method
         */
        function addToPlayedGamesList(offerId, friendNucleusId) {
            if (!playedOfferId[offerId]) {
                playedOfferId[offerId] = {};
            }
            playedOfferId[offerId][friendNucleusId] = {
                'nucleusId': friendNucleusId,
                'lastPlayed': (new Date()).getTime() // just in case we need the time
            };
        }

        /**
         * given an offerId and friendsNucleusId, removes friendNucleusId from playingOfferId[]
         * and calls emitFriendsWoArePlayingUpdate to emit that the playingOfferId has changed
         * @param {string} offerId
         * @param {string} friendNucleusId
         * @return {void}
         * @method
         */
        function removeFromPlayingGamesList(offerId, friendNucleusId) {
            if (playingOfferId[offerId]) {
                var idx = playingOfferId[offerId].playingFriends.indexOf(friendNucleusId);
                if (idx !== -1) {
                    playingOfferId[offerId].playingFriends.splice(idx, 1);
                    myEvents.fire(
                        'socialFriendsPlayingListUpdated:' + offerId,
                        getFriendsWhoArePlaying(offerId)
                    );
                }
            }
        }

        function onAuthChanged(loginObj) {
            if (loginObj) {
                if (loginObj.isLoggedIn) {
                    onJSSDKlogin();
                } else {
                    onJSSDKlogout();
                }
            }
        }

        function init() {
            if (initialized) {
                console.log('RosterData: Already initialized');
                return;
            }
            //set up to listen for login/logout
            AuthFactory.events.on('myauth:change', onAuthChanged);
            //this is so that we can set to "loading... " state
            AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanging);
            //this is so we wait for connection to JSSDK after coming back online
            AuthFactory.events.on('myauth:clientonlinestatechanged', onClientOnlineStateChanged);

            Origin.events.on(Origin.events.XMPP_CONNECTED, onXmppConnected);
            Origin.events.on(Origin.events.XMPP_DISCONNECTED, onXmppDisconnected);
            Origin.events.on(Origin.events.XMPP_BLOCKLISTCHANGED, onXmppBlockListChanged);

            //if not yet logged in then onJSSDKlogin will get called
            //otherwise check to see if xmpp is connected, and if that's also done, then check for roster loading
            if (AuthFactory.isAppLoggedIn()) {
                if (!Origin.xmpp.isConnected()) {
                    onJSSDKlogin();
                } else {
                    onXmppConnected();
                }
                initialized = true;
            } else {
                console.log('RosterData: auth not logged in');
            }
        }

        // Roster filters
        var filters = {
                'ALL' : function(item) {
                    return (!!item);
                },

                'FAVORITES' : function(item) {
                    return (!!item && false); // no favorites for now
                },

                'FRIENDS' : function(item) {
                    return ((!!item) && item.subState === 'BOTH');
                },

                'INCOMING' : function(item) {
                    return ((!!item) && (item.subState === 'NONE' && !item.subReqSent) && (!!item.presence && item.presence.presenceType === 'SUBSCRIBE'));
                },

                'OUTGOING' : function(item) {
                    return ((!!item) && (item.subState === 'NONE' && item.subReqSent));
                },

                'ONLINE': function(item) {
                    return ((!!item && item.subState === 'BOTH') && (!!item.presence && (item.presence.show === 'ONLINE' || item.presence.show === 'AWAY') && item.presence.presenceType !== 'UNAVAILABLE'));
                }
            };

            //we setup the filtered rosters here because getRoster will not be called until the directive is loaded
            //and there is a chance that directives can get loaded after initial presence is received
            initializeFilteredRosters();

         function getFriendInfoPriv(friendNucleusId){
                return new Promise(function(resolve, reject) {
                    if (rosterLoaded) {
                        resolve(roster[friendNucleusId]);
                    } else {
                        myEvents.on('socialRosterLoaded', function() {
                            resolve(roster[friendNucleusId]);
                        });

                        myEvents.on('socialRosterLoadError', function() {
                            reject(roster[friendNucleusId]);
                        });
                    }
                });
         }

        return {

            init: init,
            events: myEvents,
            getFriendInfo: getFriendInfoPriv,

            /**
             * @method retrieves the presence object for the current user
             */
            getCurrentUserPresence: function() {
                return currentUserPresence;
            },

            /**
             * @method sets the presence for the current user
             */
            requestPresence: function(presence) {
                // We need to keep an internal flag for being invisible
                var invisible = presence === 'invisible' ? true : false;
                if (invisible !== isInvisible) {
                    isInvisible = invisible;
                    myEvents.fire('visibilityChanged', isInvisible);
                }
                // Set the actual presence
                Origin.xmpp.requestPresence(presence);
            },

            /**
             * @method retrieves the visibility state for the current user
             */
            getIsInvisible: function() {
                return isInvisible;
            },

            /**
             * initiates retrieval of "friends-who-are-playing" the given offerId, and when retrieved, will initiate the registered callback
             * @param {string} offerId
             * @return {void}
             * @method
             */
            friendsWhoArePlaying: function(offerId) {
                //regardless of whether logged in, connected or not, just have it try to regenerate a list, if the roster is empty, it wil lreturn empty list
                return getFriendsWhoArePlaying(offerId);
            },

            /**
             * Get the friends who played the offer id but
             * are no longer playing the game.
             * @param {offerId} the offer id of the game
             * @return {friendWhoPlayedArray}
             * @method
             */
            getFriendsWhoPlayed: function(offerId) {
                var played = [],
                    nucleusId;
                if (playedOfferId[offerId]) {
                    for (nucleusId in playedOfferId[offerId]) {
                        if (playedOfferId[offerId].hasOwnProperty(nucleusId)) {
                            played.push(nucleusId);
                        }
                    }
                }
                return played;
            },

            /**
             * popular unowned games
             * @typedef gameFriendsPlayingObject
             * @type {object}
             * @property {string} offerId offerId
             * @property {[string]} playingFriends
             */

            /**
             * returns a list of unowned games sorted by most friends playing
             * @param {ownedGameInfoObject} list of owned games
             * @return {gameFriendsPlayingObject}
             * @method
             */
            
            popularUnownedGamesList: function(ownedGames) {
                var unownedGames = [],
                    i = 0;

                function compareListLengths(a, b) {
                    return b.playingFriends.length - a.playingFriends.length;
                }

                function entitlementExists(offerId) {
                    var exists = false;
                    for (i = 0; i < ownedGames.length; i++) {
                        if (offerId === ownedGames[i].offerId) {
                            exists = true;
                            break;
                        }
                    }
                    return exists;
                }

                for (var offer in playingOfferId) {
                    if (playingOfferId.hasOwnProperty(offer)) {
                        if (!entitlementExists(offer)  && playingOfferId[offer].playingFriends.length) {
                            unownedGames.push({
                                offerId: offer,
                                playingFriends: playingOfferId[offer].playingFriends
                            });
                        }
                    }
                }
                unownedGames.sort(compareListLengths);

                return unownedGames;
            },

            getRoster: function(name) {
                return getRoster(name, filters[name]);

            },

            getRosterFriend: function(nucleusId) {
                return roster[nucleusId];
            },

            /**
             * returns roster info for given friend's nucleusId
             * @param {string} friendNucleusId
             * @return {socialPresenceObject}
             * @method
             */
            getFriendSocialInfo: function(friendNucleusId) {
                return new Promise(function(resolve, reject) {
                    var socialPresence = {
                        presence: '',
                        ingame: false,
                        joinable: false,
                        broadcasting: false
                    };
                    getFriendInfoPriv(friendNucleusId).then(function(friend) {
                        if (friend) {
                            socialPresence.presence = friend.presence;
                            if (friend.presence.show === 'ONLINE') {
                                if (friend.presence.gameActivity && typeof friend.presence.gameActivity.title !== 'undefined' && friend.presence.gameActivity.title.length) {
                                    // If we have a gameActivity object and a title we must be playing a game.
                                    socialPresence.ingame = true;
                                    if (friend.presence.gameActivity.joinable){
                                        socialPresence.joinable = true;
                                    }

                                    // If we have a twitch presence we must be broadcasting
                                    if (!!friend.presence.gameActivity.twitchPresence && friend.presence.gameActivity.twitchPresence.length) {
                                        socialPresence.broadcasting = true;
                                    }
                                }
                            }
                            resolve(socialPresence);
                        }
                            reject(socialPresence);
                        });
                });
            },


            //TR Nuke this getNumFriends

            /**
             * returns a promise to return the total number of friends the user has
             * @return number of friends
             * @method getNumFriends
             */
            getNumFriends: function() {
                return new Promise(function(resolve, reject) {
                    var numFriends = 0;
                    if (rosterLoaded) {
                        numFriends = Object.keys(roster).length;
                        resolve(numFriends);
                    } else {
                        myEvents.on('socialRosterLoaded', function() {
                            numFriends = Object.keys(roster).length;
                            resolve(numFriends);
                        });

                        myEvents.on('socialRosterLoadError', function() {
                            reject(numFriends);
                        });

                    }
                });
            },

            /**
             * Accept a friend request from a given user
             * @method friendRequestAccept
             */
            friendRequestAccept: function(jid) {
                friendRequestAcceptInProgress = true;
                return new Promise(function(resolve, reject) {
                    Origin.xmpp.friendRequestAccept(roster[jid].jabberId);
                    myEvents.on('friendRequestAccepted', function() {
                        friendRequestAcceptInProgress = false;
                        resolve();
                    });

                    myEvents.on('friendRequestError', function() {
                        friendRequestAcceptInProgress = false;
                        reject();
                    });
                });
            },

            /**
             * Reject a friend request from a given user
             * @method friendRequestReject
             */
            friendRequestReject: function(jid) {
                Origin.xmpp.friendRequestReject(roster[jid].jabberId);
            },

            /**
             * Send a friend request to a given user
             * @method sendFriendRequest
             */
            sendFriendRequest: function(jid) {
                Origin.xmpp.friendRequestSend(Origin.xmpp.nucleusIdToJid(jid));
            },

            /**
             * Cancel a pending friend request for a given user
             * @method cancelPendingFriendRequest
             */
            cancelPendingFriendRequest: function(jid) {
                Origin.xmpp.friendRequestRevoke(roster[jid].jabberId);
            },

            /**
             * Remove a friend for a given user
             * @method removeFriend
             */
            removeFriend: function(jid) {
                Origin.xmpp.removeFriend(roster[jid].jabberId);
            },
            
            removeFriendAndBlock: function(nucleusId) {
                Origin.xmpp.removeFriendAndBlock(nucleusId, roster[nucleusId].jabberId);
            },

             /**
             * Block a given user
             * @method blockUser
             */
            blockUser: function(nucleusId) {
                Origin.xmpp.blockUser(nucleusId);
            },

             /**
             * Block a given user, and cancel pending friend request
             * @method cancelAndBlockUser
             */
            cancelAndBlockUser: function(nucleusId) {
                Origin.xmpp.cancelAndBlockUser(nucleusId);
            },

             /**
             * Block a given user, and ignore incoming friend request
             * @method ignoreAndBlockUser
             */
            ignoreAndBlockUser: function(nucleusId) {
                Origin.xmpp.ignoreAndBlockUser(nucleusId);
            },
            
             /**
             * Unblock a given user
             * @method unblockUser
             */
            unblockUser: function(nucleusId) {
                Origin.xmpp.unblockUser(nucleusId);
            },

            setFriendsListFilter: function(filterText) {
                myEvents.fire('updateFriendsListFilter', filterText);
            },

            updateFriendDirective: function(nucleusId) {
                myEvents.fire('updateFriendsDirective' + nucleusId);
            },

            setRosterViewportHeight: function(height) {
                if (height !== rosterViewportHeight) {
                    rosterViewportHeight = height;
                    myEvents.fire('updateRosterViewport');
                }
            },

            getRosterViewportHeight: function() {
                return rosterViewportHeight;
            },

            setFriendPanelHeight: function(height) {
                friendPanelHeight = height;
                myEvents.fire('updateFriendPanelHeight');
            },

            setSectionViewportFriendCount : function(count) {
                if(viewportFriendCountUsingDefault) {
                    viewportFriendCountUsingDefault = false;
                    if (count > viewportFriendCount) {
                        viewportFriendCount = count;
                        myEvents.fire('updateRosterViewport');
                    }
                }
            },

            getSectionViewportFriendCount : function() {
                return viewportFriendCount;
            },

            getFriendPanelHeight: function() {
                return friendPanelHeight;
            },

            updateRosterViewport: function() {
                if (!rosterViewportHeight || viewportFriendCountUsingDefault ) {
                    myEvents.fire('updateRosterViewportHeight');
                } else {
                    myEvents.fire('updateRosterViewport');
                }
            },

            updateSectionViewport: function(element, pos) {
                element = element;
                pos = pos;
            },

            getRosterLoadStartTime: function() {
                return rosterLoadStartTime;
            },

            isRosterLoaded: function() {
                return rosterLoaded;
            },

            isBlocked: function(nucleusId) {
                return Origin.xmpp.isBlocked(nucleusId);
            },

            deleteFromSectionRoster: function(filter, nucleusId) {
                 myEvents.fire('socialRosterDelete' + filter, nucleusId, false);
            },

            setProfilePrivacy: function(isPrivate) {
                myEvents.fire('privacyChanged', isPrivate);
            },
            
            loadBlockList: function() {
                Origin.xmpp.loadBlockList();
            }

        };


    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function RosterDataFactorySingleton(UserDataFactory, AuthFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('RosterDataFactory', RosterDataFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.RosterDataFactory

     * @description
     *
     * RosterDataFactory
     */
    angular.module('origin-components')
        .factory('RosterDataFactory', RosterDataFactorySingleton);
}());
