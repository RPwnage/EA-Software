/**
 * @file store/carousel/hero/scripts/banner.js
 */
(function () {
    'use strict';

    function OriginStoreCarouselHeroBannerCtrl($scope, $location) {
        // This seems like common banner logic.
        $scope.onBtnClick = function() {
            $location.path($scope.href);
        };

        $scope.type = 'std';
    }

    function originStoreCarouselHeroBanner(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            scope: {
            	title: '@',
            	subtitle: '@',
            	image: '@',
                href: '@',
                videoid: '@',
                restrictedage: '@',
                videoDescription: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/hero/views/banner.html'),
            controller: OriginStoreCarouselHeroBannerCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselHeroBanner
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title The title of the module
     * @param {LocalizedString} subtitle The subtitle of the module
     * @param {ImageUrl} image The banner image
     * @param {string} href Url to something
     * @param {string} videoid The youtube video ID of the video for this promo (Optional).
     * @param {Number} restrictedage The Restricted Age of the youtube video for this promo (Optional).
     * @param {LocalizedString} video-description The text for the video cta
     * Standard Store Hero Banner
     *
     */
    angular.module('origin-components')
    	.controller('OriginStoreCarouselHeroBannerCtrl', OriginStoreCarouselHeroBannerCtrl)
        .directive('originStoreCarouselHeroBanner', originStoreCarouselHeroBanner);
}());
