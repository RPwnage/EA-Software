/*jshint unused: false */
/*jshint strict: false */
define([
    'promise',
    'core/logger',
    'core/user',
    'core/dataManager',
    'core/urls',
    'core/errorhandler',
    'core/defines'
], function(Promise, logger, user, dataManager, urls, errorhandler, defines) {

    /**
     * @module module:avatar
     * @memberof module:Origin
     */

    function processAvatarInfoResponse(response) {
        //if only one user was specified, then it returns the response as users.user.avatar
        //but if more than one user was specified, it returns the response as users.user[].avatar
        //so we need to convert the single user case into returning it as an array (of one element) too.
        if (!(response.users.user instanceof Array)) {
            response.users.user = [response.users.user];
        }
        return response.users.user;
    }

    return /** @lends module:Origin.module:avatar */{

        /**
         * avatar info
         * @typedef avatarObject
         * @type {object}
         * @property {string} avatarId
         * @property {string} galleryId
         * @property {string} galleryName
         * @property {boolean} isRecent
         * @property {string} link url for the avatar image
         * @property {string} orderNumber
         * @property {string} statusId
         * @property {string} stausName
         * @property {string} typeId
         * @property {string} typeName
         */

        /**
         * avatar info for a single user
         * @typedef avatarInfoObject
         * @type {object}
         * @property {avatarObject} avatar
         * @property {string} userId nucleusId of the user
         */

        /**
         * This will return a promise for the avatar info for each of the users in the userId list
         *
         * @param {Object} list of userIds, separate by ;
         * @param {string} avatarSize AVATAR_SZ_SMALL, AVATAR_SZ_MEDIUM, AVATAR_SZ_LARGE
         * @return {promise<module:Origin.module:avatar~avatarInfoObject[]>}  name array of avatarInfoObjects
         */
        avatarInfoByUserIds: function(userIdList, avatarSize) {

            var endPoint = urls.endPoints.avatarUrls;

            var sizeVal = 0;
            if (avatarSize === defines.avatarSizes.SMALL) {
                sizeVal = 0;
            } else if (avatarSize === defines.avatarSizes.MEDIUM) {
                sizeVal = 1;
            } else if (avatarSize === defines.avatarSizes.LARGE) {
                sizeVal = 2;
            }

            var config = {
                atype: 'GET',
                headers: [{
                    'label': 'Accept',
                    'val': 'x-cache/force-write'
                }],
                parameters: [{
                    'label': 'userIdList',
                    'val': userIdList
                }, {
                    'label': 'size',
                    'val': sizeVal
                }],
                appendparams: [],
                reqauth: true,
                requser: false
            };

            var token = user.publicObjs.accessToken();
            if (token.length > 0) {
                dataManager.addHeader(config, 'AuthToken', token);
            }
            return dataManager.enQueue(endPoint, config, 0)
                .then(processAvatarInfoResponse, errorhandler.logAndCleanup('AVATAR:avatarInfoByUserIds FAILED'));
        },
    };
});