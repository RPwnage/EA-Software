/*jshint unused: false */
/*jshint strict: false */
define([
    'promise',
    'core/defines',
    'core/logger',
    'core/user',
    'core/auth',
    'core/events',
    'core/dataManager',
    'core/urls',
    'core/errorhandler',
], function(Promise, defines, logger, user, auth, events, dataManager, urls, errorhandler) {

    /**
     * @module module:email
     * @memberof module:Origin.module:settings
     */
    function handleSendEmailVerificationResponse(response) {
        return response;
    }

    function sendEmailVerification() {
        var endPoint = urls.endPoints.sendEmailVerification;
        var config = {
            atype: 'POST',
            headers: [{
                'Content-Type': 'application/x-www-form-urlencoded'
            }],
            parameters: [],
            appendparams: [],
            reqauth: true,
            requser: false,
            responseHeader: false
        };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'accessToken', token);
        }

        return dataManager.enQueue(endPoint, config)
            .then(handleSendEmailVerificationResponse, errorhandler.logAndCleanup('GETSTARTED:sendEmailVarification FAILED'));
    }

    function handleSendOptinEmailSettingResponse(response) {
        return response;
    }

    function sendOptinEmailSetting() {
        var endPoint = urls.endPoints.optinToOriginEmail;
        var config = {
            atype: 'POST',
            headers: [{
                'Content-Type': 'application/x-www-form-urlencoded'
            }],
            parameters: [{
                'label': 'pid',
                'val': user.publicObjs.userPid()
            }],
            appendparams: [],
            reqauth: true,
            requser: true,
            responseHeader: false
        };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'Authorization', 'Bearer ' + token);
        }

        return dataManager.enQueue(endPoint, config)
            .then(handleSendOptinEmailSettingResponse, errorhandler.logAndCleanup('GETSTARTED:sendOptinEmailSetting FAILED'));
    }

    return /** @lends module:Origin.module:settings.module:email */ {

        /**
         * Makes a request to send the user a verification email to the address attached to the user's account
         * @method
        */
        sendEmailVerification: sendEmailVerification,
        /**
         * Makes a request to sign the user up for origin emails
         * @method
        */
        sendOptinEmailSetting: sendOptinEmailSetting,
    };
});