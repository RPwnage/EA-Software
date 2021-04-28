/**
 * @file store/pdp/comparison/scripts/item.js
 */
(function(){
    'use strict';

    var PARENT_CONTEXT_NAMESPACE = 'origin-store-pdp-comparison-table-wrapper';
    var CONTEXT_NAMESPACE = 'origin-store-pdp-comparison-item';


    /* jshint ignore:start */
    /**
     * Selects the check marks row
     * @readonly
     * @enum {string}
     */
    var ChecksEnumeration = {
        "Checked": "checked",
        "Not Checked": "not-checked",
        "Custom": "custom"
    };
    /**
     * Selects the checkboxes
     * @readonly
     * @enum {string}
     */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function originStorePdpComparisonItem(ComponentsConfigFactory, DirectiveScope,  UtilFactory) {

        function originStorePdpComparisonItemLink(scope, element, attributes, originStorePdpComparisonTableWrapperCtrl) {
            scope.getLocalizedEdition = UtilFactory.getLocalizedEditionName;
            scope.show = false;

            /**
             * build checklist array for this table item
             */
            function buildChecklist() {

                return [
                    {
                        check: scope.check1 ? scope.check1 : 'not-checked',
                        value: scope.check1value ? scope.check1value : null
                    },
                    {
                        check: scope.check2 ? scope.check2 : 'not-checked',
                        value: scope.check2value ? scope.check2value : null
                    },
                    {
                        check: scope.check3 ? scope.check3 : 'not-checked',
                        value: scope.check3value ? scope.check3value : null
                    },
                    {
                        check: scope.check4 ? scope.check4 : 'not-checked',
                        value: scope.check4value ? scope.check4value : null
                    }];
            }

            /**
             * update scope variables with offer data
             * @param offers observable models collection
             */
            function updateModels(offers) {
                scope.models = offers;
                scope.offerLength = _.size(scope.models);
                scope.offers = Object.keys(scope.models);
                scope.checklist = buildChecklist();
            }

            /**
             * toggle extra content area
             */
            scope.toggleContent = function() {
                scope.show = !scope.show;
            };

            function evaluateScope() {
                // only display row if there is a title and/or a description
                if (scope.description || scope.titleStr) {
                    scope.showMore = UtilFactory.getLocalizedStr(scope.showMore, CONTEXT_NAMESPACE, 'showMore');
                    scope.showLess = UtilFactory.getLocalizedStr(scope.showLess, CONTEXT_NAMESPACE, 'showLess');
                    scope.vaultFlag = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'vault-flag');
                    scope.colspan = scope.image ? 1 : 2;
                    scope.anyDataFound = true;

                    originStorePdpComparisonTableWrapperCtrl.registerComparisonItem(updateModels);
                }
            }

            DirectiveScope.populateScope(scope, [PARENT_CONTEXT_NAMESPACE, CONTEXT_NAMESPACE]).then(evaluateScope);
        }

        return {
            restrict: 'E',
            replace: true,
            require: '^originStorePdpComparisonTableWrapper',
            scope: {
                check1: '@',
                check1value: '@',
                check2: '@',
                check2value: '@',
                check3: '@',
                check3value: '@',
                check4: '@',
                check4value: '@',
                description: '@',
                image: '@',
                more: '@',
                showLess: '@',
                showMore: '@',
                titleStr: '@',
                vaultFlag: '@',
                disallowedFromVault: '@'
            },
            link: originStorePdpComparisonItemLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/comparison/views/item.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpComparisonItem
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} title-str The title for this table item
     * @param {LocalizedString} description The description paragraph for this table item
     * @param {LocalizedString} more The overflow text to be displayed in the hidden part of the table item
     * @param {ImageUrl} image The small image for this table item
     * @param {ChecksEnumeration} check1 Select to display a check, x-mark or custom text
     * @param {ChecksEnumeration} check2 Select to display a check, x-mark or custom text
     * @param {ChecksEnumeration} check3 Select to display a check, x-mark or custom text
     * @param {ChecksEnumeration} check4 Select to display a check, x-mark or custom text
     * @param {LocalizedString} check1value The custom text inserted for offer 1
     * @param {LocalizedString} check2value The custom text inserted for offer 2
     * @param {LocalizedString} check3value The custom text inserted for offer 3
     * @param {LocalizedString} check4value The custom text inserted for offer 4
     * @param {LocalizedString} show-more The "Show More" button text
     * @param {LocalizedString} show-less The "Show Less" button text
     * @param {LocalizedString} vault-flag The vault flag
     * @param {BooleanEnumeration} disallowed-from-vault If checked, this item will not be displayed as part of subscription
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-pdp-comparison-item
     *          title-str="Battlefield 4 Premium"
     *          description="Get the edge on the battlefield. Premium membership awards a huge assortment of benefits."
     *          more="<br>Benefits include:<br><br><strong>All 5 Expansions:</strong> Get China Rising, Second Assault, Naval Strike, Dragon's Teeth, and Final Stand expansion packs.<br><br><strong>Priority Position:</strong> Gants priority position in the server queue, letting you get in the fight more quickly.<br><br><strong>Personalization Options:</strong> Appearance should be as unique as play style. Express your identity on the battlefield with Premium-exclusive camos, paints, emblems and dogtags.<br><br><strong>Weekly Content:</strong> Level up faster with Premium-exclusive Double XP events and get more out of Battlefield 4 with weekly updates of new content."
     *          image="https://docs.x.origin.com/proto/images/compare/bf4_premium.png"
     *          check1="checked"
     *          check2="checked"
     *          check3="no-checked"
     *          check4="custom"
     *          check1value="some i18n value"
     *          check2value="some i18n value"
     *          check3value="some i18n value"
     *          check4value="some i18n value"
     *          show-more="Show More"
     *          show-less="Show Less">
     *      </origin-store-pdp-comparison-item>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpComparisonItem', originStorePdpComparisonItem);
}());