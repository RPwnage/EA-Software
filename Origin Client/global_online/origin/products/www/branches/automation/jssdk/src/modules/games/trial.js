/*jshint unused: false */
/*jshint strict: false */
define([
    'promise',
    'core/logger',
    'core/user',
    'core/urls',
    'core/dataManager',
    'core/errorhandler',
    'core/locale',
    'core/utils',
    'modules/client/client'
], function(Promise, logger, user, urls, dataManager, errorhandler, locale, utils, client) {
    /**
     * @module module:trial
     * @memberof module:Origin
     */

    var subReqNum = 0;  //to keep track of the request #

    function getTime(contentId) {
        var token = user.publicObjs.accessToken();
        if (token.length === 0) {
            // early return if this is called when user is not authenticated
            return Promise.resolve({success: false, message: 'requires-auth'});
        }

        if (!contentId) {
            // early return if this is called without a contentId
            return Promise.resolve({success: false, message: 'requires-content-id'});
        }

        var endPoint = urls.endPoints.trialCheckTime;
        var config = {atype: 'GET', reqauth: true, requser: true };

        dataManager.addHeader(config, 'Authorization', 'Bearer '+token);
        dataManager.addAuthHint(config, 'Authorization', 'Bearer {token}');
        dataManager.addHeader(config, 'Accept', 'application/json');
        dataManager.addHeader(config, 'localeInfo', locale.locale());
        dataManager.addParameter(config, 'userId', user.publicObjs.userPid());
        dataManager.addParameter(config, 'contentId', contentId);

        return dataManager.enQueue(endPoint, config, subReqNum)
            .then(function(data){
                if (data.hasOwnProperty('checkTimeResponse')) {
                    return data.checkTimeResponse;
                } else {
                    return data;
                }
            })
            .catch(function(error){
                var errorMsg = utils.getProperty(error, ['response', 'error', 'failure', 'cause']) || 'request-failed';
                errorhandler.logAndCleanup('trial:getTime FAILED');
                return {success: false, message: errorMsg};
            });
    }

    return /** @lends module:Origin.module:trial */{

        /**
         * @typedef getTimeResponseObject
         * @type {object}
         * @property {boolean} hasTimeLeft user has time left in trial
         * @property {integer} leftTrialSec time left in trial (in seconds)
         * @property {integer} totalTrialSec total time for trial (in seconds)
         * @property {integer} totalGrantedSec total time granted (in seconds)
         */
        /**
         * Get the remaining play time for a users trial offer
         *
         * @param {string} contentId contentId of entitled offer
         * @return {promise<module:Origin.module:trial~getTimeResponseObject>} responsename getTime response object
         * @method
         */
        getTime: getTime
    };
});