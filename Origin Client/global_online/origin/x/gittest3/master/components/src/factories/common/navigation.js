/**
 * @file common/navigation.js
 */
/*jshint strict: false */
/*jshint unused: false */
/*jshint undef: false */
(function () {
    'use strict';

    function NavigationFactory() {

        return {
            /**
            * Navigate to the Settings page
            */
            goSettings: function () {
                window.location = '#/settings';
            },

            /**
            * Navigate to a product details page
            */
            goProductDetails: function (productId) {
                window.location = '#/store?productId=' + productId;
            },

            /**
            * Navigate to a user's profile page
            */
            goUserProfile: function (nucleusId) {
                window.location = '#/profile?nucleusId=' + nucleusId;
            },

            /**
            * Open an external link/url
            */
            asyncOpenUrl: function (url) {
                if (Origin.bridgeAvailable()) {
                    DesktopServices.asyncOpenUrl(url);
                }
                else {
                    window.open(url);
                }
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
        .factory('NavigationFactory', NavigationFactory);

} ());