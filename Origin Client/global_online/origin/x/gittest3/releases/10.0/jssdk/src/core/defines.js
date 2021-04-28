/*jshint unused: false */
/*jshint strict: false */

define([], function () {
    var CONFIG_URL_MODULE = 'jssdkconfig.json';
    var URL_SERVICE_BASE = 'https://s3.amazonaws.com/otk.dm.origin.com';
    var URL_SERVICE_ENDPOINT = '/config/{env}/{module}';

    var httpResponseCodes = {
        SUCCESS_200: 200,
        REDIRECT_302_FOUND: 302,
        ERROR_400_BAD_REQUEST: 400,
        ERROR_401_UNAUTHORIZED: 401,
        ERROR_403_FORBIDDEN: 403,
        ERROR_404_NOTFOUND: 404,
        ERROR_UNEXPECTED: -99
    };

    return {
        config: {
            URL_MODULE: CONFIG_URL_MODULE,
            URL_SERVICE_BASE: URL_SERVICE_BASE,
            URL_SERVICE_ENDPOINT: URL_SERVICE_ENDPOINT
        },

        http: httpResponseCodes
    };
});
