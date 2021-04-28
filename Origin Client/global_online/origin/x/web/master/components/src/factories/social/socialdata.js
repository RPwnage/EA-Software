(function() {
    'use strict';

    function SocialDataFactory($filter, DialogFactory, AuthFactory, ComponentsLogFactory) {

        var myEvents = new Origin.utils.Communicator(),
            groupNames = {},
            groupMembers = {},
            groups = {};

        var groupListLoaded = false;
		var jssdkLogin = false;
        var initialized = false;



        function updateGroupMemberPresence(newPresence) {
            // Parse out the nucleusId and originId for the groupMembers lookup
            if (!!newPresence.from) {
                var memberNucleusId = newPresence.from.split('@')[0];
                var memberOriginId = newPresence.nick;

                if ((typeof groupMembers[memberNucleusId] === 'undefined') && (memberNucleusId.length > 0) && (memberOriginId.length > 0)) {

                    groupMembers[memberNucleusId] = {};
					if (memberNucleusId === Origin.user.userPid()) {
                        groupMembers[memberNucleusId] = Origin.user.originId();

                        // provide a cross-reference for originId to nucleusId
                        groupMembers[Origin.user.originId()] = Origin.user.userPid();

                    } else {
                        groupMembers[memberNucleusId] = memberOriginId;

                        // provide a cross-reference for originId to nucleusId
                        groupMembers[memberOriginId] = memberNucleusId;
                    }

                    myEvents.fire('groupMembersUpdated');

                }
            }
            //console.log('TR: updateGroupMemberPresence: ' + JSON.stringify(groupMembers));
        }


        /**
         * @return {void}
         * @method
         */
         /*
        function getMembersUsernames(groupId) {
            return new Promise(function(resolve, reject) {

                // TR fix this
                reject = reject;

                var memberUsernameFoundCount = 0;
                var unknownMembers = [];

                // set up origin Id information for roster friends
                $.each(groups[groupId].members, function(i, member) {

                    if (typeof groupMembers[member.id] === 'undefined') {

                        groupMembers[member.id] = {};
                        // check if this member is a friend : already in the roster
                        if (typeof roster[member.id] !== 'undefined') {
                            groupMembers[member.id].originId = roster[member.id].originId;

                            // provide a cross-reference for originId to nucleusId
                            groupMembers[roster[member.id].originId] = member.id;

                            memberUsernameFoundCount++;
                        } else if (member.id === Origin.user.userPid()) {
                            groupMembers[member.id].originId = Origin.auth.originId();

                            // provide a cross-reference for originId to nucleusId
                            groupMembers[Origin.auth.originId()] = Origin.user.userPid();

                            memberUsernameFoundCount++;
                        } else {
                            // need to retrieve this data from atom. Collect in groups of five to improve performance
                            unknownMembers.push(member.id);
                        }

                    } else {
                        memberUsernameFoundCount++;
                    }

                });

                if (unknownMembers.length === 0) {
                    console.log('TR: getMembersUsernames: '+ groups[groupId].name +' resolved');
                    fullyLoadedGroups[groupId] = groups[groupId];
                    resolve();
                } else {

                    var handleAtomUserInfo = function(users) {
                        $.each(users, function(i, user) {
                            groupMembers[user.userId].originId = user.EAID;

                            // provide a cross-reference for originId to nucleusId
                            groupMembers[user.EAID] = user.userId;

                            memberUsernameFoundCount++;

                            if (memberUsernameFoundCount >= groups[groupId].members.length) {
                                console.log('TR: getMembersUsernames: '+ groups[groupId].name +' resolved');
                                fullyLoadedGroups[groupId] = groups[groupId];
                                resolve();
                            }

                        });
                    };

                    do {
                        var queryMemberList = [];
                        for (var x = 0; x< Math.min(5, unknownMembers.length); x++) {
                            queryMemberList.push(unknownMembers.pop());
                        }

                        // retrieve user info from atom
                        Origin.atom.atomUserInfoByUserIds(queryMemberList).then(handleAtomUserInfo);

                    } while (unknownMembers.length > 0);

                }

            });
        }
        */
        /**
         * @return {void}
         * @method
         */
         /*
        function getMembersList(groupId) {

            return new Promise(function(resolve, reject) {
                var group = groups[groupId];
                var pageSize = 100;
                var pageStart = 0;
                var responseCount = 0;

                function getGroupMembersPage() {
                    console.log('TR: getGroupMembersPage: ' + groups[groupId].name + ' pageStart: ' + pageStart);
                    Origin.groups.memberListByGroup(group._id, pageStart, pageSize).then(function(response) {
                        if (typeof group.members === 'undefined') {
                            group.members = [];
                        }

                        group.members = group.members.concat(response);
                        console.log('TR: group.members.length: ' + group.members.length);


                        responseCount = Object.keys(response).length;
                        if (responseCount !== pageSize) {
                            // all members are retrieved
                            resolve(groupId);
                        } else {
                            // query next page of members
                            pageStart += pageSize;
                            getGroupMembersPage();
                        }
                    },
                    function(error) {
                        reject(error);
                    }).catch(function(error) {
                        reject(error);
                    });

                }

                getGroupMembersPage();

            });
        }
*/

        /**
         * @return {void}
         * @method
         */
        function getRoomsList(groupId) {

            return new Promise(function(resolve, reject) {
                var group = groups[groupId];
                Origin.groups.roomListByGroup(group._id).then(function(response) {
                    groups[groupId].rooms = response;

                    // set up group names map
                    var roomJid = (groups[groupId].rooms[0].roomJid.split('@'))[0];
                    groupNames[roomJid] = groups[groupId].name;
                    //console.log('TR: getRoomsList '+groups[groupId].name+' resolve');
                    resolve(groupId);
                },
                function(error) {
                    reject(error);
                }).catch(function(error) {
                    reject(error);
                });
            });

        }

        /**
         * @return {void}
         * @method
         */
         /*
        function getGroupsRoomsAndMembers() {

            var groupsLoadCompletedCount = 0;

            var handleGetMembersUsernames = function() {
                groupsLoadCompletedCount++;

                if (groupsLoadCompletedCount === Object.keys(groups).length) {
                    groupListLoaded = true;
                    fullyLoadedGroups = {};
                    myEvents.fire('socialGroupListLoaded');
                } else {
                    myEvents.fire('socialGroupListUpdated', fullyLoadedGroups);
                }
            };

            var handleGetMembersList = function(id) {
                // get all members usernames
                getMembersUsernames(id).then(handleGetMembersUsernames);
            };

            var handleGetRooms = function(id) {
                // get list of members
                getMembersList(id).then(handleGetMembersList);
            };


            for (var id in groups) {
                // get list of rooms
                getRoomsList(id).then(handleGetRooms);
            }
        }
        */
        /**
         * @return {void}
         * @method
         */
        function getGroupsList() {
            Origin.groups.groupListByUserId(Origin.user.userPid()).then(function(groupsListArray) {
				//console.log('SocialDataFactory getGroupsList(): result: ' + groupsListArray.length);
                (function getNextGroup() {
                    if (groupsListArray.length > 0) {
                        var g = groupsListArray.pop();
						//console.log('SocialDataFactory getGroupsList(): groupsListArray: ' + groupsListArray.length);
                        groups[g._id] = g;

                        // Once we have a single item in the groups list signal that the results have started coming in
                        if (Object.keys(groups).length === 1) {
				            myEvents.fire('socialGroupListResults');
                        }

                        getRoomsList(g._id).then(function() {

                            if (groupsListArray.length === 0) {
                                // All current groups are now loaded, now load invite groups
                                getInviteGroups();
                            } else {
                                myEvents.fire('socialGroupListUpdated', groups);
                                getNextGroup();
                            }

                        });
                    }
                    else {
                        // There are no groups. Look for invite groups.
                        getInviteGroups();
                    }
                })();

                function getInviteGroups() {
                    Origin.groups.groupInviteListByUserId(Origin.user.userPid()).then(function(inviteListArray) {
                        while (inviteListArray.length > 0) {
                            var g = inviteListArray.pop();
                            groups[g._id] = g;
                            groups[g._id].inviteGroup = true;

                            // Once we have a single item in the groups list signal that the results have started coming in
                            if (Object.keys(groups).length === 1) {
				                myEvents.fire('socialGroupListResults');
                            }
                        }
                        groupListLoaded = true;
                        myEvents.fire('socialGroupListLoaded');
                    },
                    function(error) {
                        groupListLoaded = true;
                        myEvents.fire('socialGroupListLoadError');
                        console.log('SocialDataFactory: groupListByUserId error:', error);
                    }).catch(function(error) {
                        groupListLoaded = true;
                        myEvents.fire('socialGroupListLoadError');
                        ComponentsLogFactory.error('socialData - groupListByUserId', error);
                        console.log('SocialDataFactory: groupListByUserId error:', error);
                    });
                }
            },
            function(error) {
                groupListLoaded = true;
                myEvents.fire('socialGroupListLoadError');
                console.log('SocialDataFactory: groupListByUserId error:', error);
            }).catch(function(error) {
                groupListLoaded = true;
                myEvents.fire('socialGroupListLoadError');
                ComponentsLogFactory.error('socialData - groupListByUserId', error);
                console.log('SocialDataFactory: groupListByUserId error:', error);
            });
        }

        /**
         * listens for xmpp connection to be complete
         * @return {void}
         * @method
         */
        function onXmppConnected() {
            console.log('SocialDataFactory: onXmppConnected');
			Origin.events.on(Origin.events.XMPP_PRESENCECHANGED, onXmppPresenceChanged);

            // disabling groups
			if (false) { getGroupsList(); }
        }

        /**
         * listens for presence change notifications from xmpp
         * @param {Object} newPresence
         * @method
         */
        function onXmppPresenceChanged(newPresence) {
            // disabling groups
            if (false) { updateGroupMemberPresence(newPresence); }
        }


        /**
         * listens for xmpp disconnection
         * @return {void}
         * @method
         */
        function onXmppUserConflict() {
            DialogFactory.openAlert({
                id: 'web-disconnected-from-xmpp',
                title: 'Disconnected from XMPP',
                description: 'You have been disconnected from XMPP due to another person using this same account. Friends information as well as remote interaction with the Origin client will be unavailable.',
                rejectText: 'OK'
            });
        }

        /**
         * listens for JSSDK login to be complete, initiates xmpp connection
         * @return {void}
         * @method
         */
        function onJSSDKlogin() {
            //now that we're logged in, initiate xmpp startup
            console.log('SocialData: onJSSDKlogin');
            if (Origin.xmpp.isConnected()) {
                onXmppConnected();
            }
            else {
                //setup listening fo the connection first
                Origin.events.on(Origin.events.XMPP_CONNECTED, onXmppConnected);
                Origin.events.on(Origin.events.XMPP_USERCONFLICT, onXmppUserConflict);
                Origin.xmpp.connect();
            }
			myEvents.fire('jssdkLogin', true);
			jssdkLogin = true;
        }

        /**
         * listens for JSSDK logout
         * @return {void}
         * @method
         */
        function onJSSDKlogout() {
            initialized = false;
			jssdkLogin = false;
			myEvents.fire('jssdkLogin', false);

        }

        function onAuthChanged (loginObj) {
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
                console.log('SocialData: Already initialized');
                return;
            }

            console.log('SocialData: init()');

            //set up to listen for login/logout
            AuthFactory.events.on('myauth:change', onAuthChanged);

            //if not yet logged in then onJSSDKlogin will get called
            //otherwise check to see if xmpp is connected, and if that's also done, then check for roster loading
            if (AuthFactory.isAppLoggedIn()) {
                if (!Origin.xmpp.isConnected()) {
                    onJSSDKlogin();
                } else {
					myEvents.fire('jssdkLogin', true);
					jssdkLogin = true;

                    onXmppConnected();
                }
                initialized = true;
            } else {
                console.log('SocialData: auth not logged in');
            }
        }

        return {

            init: init,
            events: myEvents,

            /**
             * gameActivity Info associated with presence
             * @typedef gameActivityObject
             * @type {object}
             * @property {string} title title of game friend is playing
             * @property {string} productId offerId of game being played
             * @property {bool} joinable
             * @property {string} twitchPresence
             * @property {string} gamePresence INGAME, etc.
             * @property {string} multiplayerId
             */

            /**
             * presence Info
             * @typedef presenceObject
             * @type {object}
             * @property {string} jid jabberId of friend
             * @property {string} presenceType UNAVAILABLE/SUBSCRIBE/SUBSCRIBED, etc.
             * @property {string} show presence to display ONLINE/AWAY/OFFLINE, etc.
             * @property {gameActivityObject} gameActivity if friend is playing a game, may be empty/undefined if not ingame
             */

            /**
             * friend Info
             * @typedef friendInfoObject
             * @type {object}
             * @property {string} originId originId of friend
             * @property {string} fullname full name of friend
             * @property {presenceObject} presence info
             */


            /**
             * returns group member nucleusId or originId
             * @param {string} originId or nucleusId
             * @return {matching nucleusId or originId
             * @method
             */
            getGroupMemberInfo: function(id) {
                return new Promise(function(resolve) {
                    if (typeof groupMembers[id] !== 'undefined') {
                        resolve(groupMembers[id]);
                    } else {
                        if (id === Origin.user.originId()) {
                            resolve(Origin.user.userPid());
                        } else if (id === Origin.user.userPid()) {
                            resolve(Origin.user.originId());
                        } else {
                            // PDH - Wait for the data to come in
                            // TODO - there should probably be a timeout with reject here
                            myEvents.on('groupMembersUpdated', function() {
                                if (typeof groupMembers[id] !== 'undefined') {
                                    resolve(groupMembers[id]);
                                }
                            });
                        }
                    }
                });
            },


            /**
             * @method
             */
            acceptGroupInvite: function(jid) {
                Origin.groups.acceptGroupInvite(jid, Origin.user.userPid()).then(function(response) {
                    if (!!response && response.status === 'JOINED') {
                        delete groups[jid].inviteGroup;

                        getRoomsList(jid).then(function() {
                            if (!!groups[jid].rooms && groups[jid].rooms.length > 0) {
                                // Upon group invite acceptance, join the room
                                myEvents.fire('joinConversation', groups[jid].rooms[0].roomJid);
                            }
                        });

                        myEvents.fire('socialGroupListUpdated', groups);
                    }
                }
                );
            },

            /**
             * @method
             */
            declineGroupInvite: function(jid) {
                Origin.groups.cancelGroupInvite(jid, Origin.user.userPid()).then(function(response) {
                    if (response.length === 0) {
                        // need a better indicator of success?
                        delete groups[jid];
                        myEvents.fire('socialGroupListUpdated', groups);
                    }
                });
            },


            /**
             * returns groups
             * @return {groups data}
             * @method
             */
            getGroups: function() {
                //console.log('SocialDataFactory: getGroups');
                return new Promise(function(resolve, reject) {

                    if (groupListLoaded) {
                        resolve(groups);
                    } else {
                        myEvents.on('socialGroupListLoaded', function() {
                            resolve(groups);
                        });

                        myEvents.on('socialGroupListLoadError', function() {
                            reject(groups);
                        });

                    }
                });
            },

            /**
             * returns groups
             * @return {groups data}
             * @method
             */
            getGroupName: function(jid) {
                jid = jid.split('@')[0];
                return new Promise(function(resolve, reject) {

                    if (groupListLoaded) {
                        resolve(groupNames[jid]);
                    } else {
                        myEvents.on('socialGroupListLoaded', function() {
                            resolve(groupNames[jid]);
                        });

                        myEvents.on('socialGroupListLoadError', function() {
                            reject('group list load error');
                        });

                    }
                });
            },

			userLoggedIn: function() {
				return jssdkLogin;
			},

            // This is for child window so they are aware of the social status
            updateLoginStatus: function() {
                if (!this.userLoggedIn()) {
                    onJSSDKlogin();
                } else {
                    myEvents.fire('jssdkLogin', true);
                }
			}
        };


    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function SocialDataFactorySingleton($filter, DialogFactory, AuthFactory, ComponentsLogFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('SocialDataFactory', SocialDataFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.SocialDataFactory

     * @description
     *
     * SocialDataFactory
     */
    angular.module('origin-components')
        .factory('SocialDataFactory', SocialDataFactorySingleton);
}());