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
			ComponentsLogFactory.error('userData- requestAtomUserInfo', error);
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

        function handleAtomResponse(response) {
            var nucleusId = response[0].userId;

            if (!users[nucleusId]) {
                users[nucleusId] = {};
            }
            if (!users[nucleusId].atomUserInfo) {
                users[nucleusId].atomUserInfo = response[0];
            }
            return response[0];
        }

        function handleAtomPersonaResponse(response) {
            handleAtomResponse(response);

            return response[0].personaId;
        }

        function getPersonaId(nucleusId) {
            if (users[nucleusId] && users[nucleusId].atomUserInfo) {
                return Promise.resolve(users[nucleusId].atomUserInfo.personaId);
            }
            else {
                return Origin.atom.atomUserInfoByUserIds(nucleusId)
                           .then(handleAtomPersonaResponse, handleAtomUserError)
                           .catch(handleAtomUserException);
            }
        }

        function getUserInfo(nucleusId) {
            if (users[nucleusId] && users[nucleusId].atomUserInfo) {
                return Promise.resolve(users[nucleusId].atomUserInfo);
            }
            else {
                return Origin.atom.atomUserInfoByUserIds(nucleusId)
                           .then(handleAtomResponse, handleAtomUserError)
                           .catch(handleAtomUserException);
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
            ComponentsLogFactory.error('unable to retrieve avatar image from userdata service', error);
            return Promise.reject(error);
        }

        /**
         * Get user's avatar from server
         * @param {string} nucleusId
         * @param {string} avatarSize
         * @returns {Promise}
         */
        function getAvatarFromServer(nucleusId, avatarSize) {
            avatarSize = avatarSize || Origin.defines.avatarSizes.LARGE; // fall back to large size if not specified
            return Origin.avatar.avatarInfoByUserIds(nucleusId, avatarSize).then(setAvatarInfo(avatarSize), setAvatarInfoFailed);
        }

        /**
         * Get the user's avatar
         * @param {string} nucleusId
         * @param {string} avatarSize
         * @return {Promise}
         */
        function getAvatar(nucleusId, avatarSize) {
            if (users[nucleusId] && users[nucleusId].avatar && users[nucleusId].avatar[avatarSize]) {
                return Promise.resolve(users[nucleusId].avatar[avatarSize]);
            } else {
                return getAvatarFromServer(nucleusId, avatarSize);
            }
        }

        return {

			events : myEvents,

            getPersonaId: getPersonaId,

            getUserInfo: getUserInfo,

			getUserRealName: function(nucleusId) {
				return requestAtomUserInfo(nucleusId);
			},

            getAvatar: getAvatar,

            getAvatarFromServer: getAvatarFromServer,

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
