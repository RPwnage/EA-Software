/**
 * @file controllers/social.js
 */
(function() {
    'use strict';

    function SocialCtrl($scope, ComponentsConfigHelper, AuthFactory) {
        function setSocialFeaturesEnable() {
            $scope.enableSocialFeatures = ComponentsConfigHelper.enableSocialFeatures();
        }
        //wait till we've retrieved login state then set the social features, otherwise
        //the check for underage will not be accurate
        AuthFactory.waitForAuthReady()
            .then(setSocialFeaturesEnable);
    }

    /**
     * @ngdoc controller
     * @name originApp.controllers:SocialCtrl
     * @description
     *
     * determines if social features should be enabled or not
     *
     */
    angular.module('originApp')
        .controller('SocialCtrl', SocialCtrl);

}());
