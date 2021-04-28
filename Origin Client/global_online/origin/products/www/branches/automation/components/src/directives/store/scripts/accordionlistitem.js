/**
 * @file store/scripts/accordionlistitem.js
 */
(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-accordionlist-item';

    /* Directive Declaration */
    function originStoreAccordionlistItem(ComponentsConfigFactory, PriceInsertionFactory, ComponentsConfigHelper, UtilFactory) {
        function originStoreAccordionlistItemLink($scope, element) {
            var SILDER_TIMER = 300; //millisecond
            $scope.expanded = false;
            var $element = element.find('.origin-store-accordionlist-item-content');

            //Check if angular is returning jquery, if not, wrap jquery around element
            if (!$element.slideUp || !$element.slideDown) {
                $element = angular.element(element);
            }
            $scope.toggleContent = function () {
                $scope.expanded = !$scope.expanded;
                if ($scope.expanded) {
                    $element.slideDown(SILDER_TIMER);
                } else {
                    $element.hide();
                }
            };

            function getEAAccountUrl() {
                var url = ComponentsConfigHelper.getUrl('eaAccounts');
                if (!Origin.client.isEmbeddedBrowser()) {
                    url = url.replace("{locale}", Origin.locale.locale());
                    url = url.replace("{access_token}", "");
                }
                return url;
            }

            function getCustomerExperienceUrl() {
                return ComponentsConfigHelper.getHelpUrl(Origin.locale.locale(), Origin.locale.languageCode());
            }

            $scope.strings = {};
            $scope.eaccount = UtilFactory.getLocalizedStr($scope.eaccount, CONTEXT_NAMESPACE, 'eaccount');
            $scope.customerexperience = UtilFactory.getLocalizedStr($scope.customerexperience, CONTEXT_NAMESPACE, 'customerexperience');

            var eaAccountLink = '<a href="' + getEAAccountUrl() + '" class="otka external-link-sso">' + $scope.eaccount + '</a>';
            var customerExperienceLink = '<a href="' + getCustomerExperienceUrl() + '" class="otka external-link-sso">' + $scope.customerexperience + '</a>';

            PriceInsertionFactory
                .insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.topic, CONTEXT_NAMESPACE, 'topic');
            PriceInsertionFactory
                .insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.content, CONTEXT_NAMESPACE, 'content', { '%eaaccount%': eaAccountLink, '%customerexperience%': customerExperienceLink });
        }

        return {
            restrict: 'E',
            scope: {
                topic: '@',
                content: '@',
                eaccount: '@',
                customerexperience: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/accordionlistitem.html'),
            link: originStoreAccordionlistItemLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccordionlistItem
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedTemplateString} topic the topic of the accordion list item
     * @param {LocalizedTemplateString} content the content of the accordion list item
     * @param {LocalizedTemplateString} eaccount text used to replace %eaccount%
     * @param {LocalizedTemplateString} customerexperience text used to replace %customerexperience%
     *
     * @description
     *
     *  Accordion list item
     *  Designed to be used with origin-store-accordionlist directive
     *
     * @example
     * <origin-store-accordionlist>
     *     <origin-store-accordionlist-item topic="What platforms can I use Origin on?"
     *                                      content="Yes, Origin is available on all of those platforms. Other advantages include dual-platform play (Mac/PC) for select EA titles and a unified (Mac/PC) Game Library. Origin features for mobile are built into supported games so there's no need to download a standalone app.">
     *     </origin-store-accordionlist-item>
     * </origin-store-accordionlist>
     */
    angular.module('origin-components')
        .directive('originStoreAccordionlistItem', originStoreAccordionlistItem);
}());
