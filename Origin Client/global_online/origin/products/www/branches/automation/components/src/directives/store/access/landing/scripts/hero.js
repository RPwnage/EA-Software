
/** 
 * @file store/access/landing/scripts/hero.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-access-landing-hero';
    var ROTATION_DEFAULT_TIME = '5000';

    function OriginStoreAccessLandingHeroCtrl($scope, $interval, PriceInsertionFactory, UtilFactory) {

        var carouselRotation;
        $scope.active = 0;
        $scope.images = [];
        $scope.carouselRunning = false;
        $scope.speed = $scope.speed || ROTATION_DEFAULT_TIME;
        $scope.strings = {
            description: UtilFactory.getLocalizedStr($scope.description, CONTEXT_NAMESPACE, 'description'),
            buttonText: UtilFactory.getLocalizedStr($scope.buttonText, CONTEXT_NAMESPACE, 'button-text')
        };

        PriceInsertionFactory
            .insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.header, CONTEXT_NAMESPACE, 'header');

        PriceInsertionFactory
            .insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.legal, CONTEXT_NAMESPACE, 'legal');

        PriceInsertionFactory
            .insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.subtitle, CONTEXT_NAMESPACE, 'subtitle');

        this.registerBackground = function(images) {
            $scope.images.push(images);

            watchCarousel();
        };

        function startCarousel() {
            if($scope.active === _.size($scope.images) - 1) {
                $scope.active = 0;
            } else {
                $scope.active++;
            }
        }

        function watchCarousel() {
            if(_.size($scope.images) > 1 && !$scope.carouselRunning) {
                $scope.carouselRunning = true;
                carouselRotation = $interval(startCarousel, $scope.speed);
            }
        }

        function onDestroy() {
            $interval.cancel(carouselRotation);
        }

        $scope.$on('$destroy', onDestroy);
    }

    function originStoreAccessLandingHero(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                logo: '@',
                header: '@',
                description: '@',
                legal: '@',
                buttonText: '@',
                offerId: '@',
                speed: '@',
                subtitle: '@'

            },
            controller: 'OriginStoreAccessLandingHeroCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/landing/views/hero.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessLandingHero
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {ImageUrl} logo the logo above the header
     * @param {LocalizedTemplateString} header the main title
     * @param {LocalizedString} description the description paragraph
     * @param {LocalizedTemplateString} legal the legal text below the CTA
     * @param {LocalizedString} button-text The join now CTA text
     * @param {String} offer-id The offer id for the join now CTA
     * @param {String} speed the speed of the carousel rotation in milliseconds (default 5000)
     * @param {LocalizedTemplateString} subtitle the subtitle text below the CTA above the legal text
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-landing-hero />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreAccessLandingHeroCtrl', OriginStoreAccessLandingHeroCtrl)
        .directive('originStoreAccessLandingHero', originStoreAccessLandingHero);
}()); 
