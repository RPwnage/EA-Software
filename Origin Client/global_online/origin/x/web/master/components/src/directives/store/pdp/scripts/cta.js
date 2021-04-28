/**
 * @file store/pdp/cta.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-cta';

    function OriginStorePdpCtaCtrl($scope, DialogFactory, UtilFactory, StoreActionFlowFactory, ProductFactory, OwnershipFactory, AuthFactory) {
        var getPrimaryAction = StoreActionFlowFactory.getPrimaryAction,
            getSecondaryAction = StoreActionFlowFactory.getSecondaryAction,
            isSavingsAction = StoreActionFlowFactory.isSavingsAction,
            isDisabledAction = StoreActionFlowFactory.isDisabledAction;

        $scope.strings = {
            preloadText: UtilFactory.getLocalizedStr($scope.preloadText, CONTEXT_NAMESPACE, 'preload'),
            preloadedText: UtilFactory.getLocalizedStr($scope.preloadedText, CONTEXT_NAMESPACE, 'preloaded'),
            subscribeText: UtilFactory.getLocalizedStr($scope.subscribeText, CONTEXT_NAMESPACE, 'subscribe'),
            downloadText: UtilFactory.getLocalizedStr($scope.downloadText, CONTEXT_NAMESPACE, 'download'),
            playText: UtilFactory.getLocalizedStr($scope.playText, CONTEXT_NAMESPACE, 'play'),
            freeText: UtilFactory.getLocalizedStr($scope.freeText, CONTEXT_NAMESPACE, 'free'),
            orText: UtilFactory.getLocalizedStr($scope.orText, CONTEXT_NAMESPACE, 'or')
        };

        /**
         * Generic message localization function
         * @param {string} propertyName scope property to be localized
         * @return {string}
         */
        function localize(propertyName) {
            return UtilFactory.getLocalizedStr($scope[propertyName + 'Text'], CONTEXT_NAMESPACE, propertyName, {
                '%price%': '$' + $scope.model.price
            });
        }


        /**
         * Determines CTA area visibility
         * @return {boolean}
         */
        $scope.isVisible = function () {
            return getPrimaryAction(getPrimaryEdition()) !== '';
        };

        /**
         * Checks if there is an alternative (secondary) CTA for this offer
         * @return {boolean}
         */
        $scope.hasAlternativeAction = function () {

            var primaryAction, secondaryAction;

            if (userOwnsBetterEdition()) {
                return false;
            } else {
                primaryAction = getPrimaryAction(getPrimaryEdition());
                secondaryAction = getSecondaryAction($scope.model);
                return secondaryAction !== '' && secondaryAction !== primaryAction;
            }

        };

        /**
         * Get the primary CTA message for the product
         * @return {string}
         */
        $scope.getPrimaryCtaMessage = function () {
            return localize(getPrimaryAction(getPrimaryEdition()));
        };

        /**
         * Get the primary CTA message for the product
         * @return {string}
         */
        $scope.getAlternativeCtaMessage = function () {
            return localize(getSecondaryAction($scope.model));
        };

        $scope.isPrimarySavingsAction = function () {
            return $scope.model.savings && isSavingsAction(getPrimaryAction(getPrimaryEdition()));
        };

        $scope.isPrimaryActionDisabled = function () {
            return isDisabledAction(getPrimaryAction(getPrimaryEdition()));
        };

        function getPrimaryEdition() {
            return OwnershipFactory.getPrimaryEdition($scope.editions, $scope.model);
        }

        function userOwnsBetterEdition() {
            return OwnershipFactory.userOwnsBetterEdition($scope.editions, $scope.model);
        }

        /**
         * Starts Primary action flow with the 'primaryEdition'
         * @return {void}
         */
        $scope.startPrimaryActionFlow = function () {
            AuthFactory.events.fire('auth:loginWithCallback', function(){
                var primaryEdition = getPrimaryEdition();
                var primaryAction = StoreActionFlowFactory.getPrimaryAction(primaryEdition);

                StoreActionFlowFactory.startActionFlow(primaryEdition, primaryAction);
            });
        };

        /**
         * Starts Secondary action flow with this edition
         * @return {void}
         */
        $scope.startSecondaryActionFlow = function () {
            var secondaryAction = getSecondaryAction($scope.model);
            StoreActionFlowFactory.startActionFlow($scope.model, secondaryAction);
        };

        function updateModel(newData) {
            $scope.model = newData;
            $scope.strings.saveText = UtilFactory.getLocalizedStr($scope.saveText, CONTEXT_NAMESPACE, 'save', {
                '%savings%': newData.savings
            });
        }

        $scope.model = {};
        $scope.$watch('offerId', function (newValue) {
            if (newValue) {
                ProductFactory.get(newValue).attachToScope($scope, updateModel);
            }
        });
    }

    function originStorePdpCta(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@',
                editions: '=',
                buyText: '@',
                subscribeText: '@',
                preorderText: '@',
                preloadText: '@',
                preloadedText: '@',
                downloadText: '@',
                playText: '@',
                freeText: '@',
                saveText: '@',
                orText: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/cta.html'),
            controller: 'OriginStorePdpCtaCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpCta
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offer-id product offer ID
     * @param {LocalizedString|OCD} buy-text (optional) override text for the Buy button
     * @param {LocalizedString|OCD} preorder-text (optional) override text for the Preorder button
     * @param {LocalizedString|OCD} preload-text (optional) override text for the Preload button
     * @param {LocalizedString|OCD} preloaded-text (optional) override text for the Preloaded button
     * @param {LocalizedString|OCD} download-text (optional) override text for the Download button
     * @param {LocalizedString|OCD} play-text (optional) override text for the Play button
     * @param {LocalizedString|OCD} free-text (optional) override text for the Get it Now Free button
     * @param {LocalizedString|OCD} save-text (optional) override text for the Savings indicator
     * @param {LocalizedString|OCD} or-text (optional) override text for action area divider ('OR')
     * @description
     *
     * PDP CTA area with primary and optional secondary action buttons
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-cta offer-id="{{ model.offerId }}"></origin-store-pdp-cta>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpCtaCtrl', OriginStorePdpCtaCtrl)
        .directive('originStorePdpCta', originStorePdpCta);
}());
