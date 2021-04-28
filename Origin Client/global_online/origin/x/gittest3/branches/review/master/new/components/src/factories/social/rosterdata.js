(function() {
    'use strict';

	
    function RosterDataFactory(UserDataFactory) {
		
		var roster = {}; 		
		var filteredRosters = {};

        var rosterRetrievalInProgress = false;
        var rosterLoaded = false;
		var cancelRosterLoad = false;
		
		// For performance tuning
		var rosterLoadStartTime = 0;
		
		var myEvents = new Origin.utils.Communicator();
		var playingOfferId = {}; //list of users playing by offerId
		var playedOfferId = {}; // list of users that have played the offer in the session
		var currentUserPresence = { show: 'UNAVAILABLE' };
		var isInvisible = false;
		var	rosterViewportHeight = 0;
		var	friendPanelHeight = 0;
		var viewportFriendCount = 1;            			
		var viewportFriendCountUsingDefault = true;
		
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
			console.log('handleNameUpdate: ' + JSON.stringify(friend));
			
			var id = friend.userId;
			if (typeof roster[id] !== 'undefined') {
				roster[id].firstName = (friend.firstName ? friend.firstName : roster[id].firstName);
				roster[id].lastName = (friend.lastName ? friend.lastName : roster[id].lastName);
				roster[id].originId = (friend.EAID ? friend.EAID : roster[id].originId);
				
				// add or update filtered rosters
				addItemToFilteredRosters(id, roster[id]);
			}				
		}

		function removeItemFromFilteredRosters(nucleusId, friend) {
			for (var name in filteredRosters) {				
				console.log('TR removeItemFromFilteredRosters: ' + name + ' : ' + JSON.stringify(friend));
				console.log('TR removeItemFromFilteredRosters: ' + name + ' : ' + Object.keys(filteredRosters[name].roster).length);
				
				if (typeof filteredRosters[name].filter === 'function' && filteredRosters[name].filter(friend)) {
					if (!!filteredRosters[name].roster[nucleusId]) {						
						delete filteredRosters[name].roster[nucleusId];

						console.log('TR removeItemFromFilteredRosters: deleted ' + name + ' : ' + Object.keys(filteredRosters[name].roster).length);
						
						myEvents.fire('socialRosterDelete'+name, nucleusId);
					}
				}				
			}			
		}		

		function addItemToFilteredRosters(nucleusId, friend) {
			for (var name in filteredRosters) {				
				if (typeof filteredRosters[name].filter === 'function' && filteredRosters[name].filter(friend)) {
					if (typeof friend.originId !== 'undefined' && friend.originId.length) {
						console.log('addingItemToFilteredRosters: ' + name + ' ' + JSON.stringify(friend));
						filteredRosters[name].roster[nucleusId] = friend;
						myEvents.fire('socialRosterUpdate'+name, friend);						
					}					
				}				
			}			
		}

		function addItemToRoster(nucleusId, originId, jabberId, subState, subReqSent, presence) {
			console.log('addItemToRoster: ' + nucleusId + ' originId: ' + originId);

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
					presence: presence
				};
				
				//console.log('retrieveRoster: ' + JSON.stringify(roster[nucleusId]));																		
				
			} else {
				roster[nucleusId].originId = originId;
				roster[nucleusId].subState = subState;
				roster[nucleusId].subReqSent = subReqSent;

				//console.log('retrieveRoster existing roster item: ' + JSON.stringify(roster[nucleusId]));		
			}

			addItemToFilteredRosters(nucleusId, roster[nucleusId]);

            // Once we have a single item in the roster signal that the results have started coming in
            if (Object.keys(roster).length === 1) {
				myEvents.fire('socialRosterResults');
            }

			// get user's originId if not known
			if ((typeof roster[nucleusId].originId === 'undefined' || roster[nucleusId].originId.length === 0)) {
				console.log('getUserRealName: ' + nucleusId + ' originId: ' + originId);
				
				UserDataFactory.getUserRealName(nucleusId);
				UserDataFactory.events.on('socialFriendsFullNameUpdated:' + nucleusId, handleNameUpdate);							
			}
					
		}
		
		function updateRosterPresence(friendNucleusId, newPresence) {
			removeItemFromFilteredRosters(friendNucleusId, roster[friendNucleusId]);
			
			roster[friendNucleusId].presence = newPresence;
			
			addItemToFilteredRosters(friendNucleusId, roster[friendNucleusId]);
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

				//console.log('RosterDataFactory requestRoster');
				
                if (!rosterRetrievalInProgress) {
                    rosterRetrievalInProgress = true;
					rosterLoadStartTime = Date.now();
                    Origin.xmpp.requestRoster().then(function(response) {
						//if (response.length < 201) {
							//console.log('requestRoster response: ' + JSON.stringify(response));
						//} else {
							//console.log('requestRoster response: ' + response.length + ' entries');
						//}
						resolve(response);                    
						rosterRetrievalInProgress = false;
					}, function(error) {
						rosterRetrievalInProgress = false;
						console.log('requestRoster error', error);
						reject(error);
					}).catch(function(error) {
						Origin.log.exception(error, 'RosterDataFactory - requestRoster');
					});
                }

            });
        }
		
        /**
         * @method
         */
        function retrieveRoster() {
            requestRoster().then(function(data) {
				
				var processRosterResponse = function() {

                    // This checks for 'no friends' case. 
                    // If no data.length, roster object assignment will cause 'ERROR'
					if (data.length) {
                        var friend = data.pop();
						var nucleusId = getNucleusIdFromJid(friend.jid);

						// Add all roster friends as offline, until presence is determined
						addItemToRoster(nucleusId, friend.originId, friend.jid, friend.subState, friend.subReqSent, {
							jid: nucleusId,
							presenceType: '',
							show: 'UNAVAILABLE',
							gameActivity: {}
						});						                        

						if (!cancelRosterLoad) {
							setTimeout(processRosterResponse, 0);
						} else {
							data = [];
						}
					} else {						
						rosterLoaded = true;
			
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
                Origin.log.exception(error, 'RosterDataFactory - retrieveRoster');
                myEvents.fire('socialRosterLoadError');
            });
        }
		
        /**
         * listens for presence change notifications from xmpp
         * @param {Object} newPresence
         * @method
         */
        function onXmppPresenceChanged(newPresence) {
			console.log('RosterDataFactory::onXmppPresenceChanged ' + JSON.stringify(newPresence));
			/**
			 * compares to see if the object key matches
			 * @param {Object} obj1
			 * @param {Object} obj2
			 * @param {Object} key
			 * @return {bool}
			 * @method
			 */
			function objKeyMatches(obj1, obj2, key) {        
				if (obj1[key] === obj2[key]) {
					return true;
				}
				//console.log ('objKeyMatches - ', key,' failed:', obj1[key], ',', obj2[key]);
				return false;
			}
			
			/**
			 * compares current Presence to new Presence received to see it actually changed
			 * @param {Object} oldPresence
			 * @param {Object} newPresence
			 * @return {bool}
			 * @method
			 */
			function presenceDidChange(oldPresence, newPresence) {
				var changed = false;

				if (!objKeyMatches(oldPresence, newPresence, 'jid') ||
					//currently not returning presenceType in the presence Info because C++ client side doesn't retain it in its presence info
					//TR WTF above??
					!objKeyMatches(oldPresence, newPresence, 'presenceType') ||
					!objKeyMatches(oldPresence, newPresence, 'show')) {
					changed = true;
				}

				if (!changed) {
					if (typeof oldPresence.gameActivity !== typeof newPresence.gameActivity) {
						changed = true;
					} else if (typeof oldPresence.gameActivity !== 'undefined' && typeof newPresence.gameActivity !== 'undefined') {
						if (!objKeyMatches(oldPresence.gameActivity, newPresence.gameActivity, 'title') ||
							!objKeyMatches(oldPresence.gameActivity, newPresence.gameActivity, 'productId') ||
							!objKeyMatches(oldPresence.gameActivity, newPresence.gameActivity, 'joinable') ||
							!objKeyMatches(oldPresence.gameActivity, newPresence.gameActivity, 'twitchPresence') ||
							!objKeyMatches(oldPresence.gameActivity, newPresence.gameActivity, 'gamePresence') ||
							!objKeyMatches(oldPresence.gameActivity, newPresence.gameActivity, 'multiplayerId')) {
							changed = true;
						}
					}
				}
				return changed;
			}
			
            if (typeof newPresence.jid !== 'undefined') {
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
                    console.log('RosterDataFactory: onXmppPresenceChanged: receiving presence for friend not in roster:', newPresence.jid);

                    if (newPresence.presenceType === 'SUBSCRIBE') {
                        // Incoming friend request - add it to the roster
						addItemToRoster(friendNucleusId, '', newPresence.jid, '', false,  {
                                jid: friendNucleusId,
                                presenceType: '',
                                show: 'UNAVAILABLE',
                                gameActivity: newPresence.gameActivity
                            });
                    }
                    else if (newPresence.presenceType === 'UNSUBSCRIBE') {
                        // Cancel incoming friend request - remove it from the roster						
						console.log('presenceDidChange: UNSUBSCRIBE:' + JSON.stringify(roster[friendNucleusId]));		
						removeItemFromFilteredRosters(friendNucleusId, roster[friendNucleusId]);
                        delete roster[friendNucleusId];
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

                if (presenceDidChange(roster[friendNucleusId].presence, newPresence)) {                    
                    //check to see if they were playing a game, and now they're not
                    var wasPlayingOfferId = '';

                    if (typeof roster[friendNucleusId].presence.gameActivity !== 'undefined') {
                        if (typeof roster[friendNucleusId].presence.gameActivity.productId !== 'undefined') {
                            wasPlayingOfferId = roster[friendNucleusId].presence.gameActivity.productId;
                        }
                    }

					console.log('presenceDidChange: old:' + JSON.stringify(roster[friendNucleusId]));		
					console.log('presenceDidChange: new:' + JSON.stringify(newPresence));		
					
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
					console.log('presenceDidChange: fire evennt :' + JSON.stringify(roster[friendNucleusId]));		
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
			console.log('onXmppRosterChanged: ' + JSON.stringify(rosterChangeObject));
            var id = getNucleusIdFromJid(rosterChangeObject.jid);

			removeItemFromFilteredRosters(id, roster[id]);
		
			if (rosterChangeObject.subState === 'REMOVE') {
			    // change the presence on the user object even though we
			    // are removing it from the roster as some of the directives
                // could still have a reference to it
			    roster[id].presence = { show: 'UNAVAILABLE' };
			    myEvents.fire('socialPresenceChanged:' + id, roster[id].presence);

                myEvents.fire('removedFriend', id);
                delete roster[id];
            } else {
				roster[id].subState = rosterChangeObject.subState;
				addItemToFilteredRosters(id, roster[id]);				
			}
        }
		
        /**
         * listens for xmpp connection to be complete, and initiates roster retrieval
         * @return {void}
         * @method
         */
        function onXmppConnected() {
            console.log('RosterDataFactory: onXmppConnected');
            if (!rosterRetrievalInProgress) {
                retrieveRoster();
            } else {
                console.log('RosterDataFactory - roster retrieval in progress');
            }     
        }

        /**
         * listens for JSSDK login to be complete, initiates xmpp connection
         * @return {void}
         * @method
         */
        function onJSSDKlogin() {
            //now that we're logged in, initiate xmpp startup
            if (Origin.xmpp.isConnected()) {
                onXmppConnected();
            }
            else {
                //setup listening fo the connection first
                Origin.events.on(Origin.events.XMPP_CONNECTED, onXmppConnected);
            }
			myEvents.fire('jssdkLogin');
        }

        /**
         * listens for JSSDK logout, clears roster
         * @return {void}
         * @method
         */
        function onJSSDKlogout() {
			if (!rosterLoaded) {
				cancelRosterLoad = true;
			}
            rosterLoaded = false;
            //clear the roster and playing list
            roster = {};
			filteredRosters = {};
			myEvents.fire('rosterCleared');
						
            for (var offer in playingOfferId) {
                if (playingOfferId.hasOwnProperty(offer)) {
                    myEvents.fire('socialFriendsPlayingListUpdated:' + offer, []);
                }
            }
            playingOfferId = {}; //list of users playing by offerId			
			
            currentUserPresence = { show: 'UNAVAILABLE' };			
            myEvents.fire('socialPresenceChanged:' + Origin.user.userPid(), currentUserPresence);
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
				filteredRosters[name] = { 'filter' : filter, 'roster': {} };				
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
		
        function init() {
            //set up to listen for login/logout
            Origin.events.on(Origin.events.AUTH_LOGGEDOUT, onJSSDKlogout);
            Origin.events.on(Origin.events.AUTH_SUCCESSRETRY, onJSSDKlogin);

            //if not yet logged in then onJSSDKlogin will get called
            //otherwise check to see if xmpp is connected, and if that's also done, then check for roster loading
            if (Origin.auth.isLoggedIn()) {
                if (!Origin.xmpp.isConnected()) {
                    onJSSDKlogin();
                } else {
                    onXmppConnected();
                }
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
					return ((!!item) && (item.subState === 'NONE' && !item.subReqSent));
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
            getIsInvisibile: function() {
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
                    sortedOfferIds = [],
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

                for (i = 0; i < unownedGames.length; i++) {
                    sortedOfferIds.push(unownedGames[i].offerId);
                }

                return sortedOfferIds;
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
                var socialPresence = { presence: '',ingame: false, joinable: false, broadcasting: false};
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
                Origin.xmpp.friendRequestAccept(roster[jid].jabberId);
            },

            /**
             * Reject a friend request from a given user
             * @method friendRequestReject
             */
            friendRequestReject: function(jid) {
                Origin.xmpp.friendRequestReject(roster[jid].jabberId);
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
				Origin.xmpp.friendRemove(roster[jid].jabberId);
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
				if (count !== viewportFriendCount || viewportFriendCountUsingDefault) {
					viewportFriendCount = count;
					viewportFriendCountUsingDefault = false;
					myEvents.fire('updateRosterViewport');
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
			
			registerRosterSection: function(filter) {
				console.log('TR: register roster section: ' + filter);
			}
						
			
        };

        
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function RosterDataFactorySingleton(UserDataFactory, AppCommFactory, SingletonRegistryFactory) {
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