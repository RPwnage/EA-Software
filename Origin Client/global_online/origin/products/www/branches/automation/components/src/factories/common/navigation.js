
(function() {
    'use strict';

    /**
     * Get the name of the query parameters in the order
     * that they appear in the query string
     * @return {Array} query parameter names
     */
    function getQueryParamNames() {
        var search = window.location.search,
            queryParams = [];
        if (search) {
            search.substring(1).split('&').forEach(function (item) {
                queryParams.push(item.split('=')[0]);
            });
        }
        return queryParams;
    }

    /**
     * Order the pass through query params in the order that they
     * appear on the initial page load as query parameters
     * @param {Array} defaultParams - the default list of query params to be ordered
     * @return {Array}
     */
    function orderPassThroughQueryParams(defaultParams) {
        var queryParams = getQueryParamNames(),
            orderedParams = [];
        for (var i = 0, j = queryParams.length; i < j; i++) {
            var paramIndex = defaultParams.indexOf(queryParams[i]);
            if (paramIndex !== -1) {
                orderedParams.push(defaultParams[paramIndex]);
                defaultParams.splice(paramIndex, 1);
            }
        }
        return orderedParams.concat(defaultParams);
    }


    function NavigationFactory($location, $state, AppCommFactory, OriginPassThroughQueryParams, UtilFactory, UrlHelper, NucleusIdObfuscateFactory) {

        function appendOriginQueryParams(newUrl) {
            var questionMarkExists = newUrl.indexOf('?') >= 0;

            var existingQueryParams = $location.search();
                var i = 0;
                _.forEach(OriginPassThroughQueryParams, function(param) {
                    var paramIndex = newUrl.indexOf(param);
                    if (paramIndex < 0 && existingQueryParams[param]) {
                        newUrl += ((i === 0 && !questionMarkExists) ? '?' : '&') + param + '=' + existingQueryParams[param];
                        i++;
                    }
                });

            return newUrl;
        }

        /**
         * Go to a internal SPA location
         *
         * @param {String} newUrl
         * @method internalUrl
         */
        function internalUrl(newUrl) {
            newUrl = appendOriginQueryParams(newUrl);
            $location.url(newUrl);
        }

        /**
         * Go to a external URL
         * @param url
         * @param openInNewTab
         */
        function externalUrl(url, openInNewTab) {
            if (Origin.client.isEmbeddedBrowser() || openInNewTab) {
                Origin.windows.externalUrl(url);
            } else {
                window.open(url);
            }
        }

        /**
         * Go to a route state if state name is provided
         *
         * Redirect to relative page if a path is given
         *
         * Opens in new window if absolute path is given
         *
         * @param {String} url
         * @param {Object} stateParams
         * @method openLink
         */
        function openLink(urlOrState, stateParams){
            if(urlOrState) {
                if ($state.href(urlOrState)) { //If url is a angular ui state, use state go
                    $state.go(urlOrState, stateParams);
                }else if (UtilFactory.isAbsoluteUrl(urlOrState)) { //If absolute URL
                    externalUrl(urlOrState);
                } else { //If relative URL
                    var href = UtilFactory.getRelativeUrlInAngularContext(urlOrState);
                    internalUrl(href);
                }
            }
        }

        function goToState(state, stateParams) {
            AppCommFactory.events.fire('uiRouter:go', state, stateParams);
        }

        /**
         * Get URL href from state name and params
         * @param {string} stateName
         * @param {object} params
         * @param {object} options
         * @returns {string} href
         */
        function getHrefFromState(stateName, params, options){
            return $state.href(stateName, params, options);
        }

        /**
        * Go to achievements on the owned gamed details page
        * @param {String} offerId
        */
        function goOgdAchievements(offerId) {
            goToState('app.game_gamelibrary.ogd.topic', {
                'offerid': offerId,
                'topic': 'achievements'
            });
        }

        /**
         * Return an absolute URL from a list of URL paths
         * @param paths
         * @returns {string}
         */
        function getUrlWithDomainOrigin(paths){
            var origin = UrlHelper.getWindowOrigin();
            var relativeUrl = _.map(paths, function(path){
                return _.trim(path, '/');
            }).join('/');
            return [origin, relativeUrl].join('/');
        }

        /**
         * Given an relative page, absolute path or a angular route state name with state parameters,
         * this function returns the absolute URL
         *
         * @param {String} urlOrState relative URL, absolute URL or a state name
         * @param {Object} stateParams state parameters if the first argument is a state name
         * @returns {string} absoluteUrl
         */
        function getAbsoluteUrl(urlOrState, stateParams){
            var absUrl = '';
            if(urlOrState) {
                if ($state.href(urlOrState)) { //If url is a angular ui state, use state go
                    absUrl = getUrlWithDomainOrigin([$state.href(urlOrState, stateParams)]);
                }else if (UtilFactory.isAbsoluteUrl(urlOrState)) { //If absolute URL
                    absUrl = urlOrState;
                } else { //If relative URL
                    var baseHref = UtilFactory.getBaseHref(),
                        relativeUrl = urlOrState;
                    if (baseHref !== '/'){
                        relativeUrl = urlOrState.replace(baseHref, '');
                    }
                    absUrl = getUrlWithDomainOrigin(_.compact([_.trim(baseHref, '/'), _.trim(relativeUrl, '/')]));
                }
            }
            return absUrl;
        }

        /**
         * Navigate to a user's profile page with an obfuscated Id
         * @param {String} nucleusId the nucleus id
         * @param {String} topic the topic route name
         */
        function goUserProfile(nucleusId, topic) {
            NucleusIdObfuscateFactory.encode(nucleusId)
                .then(function(encodedId){
                    if (angular.isUndefined(topic)) {
                        goToState('app.profile.user', {
                            'id': encodedId.id
                        });
                    } else {
                        goToState('app.profile.user.topic', {
                            'id': encodedId.id,
                            'topic': topic
                        });
                    }
                })
                .catch(function() {
                    // Handle this here incase the encoded id was not valid
                    goToState('app.error_notfound');
                });
        }

        return {
            /**
             * Navigate to specific settings page
             */
            goSettingsPage: function(page) {
                goToState('app.settings.page', {
                    'page': page
                });
            },
            internalUrl: internalUrl,

            openLink: openLink,

            goToState: goToState,

            getAbsoluteUrl: getAbsoluteUrl,

            getHrefFromState: getHrefFromState,

            goOgdAchievements: goOgdAchievements,

            goUserProfile: goUserProfile,

            /**
             * Navigate to a user's profile wishlist
             */
            goUserWishlist: function(nucleusId) {
                if(String(nucleusId) === String(Origin.user.userPid())) {
                    //self profile
                    goToState('app.profile.topic', {
                        'topic': 'wishlist'
                    });
                } else {
                    // friend profile
                    goUserProfile(nucleusId, 'wishlist');
                }
            },

            goUserAchievementDetails: function(state, stateParams) {
                if (!angular.isUndefined(stateParams.id)) {
                    NucleusIdObfuscateFactory.encode(stateParams.id).then(function(encodedId){
                        stateParams.id = encodedId.id;
                        goToState(state, stateParams);
                    });
                }
            },

            openIGOSPA: function(location, sublocation) {
                if (location === 'PROFILE') {
                    NucleusIdObfuscateFactory.encode(sublocation)
                    .then(function (encodedId) {
                        Origin.client.oig.openIGOSPA(location, encodedId.id);
                    })
                    .catch(function(){
                        goToState('app.error_notfound');
                    });
                } else {
                    Origin.client.oig.openIGOSPA(location, sublocation);
                }
            },

            /**
             * Navigate to home page
             */
            goHome: function() {
                openLink('/'); //will redirect to store or my-home decided in origin-routing.js
            },


            /**
             * Open an external link/url
             * THIS SHOULD BE DEPRECATED
             */
            asyncOpenUrl: function(url) {
                if (Origin.client.isEmbeddedBrowser()) {
                    Origin.client.desktopServices.asyncOpenUrl(url);
                } else {
                    window.open(url);
                }
            },

            /**
             * Open an external link/url (openInNewTab affects browser only)
             */
            externalUrl: externalUrl,

            /**
             * Opens an external url in a popout window with a given width and height in browsers.
             * Defaults the client to open link in a new browser tab. @todo update when jssdk gives client option
             * to open links in a popout window.
             */
            externalUrlInPopupWindow: function (url, popoutWidth, popoutHeight) {
                var popoutSize = 'width=' + (popoutWidth || 'auto') + ', height=' + (popoutHeight  || 'auto'),
                    newWindow;

                if (Origin.client.isEmbeddedBrowser()) {
                    Origin.windows.externalUrl(url); // will (should) be updated to open qt/blink window from client
                } else {
                    newWindow = window.open(url, '', popoutSize);
                }

                return newWindow;
            },

            /**
             * Open an external link/url using EADP SSO
             */
            asyncOpenUrlWithEADPSSO: function(url) {
                if (Origin.client.isEmbeddedBrowser()) {
                    Origin.client.desktopServices.asyncOpenUrlWithEADPSSO(url);
                } else {
                    window.open(url);
                }
            },

            /**
             * check current state by state name
             */
            isState: function(state) {
                return $state.is(state);
            },

            /**
             * get the current state
             */
            getCurrentState: function() {
                return $state.current;
            }
        };

    }

    /**
     * @ngdoc service
     * @name origin-components.factories.NavigationFactory

     * @description
     *
     * NavigationFactory
     */
    angular.module('origin-components')
        .constant('OriginPassThroughQueryParams', orderPassThroughQueryParams(['cmsstage', 'env', 'config', 'enablesocial',
            'showcomponentnames', 'showmissingstrings', 'showmissingstringsfallback', 'usefixturefile', 'automation', 'oigcontext', 'tealiumenv', 'showlogging', 'hidestaticshell']))
        .factory('NavigationFactory', NavigationFactory);

}());
