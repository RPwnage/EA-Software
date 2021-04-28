/**
 * @file store/pdp/comparisontable-wrapper.js
 */
(function () {
    'use strict';

    /**
     * BooleanTypeEnumeration list of allowed types
     * @enum {string}
     */
    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-store-pdp-comparison-table-wrapper';


    function originStorePdpComparisonTableWrapperCtrl($scope) {

        var callbacks = [];

        $scope.isVisible = false;

        this.setVisibility = function(visible) {
            $scope.isVisible = visible;
        };

        this.registerComparisonItem = function(callback) {
            callbacks.push(callback);
            //If callback registration happens after the data is loaded, we want to call the callback right away for initial call
            if (!_.isEmpty($scope.models)){
                callback($scope.models);
            }
        };

        this.notifyAllComparisonItems = function(model) {
            _.forEach(callbacks, function(callback) {
                callback(model);
            });
        };

        function onDestroy() {
            callbacks = [];
        }

        $scope.$on('$destroy', onDestroy);
    }

    function originStorePdpComparisonTableWrapper(
        ComponentsConfigFactory,
        DirectiveScope,
        ObjectHelperFactory,
        OcdPathFactory,
        UserObserverFactory,
        UtilFactory,
        PdpFactory
    ) {

        function originStorePdpComparisonTableWrapperLink(scope, element, attributes, controller) {

            var getLength = ObjectHelperFactory.length;

            function getLocalizedString(key) {
                return UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, key);
            }

            scope.getLocalizedEdition = UtilFactory.getLocalizedEditionName;
            scope.models = {};
            scope.noVault = true;
            scope.buyNow = getLocalizedString('buynow');
            scope.youOwnThis = getLocalizedString('you-own-this');
            scope.vaultFlag = getLocalizedString('vault-flag');
            scope.download = getLocalizedString('download');
            scope.buy = getLocalizedString('buy');
            scope.upgrade = getLocalizedString('upgrade');
            UserObserverFactory.getObserver(scope, 'user');

            // check if user owns a lesser edition of the vault game in this table
            function ownsLesserEdition(model) {
                var ownsLesser = false;

                if(getLength(scope.models) > 0) {
                    ObjectHelperFactory.forEach(function(attributeName) {
                        if(attributeName.isOwned && attributeName.weight && attributeName.gameKey && attributeName.franchiseKey) {
                            if(attributeName.isOwned === true && attributeName.weight < model.weight && attributeName.gameKey === model.gameKey && attributeName.franchiseKey === model.franchiseKey) {
                                ownsLesser = true;
                            }
                        }

                        if(attributeName.subscriptionAvailable) {
                            scope.noVault = false;
                        }

                    }, scope.models);
                }

                return ownsLesser;
            }

            // check if the cta should be a buy button
            scope.isBuyCta = function(ocdPath) {
                return !scope.models[ocdPath].isOwned && (!scope.user.isSubscriber || !scope.models[ocdPath].subscriptionAvailable);
            };

            // check if cta should be a get it now button
            scope.isGetCta = function(ocdPath) {
                var ownsLesser = ownsLesserEdition(scope.models[ocdPath]);

                return !scope.models[ocdPath].isOwned && scope.user.isSubscriber && scope.models[ocdPath].subscriptionAvailable && !ownsLesser;
            };

            // check if the cta should be a upgrade button
            scope.isUpgradeCta = function(ocdPath) {
                var ownsLesser = ownsLesserEdition(scope.models[ocdPath]);

                return !scope.models[ocdPath].isOwned && scope.user.isSubscriber && scope.models[ocdPath].subscriptionAvailable && ownsLesser;
            };

            scope.isPlayAction = function(ocdPath) {
                return scope.models[ocdPath].isOwned && scope.models[ocdPath].vaultEntitlement;
            };

            /**
             * check if user owns a better edition
             *
             * @param model observable model data
             */
            scope.ownsBetter = function(model) {
               return PdpFactory.ownsGreaterEdition(scope.models, model);
            };

            /**
             * apply model data
             *
             * @param data observable models collection
             */
            function applyModelData(data) {
                if (data) {
                    scope.models = data;
                    scope.ocdPathLength = _.size(scope.models);
                    scope.ocdPaths = Object.keys(scope.models);
                    controller.notifyAllComparisonItems(scope.models);
                    controller.setVisibility(true);
                }
            }

            function init() {
                var ocdPaths = _.compact([scope.comparisonTableOcdPath1, scope.comparisonTableOcdPath2, scope.comparisonTableOcdPath3, scope.comparisonTableOcdPath4]);
                if (ocdPaths.length > 0) {
                    OcdPathFactory.get(ocdPaths).attachToScope(scope, applyModelData);
                }
            }

            DirectiveScope.populateScope(scope, CONTEXT_NAMESPACE).then(function(anyDataFound) {
                if (anyDataFound) {
                    init();
                }
            });
        }


        return {
            restrict: 'E',
            transclude: true,
            scope: {
                titleStr: '@',
                anchorName: '@',
                showOnNav: '@',
                headerTitle: '@',
                comparisonTableTitle: '@',
                comparisonTableSubtitle: '@',
                comparisonTableFlag: '@',
                comparisonTableOcdPath1: '@',
                comparisonTableOcdPath2: '@',
                comparisonTableOcdPath3: '@',
                comparisonTableOcdPath4: '@'
            },
            link: originStorePdpComparisonTableWrapperLink,
            controller: originStorePdpComparisonTableWrapperCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/comparisontable-wrapper.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpComparisonTableWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description wrapper to hide/show header/nav item when no data is available

     *
     * @param {BooleanEnumeration} show-on-nav determines whether or not this navigation item should show on a navigation bar
     * @param {LocalizedString} buynow *Change defaults to edit
     * @param {LocalizedString} comparison-table-flag An optional flag to show over the right most ocdPath in the table
     * @param {LocalizedString} comparison-table-subtitle The subtitle for the comparison table
     * @param {LocalizedString} comparison-table-title The main title for the comparison table
     * @param {LocalizedString} download *Change defaults to edit
     * @param {LocalizedString} header-title title of this section
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {OcdPath} comparison-table-ocd-path1 First ocdPath in the table.
     * @param {OcdPath} comparison-table-ocd-path2 Second ocdPath in the table.
     * @param {OcdPath} comparison-table-ocd-path3 Third ocdPath in the table.
     * @param {OcdPath} comparison-table-ocd-path4 Fourth ocdPath in the table.
     * @param {string} anchor-name the hash anchor name to link to
     * @param {LocalizedString} buy *buy string
     * @param {LocalizedString} upgrade *The upgrade string
     * @param {LocalizedString} vault-flag *The vault Flag
     * @param {LocalizedString} you-own-this *The you own this string
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-comparison-table-wrapper></origin-store-pdp-comparison-table-wrapper>
     * </origin-store-row>
     */

    angular.module('origin-components')
        .directive('originStorePdpComparisonTableWrapper', originStorePdpComparisonTableWrapper);
}());

