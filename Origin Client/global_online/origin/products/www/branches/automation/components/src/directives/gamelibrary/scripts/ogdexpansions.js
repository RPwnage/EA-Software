/**
 * @file gamelibrary/scripts/ogdexpansions.js
 */
(function () {

    'use strict';
    /**
     * Game Library OGD Expansions controller function
     * @controller OriginGamelibraryOgdExpansionsCtrl
     */
    function OriginGamelibraryOgdExpansionsCtrl($scope, GamesDataFactory, UtilFactory, GameRefiner) {

        var CONTEXT_NAMESPACE = 'origin-gamelibrary-ogd-expansions',
            EXPANSION_SORTORDER = 0,
            ADDON_SORTORDER = Number.MAX_VALUE - 1,
            CURRENCY_SORTORDER = Number.MAX_VALUE,
            expansionLoc = UtilFactory.getLocalizedStr($scope.expansionStr, CONTEXT_NAMESPACE, 'expansionstr'),
            addonLoc = UtilFactory.getLocalizedStr($scope.addonStr, CONTEXT_NAMESPACE, 'addonstr'),
            currencyLoc = UtilFactory.getLocalizedStr($scope.currencyStr, CONTEXT_NAMESPACE, 'currencystr');

        function isExpansion(item) {
            return (GameRefiner.isExtraContent(item) && GameRefiner.isExpansion(item));
        }

        function isAddon(item) {
            return (GameRefiner.isExtraContent(item) && GameRefiner.isAddon(item));
        }

        function isCurrency(item) {
            return GameRefiner.isCurrency(item);
        }

        $scope.noExpansionsTitleLoc = UtilFactory.getLocalizedStr($scope.noExpansionsTitleStr, CONTEXT_NAMESPACE, 'noexpansionstitlestr');
        $scope.noExpansionsDescLoc = UtilFactory.getLocalizedStr($scope.noExpansionsDescStr, CONTEXT_NAMESPACE, 'noexpansionsdescstr');

        $scope.loading = true;

        $scope.expansionGroups = []; // This is an ordered array containing each expansion group

        function setScopeVars(expansions, debug) {
            groupExpansions(expansions);
            $scope.showDebugInfo = !!debug;
        }

        /**
         * Add an expansion to the sorted expansion groups array
         * @param {Object} expansion - The expansion content
         * @param {String} group - the group name
         * @param {String} displayName - the string to display for this group (i18n)
         * @param {String} sortOrder - the sort order of this group (can be undefined)
         *
         * @method addToSortedExpansionGroupsArray
         */
        function addToSortedExpansionGroupsArray(expansion, group, displayName, sortOrder) {

            /**
             * @param {Object} group - the expansion group object, defined below
             * @return {Boolean} - Return true if this groupName matches group
             *
             * @method findGroup
             */
            function findGroup(g) {
                return g.groupName === group;
            }

            /**
             * @param {Object} group - the expansion group object, as defined below
             *
             * @return {Boolean} - Return true if this group object should be inserted before this group
             * @method findSortOrderIndex
             */
            function findSortOrderIndex(g) {
                return sortOrder < g.sortOrder;
            }

            expansion.releaseDate = _.get(expansion, ['platforms', Origin.utils.os(), 'releaseDate']);

            var groupObj = { groupName: group,
                             sortOrder: sortOrder,
                             displayName: displayName,
                             extraContent: [expansion]},
                sortIndex = -1,
                index = _.findIndex( $scope.expansionGroups, findGroup);

            // If index >= 0, there is already a group with this name
            if (index < 0) {
                sortIndex = _.findIndex($scope.expansionGroups, findSortOrderIndex);
                if (sortIndex >= 0) {
                    $scope.expansionGroups.splice(sortIndex, 0, groupObj);
                } else {
                    $scope.expansionGroups.push(groupObj);
                }

            } else {
                // Add this expansion to the group
                $scope.expansionGroups[index].extraContent.push(expansion);
            }

        }

        /**
         * Loop through expansion content for this offer, and add to the sorted expansions group array
         * @param {Object} expansions - The list of expansion content for this offer
         *
         * @method groupExpansions
         */
        function groupExpansions(expansions) {
            if (!_.isUndefined(expansions)) {
                //console.debug(expansions);
                Object.keys(expansions).forEach(function(key) {
                    if (_.isArray($scope.whitelist) && $scope.whitelist.length > 0 && $scope.whitelist.indexOf(key) === -1) {
                        return;
                    }
                    if (_.isArray($scope.blacklist) && $scope.blacklist.indexOf(key) >= 0) {
                        return;
                    }

                    var item = _.cloneDeep(expansions[key]);

                    if ( (item.countries.isPurchasable && !Origin.user.underAge()) ||
                        ( GamesDataFactory.ownsEntitlement(item.offerId))) {

                        if (item.extraContentDisplayGroup) {
                            addToSortedExpansionGroupsArray(item, item.extraContentDisplayGroup, item.i18n.extraContentDisplayGroupDisplayName, item.extraContentDisplayGroupSortAsc);
                        } else if (isAddon(item)) {
                            addToSortedExpansionGroupsArray(item, item.originDisplayType, addonLoc, ADDON_SORTORDER);
                        } else if (isExpansion(item)) {
                            addToSortedExpansionGroupsArray(item, item.originDisplayType, expansionLoc, EXPANSION_SORTORDER);
                        } else if (isCurrency(item) && $scope.context === 'addonstore') {
                            addToSortedExpansionGroupsArray(item, item.originDisplayType, currencyLoc, CURRENCY_SORTORDER);
                        }
                    }
                });
            }
        }

        function clearLoadingFlag() {
            $scope.loading = false;
            $scope.$digest();
        }

        Promise.all([
                GamesDataFactory.retrieveExtraContentCatalogInfo($scope.offerId),
                Origin.client.info.hasEACoreIni()
            ])
            .then(_.spread(setScopeVars))
            .then(clearLoadingFlag);
    }

    /**
      * directive function
      * @param {Object} ComponentsConfigFactory
      * @return {Object} directive definition object
      *
      * @method originGamelibraryOgdExpansions
      */
    function originGamelibraryOgdExpansions(ComponentsConfigFactory) {

        var directiveDefintionObj = {
            restrict: 'E',
            controller: 'OriginGamelibraryOgdExpansionsCtrl',
            scope: {
                offerId: '@offerid',
                expansionStr: '@expansionstr',
                addonStr: '@addonstr',
                currencyStr: '@currencystr',
                noExpansionsTitleStr: '@noexpansionstitlestr',
                noExpansionsDescStr: '@noexpansionsdescstr',
                context: '@',
                whitelist: '=',
                blacklist: '='
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdexpansions.html')
        };
        return directiveDefintionObj;
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgdExpansions
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid of the game
     * @param {LocalizedString} expansionstr "Expansions"
     * @param {LocalizedString} addonstr "Addons"
     * @param {LocalizedString} currencystr "Currency"
     * @param {LocalizedString} noexpansionstitlestr "No expansions found"
     * @param {LocalizedString} noexpansionsdescstr "Sorry this game has no expansions"
     * @param {string} context optional parameter that determines styling, e.g. 'addonstore'
     * @param {[string]} whitelist optional list of offers to display.  If this list is non-empty, excluded offers will not be displayed.
     * @param {[string]} blacklist optional list of offers not to display
     * @description
     *
     * OGD - Expansions tab
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-ogd-expansions offerid='OFR-ID-1234' noexpansionsdescstr="Sorry this game has no expansions" noexpansionstitlestr="No expansions found" addonstr="Add-ons" expansionstr="Expansions" currencystr="Currency"
     *         ></origin-gamelibrary-ogd-expansions>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGamelibraryOgdExpansionsCtrl', OriginGamelibraryOgdExpansionsCtrl)
        .directive('originGamelibraryOgdExpansions', originGamelibraryOgdExpansions);
}());
