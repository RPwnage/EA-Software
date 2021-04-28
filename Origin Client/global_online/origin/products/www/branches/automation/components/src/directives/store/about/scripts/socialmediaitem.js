/**
 * @file store/about/scripts/socialmediaitem.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-about-socialmedia-item';

    var SocialMediaTypeEnumeration = {
        "twitter": "twitter",
        "facebook": "facebook",
        "youtube": "youtube",
        "insider": "insider"
    };

    /* Directive Controller */
    function OriginStoreAboutSocialmediaItemCtrl($scope, UtilFactory){
        $scope.title = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, SocialMediaTypeEnumeration[$scope.socialMediaType]);
    }

    /* Directive Declaration */
    function OriginStoreAboutSocialmediaItem(ComponentsConfigFactory){
        return {
            restrict: 'E',
            scope: {
                socialMediaType: '@',
                url: '@'
            },
            replace: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/socialmediaitem.html'),
            controller: 'OriginStoreAboutSocialmediaItemCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutSocialmediaItem
     * @restrict E
     * @element ANY
     * @scope
     * @param {SocialMediaTypeEnumeration} social-media-type  - Type of social media platform
     * @param {Url} url - The URL to the social media page
     *
     * @param {LocalizedString} twitter twitter text. Set up once as default.
     * @param {LocalizedString} facebook facebook text. Set up once as default.
     * @param {LocalizedString} youtube youtube text. Set up once as default.
     * @param {LocalizedString} insider insider text. Set up once as default.
     *
     * @description
     *
     * Store about page social media item
     *
     * @example
     * <origin-store-about-socialmedia>
     *     <origin-store-about-socialmedia-item social-media-type="twitter"
     *                                           url="https://twitter.com/OriginInsider">
     *     </origin-store-about-socialmedia-item>
     * </origin-store-about-socialmedia>
     */
    angular.module('origin-components')
        .controller('OriginStoreAboutSocialmediaItemCtrl', OriginStoreAboutSocialmediaItemCtrl)
        .directive('originStoreAboutSocialmediaItem', OriginStoreAboutSocialmediaItem);
}());
