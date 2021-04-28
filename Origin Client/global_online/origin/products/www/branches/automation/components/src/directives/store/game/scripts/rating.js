/**
 * @file /store/game/scripts/rating.js
 */
(function() {
    'use strict';

    function OriginStoreGameRatingCtrl($scope) {

        function setRatingIcons() {
            $scope.ratingReasonIcons = _.map($scope.model.gameRatingDesc, function(val) {
                 return {
                     description: val,
                     className: _.kebabCase(val)
                 };
            });
        }

        function updateModel() {
            // PEGI-rated games have official icons to denote each different rating reason.
            if ($scope.model.gameRatingType === 'PEGI') {
                setRatingIcons();
            }
        }

        $scope.$watchOnce('model', updateModel);
    }
    function originStoreGameRating(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            replace: true,
            controller: 'OriginStoreGameRatingCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/game/views/rating.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreGameRating
     * @restrict E
     * @element ANY
     *
     * @param {LocalizedString} esrb-early-childhood Early Childhood
     * @param {LocalizedString} esrb-everyone Everyone
     * @param {LocalizedString} esrb-everyone-10 Everyone 10+
     * @param {LocalizedString} esrb-teen Teen
     * @param {LocalizedString} esrb-mature Mature
     * @param {LocalizedString} esrb-adults-only Adults only
     * @param {LocalizedString} esrb-rating-pending Rating Pending
     * @param {LocalizedString} esrb-alcohol-reference Alcohol Reference
     * @param {LocalizedString} esrb-alcohol-and-tobacco-reference Alcohol and Tobacco Reference
     * @param {LocalizedString} esrb-animated-blood Animated Blood
     * @param {LocalizedString} esrb-animated-blood-and-gore Animated Blood and Gore
     * @param {LocalizedString} esrb-animated-violence Animated Violence
     * @param {LocalizedString} esrb-blood Blood
     * @param {LocalizedString} esrb-blood-and-gore Blood and Gore
     * @param {LocalizedString} esrb-cartoon-violence Cartoon Violence
     * @param {LocalizedString} esrb-comic-mischief Comic Mischief
     * @param {LocalizedString} esrb-crude-humor Crude Humor
     * @param {LocalizedString} esrb-drug-reference Drug Reference
     * @param {LocalizedString} esrb-fantasy-violence Fantasy Violence
     * @param {LocalizedString} esrb-intense-violence Intense Violence
     * @param {LocalizedString} esrb-language Language
     * @param {LocalizedString} esrb-lyrics Lyrics
     * @param {LocalizedString} esrb-mature-humor Mature Humor
     * @param {LocalizedString} esrb-mild-alcohol-reference Mild Alcohol Reference
     * @param {LocalizedString} esrb-mild-animated-violence Mild Animated Violence
     * @param {LocalizedString} esrb-mild-blood Mild Blood
     * @param {LocalizedString} esrb-mild-cartoon-violence Mild Cartoon Violence
     * @param {LocalizedString} esrb-mild-fantasy-violence Mild Fantasy Violence
     * @param {LocalizedString} esrb-mild-language Mild Language
     * @param {LocalizedString} esrb-mild-lyrics Mild Lyrics
     * @param {LocalizedString} esrb-mild-sexual-themes Mild Sexual Themes
     * @param {LocalizedString} esrb-mild-suggestive-themes Mild Suggestive Themes
     * @param {LocalizedString} esrb-mild-violence Mild Violence
     * @param {LocalizedString} esrb-mild-violent-references Mild Violent References
     * @param {LocalizedString} esrb-nudity Nudity
     * @param {LocalizedString} esrb-partial-nudity Partial Nudity
     * @param {LocalizedString} esrb-real-gambling Real Gambling
     * @param {LocalizedString} esrb-realistic-violence Realistic Violence
     * @param {LocalizedString} esrb-sexual-content Sexual Content
     * @param {LocalizedString} esrb-sexual-themes Sexual Themes
     * @param {LocalizedString} esrb-sexual-violence Sexual Violence
     * @param {LocalizedString} esrb-simulated-gambling Simulated Gambling
     * @param {LocalizedString} esrb-strong-language Strong Language
     * @param {LocalizedString} esrb-strong-lyrics Strong Lyrics
     * @param {LocalizedString} esrb-strong-sexual-content Strong Sexual Content
     * @param {LocalizedString} esrb-suggestive-themes Suggestive Themes
     * @param {LocalizedString} esrb-tobacco-reference Tobacco Reference
     * @param {LocalizedString} esrb-use-of-alcohol Use of Alcohol
     * @param {LocalizedString} esrb-use-of-alcohol-and-tobacco Use of Alcohol and Tobacco
     * @param {LocalizedString} esrb-use-of-drugs Use of Drugs
     * @param {LocalizedString} esrb-use-of-tobacco Use of Tobacco
     * @param {LocalizedString} esrb-violence Violence
     * @param {LocalizedString} esrb-violent-references Violent References
     * @param {LocalizedString} esrb-downloadable-content-rated Downloadable Content Rated
     * @param {LocalizedString} esrb-includes-a-bonus-game-rated Includes a bonus game rated
     * @param {LocalizedString} esrb-may-contain-content-inappropriate-for-children-visit-www-esrb-org-for-rating-information May contain content inappropriate for children. Visit www.esrb.org for rating information.
     * @param {LocalizedString} esrb-online-interactions-not-rated Online Interactions Not Rated
     * @param {LocalizedString} esrb-online-interactions-not-rated-by-the-esrb Online Interactions Not Rated by the ESRB
     * @param {LocalizedString} esrb-online-music-not-rated Online Music Not Rated
     * @param {LocalizedString} esrb-online-music-not-rated-by-the-esrb Online Music Not Rated by the ESRB
     * @param {LocalizedString} esrb-online-music-and-interactions-not-rated Online Music and Interactions Not Rated
     * @param {LocalizedString} esrb-online-music-and-interactions-not-rated-by-the-esrb Online Music and Interactions Not Rated by the ESRB
     * @param {LocalizedString} esrb-rating-pending-title-s-may-contain-content-inappropriate-for-children Rating Pending title(s) may contain content inappropriate for children
     * @param {LocalizedString} esrb-search-for-more-detailed-rating-summaries-at-www-esrb-org Search for more detailed rating summaries at www.esrb.org
     * @param {LocalizedString} esrb-visit-www-esrb-org-for-rating-information Visit www.esrb.org for rating information.
     *
     * @description
     *
     * Product details page call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-game-rating></origin-store-game-rating>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreGameRatingCtrl', OriginStoreGameRatingCtrl)
        .directive('originStoreGameRating', originStoreGameRating);
}());
