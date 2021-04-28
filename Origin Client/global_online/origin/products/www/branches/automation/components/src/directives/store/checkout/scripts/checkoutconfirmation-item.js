/**
 * @file store/checkout/checkoutconfirmation-item.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-checkoutconfirmation-item';

    function originCheckoutconfirmationItemCtrl(
        $scope,
        $attrs,
        DialogFactory,
        ObjectHelperFactory,
        ProductFactory,
        UtilFactory,
        NavigationFactory,
        CheckoutFactory
    ) {
        $scope.canDownloadNow = false;
        $scope.isEmbeddedBrowser = Origin.client.isEmbeddedBrowser();
        $scope.strings = {
            downloadWithOrigin: UtilFactory.getLocalizedStr($attrs.downloadWithOriginStr, CONTEXT_NAMESPACE, 'download-with-origin-str'),
            havingTrouble: UtilFactory.getLocalizedStr($attrs.havingTroubleStr, CONTEXT_NAMESPACE, 'having-trouble-str'),
            havingTroubleUrl: UtilFactory.getLocalizedStr($attrs.havingTroubleUrlStr, CONTEXT_NAMESPACE, 'having-trouble-url-str'),
            visitFromComputer: UtilFactory.getLocalizedStr($attrs.visitFromComputerStr, CONTEXT_NAMESPACE, 'visit-from-computer-str')
        };

        $scope.platformLoc = UtilFactory.getLocalizedStr($scope.platformStr, CONTEXT_NAMESPACE, 'platform-str');

        $scope.downloadWithOrigin = function() {
            DialogFactory.close($scope.modalId);
            CheckoutFactory.downloadGame($scope.offerId);
        };

        /**
         * Check if this is a real platform data or mocked by GamesDataFactory
         *
         * @param {Object} obj - gamedata platforms object
         * @return {Object|null}
         */
        function checkPlatform(obj) {
            if ($scope.isVault && obj.platform === 'MAC') {
                return false; //Special case - don't show mac as playable platform if vault title
            }
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
                return ObjectHelperFactory.map(checkPlatform, game.platforms);
            }
            return null;
        }

        function setModel(data) {
            $scope.model = data;
            if (Origin.utils.os() === 'MAC' && $scope.isVault) {
                //Special case - Vault not available on mac
                $scope.canDownloadNow = false;
            } else {
                $scope.canDownloadNow = data.downloadable;
            }
            $scope.platforms = getValidPlatforms(data);
        }

        var removeWatcher = $scope.$watch('offerId', function (newValue) {
            if (newValue) {
                ProductFactory.get(newValue).attachToScope($scope, setModel);
                removeWatcher();
            }
        });
    }

    function originCheckoutconfirmationItem(ComponentsConfigFactory, ComponentsConfigHelper, LocFactory) {
        function originCheckoutconfirmationItemLink(scope) {
            var helpUrl = ComponentsConfigHelper.getHelpUrl(Origin.locale.locale(), Origin.locale.languageCode());
            var helpLink = '<a href="' + helpUrl + '" class="otkc otkc-small external-link-sso">' + scope.strings.havingTroubleUrl + '</a>';
            scope.strings.havingTrouble = LocFactory.substitute(scope.strings.havingTrouble, { '%helpurl%': helpLink });
        }
        return {
            restrict: 'E',
            scope: {
                offerId: '@',
                modalId:'@',
                isVault:'@',
                platformStr: '@'
            },
            controller: 'originCheckoutconfirmationItemCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/checkout/views/checkoutconfirmation-item.html'),
            link: originCheckoutconfirmationItemLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCheckoutconfirmationItem
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString|OCD} download-with-origin-str "Download with Origin" button text
     * @param {LocalizedString|OCD} having-trouble-str "Having trouble with this download? %helpurl%" text
     * @param {LocalizedString|OCD} having-trouble-url-str "Click Here." text
     * @param {LocalizedString|OCD} visit-from-computer-str "Visit your Game Library from your PC to start playing." text
     * @param {LocalizedString} platform-str "Platform" text
     * @description
     *
     * Individual game item representing a just-acquired entitlement in checkout confirmation view
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-checkoutconfirmation-item offer-id="Origin.OFR.50.0000500" 
     *                  platform-str="Platform" >
     *         </origin-checkoutconfirmation-item>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('originCheckoutconfirmationItemCtrl', originCheckoutconfirmationItemCtrl)
        .directive('originCheckoutconfirmationItem', originCheckoutconfirmationItem);

}());