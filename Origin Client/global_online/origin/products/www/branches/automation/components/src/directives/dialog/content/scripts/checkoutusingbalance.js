/**
 * @file dialog/content/checkoutusingbalance.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-checkoutusingbalance';

    function OriginDialogContentCheckoutusingbalanceCtrl($scope, $sce, $attrs, $timeout, UtilFactory, ObjectHelperFactory, GamesDataFactory, GamesCommerceFactory, CheckoutFactory, DialogFactory, OfferTransformer, ComponentsConfigHelper, NavigationFactory, AppCommFactory) {
        var localize = UtilFactory.getLocalizedStrGenerator(CONTEXT_NAMESPACE),
            termsOfSaleLoc = localize($scope.termsofsalestr, 'termsofsalestr'),
            termsOfSaleLink = localize($scope.termsofsalelink, 'termsofsalelink'),
            eulasLoc = localize($scope.eulasstr, 'eulasstr'),
            eulasLink = localize($scope.eulaslink, 'eulaslink'),
            aboutOriginLoc = localize($scope.aboutoriginstr, 'aboutoriginstr'),
            aboutOriginLink = localize($scope.aboutoriginlink, 'aboutoriginlink'),
            eaHelpLoc = localize($scope.eahelpstr, 'eahelpstr'),
            eaHelpLink = localize($scope.eahelplink, 'eahelplink');

        function buildLinkHtml(text, href) {
            return '<a href="' + href + '" class="otka external-link">' + text + '</a>';
        }

        $scope.reviewOrderLoc = localize($scope.revieworderstr, 'revieworderstr');
        $scope.newBalanceLoc = localize($scope.newbalancestr, 'newbalancestr');
        $scope.placeOrderLoc = localize($scope.placeorderstr, 'placeorderstr');
        $scope.thankYouLoc = localize($scope.thankyoustr, 'thankyoustr');
        $scope.orderHistoryLoc = localize($scope.orderhistorystr, 'orderhistorystr');
        $scope.orderNumberLoc = localize($scope.ordernumberstr, 'ordernumberstr');
        $scope.totalLoc = localize($scope.totalstr, 'totalstr');
        $scope.downloadWithOriginLoc = localize($scope.downloadwithoriginstr, 'downloadwithoriginstr');
        $scope.backToGameLoc = localize($scope.backtogamestr, 'backtogamestr');
        $scope.checkoutDisclaimerLoc = localize($scope.checkoutdisclaimerstr, 'checkoutdisclaimerstr', {
            '%termsofsale%': buildLinkHtml(termsOfSaleLoc, termsOfSaleLink),
            '%eulas%': buildLinkHtml(eulasLoc, eulasLink),
            '%aboutorigin%': buildLinkHtml(aboutOriginLoc, aboutOriginLink),
            '%eahelp%': buildLinkHtml(eaHelpLoc, eaHelpLink)
        });

        $scope.isOIGContext = !!ComponentsConfigHelper.getOIGContextConfig();
        $scope.purchaseSuccess = false;

        function handleBalanceResult(price, balance) {
            $scope.currentBalance = balance;
            $scope.newBalance = balance - price;
            $scope.$digest();
            AppCommFactory.events.fire('origin-dialog-content-checkoutusingbalance:ready');
        }

        function handleCheckoutResult(orderNumber) {
            $scope.purchaseSuccess = !!orderNumber;
            $scope.orderNumber = orderNumber;
            $scope.$digest();
        }

        function handleCheckoutError(error) {
            AppCommFactory.events.fire('origin-dialog-content-checkoutusingbalance:error');
            DialogFactory.close($scope.dialogId);
            CheckoutFactory.handleError(error);
        }

        $scope.onPlaceOrder = function() {
            GamesCommerceFactory.postVcCheckout($scope.offerId)
                .then(handleCheckoutResult)
                .then(CheckoutFactory.onPaymentSuccess)
                .catch(handleCheckoutError);
        };

        $scope.onBackToGame = function() {
            CheckoutFactory.backToGame();
        };

        $scope.onDownloadWithOrigin = function() {
            DialogFactory.close($scope.dialogId);
            CheckoutFactory.downloadGame($scope.offerId);
        };

        $scope.onViewOrderHistory = function() {
            var url = ComponentsConfigHelper.getUrl('eaAccountsOrderHistory');
            if (!Origin.client.isEmbeddedBrowser()){
                url = url.replace("{locale}", Origin.locale.locale());
                url = url.replace("{access_token}", "");
            }
            NavigationFactory.asyncOpenUrlWithEADPSSO(url);
        };

        function updateView(game) {
            if(game && !_.isEmpty(game)) {
                var currencyLoc = localize($scope.currencytypedefaultstr, 'currencytypedefaultstr'),
                    currencyLocMap = {
                        '_BW': localize($scope.currencytypebwstr, 'currencytypebwstr'),
                        '_FF': localize($scope.currencytypeffstr, 'currencytypeffstr'),
                        '_FW': localize($scope.currencytypefwstr, 'currencytypefwstr'),
                        '_PF': localize($scope.currencytypepfstr, 'currencytypepfstr'),
                        '_SC': localize($scope.currencytypescstr, 'currencytypescstr')
                    },
                    priceLoc = game.price,
                    priceLocMap = {
                        '_BW': localize($scope.pricetypebwstr, 'pricetypebwstr', {'%value%': game.price}),
                        '_FF': localize($scope.pricetypeffstr, 'pricetypeffstr', {'%value%': game.price}),
                        '_FW': localize($scope.pricetypefwstr, 'pricetypefwstr', {'%value%': game.price}),
                        '_PF': localize($scope.pricetypepfstr, 'pricetypepfstr', {'%value%': game.price}),
                        '_SC': localize($scope.pricetypescstr, 'pricetypescstr', {'%value%': game.price})
                    },
                    totalPriceLoc = game.price,
                    totalPriceLocMap = {
                        '_BW': localize($scope.totaltypebwstr, 'totaltypebwstr', {'%value%': game.price}),
                        '_FF': localize($scope.totaltypeffstr, 'totaltypeffstr', {'%value%': game.price}),
                        '_FW': localize($scope.totaltypefwstr, 'totaltypefwstr', {'%value%': game.price}),
                        '_PF': localize($scope.totaltypepfstr, 'totaltypepfstr', {'%value%': game.price}),
                        '_SC': localize($scope.totaltypescstr, 'totaltypescstr', {'%value%': game.price})
                    };

                if (currencyLocMap.hasOwnProperty(game.currency)) {
                    currencyLoc = currencyLocMap[game.currency];
                }

                if (priceLocMap.hasOwnProperty(game.currency)) {
                    priceLoc = priceLocMap[game.currency];
                }

                if (totalPriceLocMap.hasOwnProperty(game.currency)) {
                    totalPriceLoc = totalPriceLocMap[game.currency];
                }

                // Avoid showing default pack art if the game is not configured with actual pack art.
                game.hasPackArt = game.packArt !== GamesDataFactory.defaultBoxArtUrl();

                $scope.model = game;
                $scope.priceLoc = $sce.trustAsHtml(priceLoc);
                $scope.totalPriceLoc = totalPriceLoc;
                $scope.currentBalanceLoc = localize($scope.currentbalancestr, 'currentbalancestr', {'%currencytype%': currencyLoc});

                GamesCommerceFactory.getWalletBalance(game.fictionalCurrencyCode)
                    .then(_.partial(handleBalanceResult, game.price));
            }
        }

        GamesDataFactory.getCatalogInfo([$scope.offerId])
            .then(OfferTransformer.transformOffer)
            .then(ObjectHelperFactory.toArray)
            .then(ObjectHelperFactory.takeHead)
            .then(updateView);
    }

    function originDialogContentCheckoutusingbalance(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                dialogId: '@dialogid',
                revieworderstr: '@',
                pricetypebwstr: '@',
                pricetypeffstr: '@',
                pricetypefwstr: '@',
                pricetypepfstr: '@',
                pricetypescstr: '@',
                currencytypebwstr: '@',
                currencytypeffstr: '@',
                currencytypefwstr: '@',
                currencytypepfstr: '@',
                currencytypescstr: '@',
                currencytypedefaultstr: '@',
                currentbalancestr: '@',
                newbalancestr: '@',
                checkoutdisclaimerstr: '@',
                termsofsalestr: '@',
                termsofsalelink: '@',
                eulasstr: '@',
                eulaslink: '@',
                aboutoriginstr: '@',
                aboutoriginlink: '@',
                eahelpstr: '@',
                eahelplink: '@',
                placeorderstr: '@',
                thankyoustr: '@',
                orderhistorystr: '@',
                ordernumberstr: '@',
                totalstr: '@',
                totaltypebwstr: '@',
                totaltypeffstr: '@',
                totaltypefwstr: '@',
                totaltypepfstr: '@',
                totaltypescstr: '@',
                downloadwithoriginstr: '@',
                backtogamestr: '@'
            },
            controller: 'OriginDialogContentCheckoutusingbalanceCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/checkoutusingbalance.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentCheckoutusingbalance
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid The offer ID to purchase
     * @param {string} dialogid The dialog ID to close when necessary
     * @param {LocalizedString} revieworderstr The "Review Order" text
     * @param {LocalizedText} pricetypebwstr The "BioWare Points" text, includes a placeholder for a value
     * @param {LocalizedText} pricetypeffstr The "FIFA Points" text, includes a placeholder for a value
     * @param {LocalizedText} pricetypefwstr The "FIFA World Points" text, includes a placeholder for a value
     * @param {LocalizedText} pricetypepfstr The "Play4Free Funds" text, includes a placeholder for a value
     * @param {LocalizedText} pricetypescstr The "SIMS Currency" text, includes a placeholder for a value
     * @param {LocalizedString} currencytypebwstr The "BioWare Points" text
     * @param {LocalizedString} currencytypeffstr The "FIFA Points" text
     * @param {LocalizedString} currencytypefwstr The "FIFA World Points" text
     * @param {LocalizedString} currencytypepfstr The "Play4Free Funds" text
     * @param {LocalizedString} currencytypescstr The "SIMS Currency" text
     * @param {LocalizedString} currencytypedefaultstr The default "Currency" text
     * @param {LocalizedString} currentbalancestr The "%currencytype% Balance" text
     * @param {LocalizedString} newbalancestr The "Balance After Purchase" text
     * @param {LocalizedString} checkoutdisclaimerstr The full disclaimer text, allows the following placeholders: %termsofsale%, %eulas%, %aboutorigin%, %eahelp%
     * @param {LocalizedString} termsofsalestr The "Terms of Sale" text
     * @param {LocalizedString} termsofsalelink The link to navigate to when the user clicks "Terms of Sale"
     * @param {LocalizedString} eulasstr The "here" text
     * @param {LocalizedString} eulaslink The link to navigate to when the user clicks "here"
     * @param {LocalizedString} aboutoriginstr The "About Origin" text
     * @param {LocalizedString} aboutoriginlink The link to navigate to when the user clicks "About Origin"
     * @param {LocalizedString} eahelpstr The "Visit EA Help" text
     * @param {LocalizedString} eahelplink The link to navigate to when the user clicks "Visit EA Help"
     * @param {LocalizedString} placeorderstr The "Place Order" button text
     * @param {LocalizedString} thankyoustr The "Thank you for your purchase!" text
     * @param {LocalizedString} orderhistorystr The "View Order History" text
     * @param {LocalizedString} ordernumberstr The "Order Number" text
     * @param {LocalizedString} totalstr The "Total" text
     * @param {LocalizedString} totaltypebwstr The "BioWare Points" text, includes a placeholder for a value
     * @param {LocalizedString} totaltypeffstr The "FIFA Points" text, includes a placeholder for a value
     * @param {LocalizedString} totaltypefwstr The "FIFA World Points" text, includes a placeholder for a value
     * @param {LocalizedString} totaltypepfstr The "Play4Free Funds" text, includes a placeholder for a value
     * @param {LocalizedString} totaltypescstr The "SIMS Currency" text, includes a placeholder for a value
     * @param {LocalizedString} downloadwithoriginstr The "Download with Origin" text
     * @param {LocalizedString} backtogamestr The "Back to Game" button text
     * @description
     *
     * Individual game item representing a just-acquired entitlement in checkout confirmation view
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-checkoutusingbalance
     *          offerid="Origin.OFR.50.0000500"
     *          dialogid="origin-dialog-content-checkoutusingbalance-id"
     *          revieworderstr="Review Order"
     *          pricetypebwstr="%value%<br>BioWare Points"
     *          pricetypeffstr="%value%<br>FIFA Points"
     *          pricetypefwstr="%value%<br>FIFA World Points"
     *          pricetypepfstr="%value%<br>Play4Free Funds"
     *          pricetypescstr="%value%<br>SIMS Currency"
     *          currencytypebwstr="BioWare Points"
     *          currencytypeffstr="FIFA Points"
     *          currencytypefwstr="FIFA World Points"
     *          currencytypepfstr="Play4Free Funds"
     *          currencytypescstr="SIMS Currency"
     *          currencytypedefaultstr="Currency"
     *          currentbalancestr="%currencytype% Balance"
     *          newbalancestr="Balance After Purchase"
     *          checkoutdisclaimerstr="Legal text goes here"
     *          termsofsalestr="Terms of Sale"
     *          termsofsalelink="http://tos.ea.com/legalapp/termsofsale/US/en/PC/"
     *          eulasstr="here"
     *          eulaslink="http://www.ea.com/1/product-eulas"
     *          aboutoriginstr="About Origin"
     *          aboutoriginlink="https://www.origin.com/about"
     *          eahelpstr="Visit EA Help"
     *          eahelplink="http://help.ea.com/article/ordering-payment-information/"
     *          placeorderstr="Place Order"
     *          thankyoustr="Thank you for your purchase!"
     *          orderhistorystr="View Order History"
     *          ordernumberstr="Order Number"
     *          totalstr="Total"
     *          totaltypebwstr="%value% BioWare Points"
     *          totaltypeffstr="%value% FIFA Points"
     *          totaltypefwstr="%value% FIFA World Points"
     *          totaltypepfstr="%value% Play4Free Funds"
     *          totaltypescstr="%value% SIMS Currency"
     *          downloadwithoriginstr="Download with Origin"
     *          backtogamestr="Back to Game">
     *         </origin-dialog-content-checkoutusingbalance>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginDialogContentCheckoutusingbalanceCtrl', OriginDialogContentCheckoutusingbalanceCtrl)
        .directive('originDialogContentCheckoutusingbalance', originDialogContentCheckoutusingbalance);

}());