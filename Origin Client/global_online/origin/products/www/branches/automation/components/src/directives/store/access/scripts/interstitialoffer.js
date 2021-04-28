
/** 
 * @file store/access/scripts/interstitialoffer.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-access-interstitialoffer';
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    function OriginStoreAccessInterstitialofferCtrl($scope, PriceInsertionFactory, SubscriptionFactory, UtilFactory) {
        $scope.strings = {
            save: UtilFactory.getLocalizedStr($scope.save, CONTEXT_NAMESPACE, 'save'),
            value: UtilFactory.getLocalizedStr($scope.value, CONTEXT_NAMESPACE, 'value'),
            duration: UtilFactory.getLocalizedStr($scope.duration, CONTEXT_NAMESPACE, 'duration')
        };

        PriceInsertionFactory
            .insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.price, CONTEXT_NAMESPACE, 'price');

        $scope.$on('originStoreAccessInterstitial:highlightOffer', function(e, data) {
            if(_.get(data, 'offerId') === _.get($scope, 'offerId') || _.get(data, 'offerId') === _.get($scope, 'trialOfferId')) {
                $scope.highlight = BooleanEnumeration.true;
            } else {
                $scope.highlight = BooleanEnumeration.false;
            }
        });

        $scope.hasUserUsedFreeTrial = SubscriptionFactory.hasUsedFreeTrial();
    }

    function originStoreAccessInterstitialoffer(ComponentsConfigFactory) {
        function originStoreAccessInterstitialofferLink(scope, elem, attrs, OriginStoreAccessInterstitialCtrl) {
            scope.highlightOffer = function() {
                if(!scope.hasUserUsedFreeTrial && scope.trialOfferId) {
                    OriginStoreAccessInterstitialCtrl.setHighlightedOfferId(_.get(scope, 'trialOfferId'));
                } else {
                    OriginStoreAccessInterstitialCtrl.setHighlightedOfferId(_.get(scope, 'offerId'));
                }
            };

            if(scope.highlight === BooleanEnumeration.true) {
                scope.highlightOffer();
            }
        }

        return {
            restrict: 'E',
            require: '^originStoreAccessInterstitial',
            replace: true,
            scope: {
                offerId: '@',
                trialOfferId: '@',
                value: '@',
                save: '@',
                highlight: '@',
                duration: '@',
                price: '@'
            },
            controller: OriginStoreAccessInterstitialofferCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/views/interstitialoffer.html'),
            link: originStoreAccessInterstitialofferLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessInterstitialoffer
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {String} offer-id the offer id of the subscription offer to display
     * @param {String} trial-offer-id the corresponding trial offer id of the subscription offer to display
     * @param {LocalizedString} value the value prop above the price
     * @param {LocalizedTemplateString} price the price per duration string (ex: %~Origin.OFR.50.0001171~% per month)
     * @param {LocalizedString} save the save prop below the price
     * @param {BooleanEnumeration} highlight Highlight this offer by default
     * @param {LocalizedString} duration the per year/month text
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-interstitialoffer />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessInterstitialoffer', originStoreAccessInterstitialoffer);
}()); 
