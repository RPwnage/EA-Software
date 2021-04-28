/**
 * factory for an error logging service
 */

(function() {
    'use strict';

    function PopOutFactory() {

        // Temporary location to test OIG - not sure where to put that guy yet
        var supportedTypes = {
                'settings' : 'index.html',
                'profile' : '#/profile',
                'socialwindow' : 'social-chatwindow.html',
                'socialhub' :'social-hub.html'
            };
        var chatWindow, SPAWindow;

        function getChildWindow(requestType) {
            switch(requestType) {
                case 'socialwindow':
                    return chatWindow;
                case 'profile':
                    return SPAWindow;
                default:
                    return;
            }
        }

        function openWindowImpl(requestType, requestExtraParams,  size) {
            // first look up the request type and process the URL
            var url = supportedTypes[requestType];
            var requestUrl;
            var returnWindow = true;
            var childWindow;
            if (url) {

                //IE10 doesn't support window.location.origin
                if (!Origin.client.isEmbeddedBrowser()) {
                    window.location.origin = window.location.protocol + '//' + window.location.host;
                }

                requestUrl = window.location.origin + window.location.pathname + url;
                if (requestExtraParams !== undefined && requestExtraParams !== '') {
                    console.log('We have extra params: ', requestExtraParams);
                    requestUrl += (url.indexOf('?') === -1) ? '?' : '&';
                    requestUrl += requestExtraParams;
                    // If this is an OIG window we can't return the window object as it breaks the C++ client
                    if (requestExtraParams.indexOf('oigwindowid') !== -1){
                        returnWindow = false;
                    }
                }
            }
            else {
                console.error('Uknown window type: ' + requestType);
            }

            // Make sure we have access to the bridge OriginIGOProxy object to allow the createWindow call to have access to the request info
            if (Origin.client.isEmbeddedBrowser()) {
                    // Make sure the QWebPage::createWindow override can access the request to define on the appropriate resolution of the call
                    Origin.client.oig.setCreateWindowRequest(requestUrl);
                    childWindow =  window.open(requestUrl); // this will trigger the QWebPage::createWindow method
            }
            else {
                childWindow =  window.open(url, requestType, 'location=0,toolbar=no,menubar=no,dialog=yes,top='+size.top+',left='+size.left+',width='+size.width+ ',height='+ (size.height));
            }
            childWindow.url = url;
            childWindow.extraParams = requestExtraParams;
            switch(requestType) {
                case 'socialwindow':
                    chatWindow = childWindow; // this will trigger the QWebPage::createWindow method
                break;
                case 'profile':
                    SPAWindow = childWindow; // this will trigger the QWebPage::createWindow method
                break;
            }

            if (returnWindow) {
                return childWindow;
            }
        }

        function navigateWindowImpl(requestType, requestExtraParams){
            /// Break this out into it's own function
            // first look up the request type and process the URL
            console.log('We are in navigateWindowImpl');
            var url = supportedTypes.profile;
            var requestUrl;
//            var returnWindow = true;
            if (url) {

                //IE10 doesn't support window.location.origin
                if (!Origin.client.isEmbeddedBrowser()) {
                    window.location.origin = window.location.protocol + '//' + window.location.host;
                }

                requestUrl = window.location.origin + window.location.pathname + url;
                if (requestExtraParams !== undefined && requestExtraParams !== '') {
                    console.log('We have extra params: ', requestExtraParams);
                    requestUrl += (url.indexOf('?') === -1) ? '?' : '&';
                    requestUrl += requestExtraParams;
                }
            }
            else {
                console.error('Uknown window type: ' + requestType);
            }
            //console.log('Navigating From C++ to profile id: ', userId);
            console.log('This should be the new url: ', requestUrl);
            var childWindow = getChildWindow(requestType);
            childWindow.location = requestUrl;
        }
        
        function moveWindowToFrontImpl(hubWindow) {
            
            var windowFile = null;
            if (hubWindow) {
                var location = hubWindow.location.href;
                
                for (var key in supportedTypes) {
                    var value = supportedTypes[key];
                    if (location.indexOf(value) > 0) {
                        windowFile = value;
                        break;
                    }
                }
                
                if (Origin.client.isEmbeddedBrowser() && !!windowFile) {
                    Origin.client.oig.moveWindowToFront(windowFile);
                } else {
                    hubWindow.focus();
                }
            }
        }

        function init(){
            window._openPopOutWindow = openWindowImpl;
            window._navigatePopOutWindow = navigateWindowImpl;
            window._moveWindowToFront = moveWindowToFrontImpl;
        }

        return {
            openWindow : openWindowImpl,
            navigateWindow : navigateWindowImpl,
            init : init
        };     
    }
    
     /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a pop-up)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function PopOutFactorySingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('PopOutFactory', PopOutFactory, this, arguments);
    }
    
    /**
     * @ngdoc service
     * @name originApp.factories.LogFactory
     * @requires $log
     * @description
     *
     * LogFactory
     */
    angular.module('originApp').factory('PopOutFactory', PopOutFactorySingleton);
}());

