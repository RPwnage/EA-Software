/*jshint unused: false */
/*jshint strict: false */

define([
    'promise',
    'core/logger',
    'core/user',
    'core/dataManager',
    'core/urls',
    'core/errorhandler',
    'core/utils'
], function(Promise, logger, user, dataManager, urls, errorhandler, utils) {
    /**
     * @module module:idObfuscate
     * @memberof module:Origin
     */

    /**
     * Id Response Container - defines a response object from the obfuscation system
     * @typedef {Object} IdObject
     * @property {string} id the user id
     */

    /**
     * Encode a user's nucleus id into an obfuscated id for use in the URL
     * @param {String} userId  the nucleus userid to encode
     * @return {Promise.<IdObject, Error>}
     */
    function encodeUserId(userId) {
        var endPoint = urls.endPoints.userIdEncode;

        var config = {
            atype: 'GET',
            headers: [{
                'label': 'X-Origin-Platform',
                'val': utils.os()
            }],
            parameters: [{
                'label': 'userId',
                'val': userId
            }],
            appendparams: [],
            reqauth: true,
            requser: true
        };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'AuthToken', token);
        }

        return dataManager.enQueue(endPoint, config, 0);
    }

    /**
     * Decode an obfuscated user id into a nucleus user Id for use with backend services
     * @param {String} obfuscatedId the obfuscated user id to decode
     * @return {Promise.<IdObject, Error>}
     */
    function decodeUserId(obfuscatedId) {
        var endPoint = urls.endPoints.userIdDecode;

        var config = {
            atype: 'GET',
            headers: [{
                'label': 'X-Origin-Platform',
                'val': utils.os()
            }],
            parameters: [{
                'label': 'userId',
                'val': obfuscatedId
            }],
            appendparams: [],
            reqauth: true,
            requser: true
        };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'AuthToken', token);
        }

        return dataManager.enQueue(endPoint, config, 0);
    }

    return {
        encodeUserId: encodeUserId,
        decodeUserId: decodeUserId
    };
});
