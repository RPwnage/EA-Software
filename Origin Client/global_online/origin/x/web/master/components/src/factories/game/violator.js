/**
 * @file game/violator.js
 */
(function() {

    'use strict';

    function GameViolatorFactory(GameAutomaticViolatorTestsFactory, GameProgrammedViolatorFactory, GameViolatorModelFactory, ObjectHelperFactory, FunctionHelperFactory) {
        var map = ObjectHelperFactory.map,
            toArray = ObjectHelperFactory.toArray,
            executeWith = FunctionHelperFactory.executeWith;

        /**
         * Priority level enum
         * @type {Object}
         */
        var priority = {
            critical: 'critical',
            high: 'high',
            normal: 'normal',
            low: 'low'
        };

        /**
         * The order of prioritization
         * @type {Array}
         */
        var priorityOrder = [
            priority.critical,
            priority.high,
            priority.normal,
            priority.low
        ];

        /**
         * The view type enum
         * @type {Object}
         */
        var viewType = {
            gametile: 'gametile',
            importantinfo: 'importantinfo'
        };

        /**
         * the violator type enum
         * @type {Object}
         */
        var violatorType = {
            programmed: 'programmed',
            automatic: 'automatic'
        };

        /**
         * The types of merchandised programmed violators / important info
         * @type {Object[]}
         */
        var programmedViolators = {
            'gametile': {
                parentDirectiveName: 'origin-game-tile',
                directiveName: 'origin-game-violator-programmed',
                violatorType: violatorType.programmed,
                getter: GameProgrammedViolatorFactory.getContent
            },
            'importantinfo': {
                parentDirectiveName: 'origin-gamelibrary-ogd-header',
                directiveName: 'origin-gamelibrary-ogd-important-info',
                violatorType: violatorType.programmed,
                getter: GameProgrammedViolatorFactory.getContent
            }
        };

        /**
         * The list of automatic violators in priority sequence
         * @type {Object[]}
         */
        var violatorList = [
            {
                label: 'gameexpires',
                filter: GameAutomaticViolatorTestsFactory.gameExpires,
                enddateDataSource: 'entitlement',
                enddateProperty: 'terminationDate',
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'trialexpired',
                filter: GameAutomaticViolatorTestsFactory.trialExpired,
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'preloadon',
                filter: GameAutomaticViolatorTestsFactory.preloadOn,
                enddateDataSource: 'catalog',
                enddateProperty: 'preloadDate',
                priority: priority.high,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'preloadavailable',
                filter: GameAutomaticViolatorTestsFactory.preloadAvailable,
                priority: priority.high,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'releaseson',
                filter: GameAutomaticViolatorTestsFactory.releasesOn,
                enddateDataSource: 'catalog',
                enddateProperty: 'releaseDate',
                priority: priority.high,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'updateavailable',
                filter: GameAutomaticViolatorTestsFactory.updateAvailable,
                priority: priority.high,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'downloadoverride',
                filter: GameAutomaticViolatorTestsFactory.newDlcAvailable,
                priority: priority.high,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'trialnotactivated',
                filter: GameAutomaticViolatorTestsFactory.trialNotActivated,
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'trialnotexpired',
                filter: GameAutomaticViolatorTestsFactory.trialNotExpired,
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'newdlcavailable',
                filter: GameAutomaticViolatorTestsFactory.newDlcExpansionAvailable,
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'newdlcexpansionavailable',
                filter: GameAutomaticViolatorTestsFactory.newDlcReadyForInstall,
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'newdlcreadyforinstall',
                filter: GameAutomaticViolatorTestsFactory.newDlcInstalled,
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }
        ];

        /**
         * Map violator filter functions into an array
         * @param {Object} violatorListItem an item from the violator list
         * @return {function} the violator function
         */
        function mapFilters(violatorListItem) {
            return violatorListItem.filter;
        }

        /**
         * Given an array collection find all priority level matches
         * @param  {Object[]} collection a list of items to check
         * @param  {string} priorityLevel the priority level to match
         * @return {Object[]} matched records
         */
        function findByPriority(collection, priorityLevel) {
            var matches = [];
            if(!collection || !collection.length) {
                return [];
            }

            for(var i = 0; i < collection.length; i++) {
                if(collection[i].priority === priorityLevel) {
                    matches.push(collection[i]);
                }
            }

            return matches;
        }

        /**
         * Accumulate the resulting violators into their final priority sequence
         * @param {Array.<Object[],Object[]>} data [automaticViolators, programmedViolators]
         * @return {Object[]} the merged violator result
         */
        function interleaveProgrammedAndAutomaticViolators(data) {
            var automaticViolators = data[0],
                programmedViolators = data[1],
                interleavedList = [];

            for(var i = 0; i < priorityOrder.length; i++) {
                var priorityLevel = priorityOrder[i];
                interleavedList = interleavedList.concat(findByPriority(programmedViolators, priorityLevel));
                interleavedList = interleavedList.concat(findByPriority(automaticViolators, priorityLevel));
            }

            return interleavedList;
        }

        /**
         * Choose the applicable violator list items
         * @param  {Array} result the test result matrix
         * @return {Object[]} an object array eg. [{label: 'preloadon',...}, ...]
         */
        function filterViolatorList(result) {
            var viewType = result.pop(),
                entitlementData = result.pop(),
                catalogData = result.pop(),
                applicableViolators = [];

            if(!result.length || result.length !== violatorList.length) {
                return applicableViolators;
            }

            for (var i = 0; i < violatorList.length; i++) {
                if (result[i] && violatorList[i].context.indexOf(viewType) > -1) {
                    applicableViolators.push(violatorList[i]);
                }
            }

            applicableViolators.push(catalogData);
            applicableViolators.push(entitlementData);

            return applicableViolators;
        }

        /**
         * Resolve end date properties if set
         * @param  {Object[]} violatorList the filtered list of violators
         * @return {Promise.<Object[], Error>
         */
        function resolveEndDates(violatorList) {
            var entitlementData = violatorList.pop(),
                catalogData = violatorList.pop(),
                collection = {catalog: catalogData, entitlement: entitlementData};

            if(!violatorList || !violatorList.length) {
                return violatorList;
            }

            for(var i = 0; i < violatorList.length; i++) {
                if( violatorList[i].enddateDataSource &&
                    violatorList[i].enddateProperty &&
                    collection[violatorList[i].enddateDataSource] &&
                    collection[violatorList[i].enddateDataSource][violatorList[i].enddateProperty]) {
                    violatorList[i].endDate = collection[violatorList[i].enddateDataSource][violatorList[i].enddateProperty];
                }
            }

            return violatorList;
        }

        /**
         * Retrieve a list of violators from the game/dlc states from the violator list config
         * @param {string} offerId the offerId to test
         * @param {string} viewType a member of viewType
         * @return {Promise.<Object[], Error>} an object array eg. [{label: 'preloadon',...}, ...]
         */
        function getAutomaticViolators(offerId, viewType) {
            var filterFunctions = violatorList.map(mapFilters),
                promiseChain = toArray(map(executeWith(offerId), filterFunctions));

            promiseChain.push(GameViolatorModelFactory.getCatalog(offerId));
            promiseChain.push(GameViolatorModelFactory.getEntitlement(offerId));
            promiseChain.push(viewType);

            return Promise.all(promiseChain)
                .then(filterViolatorList)
                .then(resolveEndDates);
        }

        /**
         * Get Manually added textual violators from OCD
         * @param {string} offerId the offer to analyze
         * @param {string} viewType a member of viewType
         * @return {Promise.<Object[], Error>} Resolves to an array of programmed violators
         */
        function getProgrammedViolators(offerId, viewType) {
            if(!programmedViolators[viewType]) {
                Promise.reject('illegal view type: ' + viewType);
            }

            return programmedViolators[viewType].getter.call(
                this,
                offerId,
                programmedViolators[viewType].parentDirectiveName,
                programmedViolators[viewType].directiveName
            );
        }

        /**
         * Get violators for a specific view context
         * @param  {string} offerId  the offer ID to lookup
         * @param  {string} viewType a member of viewType eg. gametile | importantinfo
         * @return {Promise.<Object[], Error>} resolves to a conclusive list of violators
         */
        function getViolators(offerId, viewType){
            return Promise.all([
                getAutomaticViolators(offerId, viewType),
                getProgrammedViolators(offerId, viewType)
            ]).then(interleaveProgrammedAndAutomaticViolators);
        }

        return {
            getViolators: getViolators
        };
    }

    /**
    * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
    * @param {object} SingletonRegistryFactory the factory which manages all the other factories
    */
    function GameViolatorFactorySingleton(GameAutomaticViolatorTestsFactory, GameProgrammedViolatorFactory, GameViolatorModelFactory, ObjectHelperFactory, FunctionHelperFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GameViolatorFactory', GameViolatorFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GameViolatorFactory

     * @description
     *
     * GameViolatorFactory
     */
    angular.module('origin-components')
        .factory('GameViolatorFactory', GameViolatorFactorySingleton);
}());