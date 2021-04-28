
/** 
 * @file store/access/trials/scripts/hero.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-access-trials-hero';
    var YOUTUBE_CONTAINER = '.origin-store-access-trials-video-player-location';

    /* jshint ignore:start */
    var ShadeEnumeration = {
        "light": "light",
        "dark": "dark"
    };
    /* jshint ignore:end */

    function OriginStoreAccessTrialsHeroCtrl($scope, UtilFactory, PriceInsertionFactory) {
        $scope.strings = {
            button: UtilFactory.getLocalizedStr($scope.button, CONTEXT_NAMESPACE, 'button'),
            header: UtilFactory.getLocalizedStr($scope.header, CONTEXT_NAMESPACE, 'header'),
            platform: UtilFactory.getLocalizedStr($scope.platform, CONTEXT_NAMESPACE, 'platform')
        };

        PriceInsertionFactory
            .insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.legal, CONTEXT_NAMESPACE, 'legal');
    }

    function originStoreAccessTrialsHero(ComponentsConfigFactory, YoutubeFactory) {
        function originStoreAccessTrialsHeroLink(scope, elem) {
            var youtubeElement = elem.find(YOUTUBE_CONTAINER);

            YoutubeFactory
                .loadYoutubePlayer(youtubeElement, scope.video, {controls: 1}, '100%', '100%');
        }

        return {
            restrict: 'E',
            transclude: true,
            scope: {
                background: '@',
                foreground: '@',
                logo: '@',
                header: '@',
                button: '@',
                offerId: '@',
                legal: '@',
                platform: '@',
                shade: '@',
                video: '@'
            },
            controller: OriginStoreAccessTrialsHeroCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/trials/views/hero.html'),
            link: originStoreAccessTrialsHeroLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessTrialsHero
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {ImageUrl} background the background image
     * @param {ImageUrl} foreground the foreground image
     * @param {ImageUrl} logo the logo above the header
     * @param {LocalizedString} header the header content
     * @param {LocalizedString} button the button text
     * @param {String} offer-id The offer id for the join now CTA
     * @param {LocalizedString} platform The platform text
     * @param {LocalizedTemplateString} legal the paragraph content
     * @param {Video} video the youtube id of the video
     * @param {ShadeEnumeration} shade The shade of the text
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-trials-hero />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessTrialsHero', originStoreAccessTrialsHero);
}()); 
