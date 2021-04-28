/**
 * @file home/recommended/trialinprogress.js
 */
(function() {
    'use strict';

    function OriginHomeRecommendedActionTrialInProgressCtrl($scope, $controller, UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-home-recommended-action-trial-in-progress';

        $scope.showTrialViolator = true;

        /**
         * custom description setup used for trial RNA passed into the shared controller
         */
        function customDescriptionFunction() {
            $scope.description = UtilFactory.getLocalizedStr($scope.descriptionRaw, CONTEXT_NAMESPACE, 'description');
        }

        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and subtitle/description function (if needed)
        $controller('OriginHomeRecommendedActionCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customSubtitleAndDescriptionFunction: customDescriptionFunction
        });

    }

    function originHomeRecommendedActionTrialInProgress(ComponentsConfigFactory, ComponentsLogFactory, OcdHelperFactory, GamesDataFactory, GamesCatalogFactory, ViolatorObserverFactory, $compile) {

        function originHomeRecommendedActionTrialInProgressLink(scope, element) {
            /**
             * compiles the xml with the current directives scope
             * @param  {string} xml the raw uncompiled xml
             * @return {object}     returns an angular compiled html object
             */
            function compileXML(data) {
                return $compile(data.inlineMessages)(scope);
            }

            /**
             * adds the element to the dom
             */
            function addToDom(compiledTag) {
                var container = element.find('.origin-home-recommended-violator');
                container.html('');
                container.append(compiledTag);
                scope.$digest();

            }

            /**
             * logs error message
             * @param  {Error} error the error object
             */
            function handleError(error) {
                ComponentsLogFactory.error('[originHomeRecommendedActionTrialInProgress]', error);
            }

            /**
             * adds a trial violator to the template of the trial directive
             */
            function addTrialViolatorToDOM() {
                return ViolatorObserverFactory.getModel(scope.offerId, 'myhometrial', 1)
                    .then(compileXML)
                    .then(addToDom);
            }

            addTrialViolatorToDOM()
                .catch(handleError);
        }

        return {
            restrict: 'E',
            scope: {
                gamename: '@gamename',
                subtitleRaw: '@subtitle',
                descriptionRaw: '@description',
                priceOfferId: '@priceofferid',
                priceOcdPath: '@',
                offerId: '@offerid',
                ocdPath: '@',
                discoverTileImage: '@discovertileimage',
                discoverTileColor: '@discovertilecolor',
                sectionTitle: '@',
                sectionSubtitle: '@'
            },
            controller: 'OriginHomeRecommendedActionTrialInProgressCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html'),
            link: originHomeRecommendedActionTrialInProgressLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionTrialInProgress
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} gamename the name of the game, if not passed it will use the name from catalog
     * @param {LocalizedString} subtitle the subtitle string
     * @param {LocalizedString} description the description string
     * @param {string} offerid the offerid of the game you want to interact with
     * @param {OcdPath} ocd-path the ocd path of the game you want to interact with
     * @param {string} priceofferid the offerid of the game you want to show pricing for
     * @param {OcdPath} price-ocd-path the ocd path of the game you want to show pricing for
     * @param {ImageUrl} discover-tile-image tile image 1000x250
     * @param {string} discover-tile-color the background color
     * @param {LocalizedString} section-title the text to show in the area title
     * @param {LocalizedString} section-subtitle the text to show in the area subtitle
     *
     * @description
     *
     * trial recommended next action (in progress)
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-home-recommended-action-trial-in-progress offerid="OFB-EAST:109552444"></origin-home-recommended-action-trial-in-progress>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionTrialInProgressCtrl', OriginHomeRecommendedActionTrialInProgressCtrl)
        .directive('originHomeRecommendedActionTrialInProgress', originHomeRecommendedActionTrialInProgress);
}());