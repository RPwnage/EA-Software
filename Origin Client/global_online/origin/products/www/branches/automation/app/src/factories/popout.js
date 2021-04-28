/**
 * factory for an error logging service
 */

(function() {
    'use strict';

    function PopOutFactory(DialogFactory, UtilFactory, UrlHelper, OriginPassThroughQueryParams, APPCONFIG, NucleusIdObfuscateFactory, ComponentsLogFactory) {

        // Temporary location to test OIG - not sure where to put that guy yet
        var supportedTypes = {
                'home' : '/',
                'profile' : '/profile',
                'settings' : '/settings/',
                'socialwindow' : 'social-chatwindow.html',
                'socialhub' :'social-hub.html',
                'game-library' : '/game-library',
                'addon-store' : '/oig-addon-store',
                'checkout' : '/oig-checkout',
                'download-manager' : '/oig-download-manager'
            },
            chatWindow, SPAWindow;

        /**
        * Add the OIG query params to the list of query parameters.
        * Don't worry, strings are passed by copy :)
        * @param {String} queryParam - the string of query params
        * @return {String} new query parameters
        */
        function addOigQueryParam(queryParams) {
            return UrlHelper.addToQueryString(queryParams, 'oigcontext=true');
        }

        /**
        * Add query parameters that should be passed through to any opening window
        * @param {String} queryParams the current set of query params
        * @return {String} the new query params with the pass throughs
        */
        function addPassThroughQueryParams(queryParams) {
            var passedParams = [];
            _.forEach(OriginPassThroughQueryParams, function(param) {
                if (APPCONFIG.overrides[param]) {
                    passedParams.push(param + '=' + APPCONFIG.overrides[param]);
                }
            });
            return UrlHelper.addToQueryString(queryParams, passedParams.join('&'));
        }

        /**
        * Encode the nucleus ID and add it to the extraParams
        * @param {String} url
        * @param {String} extraParams
        * @return {String} the new extraParams with encoded id
        */
        function encodeId(url, extraParams) {
            var paramList = extraParams.split('/');
            return NucleusIdObfuscateFactory.encode(_.last(paramList))
                .then(function (encodedId) {
                    extraParams = extraParams.replace(_.last(paramList), encodedId.id);
                    return extraParams;
                })
                .catch(function(error){
                    ComponentsLogFactory.error('Popout Factory error encodeing ID: ' + error);
                    // If we fail to encode here send the user to a 404 page
                    extraParams = '/user/404';
                    return extraParams;
                });
        }

        /**
        * Check to see if we are trying to access the Profile page so that
        * we can properly encode the nucleus id
        * @param {String} url
        * @param {String} extraParams
        * @return {String} the extra parameters
        */
        function checkForProfile(url, extraParams) {
            // Check to see if this is the Profile and the id has not alraedy been encoded
            if (url === supportedTypes.profile && Number(_.last(extraParams.split('/')))) {
                return encodeId(url, extraParams);
            }else {
                return Promise.resolve(extraParams);
            }
        }

        /**
        * This function is to be changed once we get the C++ code
        * to pass only oig as the extra parameter
        * @param {String} url
        * @param {String} extraParams
        * @return {String} url
        */
        function addExtraParams(url, extraParams) {
            if (angular.isUndefined(extraParams)) {
                return url;
            }

            var boom = extraParams.split('&');
            if (boom.length <= 1) {
                return url + extraParams;
            } else {
                for (var i=0, j=boom.length; i<j; i++) {
                    var keyValPair = boom[i].split('=');
                    if (keyValPair[0] === 'page') {
                        return url + '/' + keyValPair[1];
                    }
                }
                return url;
            }
        }

        function getChildWindow(requestType) {
            switch(requestType) {
                case 'socialwindow':
                    return chatWindow;
                case 'profile':
                case 'home':
                case 'settings':
                case 'addon-store':
                case 'checkout':
                case 'download-manager':
                    return SPAWindow;
                default:
                    return;
            }
        }

        /**
        * @param {String} requestType
        * @param {String} requestUrl - this is the page we want to navigate to.
        * @param {String} requestExtraParams - this is the page type, like voice or installs for settings
        *   so we want to add this value to the end of the url instead of as a query parameter
        * @param {String} oigParam
        * @param {boolean} isOIG - are we in the OIG context
        * @param {object} size - optional size parameter
        */
        function processOpenWindow(requestType, requestUrl, requestExtraParams, oigParam, isOIG, size) {
            var childWindow;
            var url = supportedTypes[requestType];
            // Make sure we have access to the bridge OriginIGOProxy object to allow the createWindow call to have access to the request info
            if (Origin.client.isEmbeddedBrowser()) {
                // Make sure the QWebPage::createWindow override can access the request to define on the appropriate resolution of the call
                Origin.client.oig.setCreateWindowRequest(requestUrl, oigParam);
                childWindow =  window.open(requestUrl); // this will trigger the QWebPage::createWindow method
            } else {
                childWindow =  window.open(url, requestType, 'location=0,toolbar=no,menubar=no,dialog=yes,top='+size.top+',left='+size.left+',width='+size.width+ ',height='+ (size.height));
            }

            childWindow.url = url;
            childWindow.extraParams = requestExtraParams;
            childWindow.oigParams = oigParam;

            switch(requestType) {
                case 'socialwindow':
                    chatWindow = childWindow; // this will trigger the QWebPage::createWindow method
                break;
                case 'profile':
                case 'home':
                case 'settings':
                case 'addon-store':
                case 'checkout':
                case 'download-manager':
                    SPAWindow = childWindow; // this will trigger the QWebPage::createWindow method
                break;
            }

            if (!isOIG) {
                return childWindow;
            }
        }

        /**
        * @param {String} requestType
        * @param {String} requestExtraParams - this is the page type, like voice or installs for settings
        *   so we want to add this value to the end of the url instead of as a query parameter
        * @param {String} oigParam
        * @param {object} size - optional size parameter
        */
        function openWindowImpl(requestType, requestExtraParams, oigParam, size) {

            // first look up the request type and process the URL
            var url = supportedTypes[requestType];
            var isOIG = (oigParam !== undefined && oigParam !== '');
            var queryParams = window.location.search;
            var requestUrl;

            if (url) {
                if (isOIG) {
                    queryParams = addOigQueryParam(queryParams);
                }
                queryParams = addPassThroughQueryParams(queryParams);
                return checkForProfile(url, requestExtraParams).then(function(newParams) {        
                    url = addExtraParams(url, newParams);
                    requestUrl = UrlHelper.buildAbsoluteUrl(url, queryParams);
                    return processOpenWindow(requestType, requestUrl, newParams, oigParam, isOIG, size);
                });

            } else {
                ComponentsLogFactory.error('Unknown window type: ' + requestType);
                return Promise.resolve(null);
            }
        }

        /**
        * @param {String} requestType
        * @param {String} requestUrl - this is the page we want to navigate to.
        */
        function processNavigateWindow(requestType, requestUrl) {

            var childWindow = getChildWindow(requestType);
            if (childWindow) {
                childWindow.location = requestUrl;
            } else {
                ComponentsLogFactory.error('PopoutFactory: no child window found of requestType: ', requestType);
            }

            // Now that we are at the correct location check for pending dialogs
            openOIGPendingDialogsImpl();
        }

        /**
        * @param {String} requestType
        * @param {String} requestExtraParams - this is the page type, like voice or installs for settings
        *   so we want to add this value to the end of the url instead of as a query parameter
        */
        function navigateWindowImpl(requestType, requestExtraParams){
            // first look up the request type and process the URL
            var url = supportedTypes[requestType];
            var requestUrl, queryParams;

            if (url) {
                queryParams = addOigQueryParam(window.location.search);
                queryParams = addPassThroughQueryParams(queryParams);
                checkForProfile(url, requestExtraParams).then(function(newParams) {        
                    url = addExtraParams(url, newParams);
                    requestUrl = UrlHelper.buildAbsoluteUrl(url, queryParams);
                    processNavigateWindow(requestType, requestUrl);
                });
            } else {
                ComponentsLogFactory.error('PopoutFactory: Unknown window type: ' + requestType);
            }
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

                if (Origin.client.isEmbeddedBrowser()) {
                    if (hubWindow.hasOwnProperty('OriginWindowUUID') && angular.isDefined(hubWindow.OriginWindowUUID)) {
                        // OriginWindowUUID is a special property that gets added to windows that we create in C++ so that we can identify them from Javascript
                        Origin.client.desktopServices.moveWindowToForeground(hubWindow.OriginWindowUUID);
                    } else if (!!windowFile) {
                        // window not tracked in C++, fall back on showWindow to move it to the front
                        Origin.client.oig.moveWindowToFront(windowFile);
                    } else {
                        // ??? - no way to identify the window, try focus() from Javascript?
                        hubWindow.focus();
                    }
                } else {
                    // in the browser, all that we can do is try focus() for pulling the window to the front
                    hubWindow.focus();
                }
            }
        }

        function openOIGPendingDialogsImpl() {
            DialogFactory.openPendingDialog();
        }

        function init(){
            //TODO: Move all this into the JSSDK event system
            window._openPopOutWindow = openWindowImpl;
            window._navigatePopOutWindow = navigateWindowImpl;
            window._moveWindowToFront = moveWindowToFrontImpl;
            window._openOIGPendingDialogs = openOIGPendingDialogsImpl;
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
    function PopOutFactorySingleton(DialogFactory, UtilFactory, UrlHelper, OriginPassThroughQueryParams, APPCONFIG, NucleusIdObfuscateFactory, ComponentsLogFactory, SingletonRegistryFactory) {
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
