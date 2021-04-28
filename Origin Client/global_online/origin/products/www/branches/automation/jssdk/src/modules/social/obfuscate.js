/*jshint unused: false */
/*jshint strict: false */

define([
    'core/logger',
    'core/user',
    'core/dataManager',
    'core/urls',
    'core/errorhandler',
], function(logger, user, dataManager, urls, errorhandler) {

    return /** @lends module:Origin.module:obfuscate */ {

        /**
         * This will return an obfuscated id for the given user
         * @return {string} return obfuscated string
         */
        encode: function(nucleusId) {
            var endPoint = urls.endPoints.idObsfucationEncodePair;

            var config = {
                atype: 'GET',
                headers: [],
                parameters: [{
                    'label': 'id',
                    'val': nucleusId
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
        },
        /**
         * Takes in a list of nucleus ids and adds them to a ignore list for friends recommendations
         * @param  {string} nucleusIds a single/or array of nucleus ids
         * @return {promise} retVal resolves when the post call has completed
         */
        decode: function(encodedId) {
            var endPoint = urls.endPoints.idObsfucationDecodePair;

            var config = {
                atype: 'GET',
                headers: [],
                parameters: [{
                    'label': 'id',
                    'val': encodedId
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
    };
});
