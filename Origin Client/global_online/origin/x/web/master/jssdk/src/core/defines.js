/*jshint unused: false */
/*jshint strict: false */

define([], function () {
        /**
     * Contains authentication related methods
     * @module module:defines
     * @memberof module:Origin
     */
    var httpResponseCodes = {
        SUCCESS_200: 200,
        REDIRECT_302_FOUND: 302,
        ERROR_400_BAD_REQUEST: 400,
        ERROR_401_UNAUTHORIZED: 401,
        ERROR_403_FORBIDDEN: 403,
        ERROR_404_NOTFOUND: 404,
        ERROR_UNEXPECTED: -99
    };

    var loginTypes = {
        APP_INITIAL_LOGIN: 'login_app_initial', //login from the APP (via login window or auto-login), NOT retry for renewing sid
        APP_RETRY_LOGIN: 'login_app_retry', //login from the APP as part of retry (after jssdk retry due to AUTH_INVALID fails)
        AUTH_INVALID: 'login_auth_invalid', //after receiving 401/403 from http request
        POST_OFFLINE: 'login_post_offline' //after coming back from offline
    };

    return  /** @lends module:Origin.module:defines */{
        /**
         * @typedef httpResponseCodesObject
         * @type {object}
         * @property {number} SUCCESS_200 200
         * @property {number} REDIRECT_302_FOUND 302
         * @property {number} ERROR_400_BAD_REQUEST 400
         * @property {number} ERROR_401_UNAUTHORIZED 401
         * @property {number} ERROR_403_FORBIDDEN 403
         * @property {number} ERROR_404_NOTFOUND 404
         * @property {number} ERROR_UNEXPECTED -99
         */

        /**
         *  aliases for http codes
         * @type {module:Origin.module:defines~httpResponseCodesObject}
         */
        http: httpResponseCodes,

        /**
         * @typedef loginTypesObject
         * @type {object}
         * @property {string} APP_INITIAL_LOGIN default login, associated with app login
         * @property {string} APP_RETRY_LOGIN when app retries to login after failed session
         * @property {string} AUTH_INVALID login for auto-retry after INVALID_AUTH error is returned on http request
         * @property {string} POST_OFFLINE login for going back online after being offline (client)
         */
        /**
         *  enums for login types
         * @type {module:Origin.module:defines~loginTypesObject}
         */
        login: loginTypes
    };
});
