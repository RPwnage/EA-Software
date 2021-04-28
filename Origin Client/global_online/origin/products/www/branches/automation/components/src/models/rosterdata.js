(function() {
    'use strict';

    function RosterDataFactory(UserDataFactory, AuthFactory, EventsHelperFactory, ComponentsLogFactory, ObservableFactory, GamesDataFactory, ComponentsConfigHelper, ConversationService) {

        var roster = {},
            blockListWatch = ObservableFactory.observe({}),
		    xmppConnectionObservable = ObservableFactory.observe({isConnected: false}),
            onlineCountObservable = ObservableFactory.observe({count: 0}),

            filteredRosters = {},

            rosterRetrievalInProgress = false,
            rosterLoaded = false,
            cancelRosterLoad = false,
            requestRosterOnXmppConnect = false,

            RELATIONSHIP_SELF = 'SELF',
			RELATIONSHIP_FRIEND = 'FRIEND',
			RELATIONSHIP_PENDING_IN = 'PENDING_IN',
			RELATIONSHIP_PENDING_OUT = 'PENDING_OUT',
			RELATIONSHIP_NON_FRIEND = 'NON_FRIEND',
			RELATIONSHIP_BLOCKED = 'BLOCKED',

            PRESENCE_ONLINE = 'ONLINE',
			PRESENCE_INGAME = 'INGAME',
			PRESENCE_UNAVAILABLE = 'UNAVAILABLE',
			PRESENCE_SUBSCRIBE = 'SUBSCRIBE',
			PRESENCE_UNSUBSCRIBED = 'UNSUBSCRIBED',

            STATUS_LOADING = 'ROSTER_LOADING',
            STATUS_LOADED = 'ROSTER_LOADED',
            STATUS_ERROR = 'ROSTER_LOAD_ERROR',
            STATUS_TIMEOUT = 'ROSTER_LOAD_TIMEOUT',

            OFFER_ID_INDEX = 'offerId:',
            SINGLE_PLAYER_ID = 'SINGLE_PLAYER',

            rosterStatusObservable = ObservableFactory.observe({status: STATUS_LOADING, friendCount: 0, pendingCount: 0}),
            connectionTimeout,

            // For performance tuning
            rosterLoadStartTime = 0,

            myEvents = new Origin.utils.Communicator(),

            // Observables for users playing, or played this session
            friendsPlaying = {}, //list of users playing by masterTitleId + multiuserId
            friendsPlayingListObservable = ObservableFactory.observe(friendsPlaying),
            friendsPlayed = {}, // list of users that have played the title in the session

            currentUserPresence = {show: PRESENCE_UNAVAILABLE},
            isInvisible = false,
            rosterViewportHeight = 0,
            friendPanelHeight = 0,
            viewportFriendCount = 10,
            viewportFriendCountUsingDefault = true,
            initialized = false,
            handles = [],

            friendRequestAcceptInProgress = false;


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
            var id = friend.userId;
            var rosterItem = roster[id];
            if (typeof rosterItem !== 'undefined') {
                var hasNameChanged = (friend.firstName !== rosterItem.firstName) || (friend.lastName !== rosterItem.lastName) || (friend.EAID !== rosterItem.originId);
                if (hasNameChanged) {
                    rosterItem.firstName = (friend.firstName ? friend.firstName : rosterItem.firstName);
                    rosterItem.lastName = (friend.lastName ? friend.lastName : rosterItem.lastName);
                    rosterItem.originId = (friend.EAID ? friend.EAID : rosterItem.originId);
                    angular.merge(rosterItem.name.data,
                        { firstName: rosterItem.firstName, lastName: rosterItem.lastName, originId: rosterItem.originId });
                    rosterItem.name.commit();

                    // add or update filtered rosters
                    addItemToFilteredRosters(id, rosterItem);

                }

            }
        }

        function removeItemFromFilteredRosters(nucleusId, deferSignalHandler) {
            for (var name in filteredRosters) {
                if (!!filteredRosters[name].roster[nucleusId]) {
                    delete filteredRosters[name].roster[nucleusId];

                    myEvents.fire('socialRosterDelete' + name, nucleusId, (deferSignalHandler || false));

                    if (name==='ONLINE') {
                        onlineCountObservable.data.count--;
                        onlineCountObservable.commit();
                    }
                    if (name==='FRIENDS') {
                        angular.merge(rosterStatusObservable.data, {friendCount : Object.keys(filteredRosters[name].roster).length});
                        rosterStatusObservable.commit();
                    }
                    if (name==='INCOMING' || name==='OUTGOING') {
                        rosterStatusObservable.data.pendingCount--;
                        rosterStatusObservable.commit();
                    }
                }
            }
        }

        function addItemToFilteredRosters(nucleusId, friend) {
            for (var name in filteredRosters) {
                if (typeof filteredRosters[name].filter === 'function' && filteredRosters[name].filter(friend) && !filteredRosters[name].roster[nucleusId]) {
                    if (friend.originId) {
                        filteredRosters[name].roster[nucleusId] = friend;

                        myEvents.fire('socialRosterUpdate' + name, friend);
                        if (name==='ONLINE') {
                            onlineCountObservable.data.count++;
                            onlineCountObservable.commit();
                        }

                        if (name==='FRIENDS') {
                            angular.merge(rosterStatusObservable.data, {friendCount : Object.keys(filteredRosters[name].roster).length});
                            rosterStatusObservable.commit();
                        }

                        if (name==='INCOMING' || name==='OUTGOING') {
                            rosterStatusObservable.data.pendingCount++;
                            rosterStatusObservable.commit();
                        }
                    }
                }
            }
        }

        function clearFilteredRosters() {
            for (var name in filteredRosters) {
                if (Object.keys(filteredRosters[name].roster).length > 0) {

					myEvents.fire('socialRosterClear' + name);
                    filteredRosters[name].roster = {};
                }
            }

            onlineCountObservable.data.count = 0;
            onlineCountObservable.commit();

            angular.merge(rosterStatusObservable.data, {friendCount : 0 , pendingCount: 0});
            rosterStatusObservable.commit();

        }

		function getRelationship(nucleusId, subState, subReqSent, presenceType) {
			var relationship = RELATIONSHIP_NON_FRIEND;

            if (Number(nucleusId) === Number(Origin.user.userPid())) {
                relationship = RELATIONSHIP_SELF;
            }
            else if (subState === 'BOTH') {
				relationship = RELATIONSHIP_FRIEND;
			} else if (subState === 'NONE' && subReqSent) {
				relationship = RELATIONSHIP_PENDING_OUT;
			} else if (subState === 'NONE' && !subReqSent && presenceType === PRESENCE_SUBSCRIBE) {
				relationship = RELATIONSHIP_PENDING_IN;
			} else {
				relationship = RELATIONSHIP_NON_FRIEND;
			}
			return relationship;
		}

		function addOrUpdateRoster(nucleusId, originId, jabberId, subState, subReqSent, presence) {
            // check if this user has already been added to roster
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

				// Set up Observable objects here
				roster[nucleusId].subscription = ObservableFactory.observe({
					state: subState,
					requestSent: subReqSent,
					relationship: getRelationship(nucleusId, subState, subReqSent, presence.presenceType)
					});

				roster[nucleusId]._presence = ObservableFactory.observe(angular.merge(presence, {isJoinable: false, isBroadcasting: false, broadcastingUrl: ''}));

				roster[nucleusId].privacy = ObservableFactory.observe({isPrivate: false});

				roster[nucleusId].name = ObservableFactory.observe({firstName: '', lastName: '', originId: originId});


            } else { // this user is already on the roster, update it
                rosterEntry.originId = originId;
                rosterEntry.subState = subState;
                rosterEntry.subReqSent = subReqSent;

				angular.merge(rosterEntry.subscription.data,
									{ 	state: subState,
										requestSent: subReqSent,
										relationship: getRelationship(nucleusId, subState, subReqSent, presence.presenceType)
									});
				rosterEntry.subscription.commit();

				angular.merge(rosterEntry.name.data, {originId: originId});
				rosterEntry.name.commit();
            }

            addItemToFilteredRosters(nucleusId, roster[nucleusId]);

            // check to see if the added friend supports voice chat
            updateFriendVoiceSupported(nucleusId);

            // Once we have a single item in the roster signal that the results have started coming in
            if (Object.keys(roster).length === 1) {
                myEvents.fire('socialRosterResults');

                rosterStatusObservable.data.status = STATUS_LOADING;
                rosterStatusObservable.commit();
            }

            // get user's originId if not known
            if ((_.isUndefined(roster[nucleusId].originId) || !roster[nucleusId].originId || roster[nucleusId].originId.length === 0)) {
                UserDataFactory.getUserRealName(nucleusId);
                handles[handles.length] = UserDataFactory.events.on('socialFriendsFullNameUpdated:' + nucleusId, handleNameUpdate);
            }
        }

        function updateRosterPresence(friendNucleusId, newPresence) {

            /** Only retrieve master title for current login user.
             *
             * @param {object} updatedPresence updated user presence
             */
            function setMasterTitleForCurrentUserGameActivity(updatedPresence) {
                var jid = _.get(currentUserPresence, 'jid');
                if ( jid && (jid ===  _.get(updatedPresence, 'jid'))) {
                    var offerId = _.get(updatedPresence, ['gameActivity','productId']);
                    if(offerId) {
                        GamesDataFactory.getCatalogInfo([offerId])
                            .then(function(offer) {
                                _.set(currentUserPresence, ['gameActivity', 'masterTitle'],
                                    _.get(offer, [offerId, 'masterTitle']));
                            });
                    }
                }
            }

            if (!xmppConnectionObservable.data.isConnected) {
                // Ignore - we don't care about presence changes if we are disconnected from xmpp
                return;
            }

            //console.log('TR: updateRosterPresence: ' + JSON.stringify(newPresence));

            removeItemFromFilteredRosters(friendNucleusId);

			var rosterItem = roster[friendNucleusId];
			if (!!rosterItem) {
                if (!newPresence.groupActivity) {
                    newPresence.groupActivity = {groupId: '', groupName: ''};
                }

				rosterItem.presence = newPresence;

				var updatedPresence = {
					jid: newPresence.jid,
					show: newPresence.show,
					gameActivity: newPresence.gameActivity,
                    groupActivity: newPresence.groupActivity
				};
				updatedPresence.gameActivity.gameTitle = '';
				updatedPresence.isJoinable = false;
                updatedPresence.isGroupJoinable = false;
				updatedPresence.isBroadcasting = false;
				updatedPresence.isCompatiblePlatform = (Origin.voice.supported() === rosterItem.isVoiceSupported);

				if (newPresence.show === PRESENCE_ONLINE) {
					if (!!_.get(newPresence, ['gameActivity', 'title'], '')) {
						// If we have a gameActivity object and a title we must be playing a game.
						updatedPresence.show = PRESENCE_INGAME;

						updatedPresence.gameActivity.gameTitle = newPresence.gameActivity.title;
                        updatedPresence.gameActivity.productId = newPresence.gameActivity.productId;
                        updatedPresence.gameActivity.multiplayerId = newPresence.gameActivity.multiplayerId;
                        setMasterTitleForCurrentUserGameActivity(updatedPresence);

						// check for invitation - a invitation is required for "invite only" friends to be seen as joinable
						if (!!rosterItem) {
							if (!!_.get(updatedPresence, ['gameActivity', 'title'], '')) {

								var conversation = ConversationService.getConversationById(friendNucleusId);
								var invited = false;

								if (conversation) {
									if (conversation.state === 'ONE_ON_ONE') {
										if (!!conversation.gameInvite.from && !!conversation.gameInvite.productId && !!conversation.gameInvite.multiplayerId) {
											invited = true;
										}
									}

									// Joinable Invite Only state?
									if (updatedPresence.gameActivity.joinableInviteOnly && invited) {
										updatedPresence.isJoinable = true;
									}
								}

                                if (updatedPresence.gameActivity.joinable && !updatedPresence.gameActivity.joinableInviteOnly) {
                                    updatedPresence.isJoinable = true;
                                }
							}
						}

						// If we have a twitch presence we must be broadcasting
						if (!!newPresence.gameActivity.twitchPresence && newPresence.gameActivity.twitchPresence.length) {
							updatedPresence.isBroadcasting = true;
							updatedPresence.broadcastingUrl = newPresence.gameActivity.twitchPresence;
						}
					}
				}

                if (newPresence.groupActivity && newPresence.groupActivity.groupId !== '') {
                    updatedPresence.isGroupJoinable = true;
                }

				if (rosterItem._presence.data.presenceType !== newPresence.presenceType) {
					updatedPresence.presenceType = newPresence.presenceType;

					if (newPresence.presenceType === PRESENCE_SUBSCRIBE || newPresence.presenceType === PRESENCE_UNSUBSCRIBED || newPresence.presenceType === PRESENCE_UNAVAILABLE) {
						updatedPresence.show = PRESENCE_UNAVAILABLE;
					}

					var relationship = getRelationship(friendNucleusId, rosterItem.subState, rosterItem.subReqSent, newPresence.presenceType);

					if (relationship !== rosterItem.subscription.data.relationship) {
						rosterItem.subscription.data.relationship = relationship;
						rosterItem.subscription.commit();
					}
				}

				angular.merge(rosterItem._presence.data, updatedPresence);
				rosterItem._presence.commit();

			    // Any presence update (?) indicates that this profile is no longer private
				if (rosterItem.privacy.data.isPrivate) {
				    rosterItem.privacy.data.isPrivate = false;
				    rosterItem.privacy.commit();
				}

				addItemToFilteredRosters(friendNucleusId, rosterItem);

			}

            if (newPresence.show !== PRESENCE_UNAVAILABLE) {
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

					// Add self to roster
					addOrUpdateRoster(Origin.user.userPid(), Origin.user.originId(), Origin.xmpp.nucleusIdToJid(Origin.user.userPid()), 'SELF', false, {
						jid: Origin.xmpp.nucleusIdToJid(Origin.user.userPid()),
						presenceType: '',
						show: PRESENCE_UNAVAILABLE,
						gameActivity: {},
                        groupActivity: {}
					});

                    var previousTime = +new Date();

                    var processRosterResponse = function() {

                        var timeNow = +new Date();

                        // This checks for 'no friends' case.
                        // If no data.length, roster object assignment will cause 'ERROR'
                        if (data.length) {
                            var friend = data.pop();
                            var nucleusId = getNucleusIdFromJid(friend.jid);

                            //console.log('TR rosterDataFactory retrieveRoster addOrUpdateRoster: ' + friend.originId);

                            // Add all roster friends as offline, until presence is determined
                            addOrUpdateRoster(nucleusId, friend.originId, friend.jid, friend.subState, friend.subReqSent, {
                                jid: friend.jid,
                                presenceType: '',
                                show: PRESENCE_UNAVAILABLE,
                                gameActivity: {},
                                groupActivity: {}
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

                            angular.merge(rosterStatusObservable.data, {status : STATUS_LOADED});
                            rosterStatusObservable.commit();
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

                    rosterStatusObservable.data.status = STATUS_ERROR;
                    rosterStatusObservable.commit();

                }).catch(function(error) {
                    rosterLoaded = true;
                    console.log('retrieveRoster error:', error);
                    ComponentsLogFactory.error('RosterDataFactory - retrieveRoster', error);
                    myEvents.fire('socialRosterLoadError');

                    rosterStatusObservable.data.status = STATUS_ERROR;
                    rosterStatusObservable.commit();
                });
            }
        }

        /**
         * checks to see if the presence is embargo'd
         * @param  {object} the presence object
         * @return {Boolean} true if the game information shouldn't  be seen false if its ok (normal game)
         */
        function isEmbargoedPresence(presence) {
            //**HACK** For embargo'd games we really shouldn't be passing the offer id and we should use that to determine
            //When a game is embargo'd, however thats too risky to do right now, so for now we're using the embargo'd game title
            //Which is not localized as an "id" for embargo'd games. super fragile, but its better than nothing for now. Have another ticket
            //to do this properly open -- https://developer.origin.com/support/browse/ORCORE-4207

            var gameTitle = _.get(presence, ['gameActivity', 'gameTitle'], '');
            return gameTitle.indexOf('Unknown game [R&D Mode]') > -1;
        }


        /**
         * listens for presence change notifications from xmpp
         * @param {Object} newPresence
         * @method
         */
        function onXmppPresenceChanged(newPresence) {
            //console.log('TR: RosterDataFactory::onXmppPresenceChanged ' + JSON.stringify(newPresence));

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
                }

                //NOTE:  we may get presence change for things other than existing friends, e.g. when
                // adding a friend or receiving a friend request...
                if (!roster[friendNucleusId]) {
                    //console.log('RosterDataFactory: onXmppPresenceChanged: receiving presence for friend not in roster:', newPresence.jid);

                    if (newPresence.presenceType === PRESENCE_SUBSCRIBE) {
                        // Incoming friend request - add it to the roster
                        addOrUpdateRoster(friendNucleusId, '', newPresence.jid, 'NONE', false,  {
                                jid: newPresence.jid,
                                presenceType: newPresence.presenceType,
                                show: PRESENCE_UNAVAILABLE,
                                gameActivity: newPresence.gameActivity,
                                groupActivity: newPresence.groupActivity
                            });
                        myEvents.fire('socialPresenceChanged:' + friendNucleusId, newPresence);
                    } else if (newPresence.presenceType === 'UNSUBSCRIBE') {
                        // Cancel incoming friend request - remove it from the roster
                        removeItemFromFilteredRosters(friendNucleusId);
                    } else if (!rosterLoaded) {
                        // friend presence update before loaded into roster - add it to the roster

                        // the presence is not "offline". Offline friends will be added after roster load
                        if (newPresence.show !== PRESENCE_UNAVAILABLE) {
                            addOrUpdateRoster(friendNucleusId, '', newPresence.jid, '', false, {
                                    jid: newPresence.jid,
                                    presenceType: '',
                                    show: newPresence.show,
                                    gameActivity: newPresence.gameActivity,
                                    groupActivity: newPresence.groupActivity
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

                    if (!!roster[friendNucleusId].presence.gameActivity && !!roster[friendNucleusId].presence.gameActivity.productId) {
                        wasPlayingOfferId = roster[friendNucleusId].presence.gameActivity.productId;
                    }

                    updateRosterPresence(friendNucleusId, newPresence);

                    if ((Number(friendNucleusId) !== Number(Origin.user.userPid())) &&
                        (!isEmbargoedPresence(newPresence)))
                    {
                        if (!!wasPlayingOfferId.length) {
                            // remove the user from the list of users who were playing
                            // and add them to the list of users who have played the
                            // game with the last time that they played it
                            addToPlayedGamesList(wasPlayingOfferId, friendNucleusId);
                            removeFromPlayingGamesList(wasPlayingOfferId, friendNucleusId);
                        }

                        if (!!newPresence.gameActivity && !!newPresence.gameActivity.productId) {
                            addToPlayingGamesList(newPresence.gameActivity.productId, friendNucleusId);
                        }

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
            //console.log('TR: RosterDataFactory::onXmppRosterChanged ' + JSON.stringify(rosterChangeObject));
            var id = getNucleusIdFromJid(rosterChangeObject.jid);
			var rosterItem = roster[id];

            // Jingle sends a roster update event on presence change events
            if (typeof rosterItem === 'undefined') {
				// Do not re-add users to roster if this is a "block" operation
				// A "block" roster change notice looks like subState=="NONE" and subReqSent is false

				// Do add users to roster if this is "send friend request" operation.
				// A "send friend request" looks like subState=="NONE" and subReqSent is true.
				addOrUpdateRoster(id, '', rosterChangeObject.jid, rosterChangeObject.subState, rosterChangeObject.subReqSent,
					{	jid: rosterChangeObject.jid,
						presenceType: rosterChangeObject.presenceType,
						show: PRESENCE_UNAVAILABLE,
						gameActivity: {},
                        groupActivity: {}
					});
            // If the substate is 'NONE' and subReqSent == true treat this as a friend request
            // If this is a pending friend request update this use in the roster
            } else if (rosterChangeObject.subState === 'NONE' && rosterChangeObject.subReqSent) {
                addOrUpdateRoster(id, '', rosterChangeObject.jid, rosterChangeObject.subState, rosterChangeObject.subReqSent,
                    {   jid: rosterChangeObject.jid,
                        presenceType: rosterChangeObject.presenceType,
                        show: PRESENCE_UNAVAILABLE,
                        gameActivity: {},
                        groupActivity: {}
                    });
            }else if (rosterItem.subState !== rosterChangeObject.subState || rosterItem.subReqSent !== rosterChangeObject.subReqSent) {
				// Ignore the roster change event if there is no actual change to the roster

                if (rosterChangeObject.subState === 'REMOVE') {
                    // change the presence on the user object even though we
                    // are removing it from the roster as some of the directives
                    // could still have a reference to it
                    myEvents.fire('socialPresenceChanged:' + id, { show: PRESENCE_UNAVAILABLE });

                    myEvents.fire('removedFriend', id);

                    rosterItem.subState = 'NONE';
					rosterItem.subReqSent = false;

					angular.merge(rosterItem.subscription.data,
												{	state: 'NONE',
													requestSent: false,
													relationship: getRelationship(id, 'NONE', false)
												});
					rosterItem.subscription.commit();

					//TR 01-05-2016 try this for now, moved from profile/header.js
					Origin.xmpp.loadBlockList();

                    removeItemFromFilteredRosters(id);
                } else {
                    var friendRequestAccepted = false;
                    if (friendRequestAcceptInProgress && rosterItem.subState === 'NONE' && !rosterItem.subReqSent && rosterChangeObject.subState === 'BOTH') {
                        myEvents.fire('friendRequestAccepted');
                        friendRequestAccepted = true;
                    }

                    removeItemFromFilteredRosters(id, friendRequestAccepted);

                    rosterItem.subState = rosterChangeObject.subState;
					rosterItem.subReqSent = rosterChangeObject.subReqSent;

					angular.merge(rosterItem.subscription.data,
											{	state: rosterChangeObject.subState,
												requestSent: rosterChangeObject.subReqSent,
												relationship: getRelationship(id, rosterChangeObject.subState, rosterChangeObject.subReqSent)
											});
					rosterItem.subscription.commit();

                    addItemToFilteredRosters(id, rosterItem);
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
            window.clearTimeout(connectionTimeout);
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

			xmppConnectionObservable.data.isConnected = true;
			xmppConnectionObservable.commit();

            Origin.xmpp.loadBlockList();
        }

        function onXmppDisconnected() {
            rosterLoaded = false;
            rosterRetrievalInProgress = false;
            requestRosterOnXmppConnect = true;

			xmppConnectionObservable.data.isConnected = false;
			xmppConnectionObservable.commit();

            myEvents.fire('xmppDisconnected');

            angular.merge(rosterStatusObservable.data, {status: STATUS_LOADING });
            rosterStatusObservable.commit();

            window.clearTimeout(connectionTimeout);
            // Set up timeout to error out if no reconnection in 30 seconds
            connectionTimeout = window.setTimeout(function () {
                angular.merge(rosterStatusObservable.data, { status: STATUS_TIMEOUT });
                rosterStatusObservable.commit();
            }, 30000); // 30 seconds

        }


        function onXmppBlockListChanged() {
			blockListWatch.commit();
			function compareNucleusRelationships( nucleusId ){
                return function(isBlocked) {
                    var friend = getRosterItem(nucleusId);
                    if (friend.subscription.data.relationship === RELATIONSHIP_BLOCKED && !isBlocked) {
                        friend.subscription.data.relationship = RELATIONSHIP_NON_FRIEND;
                        friend.subscription.commit();
                    } else if (friend.subscription.data.relationship !== RELATIONSHIP_BLOCKED && isBlocked) {
                        friend.subscription.data.relationship = RELATIONSHIP_BLOCKED;
                        friend.subscription.commit();
                    }
                };
            }
            // check for blocked users
			//jshint loopfunc:true, unused:false
            for (var nucleusId in roster) {
                Origin.xmpp.isBlocked(nucleusId)
                    .then(compareNucleusRelationships(nucleusId));
            }
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

        function onClientOnlineStateChanging(onlineObj) {
            if (onlineObj && !onlineObj.isOnline) {
                window.clearTimeout(connectionTimeout);
                onJSSDKlogout();
            }
        }

        function onClientOnlineStateChanged(onlineObj) {
            if (onlineObj && onlineObj.isOnline) {
                window.clearTimeout(connectionTimeout);
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

            // automatically load the roster if social features are not enabled
            requestRosterOnXmppConnect = !ComponentsConfigHelper.enableSocialFeatures();

            if (Origin.xmpp.isConnected()) {
                requestRosterOnXmppConnect = true;
                onXmppConnected();
            } else {
                Origin.xmpp.connect();
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


            angular.merge(rosterStatusObservable.data, {friendCount: 0, pendingCount: 0});
            rosterStatusObservable.commit();

            myEvents.fire('rosterCleared');

            _.forEach(friendsPlaying, function(friendsPlayingObservable, index) {
                if (index.indexOf(OFFER_ID_INDEX) < 0) {
                    if (friendsPlayingObservable.data.list.length) {
                        friendsPlayingObservable.data.list = [];
                        friendsPlayingObservable.commit();
                    }
                }
            });

            _.forEach(friendsPlayed, function(friendsPlayedObservable, index) {
                if (index.indexOf(OFFER_ID_INDEX) < 0) {
                    if (friendsPlayedObservable.data.list.length) {
                        friendsPlayedObservable.data.list = [];
                        friendsPlayedObservable.commit();
                    }
                }
            });

            currentUserPresence = {
                show: PRESENCE_UNAVAILABLE
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
          * @param {Object} catalog
          * @return {Promise} a promise to the observable, if it does not exist it is created here
          *
          * @method getPlayingActivityObservable
          *
          * The observables are indexed by masterTitleId + multiuserId so that compatible games are included in the counts
          * Also providing a offerId -> masterTitleId + multiuserId cross-reference for synchronous data responses
          */
        function getPlayingActivityObservable(catalog) {
            var context = this;
            return new Promise(function(resolve) {
                if (Object.keys(catalog).length > 0) {
                    var offerId = Object.keys(catalog)[0],
                        catalogInfo = catalog[offerId],
                        masterTitleId = catalogInfo.masterTitleId,
                        multiPlayerId = (Origin.utils.getProperty(catalog, [offerId, 'platforms', Origin.utils.os(), 'multiPlayerId']) || SINGLE_PLAYER_ID),
                        observable = context[masterTitleId + '|' + multiPlayerId];

                    // Create new obserable if it doesn't exist
                    if (!observable) {
                        context[masterTitleId + '|' + multiPlayerId] = ObservableFactory.observe({list: [], friends: []});

                        // create a offerId / masterTitleId cross reference for returning friends
                        // playing data synchronously
                        context[OFFER_ID_INDEX + offerId] = masterTitleId + '|' + multiPlayerId;
                        observable = context[masterTitleId + '|' + multiPlayerId];
                    }
                    resolve(observable);
                } else {
                    resolve(null);
                }
            });
        }

        /**
          * returns the an observable of friends playing a game
          * @param {string} offerId
          * @return {Promise} a promise to the observable
          *
          * @method getFriendsPlayingObservable
          */
        function getFriendsPlayingObservable(offerId) {
            return GamesDataFactory.getCatalogInfo([offerId])
                    .then(getPlayingActivityObservable.bind(friendsPlaying));
        }

        /**
          * returns the an observable of friends that have played a game
          * @param {string} offerId
          * @return {Promise} a promise to the observable
          *
          * @method getFriendsPlayedObservable
          */
        function getFriendsPlayedObservable(offerId) {
            return GamesDataFactory.getCatalogInfo([offerId])
                    .then(getPlayingActivityObservable.bind(friendsPlayed));
        }


        /**
         * @param {string} friendNucleusId
         * @return {Function}
         * @method getAddFriendToObservableFn
         */
        function getAddFriendToObservableFn(friendNucleusId) {
            return function(observable) {
                if (!!observable) {
                    var index = observable.data.list.indexOf(friendNucleusId);
                    if (index < 0) {
                        observable.data.list.push(friendNucleusId);
                    }

                    var friendsIndex = _.findIndex(observable.data.friends, {'nucleusId': friendNucleusId});
                    if (friendsIndex < 0) {
                        observable.data.friends.push(roster[friendNucleusId]);
                    }

                    if (index < 0 || friendsIndex < 0) {
                        observable.commit();
                    }

                }
            };
        }

        /**
         * @param {string} friendNucleusId
         * @return {Function}
         * @method getRemoveFriendFromObservableFn
         */
        function getRemoveFriendFromObservableFn(friendNucleusId) {
            return function(observable) {
                if (!!observable) {
                    var index = observable.data.list.indexOf(friendNucleusId);
                    if (index >= 0) {
                        observable.data.list.splice(index, 1);
                    }

                    var friendsIndex = _.findIndex(observable.data.friends, {'nucleusId': friendNucleusId});
                    if (friendsIndex >= 0) {
                        observable.data.friends.splice(friendsIndex, 1);
                    }

                    if (index >= 0 || friendsIndex >= 0) {
                        observable.commit();
                    }

                }
            };
        }

        /**
         * given an offerId and friendsNucleusId, adds the friend to keep track of friends playing a game
         * @param {string} offerId
         * @param {string} friendNucleusId
         * @return {void}
         * @method addToPlayingGamesList
         */
        function addToPlayingGamesList(offerId, friendNucleusId) {
            getFriendsPlayingObservable(offerId)
                .then(getAddFriendToObservableFn(friendNucleusId))
                .then(friendsPlayingListObservable.commit.bind(friendsPlayingListObservable));
        }

        /**
         * given an offerId and friendsNucleusId, removes friend from the list of friends playing
         * @param {string} offerId
         * @param {string} friendNucleusId
         * @return {void}
         * @method removeFromPlayingGamesList
         */
        function removeFromPlayingGamesList(offerId, friendNucleusId) {
            getFriendsPlayingObservable(offerId)
                .then(getRemoveFriendFromObservableFn(friendNucleusId))
                .then(friendsPlayingListObservable.commit.bind(friendsPlayingListObservable));
        }

        /**
         * Maintains a collection of past players of a game
         * @param {string} offerId
         * @param {string} friendNucleusId
         * @return {void}
         * @method addToPlayedGamesList
         */
        function addToPlayedGamesList(offerId, friendNucleusId) {
            var thenFn = getAddFriendToObservableFn(friendNucleusId);
            getFriendsPlayedObservable(offerId).then(thenFn);
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

            window.clearTimeout(connectionTimeout);
            // Set up timeout to error out if no connection in 30 seconds
            connectionTimeout = window.setTimeout(function () {
                angular.merge(rosterStatusObservable.data, {status: STATUS_TIMEOUT, friendCount: 0, pendingCount: 0});
                rosterStatusObservable.commit();
                onJSSDKlogout();
            }, 30000); // 30 seconds

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

                'INCOMING': function (item) {
                    return ((!!item) && (item.subState === 'NONE' && !item.subReqSent) && (!!item.presence));
                },

                'OUTGOING' : function(item) {
                    return ((!!item) && (item.subState === 'NONE' && item.subReqSent));
                },

                'ONLINE': function(item) {
                    return ((!!item && item.subState === 'BOTH') && (!!item.presence && (item.presence.show === PRESENCE_ONLINE || item.presence.show === 'AWAY') && item.presence.presenceType !== PRESENCE_UNAVAILABLE));
                }
            };

        //we setup the filtered rosters here because getRoster will not be called until the directive is loaded
        //and there is a chance that directives can get loaded after initial presence is received
        initializeFilteredRosters();

		function getRosterItem(id) {
			// If no ID is specified, return current user
			if (!id || id.length === 0) {
				id = Origin.user.userPid();
			}
			if	(!roster[id]) {
				addOrUpdateRoster(id, '', Origin.xmpp.nucleusIdToJid(id), 'UNKNOWN', false, {
					jid: Origin.xmpp.nucleusIdToJid(id),
					presenceType: '',
					show: PRESENCE_UNAVAILABLE,
					gameActivity: {},
                    groupActivity: {}
				});

			}
			return roster[id];
		}

         function getFriendInfoPriv(id){
                return new Promise(function(resolve, reject) {
                    var friend;

                    if (rosterLoaded) {
                        friend = getRosterItem(id);

                        if (friend.originId === '') {
                            UserDataFactory.getUserInfo(id)
                                .then(function(userinfo) {
                                    friend.originId = userinfo.EAID;
                                    friend.firstName = userinfo.firstName;
                                    friend.lastName = userinfo.lastName;

                                    resolve(friend);
                                })
                                .catch(function(){
                                    resolve(friend);
                                });
                        } else {
                            resolve(friend);
                        }
                    } else {
                        myEvents.on('socialRosterLoaded', function() {
                            friend = getRosterItem(id);

                            resolve(friend);
                        });

                        myEvents.on('socialRosterLoadError', function() {
                            // This is probably not doing what we are expecting...
                            reject(getRosterItem(id));
                        });
                    }
                });
         }

        function isBlocked(nucleusId) {
            return Origin.xmpp.isBlocked(nucleusId);
        }



        return {

            init: init,
            events: myEvents,
            getRosterUser: getFriendInfoPriv,
			getFriendInfo: getFriendInfoPriv,

			getBlockListWatch: function() {
				return blockListWatch;
			},

			getXmppConnectionWatch: function() {
				return xmppConnectionObservable;
			},

            getOnlineCountWatch: function() {
                return onlineCountObservable;
            },

            getRosterStatusWatch: function() {
                return rosterStatusObservable;
            },

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

                    var id = Origin.user.userPid();
                    var rosterItem = roster[id];
                    var p = {};

                    if (presence === 'invisible') {
                        // Update roster object if "invisible" is requested
                        if (!!rosterItem) {

                            angular.copy(rosterItem.presence, p);
                            p.presenceType = PRESENCE_UNAVAILABLE;
                            updateRosterPresence(id, p);

                        }

                    } else {

                        // simply going back to visible, update roster object here as no
                        // presence change response is expected back from the XMPP server
                        if (presence.toUpperCase() === rosterItem.presence.show) {

                            angular.copy(rosterItem.presence, p);
                            p.presenceType = '';
                            updateRosterPresence(id, p);

                        }

                    }

                }


                // Set the actual presence
                Origin.xmpp.requestPresence(presence);
            },

            updatePresence: function(nucleusId, newPresence) {
                updateRosterPresence(nucleusId, newPresence);
            },

            /**
             * @method retrieves the visibility state for the current user
             */
            getIsInvisible: function() {
                return isInvisible;
            },

            /**
             * regardless of whether logged in, connected or not, just have it try to return a list of friends playing, if the roster is empty, it will return empty list
             * @param {string} offerId
             * @return {void}
             * @method friendsWhoArePlaying
             */
            friendsWhoArePlaying: function(offerId) {
                var result = [];
                var friendsPlayingIndex = friendsPlaying[OFFER_ID_INDEX + offerId];
                if (!!friendsPlayingIndex) {
                    result = friendsPlaying[friendsPlayingIndex].data.list;
                }
                return result;
            },

            /**
             * returns an Observable object that contains a list of friends playing a game (offer Id), or compatible game
             * @param {string} offerId
             * @return {void}
             * @method getFriendsPlayingObservable
             */
            getFriendsPlayingObservable: getFriendsPlayingObservable,

            /**
             * returns an Observable object that contains a list of friends that have played a game (offer Id), or compatible game, this login session
             * @param {string} offerId
             * @return {void}
             * @method getFriendsPlayingObservable
             */
            getFriendsPlayedObservable: getFriendsPlayedObservable,

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

            populateUnownedGamesList: function(ownedGames) {
                var unownedGames = [];

                function compareListLengths(a, b) {
                    return b.playingFriends.length - a.playingFriends.length;
                }

                function getCompareOfferIdFn(offerId) {
                    return function(o) {
                        return (offerId === o.offerId);
                    };
                }

                function entitlementExists(offerId) {
                    var compareFn = getCompareOfferIdFn(offerId);
                    return !!(_.find(ownedGames[0], compareFn));
                }

                _.forEach(friendsPlaying, function(value, key) {
                    if (key.indexOf(OFFER_ID_INDEX) === 0) {
                        var offerId = key.slice(OFFER_ID_INDEX.length);
                        var friendsPlayingIndex = value;
                        if (!entitlementExists(offerId) && friendsPlaying[friendsPlayingIndex].data.list.length) {
                            unownedGames.push({
                                offerId: offerId,
                                playingFriends: friendsPlaying[friendsPlayingIndex].data.list
                            });
                        }
                    }

                });

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
                            if (friend.presence.show === PRESENCE_ONLINE) {
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
                Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_FRIEND_REQUEST_ACCEPT, jid);
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
                Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_FRIEND_REQUEST_REJECT, jid);
                Origin.xmpp.friendRequestReject(roster[jid].jabberId);
            },

            /**
             * Send a friend request to a given user
             * @method sendFriendRequest
             */
            sendFriendRequest: function(jid, source) {
                Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_FRIEND_REQUEST_SEND, jid,
                    source ? source : 'chat');
                Origin.xmpp.friendRequestSend(Origin.xmpp.nucleusIdToJid(jid), 'chat');
            },

            /**
             * Cancel a pending friend request for a given user
             * @method cancelPendingFriendRequest
             */
            cancelPendingFriendRequest: function(jid) {
                Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_FRIEND_REQUEST_CANCEL, jid);
                Origin.xmpp.friendRequestRevoke(roster[jid].jabberId);
            },

            /**
             * Remove a friend for a given user
             * @method removeFriend
             */
            removeFriend: function(jid) {
                Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_FRIEND_REMOVE, jid);
                Origin.xmpp.removeFriend(roster[jid].jabberId);
            },

            removeFriendAndBlock: function(nucleusId) {
                Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_FRIEND_REMOVE_BLOCK, nucleusId);
                Origin.xmpp.removeFriendAndBlock(nucleusId, roster[nucleusId].jabberId);
            },

             /**
             * Block a given user
             * @method blockUser
             */
            blockUser: function(nucleusId) {
                Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_FRIEND_BLOCK, nucleusId);
                Origin.xmpp.blockUser(nucleusId);
            },

             /**
             * Block a given user, and cancel pending friend request
             * @method cancelAndBlockUser
             */
            cancelAndBlockUser: function(nucleusId) {
                Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_FRIEND_REQUEST_CANCEL_BLOCK, nucleusId);
                Origin.xmpp.cancelAndBlockUser(nucleusId);
            },

             /**
             * Block a given user, and ignore incoming friend request
             * @method ignoreAndBlockUser
             */
            ignoreAndBlockUser: function(nucleusId) {
                Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_FRIEND_REQUEST_IGNORE_BLOCK, nucleusId);
                Origin.xmpp.ignoreAndBlockUser(nucleusId);
            },

             /**
             * Unblock a given user
             * @method unblockUser
             */
            unblockUser: function(nucleusId) {
                Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_FRIEND_UNBLOCK, nucleusId);
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

            isBlocked: isBlocked,

            deleteFromSectionRoster: function(filter, nucleusId) {
                 myEvents.fire('socialRosterDelete' + filter, nucleusId, false);
            },

            setProfilePrivacy: function(nucleusId, isPrivate) {
				getFriendInfoPriv(nucleusId).then(function(user) {
					if (user.privacy.data.isPrivate !== isPrivate) {
						user.privacy.data.isPrivate = isPrivate;
						user.privacy.commit();
					}
				});
            },

            loadBlockList: function() {
                Origin.xmpp.loadBlockList();
            },

            isEmbargoedPresence: isEmbargoedPresence ,

            getFriendsPlaying: function(){
                return friendsPlaying;
            },

            observeFriendsPlayingList: function(fn){
                friendsPlayingListObservable.addObserver(fn);
            },

            unobserveFriendsPlayingList: function(fn){
                friendsPlayingListObservable.removeObserver(fn);
            }
        };


    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function RosterDataFactorySingleton(UserDataFactory, AuthFactory, EventsHelperFactory, ComponentsLogFactory, ObservableFactory, GamesDataFactory, ComponentsConfigHelper, ConversationService, SingletonRegistryFactory) {
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
