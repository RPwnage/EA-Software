/**
 *  * @file factories/store/sitestripe.js
 */
(function () {
    'use strict';

    var LOCAL_STORAGE_KEY = 'originsitestripe';
    
    var FontColorEnumeration = {
        'light': '#FFFFFF',
        'dark': '#1E262C'
    };

    function SiteStripeFactory(LocalStorageFactory, TemplateResolver, UtilFactory, NavigationFactory, AppCommFactory) {
        var URL_PREFIX = '/' + Origin.locale.threeLetterCountryCode().toLowerCase() + '/' + Origin.locale.locale().replace('_', '-').toLowerCase();

        // Check localStorage key and hide site-stripe if set
        function getSiteStripeArray() {
            var sitestripeKeys = LocalStorageFactory.get(LOCAL_STORAGE_KEY);
            return sitestripeKeys && _.isArray(sitestripeKeys) ? sitestripeKeys : [];
        }

        // check if this sitestripe has been closed by the user
        function isSitestripeClosed(scope) {
            var sitestripeKeys = getSiteStripeArray();
            return sitestripeKeys.indexOf(scope.uniqueSitestripeId) > -1;
        }

        // build current path, without storefront, locale and parameters to match
        // paths entered into CQ5 for targeting
        function buildPathnameWithoutLocale() {
            var path = NavigationFactory.getHrefFromState(NavigationFactory.getCurrentState()).replace(URL_PREFIX, '');

            return path.slice(0, path.indexOf('?'));
        }

        function splitPaths(pathsStr) {
            return _.compact(pathsStr.split(','));
        }

        // check if the current route matches a list of paths
        function isMatchingPath(paths) {
            var currentPath = buildPathnameWithoutLocale();

            return !_.isUndefined(_.find(paths, function(path) {
                return path.trim() === currentPath;
            }));
        }

        // given a list of paths to show a sitestripe on, determin if the current route 
        // is a match and hide or show the sitestripe accordingly
        function isTargetedToShowOnCurrentPage(scope) {
            var match = false,
                paths = splitPaths(_.get(scope, 'showOnPage', ''));

            if(_.size(paths) > 0) {
                if(isMatchingPath(paths)) {
                    match = true;
                }
            } else {
                match = true;
            }

            return match;
        }

        // given a list of paths to hide a sitestripe on, determin if the current route 
        // is a match and hide or show the sitestripe accordingly
        function isNotTargetedToHideOnCurrentPage(scope) {
            var match = true,
                paths = splitPaths(_.get(scope, 'hideOnPage', ''));

            if(_.size(paths) > 0) {
                if(isMatchingPath(paths)) {
                    match = false;
                }
            }

            return match;
        }

        function isSiteStripeVisible(scope) {
            scope.siteStripeVisible = !isSitestripeClosed(scope) && isTargetedToShowOnCurrentPage(scope) && isNotTargetedToHideOnCurrentPage(scope);
            
            if (scope.siteStripeVisible) {
                Origin.telemetry.events.fire('originStoreSitestripeVisible');
            }
        }

        /**
         * Hide the site-stripe and update localStorage key to record the "dismissed" state
         * @return {void}
         */
        function dismissSiteStripe(localStorageKey) {
            if (localStorageKey) {
                var sitestripeKeys = getSiteStripeArray();
                if (sitestripeKeys) {
                    if (sitestripeKeys.indexOf(localStorageKey) < 0) {
                        sitestripeKeys.push(localStorageKey);
                        LocalStorageFactory.set(LOCAL_STORAGE_KEY, sitestripeKeys);
                    }
                } else {
                    LocalStorageFactory.set(LOCAL_STORAGE_KEY, [localStorageKey]);
                }
            }
        }

        /**
         * Show all sitestripes
         * @return {undefined}
         */
        function showAllSiteStripes() {
            LocalStorageFactory.set(LOCAL_STORAGE_KEY, []);
        }

        /**
         * retrieves the sitestripe template from cq5
         * @param {Boolean} force by default a template is cached. To force a fresh request, pass true
         * @return {object} a promise that resolves as the site stripe template string.
         */
        function retrieveTemplate(force) {
            return TemplateResolver.getTemplate('sitestripe', {}, force);
        }

        function siteStripeLinkFn(contextNamespace) {
            return function(scope) {
                var setSiteStripeEvent;
                var href = UtilFactory.getLocalizedStr(scope.href, contextNamespace, 'href');
                isSiteStripeVisible(scope);
                scope.dismissSiteStripe = function () {
                    dismissSiteStripe(scope.uniqueSitestripeId);
                    scope.siteStripeVisible = false;
                };

                scope.strings = {
                    href: href,
                    hrefAbsoluteUrl: NavigationFactory.getAbsoluteUrl(href)
                };

                scope.fontcolor = scope.fontcolor || FontColorEnumeration.light;
                
                // set an event to watch page transitions if site stripe should be hidden on any store pages
                if(!_.isUndefined(scope.showOnPage) || !_.isUndefined(scope.hideOnPage)) {
                    setSiteStripeEvent = _.partial(isSiteStripeVisible, scope);
                    AppCommFactory.events.on('uiRouter:stateChangeSuccess', setSiteStripeEvent);
                    scope.$on('siteStripeWrapper:cleanUp', function () {
                        AppCommFactory.events.off('uiRouter:stateChangeSuccess', setSiteStripeEvent);
                    });
                }
            };
        }

        return {
            siteStripeLinkFn: siteStripeLinkFn,
            retrieveTemplate: retrieveTemplate,
            showAllSiteStripes: showAllSiteStripes
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function SiteStripeFactorySingleton(LocalStorageFactory, TemplateResolver, UtilFactory, NavigationFactory, AppCommFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('SiteStripeFactory', SiteStripeFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.SiteStripeFactory
     * @description
     *
     * SiteStripeFactory
     */
    angular.module('origin-components')
        .factory('SiteStripeFactory', SiteStripeFactorySingleton);
}());
