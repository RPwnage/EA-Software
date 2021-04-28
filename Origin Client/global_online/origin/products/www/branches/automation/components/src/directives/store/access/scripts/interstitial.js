
/** 
 * @file store/access/scripts/interstitial.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-access-interstitial';

    function OriginStoreAccessInterstitialCtrl($scope, DirectiveScope, DialogFactory, StoreAccessInterstitialConstants, AuthFactory, PurchaseFactory, SubscriptionFactory) {
        var hasUserUsedFreeTrial = SubscriptionFactory.hasUsedFreeTrial();

        $scope.onClick = function() {
            if($scope.highlightedOfferId) {
                DialogFactory.close(StoreAccessInterstitialConstants.id);

                AuthFactory.promiseLogin()
                    .then(_.partial(PurchaseFactory.subscriptionCheckout, $scope.highlightedOfferId));
            }
        };

        this.setHighlightedOfferId = function(offerId) {
            $scope.highlightedOfferId = offerId;
            $scope.$broadcast('originStoreAccessInterstitial:highlightOffer', {offerId: offerId});
        };

        // given a string, see if there is a trial string and that the user has not consumed the trial already
        function getString(string, trialString) {
            // has string and not used trial or no trial string
            if (string && (hasUserUsedFreeTrial || !trialString)) {
                return string;
            // not used trial and has trial string
            } else if(!hasUserUsedFreeTrial && trialString) {
                return trialString;
            }
        }

        function setStrings() {
            $scope.strings = {
                header: getString($scope.header, $scope.trialHeader),
                description: getString($scope.description, $scope.trialDescription),
                buttonText: getString($scope.buttonText, $scope.trialButtonText)
            };

            $scope.$digest();
        }
        
        DirectiveScope
            .populateScope($scope, CONTEXT_NAMESPACE)
            .then(setStrings);
    }

    function originStoreAccessInterstitial(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                header: '@',
                trialHeader: '@',
                buttonText: '@',
                logo: '@',
                description: '@',
                trialDescription: '@',
                trialButtonText: '@'
            },
            controller: 'OriginStoreAccessInterstitialCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/views/interstitial.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessInterstitial
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} header the header text
     * @param {LocalizedString} trial-header the trial header text
     * @param {LocalizedString} button-text the CTA text
     * @param {LocalizedString} trial-button-text the trial CTA text
     * @param {ImageUrl} logo the logo next to the description text
     * @param {LocalizedString} description the description text
     * @param {LocalizedString} trial-description the trial description text
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-interstitial />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessInterstitial', originStoreAccessInterstitial)
        .controller('OriginStoreAccessInterstitialCtrl', OriginStoreAccessInterstitialCtrl)
        .constant('StoreAccessInterstitialConstants', {'id': 'origin-store-access-interstitial', 'template': 'oax-interstitial'});
}()); 
