/**
 * @file common/navigation.js
 */
/*jshint strict: false */
/*jshint unused: false */

(function () {
    'use strict';

    function NavigationFactory($state, $location, DialogFactory, AppCommFactory, GamesDataFactory) {

        return {
            /**
            * Navigate to specific settings page
            */
            goSettingsPage: function (page) {
                $state.go('app.settings', { 'page': page });
            },

            /**
            * Go to the OGD page if the game is owned or if we own an entitlement with the same multiplayerId, otherwise the store PDP
            */
            goProductDetailsWithMultiplayerId: function (productId, multiplayerId) {
                if (GamesDataFactory.ownsEntitlement(productId)) {
                    // owned
                    $state.go('app.game_gamelibrary.ogd', { 'offerid': productId });
                }
                else {
                    // See if we own a game with the same multiplayerId
                    var ent = GamesDataFactory.getBaseGameEntitlementByMultiplayerId(multiplayerId);
                    if (!!ent) {
                        // We own something with the same multiplayerId - show that
                        var multiplayerProductId = ent.productId;
                        $state.go('app.game_gamelibrary.ogd', { 'offerid': multiplayerProductId });
                    }
                    else {
                        // not owned
                        $state.go('app.store.wrapper.skupdp', { 'offerid': productId });
                    }
                }
            },

            /**
            * Go to the OGD page if the game is owned, otherwise the store PDP
            */
            goProductDetails: function (productId) {
                if (GamesDataFactory.ownsEntitlement(productId)) {
                    // owned
                    $state.go('app.game_gamelibrary.ogd', { 'offerid': productId });
                }
                else {
                    // not owned
                    $state.go('app.store.wrapper.skupdp', { 'offerid': productId });
                }
            },

            /**
            * Navigate to a user's profile page
            */
            goUserProfile: function (nucleusId) {
               $state.go('app.profile.user', {'id' : nucleusId});
            },

            /**
            * Open an external link/url
            * THIS SHOULD BE DEPRECATED
            */
            asyncOpenUrl: function (url) {
                if (Origin.client.isEmbeddedBrowser()) {
                    Origin.client.desktopServices.asyncOpenUrl(url);
                }
                else {
                    window.open(url);
                }
            },

            /**
            * Open an external link/url (openInNewTab affects browser only)
            */
            externalUrl: function(url, openInNewTab)
            {
                if (Origin.client.isEmbeddedBrowser() || openInNewTab) {
                    Origin.windows.externalUrl(url);
                }
                else {
                    window.open(url);
                }
            },

            /**
            * Open an external link/url using EADP SSO
            */
            asyncOpenUrlWithEADPSSO: function (url) {
                if (Origin.client.isEmbeddedBrowser()) {
                    Origin.client.desktopServices.asyncOpenUrlWithEADPSSO(url);
                }
                else {
                    window.open(url);
                }
            },

            /**
            * Show the "Search for Friends" prompt
            */
            searchForFriends: function () {
                setTimeout(function () { // Need this or DialogFactory freaks out
                    DialogFactory.openDirective({
                        id: 'origin-dialog-content-searchforfriends-id',
                        name: 'origin-dialog-content-searchforfriends'
                    });
                }, 0);
            },

            /**
            * Show the "Report User" dialog
            */
            reportUser: function (userid, username, mastertitle) {
                setTimeout(function () { // Need this or DialogFactory freaks out
                    DialogFactory.openDirective({
                        id: 'origin-dialog-content-reportuser-id',
                        name: 'origin-dialog-content-reportuser',
                        size: 'large',
                        data: {
                            userid: userid,
                            username: username,
                            mastertitle: mastertitle
                        }
                    });
                }, 0);
            },

            /**
            * Opens a given link in a new tab if given an absolute url, otherwise opens in same tab.
            * @param {String} url
            * @method openLink
            */
            openLink: function (url, params) {
                params = params || {};
                AppCommFactory.events.fire('uiRouter:go', url, params);
            }

        };

    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function NavigationFactorySingleton($state, $location, DialogFactory, AppCommFactory, GamesDataFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('NavigationFactory', NavigationFactory, this, arguments);
    }

    /**
    * @ngdoc service
    * @name origin-components.factories.NavigationFactory

    * @description
    *
    * NavigationFactory
    */
    angular.module('origin-components')
        .factory('NavigationFactory', NavigationFactorySingleton);

} ());
