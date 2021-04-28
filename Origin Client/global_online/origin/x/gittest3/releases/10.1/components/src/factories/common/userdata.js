/**
 * @file common/userdata.js
 */
(function() {
    'use strict';
	
	var MAX_ATOM_REQUEST_IDS = 5;

    function UserDataFactory(UtilFactory, ComponentsLogFactory) {

        var users = {};
        var authReady = false;
        var isLoggedIn = false;
		var myEvents = new Origin.utils.Communicator();
		var atomRequestIds = [];
		
		var handleAtomUserInfoResponse = function(response) {
			for (var j = 0; j < response.length; j++) {
				//extract out the full name
				var nucleusId = response[j].userId;
				
				if (!users[nucleusId]) {
					users[nucleusId] = {};
				}
				
				if (!users[nucleusId].atomUserInfo) {
					users[nucleusId].atomUserInfo = response[j];
				}
				
				myEvents.fire( 'socialFriendsFullNameUpdated:' + nucleusId, response[j]);                        
			}
		};
		
		var handleAtomUserError = function(error) {
			ComponentsLogFactory.log('requestAtomUserInfo atomUserInfoByUserIds: error', error);
		};
		
		var handleAtomUserException = function(error) {
			ComponentsLogFactory.error(error, 'userData- requestAtomUserInfo');
		};
		
		var directAtomUserRequest = function() {			
			while (atomRequestIds.length) {
				Origin.atom.atomUserInfoByUserIds(atomRequestIds.splice(0, MAX_ATOM_REQUEST_IDS))
					.then(handleAtomUserInfoResponse, handleAtomUserError)
					.catch(handleAtomUserException);
			}

		};
		
		var throttledAtomUserRequest = UtilFactory.throttle(directAtomUserRequest, 500);
			
        /**
         * @method requestAtomUserInfo
         */
        function requestAtomUserInfo(nucleusId) {
			if (users[nucleusId] && users[nucleusId].atomUserInfo) {
				setTimeout(function() {
					myEvents.fire( 'socialFriendsFullNameUpdated:' + nucleusId, users[nucleusId].atomUserInfo);
				}, 0); 
			} else {				
				if (atomRequestIds.indexOf(nucleusId) < 0) { 
					atomRequestIds.push(nucleusId);
				}
				if (atomRequestIds.length < MAX_ATOM_REQUEST_IDS) {
					throttledAtomUserRequest();				
				} else {
					directAtomUserRequest();
				}
			}
			
        }
		
        /**
         * Store the avatar info
         * @param {string} nucleusId
         * @param {string} avatarSize
         * @param {object} data - the data for the avatar
         * @return {void}
         * @method setAvatarInfo
         */
        function setAvatarInfo(avatarSize) {
            return function(response) {
                var nucleusId = response[0].userId;
                if (!users[nucleusId]) {
                    users[nucleusId] = {};
                }
                if (!users[nucleusId].avatar) {
                    users[nucleusId].avatar = {};
                }
                users[nucleusId].avatar[avatarSize] = response[0].avatar.link;

                return users[nucleusId].avatar[avatarSize];
            };
        }

        function setAvatarInfoFailed(error) {
            ComponentsLogFactory.error('unable to retrieve avatar image from userdata service');
            return Promise.reject(error);
        }

        return {
			
			events : myEvents,
			
			getUserRealName: function(nucleusId) {
				return requestAtomUserInfo(nucleusId);
			},

            /**
             * Get the user's avatar
             * @param {string} nucleusId
             * @param {string} avatarSize
             * @return {Promise}
             * @method getAvatar
             */
             getAvatar: function(nucleusId, avatarSize) {
                var promise = null;

                if (users[nucleusId] && users[nucleusId].avatar && users[nucleusId].avatar[avatarSize]) {
                    promise = Promise.resolve(users[nucleusId].avatar[avatarSize]);
                } else {
                    if (avatarSize === 'AVATAR_SZ_SMALL') {
                        promise = Origin.avatar.avatarInfoByUserIds(nucleusId, avatarSize).then(setAvatarInfo('AVATAR_SZ_SMALL'), setAvatarInfoFailed);
                    } else if (avatarSize === 'AVATAR_SZ_MEDIUM') {
                        promise = Origin.avatar.avatarInfoByUserIds(nucleusId, avatarSize).then(setAvatarInfo('AVATAR_SZ_MEDIUM'), setAvatarInfoFailed);
                    } else {
                        promise = Origin.avatar.avatarInfoByUserIds(nucleusId, avatarSize).then(setAvatarInfo('AVATAR_SZ_LARGE'), setAvatarInfoFailed);
                    }
                }
                return promise;

            },

            setAuthReady: function(obj) {
                authReady = true;
                isLoggedIn = obj.isLoggedIn;
            },
            isLoggedIn: function() {
                return isLoggedIn;
            },

            isAuthReady: function() {
                return authReady;
            }
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function UserDataFactorySingleton(UtilFactory, ComponentsLogFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('UserDataFactory', UserDataFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.UserDataFactory
     
     * @description
     *
     * UserDataFactory
     */
    angular.module('origin-components')
        .factory('UserDataFactory', UserDataFactorySingleton);

}());