/**
 * @file dialog/content/scripts/bundledlcpurchaseprevention.js
 */
(function(){
    'use strict';
    var CONTEXT_NAMESPACE = 'origin-dialog-content-bundledlcpurchaseprevention';

    var BTN_CANCEL = "btncancel";
    var TITLE = "title";
    var DESCRIPTION = "description";

    function originDialogContentBundledlcpurchasepreventionCtrl(
        $scope,
        DialogFactory,
        StoreBundleFactory,
        PurchasePreventionFactory,
        OwnershipFactory,
        ObjectHelperFactory,
        GamesDataFactory,
        UtilFactory,
        PdpUrlFactory
    ) {
        /**
         * Close the dialog and set the response values
         * @param  {boolean} success If the user selected true or false
         */
        $scope.closeResponse = function() {
            DialogFactory.close($scope.dialogid);
        };
        /**
         * A data struct for the required games and the dlc that is applied to it
         *
         * format: [
         *         {
         *             baseGame: {Offer Data Object}
         *             dlc: [
         *                 {Offer Data Object},
         *                 {Offer Data Object}
         *             ]
         *         },
         *         {
         *             baseGame: {Offer Data Object}
         *             dlc: [
         *                 {Offer Data Object},
         *                 {Offer Data Object}
         *             ]
         *         },
         *     ]
         * @type {Array}
         */
        $scope.requiredBaseGameandDLC = [];

        /**
         * Builds the requiredBaseGameandDLC data structure and attaches it to the scope.
         * @param  {object} requiredGames Data structure of offerId for the base game and dlc
         * eg: {
         *  'offerId': [
         *      {DLC Offer Data Object},
         *      {DLC Offer Data Object},
         *      {DLC Offer Data Object}
         *  ],
         *  'offerId': [
         *      {DLC Offer Data Object},
         *      {DLC Offer Data Object},
         *      {DLC Offer Data Object}
         *  ]
         * }
         */
        function requiredGamesToProducts(requiredGames) {
            var myOfferIds = _.keys(requiredGames);
            GamesDataFactory.getCatalogInfo(myOfferIds)
            .then(function(result) {
                _.forEach(result, function(baseOffer) {
                    $scope.requiredBaseGameandDLC.push({
                        'baseGame': baseOffer,
                        'dlc': requiredGames[baseOffer.offerId]
                    });
                });

                if($scope.requiredBaseGameandDLC.length) {
                    buildStrings();
                    $scope.bindClick();
                } else {
                    DialogFactory.close($scope.dialogid);
                }

            });
        }

        /**
         * Builds a link with text and href
         * @param  {string} text The text to show in the link
         * @param  {string} href The location the link should go to
         * @return {html}        HTML that can be injected into the DOM
         */
        function buildAnchorMarkup(text, href) {
            return '<a class="otka baseGame" href="' +  href + '">' + text + '</a>';
        }

        /**
         * Initailize the modal so it show up correctly.  Fetches entitlement data and builds the required data structures.
         */
        function setupModal() {
            var bundle = StoreBundleFactory.getRawBundle();
            OwnershipFactory.getGameDataWithEntitlements(bundle)
            .then(ObjectHelperFactory.toArray)
            .then(function(productObjs){
                var productWithoutBaseGames = PurchasePreventionFactory.getProductsWithBaseGameNotInCart(productObjs);

                PurchasePreventionFactory.buildRequiredBaseGames(productWithoutBaseGames)
                .then(requiredGamesToProducts);
            });
        }

        /**
        * Create the strings that will be shown in the UI
        */
        function buildStrings() {

            $scope.basehref = PdpUrlFactory.getPdpUrl($scope.requiredBaseGameandDLC[0].baseGame);
            // localization string replacements
            var params = {
                '%basegameLink%': buildAnchorMarkup(
                    $scope.requiredBaseGameandDLC[0].baseGame.i18n.displayName,
                    $scope.basehref
                ),
                '%basegame%': $scope.requiredBaseGameandDLC[0].baseGame.i18n.displayName
            };
            $scope.strings = {};
            $scope.strings.btnCancel = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, BTN_CANCEL);
            $scope.strings.title = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, TITLE, params);
            $scope.strings.description = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, DESCRIPTION, params);
            $scope.$digest();
        }

        setupModal();
    }

    function originDialogContentBundledlcpurchaseprevention(ComponentsConfigFactory, DialogFactory, NavigationFactory) {
        function originDialogContentBundledlcpurchasepreventionLink(scope, elem) {
            scope.bindClick = function() {
                setTimeout(function() {
                    elem.find('.baseGame').click(function() {
                        DialogFactory.close(scope.dialogid);
                        NavigationFactory.internalUrl(decodeURIComponent(scope.basehref));
                    });
                }, 0);
            };
        }

        return {
            restrict: 'E',
            scope: {
                dialogid: '@'
            },
            link: originDialogContentBundledlcpurchasepreventionLink,

            controller: originDialogContentBundledlcpurchasepreventionCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/bundledlcpurchaseprevention.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentBundledlcpurchaseprevention
     * @restrict E
     * @element ANY
     * @scope
     * @description DLC purchase prevention / warning dialog
     * @param {string} dialogid The ID of the dialog
     * @param {LocalizedString} btncancel * Cancel text - merchandized in defaults
     * @param {LocalizedString} title * Title text - merchandized in defaults
     * @param {LocalizedString} description * Description text - merchandized in defaults
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-dialog-content-bundledlcpurchaseprevention dialogid="web-pdp-dlc-purchase-prevention-flow"  />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('originDialogContentBundledlcpurchasepreventionCtrl', originDialogContentBundledlcpurchasepreventionCtrl)
        .directive('originDialogContentBundledlcpurchaseprevention', originDialogContentBundledlcpurchaseprevention);
}());
