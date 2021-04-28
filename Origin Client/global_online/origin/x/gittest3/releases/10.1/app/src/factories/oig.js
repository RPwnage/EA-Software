/**
 * factory for an error logging service
 */

(function() {
    'use strict';

    function OIGFactory() {

        // Temporary location to test OIG - not sure where to put that guy yet
        var supportedTypes = {
                'settings' : 'index.html'
            };

        function openWindowImpl(requestType, requestExtraParams) {
            // Make sure we have access to the bridge OriginIGOProxy object to allow the createWindow call to have acces to the request info
            if (window.OriginIGO !== undefined) {
                var url = supportedTypes[requestType];
                if (url !== undefined && url !== '') {

                    //IE10 doesn't support window.location.origin
                    if (typeof window.location.origin === 'undefined') {
                        window.location.origin = window.location.protocol + '//' + window.location.host;                        
                    }

                    var requestUrl = window.location.origin + '/' + url;
                    if (requestExtraParams !== undefined && requestExtraParams !== '') {
                        requestUrl += (url.indexOf('?') === -1) ? '?' : '&';
                        requestUrl += requestExtraParams;
                    }
                    // Make sure the QWebPage::createWindow override can access the request to define on the appropriate resolution of the call
                    window.OriginIGO.setCreateWindowRequest(requestUrl); 

                    /*var child =*/ window.open(requestUrl); // this will trigger the QWebPage::createWindow method
                    //child.Origin = window.Origin;
                }
            }
        }

        return {
            openWindow : openWindowImpl
        };     
    }
    /**
     * @ngdoc service
     * @name originApp.factories.LogFactory
     * @requires $log
     * @description
     *
     * LogFactory
     */
    angular.module('originApp').factory('OIGFactory', OIGFactory);

    // Make this visible to the C++ client too
    window.openIGOWindow = new OIGFactory().openWindow;
}());

