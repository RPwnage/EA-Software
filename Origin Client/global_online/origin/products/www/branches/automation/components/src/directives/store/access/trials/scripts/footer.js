
/** 
 * @file store/access/trials/scripts/footer.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-access-trials-footer';
    var YOUTUBE_CONTAINER = '.origin-store-access-trials-footer-player-location';

    function OriginStoreAccessTrialsFooterCtrl($scope, UtilFactory, PriceInsertionFactory) {
        $scope.strings = {
            button: UtilFactory.getLocalizedStr($scope.button, CONTEXT_NAMESPACE, 'button'),
            header: UtilFactory.getLocalizedStr($scope.header, CONTEXT_NAMESPACE, 'header'),
            platform: UtilFactory.getLocalizedStr($scope.platform, CONTEXT_NAMESPACE, 'platform'),
            link: UtilFactory.getLocalizedStr($scope.link, CONTEXT_NAMESPACE, 'link')
        };

        PriceInsertionFactory
            .insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.description, CONTEXT_NAMESPACE, 'description');
    }

    function originStoreAccessTrialsFooter(ComponentsConfigFactory, YoutubeFactory) {
        function originStoreAccessTrialsFooterLink(scope, elem) {
            var youtubeElement = elem.find(YOUTUBE_CONTAINER);

            YoutubeFactory
                .loadYoutubePlayer(youtubeElement, scope.video, {controls: 1}, '100%', '100%');
        }
        return {
            restrict: 'E',
            transclude: true, 
            scope: {
                logo: '@',
                header: '@',
                description: '@',
                platform: '@',
                link: '@',
                button: '@',
                offerId: '@',
                video: '@'
            },
            controller: OriginStoreAccessTrialsFooterCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/trials/views/footer.html'),
            link: originStoreAccessTrialsFooterLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessTrialsFooter
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {ImageUrl} logo the logo above the header
     * @param {LocalizedString} header the header content
     * @param {LocalizedTemplateString} description the description content
     * @param {LocalizedString} link the link content
     * @param {LocalizedString} button the button text
     * @param {String} offer-id The offer id for the join now CTA
     * @param {LocalizedString} platform The platform text
     * @param {Video} video the youtube id of the video
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-trials-footer />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessTrialsFooter', originStoreAccessTrialsFooter);
}()); 
