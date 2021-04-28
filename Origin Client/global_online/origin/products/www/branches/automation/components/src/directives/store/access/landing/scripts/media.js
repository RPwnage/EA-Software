
/** 
 * @file store/access/landing/scripts/media.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-access-landing-media';
    var YOUTUBE_CONTAINER = '.origin-store-access-landing-media-player-location';

    function OriginStoreAccessLandingMediaCtrl($scope, UtilFactory, PriceInsertionFactory) {
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
    }

    function originStoreAccessLandingMedia(ComponentsConfigFactory, YoutubeFactory) {
        function originStoreAccessLandingMediaLink(scope, elem) {
            var youtubeElement = elem.find(YOUTUBE_CONTAINER);

            YoutubeFactory
                .loadYoutubePlayer(youtubeElement, scope.video, {controls: 1}, '100%', '100%');
        }
        return {
            restrict: 'E',
            scope: {
                video: '@',
                logo: '@',
                header: '@',
                description: '@',
                legal: '@',
                buttonText: '@',
                offerId: '@',
                subtitle: '@',
                image: '@'
            },
            controller: OriginStoreAccessLandingMediaCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/landing/views/media.html'),
            link: originStoreAccessLandingMediaLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessLandingMedia
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {Video} video the youtube id of the video
     * @param {ImageUrl} logo the logo above the header
     * @param {LocalizedTemplateString} header the main title
     * @param {LocalizedString} description the description paragraph
     * @param {LocalizedTemplateString} legal the legal text below the CTA
     * @param {LocalizedString} button-text The join now CTA text
     * @param {String} offer-id The offer id for the join now CTA
     * @param {LocalizedTemplateString} subtitle the subtitle text below the CTA above the legal text
     * @param {ImageUrl} image an image to replace the video area. video needs to be undefined.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-landing-media />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessLandingMedia', originStoreAccessLandingMedia);
}()); 
