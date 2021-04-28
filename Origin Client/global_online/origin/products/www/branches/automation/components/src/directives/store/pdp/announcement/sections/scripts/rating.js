/**
 * @file /store/pdp/announcement/scripts/rating.js
 */
(function() {
    'use strict';

    function originStoreAnnouncementGameRating(ComponentsConfigFactory) {

        function originStoreAnnouncementGameRatingLink(scope) {

            scope.model = {
                gameRatingUrl : scope.url,
                gameRatingSystemIcon: scope.systemIcon,
                gameRatingTypeValue: scope.systemIconAlt,
                gameRatingDesc: scope.descriptionShort ? scope.descriptionShort.split('|') : [],
                gameRatingDescriptionLong: scope.descriptionLong
            };
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                url: '@',
                systemIcon: '@',
                systemIconAlt: '@',
                descriptionShort: '@',
                descriptionLong: '@'
            },
            link: originStoreAnnouncementGameRatingLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/game/views/rating.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAnnouncementGameRating
     * @restrict E
     * @element ANY
     * @scope
     * @param {Url} url Game rating url (ex: http://www.esrb.com),
     * @param {Url} system-icon Game Rating System Url,
     * @param {LocalizedString} system-icon-alt icon image alternate text
     * @param {LocalizedString} description-short short description (| seperated list of descriptions)
     * @param {LocalizedString} description-long long description
     * @description
     *
     * Product details page call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-announcement-game-rating></origin-store-announcement-game-rating>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAnnouncementGameRating', originStoreAnnouncementGameRating);
}());
