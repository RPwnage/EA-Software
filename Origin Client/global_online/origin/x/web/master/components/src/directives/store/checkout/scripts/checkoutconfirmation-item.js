/**
 * @file store/checkout/checkoutconfirmation-item.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-checkoutconfirmation-item';

    function originCheckoutconfirmationItemCtrl($scope, $attrs, DialogFactory, ObjectHelperFactory, ProductFactory, UtilFactory) {
        $scope.canDownloadNow = false;
        
        $scope.strings = {
            downloadWithOrigin: UtilFactory.getLocalizedStr($attrs.downloadWithOriginStr, CONTEXT_NAMESPACE, 'downloadWithOrigin'),
            havingTrouble: UtilFactory.getLocalizedStr($attrs.havingTroubleStr, CONTEXT_NAMESPACE, 'havingTrouble'),
            visitFromComputer: UtilFactory.getLocalizedStr($attrs.visitFromComputerStr, CONTEXT_NAMESPACE, 'visitFromComputer')
        };

        $scope.downloadWithOrigin = function() {
            DialogFactory.openDirective({
                contentDirective: 'origin-dialog-content-youneedorigin',
                id: 'need-origin-download',
                name: 'origin-dialog-content-youneedorigin',
                size: 'large',
                data: {
                    "offer-id": $scope.offerId
                }
            });
        };

        function setModel(data) {
            $scope.model = data;
            // TODO: reduce over-specificity -- this evaluation is checking "releaseDate" as current cataloguefactory
            // will add your current platform as mock data. This checks that it's legit platform data, not mock/test data
            $scope.canDownloadNow = !!ObjectHelperFactory.getProperty(['platforms', Origin.utils.os(), 'releaseDate'])(data);
            $scope.platforms = getValidPlatforms(data);
        }

        /**
         * Check if this is a real platform data or mocked by GamesDataFactory
         *
         * @param {Object} obj - gamedata platforms object
         * @return {Object|null}
         */
        function checkForReleaseDate(obj) {
            return !!obj.releaseDate;
        }

        /**
         * Provide shortlist of available platforms for this game
         *
         * @param {Object} game - game offer to parse
         * @return {Object|null}
         */
        function getValidPlatforms(game) {
            if (game.platforms) {
                return ObjectHelperFactory.map(checkForReleaseDate, game.platforms);
            }
            return null;
        }

        var removeWatcher = $scope.$watch('offerId', function (newValue) {
            if (newValue) {
                ProductFactory.get(newValue).attachToScope($scope, setModel);
                removeWatcher();
            }
        });
    }

    function originCheckoutconfirmationItem(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@'
            },
            controller: 'originCheckoutconfirmationItemCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/checkout/views/checkoutconfirmation-item.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCheckoutconfirmationItem
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString|OCD} download-with-origin-str "Download with Origin" button text
     * @param {LocalizedString|OCD} having-trouble-str "Having trouble with this download? Click Here." text
     * @param {LocalizedString|OCD} visit-from-computer-str "Visit your Game Library from your PC to start playing." text
     * @description
     *
     * Individual game item representing a just-acquired entitlement in checkout confirmation view
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-checkoutconfirmation-item offer-id="Origin.OFR.50.0000500">
     *         </origin-checkoutconfirmation-item>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('originCheckoutconfirmationItemCtrl', originCheckoutconfirmationItemCtrl)
        .directive('originCheckoutconfirmationItem', originCheckoutconfirmationItem);

}());