
/** 
 * @file store/access/landing/scripts/trialtile.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-access-landing-trialtile';

    function OriginStoreAccessLandingTrialtileCtrl($scope, GamesDataFactory, NavigationFactory, PdpUrlFactory, UtilFactory, OfferTransformer) {

        function applyModel(catalogData) {
            $scope.model = _.first(_.values(catalogData));

            var time = _.get($scope, ['model', 'trialLaunchDuration'], 0),
                date =  moment(_.get($scope, ['model', 'releaseDate'])).format('LL');

            $scope.strings = {
                header: UtilFactory.getLocalizedStr($scope.header, CONTEXT_NAMESPACE, 'header'),
                description: UtilFactory.getLocalizedStr($scope.description, CONTEXT_NAMESPACE, 'description', {'%date%': date, '%time%': time})
            };

            $scope.onClick = function() {
                if(!_.isUndefined($scope.link)) {
                    NavigationFactory.openLink($scope.link);
                } else if(!_.isUndefined($scope.model)) {
                    PdpUrlFactory.goToPdp($scope.model);
                }
            };

            $scope.$evalAsync();
        }

        GamesDataFactory
            .getCatalogByPath($scope.ocdPath)
            .then(OfferTransformer.transformOffer)
            .then(applyModel);
    }

    function originStoreAccessLandingTrialtile(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                ocdPath: '@',
                image: '@',
                header: '@',
                description: '@',
                link: '@'
            },
            controller: OriginStoreAccessLandingTrialtileCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/landing/views/trialtile.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessLandingTrialtile
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {ImageUrl} image the image above the content
     * @param {OcdPath} ocd-path the offer path
     * @param {LocalizedString} header Override fro the game title
     * @param {LocalizedString} description Override for the games short description. Available variables: %date% (release date), %time%: trial time in hours
     * @param {Url} link Override for the pdp url
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-landing-trialtile />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessLandingTrialtile', originStoreAccessLandingTrialtile);
}()); 
