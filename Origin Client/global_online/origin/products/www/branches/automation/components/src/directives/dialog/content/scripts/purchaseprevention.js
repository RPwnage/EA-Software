
/**
 * @file dialog/content/scripts/bundlepurchaseprevention.js
 */
(function(){
    'use strict';
    var CONTEXT_NAMESPACE = 'origin-dialog-purchaseprevention';
    var OWN_SOME_GAMES_TITLE = 'own-some-games-title';
    var OWN_SOME_GAMES_DESCRIPTION = 'own-some-games-description';
    var OWN_SOME_GAME_REMOVE = 'own-some-games-remove';
    var OWN_ALL_GAMES_TITLE = 'own-all-games-title';
    var OWN_ALL_GAMES_DESCRIPTION = 'own-all-games-description';
    var OWN_GAME_TITLE = 'own-game-title';
    var OWN_GAME_DESCRIPTION_CART_BUNDLE = 'own-game-description-cart-bundle';
    var OWN_GAME_DESCRIPTION_OFFER_BUNDLE = 'own-game-description-offer-bundle';
    var OWN_GAME_OF_MANY_DESCRIPTION = 'own-game-of-many-description';
    var DESCRIPTION_GAME_PLACEHOLDER = '%edition%';

    function originDialogPurchasepreventionCtrl($scope, DialogFactory, OwnershipFactory, PdpUrlFactory, ObjectHelperFactory) {
        /**
         * Set the modal up to be displayed after all the required information is ready.
         * @param  {Object} bundleProducts  Products the user is trying to purchase
         */
        function setupModal(bundleProducts) {
            $scope.ownedProducts = OwnershipFactory.getOutrightOwnedProducts(bundleProducts);

            $scope.ownedCount = $scope.ownedProducts.length;
            $scope.purchaseCount = ObjectHelperFactory.length(bundleProducts);

            for(var i=0; i < $scope.ownedCount; i++) {
                $scope.ownedProducts[i].pdpLink = PdpUrlFactory.getPdpUrl($scope.ownedProducts[i], true);
            }

            $scope.buildStrings(bundleProducts, $scope.ownedProducts, $scope.isOfferBundle);
            $scope.$digest();
        }
        /**
         * Close the dialog and set the response values
         * @param  {boolean} success If the user selected true or false
         */
        $scope.closeResponse = function(success, $event) {
            $event.preventDefault();
            DialogFactory.setReturnValues(success, {id: $scope.id});
            DialogFactory.close($scope.modalId);
        };

        OwnershipFactory.getGameDataWithEntitlements($scope.bundle)
            .then(setupModal)
            .catch(function(e) {
               console.log(e);
               $scope.closeResponse(false);
        });
    }


    function originDialogPurchaseprevention(ComponentsConfigFactory, UtilFactory, ObjectHelperFactory, PdpUrlFactory, $compile, $sanitize) {
        function originDialogPurchasepreventionLink(scope) {
            var $description = angular.element('.origin-dialog-content-pp-text-single');
            var $descriptionContainer = angular.element('<div>');

            function buildAnchorMarkup(text, product) {
                return '<a class="otka" ng-click="closeResponse(false)" href="' +  PdpUrlFactory.getPdpUrl(product) + '">' + text + '</a>';
            }

            /**
            * Create the strings that will be shown in the UI
            * @param  {Object} bundleProducts   Products the user is trying to buy
            * @param  {Array} ownedProducts  The set of products in the bundleProducts the user already owns
            * @param  {Boolean} isOfferBundle  is this bundle an offer bundle or cart bundle
            */
            scope.buildStrings = function(bundleProducts, ownedProducts, isOfferBundle) {

                // Default strings
                var bundleCount = ObjectHelperFactory.length(bundleProducts),
                    ownedCount = ownedProducts.length,
                    titleKey = OWN_SOME_GAMES_TITLE,
                    descriptionKey = OWN_SOME_GAMES_DESCRIPTION,
                    params = {
                    '%count%': ownedCount
                    },
                    anchorHtml,
                    descriptionHtml,
                    $descriptionCompiled;

                if (ownedCount === 1) {
                    if (bundleCount === 1) { // Owns the one game in the bundle
                        titleKey = OWN_GAME_TITLE;
                        if (isOfferBundle) {
                            descriptionKey = OWN_GAME_DESCRIPTION_OFFER_BUNDLE;
                        } else {
                            descriptionKey = OWN_GAME_DESCRIPTION_CART_BUNDLE;
                        }
                    } else { // Owns one game, but bundle has many games
                        if (isOfferBundle) {
                            // Only mention the game -- don't offer to remove since we can't remove from offer bundle
                            descriptionKey = OWN_GAME_DESCRIPTION_OFFER_BUNDLE;
                        } else {
                            descriptionKey = OWN_GAME_OF_MANY_DESCRIPTION;
                        }
                    }

                    // Build anchor to link to single game
                    anchorHtml = buildAnchorMarkup(
                        ownedProducts[0].i18n.displayName,
                        scope.ownedProducts[0]
                    );
                }

                if (bundleCount === ownedCount && ownedCount > 1) { // Owns all games in the multi-item bundle
                    titleKey = OWN_ALL_GAMES_TITLE;
                    descriptionKey = OWN_ALL_GAMES_DESCRIPTION;
                }

                scope.strings = scope.strings || {};
                if (!isOfferBundle){
                    scope.strings.remove = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, OWN_SOME_GAME_REMOVE, params, ownedCount);
                }
                scope.strings.btnRemoveAndContinue = UtilFactory.getLocalizedStr(scope.btnRemoveAndContinue, CONTEXT_NAMESPACE, 'btn-remove-and-continue');
                scope.strings.btnNo = UtilFactory.getLocalizedStr(scope.btnNo, CONTEXT_NAMESPACE, 'btn-no');
                scope.strings.btnOkay =  UtilFactory.getLocalizedStr(scope.btnOkay, CONTEXT_NAMESPACE, 'btn-okay');
                scope.strings.continueButtonText = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'continue-purchase-text');
                scope.strings.title = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, titleKey, params, ownedCount);
                descriptionHtml = $sanitize(UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, descriptionKey, params, bundleCount));
                descriptionHtml = descriptionHtml.replace(DESCRIPTION_GAME_PLACEHOLDER, anchorHtml); //Manually replace %game% placeholder because of ng-click. Sanitize or ng-bind-html removes it.
                $descriptionCompiled = $compile($descriptionContainer.html(descriptionHtml))(scope);
                $description.append($descriptionCompiled);
            };
        }

        return {
            restrict: 'E',
            scope: {
                bundle: '=',
                modalId: '@',
                isOfferBundle: '@'
            },
            link: originDialogPurchasepreventionLink,
            controller: originDialogPurchasepreventionCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/purchaseprevention.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogPurchaseprevention
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {Array} bundle The array of offerIds in the bundle.
     * @param {string} modal-id The id of the modal
     *
     * @param {LocalizedString} btn-okay okay button text. Set up once as default E.G., "Okay"
     * @param {LocalizedString} btn-remove-and-continue remove item from bundle and continue. Set up once as default. E.G., "Yes, remove & continue",
     * @param {LocalizedString} continue-purchase-text continue purchase anyway text. Set up once as default. E.G., "Purchase Anyway"
     * @param {LocalizedString} btn-no no button text. Set up once as default. E.G., "No, go back"
     * @param {LocalizedString} own-game-title own game dialog title text. Set up once as default. E.G., "You already own this!"
     * @param {LocalizedString} own-game-description-cart-bundle own game description for bundle cart dialog. Set up once as default. E.G., "No need to buy it again... you already own %game%! Head over to your Game Library to start playing.",
     * @param {LocalizedString} own-game-description-offer-bundle own game description for offer bundles. Set up once as default. E.G., "No need to buy it again... you already own %game%!",
     * @param {LocalizedString} own-game-of-many-description own game of many. Set up once as default. E.G., "No need to buy it again... you already own %game%."
     * @param {LocalizedString} own-all-games-title own all games title. Set up once as default. E.G., "You already own all of the games in this bundle!"
     * @param {LocalizedString} own-all-games-description own all games description. Follows by a list if owned products. Set up once as default. E.G., "No need to buy them again... you already own:"
     * @param {LocalizedString} own-some-games-title own some games title. Set up once as default. E.G., "You already own %count% game in this bundle!"
     * @param {LocalizedString} own-some-games-description own some games description. Follows by a list of owned games. Set up once as default. E.G., "No need to buy them again... you already own:"
     * @param {LocalizedString} own-some-games-remove own some games remove text. Set up once as default. E.G., "Would you like us to remove them from your bundle?"
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-dialog-purchaseprevention origin-dialog-purchaseprevention bundle="['id','id']" modal-id="DIALOG_ID"/>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('originDialogPurchasepreventionCtrl', originDialogPurchasepreventionCtrl)
        .directive('originDialogPurchaseprevention', originDialogPurchaseprevention);
}());
