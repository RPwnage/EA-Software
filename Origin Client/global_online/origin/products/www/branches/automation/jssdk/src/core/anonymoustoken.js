define([
    'core/urls',
    'core/dataManager',
    'core/errorhandler'
], function(urls, dataManager, errorhandler) {

    'use strict';
    var TEN_MINUTES = 1000 /* milliseconds */ * 60 /* seconds */ * 10 /* minutes */;
    var token;
    /**
     * Create and return a AnonymourToken for the app to use (Singleton)
     * @return {object} A promise that will return the token when complete.
     */
    function getAnonymousToken() {
        token = token ? token : token = new AnonymousToken();
        return token.ensureAnonymousToken();
    }

    /**
     * Private data layer function  that will call th server end point to get an anonymous token
     * @return {Promise} A Rest call promise
     */
    function retrieveAnonymousToken() {
        var endPoint = urls.endPoints.anonymousToken,
            config = {
                atype: 'POST',
                headers: [{
                    'label': 'accept',
                    'val': 'application/json'
                }, {
                    'label': 'Content-Type',
                    'val' : 'application/x-www-form-urlencoded'
                }],
                parameters: [],
                reqauth: false, //set these to false so that dataREST doesn't automatically trigger a relogin
                requser: false
            },
            requestBody = 'grant_type=anonymous_token&client_id=ORIGIN_JS_SDK&client_secret=68BEdbSaTQEQba8DvV2RGbNl9KRrY5stIN1IDVjPwm1ycRea3u9nlKCcuHD1uQybjQK7s2j4M3E7V1J8';
            config.body = requestBody;

        return dataManager.dataREST(endPoint, config)
            .catch(errorhandler.logAndCleanup('AUTH: anonymous token retrieval failed'));
    }

    // ANONYMOUS TOKEN //
    /**
     * Constructor for the cart token object
     * The token is an inner class used to handle the expire logic around a cart token
     */
    function AnonymousToken() {
        this.id = undefined;
        this.expires = 0;
    }

    /**
     * Check to see if a token is expired
     * @return {Boolean} is the token expired
     */
    AnonymousToken.prototype.isExpired = function() {
        return (this.expires - Date.now() < 0);
    };

    /**
     * Simple getter for the token id
     * @return {string} the token id.
     */
    AnonymousToken.prototype.getId = function() {
        return this.id;
    };

    /**
     * Make sure that the current token is up to date and valid.
     * @return {Promise} This promise will return the AnonymousToken object
     */
    AnonymousToken.prototype.ensureAnonymousToken = function() {
        var token = this;
        return new Promise(function(resolve) {
            if (token.isExpired()) {
                // We don't return the promise from refresh token because it returns the
                // http response from the server. This way the data is consistent.
                token.refreshAnonymousToken().then(function() {
                    resolve(token);
                });
            } else {
                resolve(token);
            }
        });
    };

    /**
     * Get a new token from the cart service
     * @return {Promise} The promise that will resolve when a new token is returned
     */
    AnonymousToken.prototype.refreshAnonymousToken = function() {
        var token = this;
        return retrieveAnonymousToken().then(function(response) {
            /* jshint camelcase:false */
            if (response && response.access_token  && response.expires_in){
                token.id = response.access_token;
                token.expires = Date.now() + (response.expires_in * 1000) - TEN_MINUTES;
            }
            /* jshint camelcase:true */
        });
    };

    return {
        getAnonymousToken: getAnonymousToken
    };
});
