/**
 * @file game/violator.js
 */
(function() {

    'use strict';


    var STORAGE_KEY_PREFIX = 'vltr', //The local storage prefix name
        DISMISS_TIMEOUT_IN_DAYS = 30; //Timeout in 30 days so that automatic violators will show up again eventually

    function GameViolatorFactory(ViolatorBaseGameFactory, ViolatorDlcFactory, ViolatorPlatformFactory, ViolatorLegacyTrialFactory, ViolatorVaultFactory, ViolatorProgrammedFactory, ViolatorModelFactory, ObjectHelperFactory, FunctionHelperFactory, ComponentsLogFactory, DateHelperFactory, Md5Factory, LocalStorageFactory) {
        var map = ObjectHelperFactory.map,
            toArray = ObjectHelperFactory.toArray,
            where = ObjectHelperFactory.where,
            filterCollectionBy = ObjectHelperFactory.filterCollectionBy,
            executeWith = FunctionHelperFactory.executeWith,
            currentPlatform = Origin.utils.os(),
            isValidDate = DateHelperFactory.isValidDate,
            isInTheFuture = DateHelperFactory.isInTheFuture,
            isInThePast = DateHelperFactory.isInThePast,
            addDays = DateHelperFactory.addDays,
            createStorageKey = Md5Factory.createStorageKey,
            events = new window.Origin.utils.Communicator();

        /**
         * Compatible platforms checks
         * @type {Object}
         */
        var platform = {
            pcwin: 'PCWIN',
            mac: 'MAC'
        };

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
            importantinfo: 'importantinfo',
            myhometrial: 'myhometrial'
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
                getter: ViolatorProgrammedFactory.getContent
            },
            'importantinfo': {
                parentDirectiveName: 'origin-gamelibrary-ogd-header',
                directiveName: 'origin-gamelibrary-ogd-important-info',
                violatorType: violatorType.programmed,
                getter: ViolatorProgrammedFactory.getContent
            },
            'myhometrial': {
                parentDirectiveName: 'origin-gamelibrary-ogd-header',
                directiveName: 'origin-gamelibrary-ogd-important-info',
                violatorType: violatorType.programmed,
                getter: ViolatorProgrammedFactory.getContent
            }            
        };

        /**
         * The list of automatic violators in priority sequence
         * @type {Object[]}
         */
        var violatorList = [
            {
                label: 'offlinemodegametile',
                filter: ViolatorVaultFactory.offlineTrial,
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.gametile]
            }, {
                label: 'offlinemode',
                filter: ViolatorVaultFactory.offlineTrial,
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'downloadoverride',
                filter: ViolatorBaseGameFactory.downloadOverride,
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'preorderfailed',
                filter: ViolatorPlatformFactory.preorderFailed(currentPlatform),
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.gametile]
            }, {
                label: 'preorderfailedogd',
                filter: ViolatorPlatformFactory.preorderFailed(currentPlatform),
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'oaexpired',
                filter: ViolatorVaultFactory.subscriptionExpired,
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'gameexpires',
                filter: ViolatorBaseGameFactory.gameExpires,
                enddateDataSource: 'entitlement',
                enddateProperty: 'terminationDate',
                priority: priority.critical,
                violatorType: violatorType.automatic,
                timerformat: 'trial',
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'oagameexpiringgametile',
                filter: ViolatorPlatformFactory.vaultGameExpiresOn(currentPlatform),
                enddateDataSource: 'catalog',
                enddateProperty: ['platforms', currentPlatform, 'originSubscriptionUseEndDate'],
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.gametile]
            }, {
                label: 'oagameexpiring',
                filter: ViolatorPlatformFactory.vaultGameExpiresOn(currentPlatform),
                enddateDataSource: 'catalog',
                enddateProperty: ['platforms', currentPlatform, 'originSubscriptionUseEndDate'],
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'expireson',
                filter: ViolatorPlatformFactory.expiresOn(currentPlatform),
                enddateDataSource: 'catalog',
                enddateProperty: ['platforms', currentPlatform, 'useEndDate'],
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'oagameexpiredgametile',
                filter: ViolatorPlatformFactory.vaultGameExpired(currentPlatform),
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.gametile]
            }, {
                label: 'oagameexpired',
                filter: ViolatorPlatformFactory.vaultGameExpired(currentPlatform),
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'expired',
                filter: ViolatorPlatformFactory.expired(currentPlatform),
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'preloaded',
                filter: ViolatorPlatformFactory.preloaded(currentPlatform),
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'oatrialexpiredgametile',
                filter: ViolatorVaultFactory.trialExpired,
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.gametile]
            }, {
                label: 'oatrialexpired',
                filter: ViolatorVaultFactory.trialExpiredAndVisibleNotReleased,
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'oatrialexpiredreleased',
                filter: ViolatorVaultFactory.trialExpiredAndVisibleReleased,
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'oatrialexpiredhidden',
                filter: ViolatorVaultFactory.trialExpiredAndHiddenNotReleased,
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'oatrialexpiredhiddenreleased',
                filter: ViolatorVaultFactory.trialExpiredAndHiddenReleased,
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'oaexpiredgametile',
                filter: ViolatorVaultFactory.subscriptionExpired,
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.gametile]
            }, {
                label: 'legacytrialexpired',
                filter: ViolatorLegacyTrialFactory.trialExpired,
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'offlineplayexpiredgametile',
                filter: ViolatorVaultFactory.offlinePlayExpired,
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.gametile]
            }, {
                label: 'offlineplayexpired',
                filter: ViolatorVaultFactory.offlinePlayExpired,
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            },  {
                label: 'oagameupgradenoticegametile',
                filter: ViolatorVaultFactory.vaultGameUpgrade,
                priority: priority.high,
                violatorType: violatorType.automatic,
                context: [viewType.gametile]
            }, {
                label: 'oagameupgradenotice',
                filter: ViolatorVaultFactory.vaultGameUpgrade,
                priority: priority.high,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'preloadon',
                filter: ViolatorPlatformFactory.preloadOn(currentPlatform),
                enddateDataSource: 'catalog',
                enddateProperty: ['platforms', currentPlatform, 'downloadStartDate'],
                priority: priority.high,
                violatorType: violatorType.automatic,
                context: [viewType.gametile]
            }, {
                label: 'preloadavailable',
                filter: ViolatorPlatformFactory.preloadAvailable(currentPlatform),
                priority: priority.high,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'justreleased',
                filter: ViolatorPlatformFactory.justReleased(currentPlatform),
                priority: priority.high,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'releaseson',
                filter: ViolatorPlatformFactory.releasesOn(currentPlatform),
                enddateDataSource: 'catalog',
                enddateProperty: ['platforms', currentPlatform, 'releaseDate'],
                priority: priority.high,
                violatorType: violatorType.automatic,
                context: [viewType.gametile]
            }, {
                label: 'updateavailable',
                filter: ViolatorBaseGameFactory.updateAvailable,
                priority: priority.high,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'oatrialnotactivated',
                filter: ViolatorVaultFactory.trialNotActivated,
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'oatrialnotexpiredgametile',
                filter: ViolatorVaultFactory.trialNotExpired,
                priority: priority.normal,
                violatorType: violatorType.automatic,
                timerformat: 'trial',
                context: [viewType.gametile]
            }, {
                label: 'oatrialnotexpired',
                filter: ViolatorVaultFactory.trialNotExpiredNotReleased,
                priority: priority.normal,
                violatorType: violatorType.automatic,
                timerformat: 'trial',
                context: [viewType.importantinfo, viewType.myhometrial]
            }, {
                label: 'oatrialnotexpiredreleased',
                filter: ViolatorVaultFactory.trialNotExpiredReleased,
                priority: priority.normal,
                violatorType: violatorType.automatic,
                timerformat: 'trial',
                context: [viewType.importantinfo, viewType.myhometrial]
            }, {
                label: 'legacytrialnotactivated',
                filter: ViolatorLegacyTrialFactory.trialNotActivated,
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'legacytrialnotexpired',
                filter: ViolatorLegacyTrialFactory.trialNotExpired,
                priority: priority.normal,
                enddateDataSource: 'entitlement',
                enddateProperty: 'terminationDate',
                violatorType: violatorType.automatic,
                timerformat: 'trial',
                context: [viewType.gametile, viewType.importantinfo, viewType.myhometrial]
            }, {
                label: 'offlineplayexpiringgametile',
                filter: ViolatorVaultFactory.offlinePlayExpiring,
                enddateDataSource: 'client',
                enddateProperty: 'offlinePlayExpirationDate',
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.gametile]
            }, {
                label: 'offlineplayexpiring',
                filter: ViolatorVaultFactory.offlinePlayExpiring,
                enddateDataSource: 'client',
                enddateProperty: 'offlinePlayExpirationDate',
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'newdlcavailable',
                filter: ViolatorDlcFactory.newDlcAvailable,
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'newdlcreadyforinstall',
                filter: ViolatorDlcFactory.newDlcReadyForInstall,
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'newdlcexpansionreadyforinstall',
                filter: ViolatorDlcFactory.newDlcExpansionReadyForInstall,
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'newdlcinstalled',
                filter: ViolatorDlcFactory.newDlcInstalled,
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.gametile, viewType.importantinfo]
            }, {
                label: 'oaplatformwin',
                filter: ViolatorPlatformFactory.hasVaultMacAlternative(currentPlatform),
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'platformwin',
                filter: ViolatorPlatformFactory.compatibleSinglePlatform(platform.pcwin),
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'platformmac',
                filter: ViolatorPlatformFactory.compatibleSinglePlatform(platform.mac),
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'platformwinmac',
                filter: ViolatorPlatformFactory.incompatibleHybridPlatform,
                priority: priority.normal,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'wrongarchitecture',
                filter: ViolatorPlatformFactory.wrongArchitecture(currentPlatform),
                priority: priority.critical,
                violatorType: violatorType.automatic,
                context: [viewType.importantinfo]
            }, {
                label: 'preloadonwin',
                filter: ViolatorPlatformFactory.preloadOn(platform.pcwin),
                priority: priority.normal,
                violatorType: violatorType.automatic,
                enddateDataSource: 'catalog',
                enddateProperty: ['platforms', platform.pcwin, 'downloadStartDate'],
                context: [viewType.importantinfo]
            }, {
                label: 'preloadonmac',
                filter: ViolatorPlatformFactory.preloadOn(platform.mac),
                priority: priority.normal,
                violatorType: violatorType.automatic,
                enddateDataSource: 'catalog',
                enddateProperty: ['platforms', platform.mac, 'downloadStartDate'],
                context: [viewType.importantinfo]
            }, {
                label: 'releasesonwin',
                filter: ViolatorPlatformFactory.releasesOn(platform.pcwin),
                priority: priority.normal,
                violatorType: violatorType.automatic,
                enddateDataSource: 'catalog',
                enddateProperty: ['platforms', platform.pcwin, 'releaseDate'],
                context: [viewType.importantinfo]
            }, {
                label: 'releasesonmac',
                filter: ViolatorPlatformFactory.releasesOn(platform.mac),
                priority: priority.normal,
                violatorType: violatorType.automatic,
                enddateDataSource: 'catalog',
                enddateProperty: ['platforms', platform.mac, 'releaseDate'],
                context: [viewType.importantinfo]
            }, {
                label: 'preannouncementdisplaydate',
                filter: ViolatorPlatformFactory.preAnnouncementDisplayDate(currentPlatform),
                priority: priority.normal,
                violatorType: violatorType.automatic,
                enddateDataSource: 'catalog',
                enddateProperty: ['i18n','preAnnouncementDisplayDate'],
                context: [viewType.gametile, viewType.importantinfo]
            }
        ];

        /**
         * Is the violator data in the right fomat?
         * @return {Boolean}
         */
        function isValidViolatorData(violatorData) {
            return _.isArray(violatorData);
        }

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
            return where({'priority': priorityLevel})(collection);
        }

        /**
         * Filter violator list by visible dates
         * @param {Object} item member item of the violator collection/scope
         * @return {Boolean}
         */
        function isWithinViewableDateRange(item) {
            var startDate, endDate;

            if (_.has(item, 'startdate')) {
                startDate = new Date(_.get(item, 'startdate'));
            }

            if (_.has(item, 'enddate')) {
                endDate = new Date(_.get(item, 'enddate'));
            }

            return !(startDate && isInTheFuture(startDate)) && !(endDate && isInThePast(endDate));
        }

        /**
         * Get the storage key for violators
         * @param {string} offerId   the offer id
         * @param {string} priority  priority level
         * @param {string} title     the title message
         * @param {string} endDate   the start date
         * @param {string} startDate the end date
         * @param {string} violatorLabel the automatic violator type or programmed
         * @return {string} an md5 hash with a prefix
         */
        function getKey(offerId, priority, title, endDate, startDate, violatorLabel) {
            var trimmedTitle = '';

            // only use the title key for programmed violators. Automatic violator titles (ones with labels)
            // are dynamic and populated from the CQ5 dictionary above this factory. Trimming whitespace
            // from the title is needed because CQ5 stores whitespace.
            if(!violatorLabel && _.isString(title)) {
                trimmedTitle = title.trim();
            }

            return createStorageKey(STORAGE_KEY_PREFIX, [
                offerId || '',
                priority || '',
                trimmedTitle,
                endDate || '',
                startDate || '',
                violatorLabel || ''
            ]);
        }

        /**
         * Has a message been dismissed?
         * @param {string} offerId   the offer id
         * @param {string} priority  priority level
         * @param {string} title     the title message
         * @param {string} endDate   the start date
         * @param {string} startDate the end date
         * @param {string} violatorLabel the automatic violator type or programmed
         * @return {Boolean}
         */
        function isDismissed(offerId, priority, title, endDate, startDate, violatorLabel) {
            var key = getKey(offerId, priority, title, endDate, startDate, violatorLabel),
                dismissDate = new Date(LocalStorageFactory.get(key, null, true));

            if (isValidDate(dismissDate) &&
                isInTheFuture(addDays(dismissDate, DISMISS_TIMEOUT_IN_DAYS))
            ) {
                return true;
            }

            return false;
        }

        /**
         * Dismiss a message.
         * @param {string} offerId   the offer id
         * @param {string} priority  priority level
         * @param {string} title     the title message
         * @param {string} endDate   the start date
         * @param {string} startDate the end date
         * @param {string} violatorLabel the automatic violator type or programmed
         */
        function dismiss(offerId, priority, title, endDate, startDate, violatorLabel) {
            var key = getKey(offerId, priority, title, endDate, startDate, violatorLabel),
                now = new Date();

            LocalStorageFactory.set(key, now.toISOString(), true);

            events.fire(['violator', 'dismiss', offerId].join(':'));
        }

        /**
         * Notify the violator consumers that an end date has been reached on the
         * UI side, so the model can be fetched and redrawn
         * @param {string} offerId   the offer id
         */
        function enddateReached(offerId) {
            events.fire(['violator', 'enddateReached', offerId].join(':'));
        }

        /**
         * Filter helper for dismissed status
         * @return Function
         */
        function isNotDismissed(offerId) {
            /**
             * Invert the isDimissed functionality to work as a filter
             * @param {Object} item the violator item to analyze
             * @return {Boolean} true if user has NOT dismissed the message before
             */
            return function(item) {
                return !isDismissed(offerId, item.priority, item['title-str'], item.enddate, item.startdate, item.label);
            };
        }

        /**
         * Accumulate the resulting violators into their final priority sequence
         * @param {Array.<Object[],Object[]>} data [automaticViolators, programmedViolators]
         * @return {Object[]} the merged violator result
         */
        function interleaveProgrammedAndAutomaticViolators(automaticViolators, programmedViolators) {
            var interleavedList = [];

            _.forEach(priorityOrder, function(priorityLevel) {
                interleavedList = interleavedList.concat(findByPriority(programmedViolators, priorityLevel));
                interleavedList = interleavedList.concat(findByPriority(automaticViolators, priorityLevel));
            });

            return interleavedList;
        }

        /**
         * Choose the applicable violator list items
         * @param  {Array} result the test result matrix
         * @return {Object[]} an object array eg. [{label: 'preloadon',...}, ...]
         */
        function filterViolatorList(result) {
            var viewType = result.pop(),
                clientData = result.pop(),
                entitlementData = result.pop(),
                catalogData = result.pop(),
                trialTimeData = result.pop(),
                applicableViolators = [];

            if(!result.length || result.length !== violatorList.length) {
                return applicableViolators;
            }

            for (var i = 0; i < violatorList.length; i++) {
                if (result[i] && violatorList[i].context.indexOf(viewType) > -1) {
                    applicableViolators.push(_.cloneDeep(violatorList[i]));
                }
            }

            applicableViolators.push(trialTimeData);
            applicableViolators.push(catalogData);
            applicableViolators.push(entitlementData);
            applicableViolators.push(clientData);

            return applicableViolators;
        }

        /**
         * Resolve end date properties if set
         * @param  {Object[]} violatorList the filtered list of violators
         * @return {Promise.<Object[], Error>
         */
        function resolveEndDates(violatorList) {
            var clientData = violatorList.pop(),
                entitlementData = violatorList.pop(),
                catalogData = violatorList.pop(),
                trialTimeData = violatorList.pop(),
                collection = {catalog: catalogData, entitlement: entitlementData, client: clientData};

            if(!violatorList || !violatorList.length) {
                return violatorList;
            }

            _.forEach(violatorList, function(violator) {
                var enddateDataSource = violator.enddateDataSource,
                    enddateProperty = violator.enddateProperty;

                if(enddateDataSource &&
                    enddateProperty &&
                    collection[enddateDataSource] &&
                    _.has(collection[enddateDataSource], enddateProperty)) {
                    violator.endDate = _.get(collection[enddateDataSource], enddateProperty);
                }

                if(trialTimeData && trialTimeData.leftTrialSec) {
                    violator.timeRemaining = trialTimeData.leftTrialSec;
                } else if(catalogData.trialLaunchDuration) {
                    violator.timeRemaining = catalogData.trialLaunchDuration * 60 * 60;
                } else if(isValidDate(_.get(catalogData, ['platforms', currentPlatform, 'originSubscriptionUseEndDate']))) {
                    var countdown = DateHelperFactory.getCountdownData(_.get(catalogData, ['platforms', currentPlatform, 'originSubscriptionUseEndDate']));
                    violator.timeRemaining = countdown.totalSeconds;
                }
            });

            return violatorList;
        }

        /**
         * Handle errors thrown in the process of building the automatic violator list
         * @param  {Object} error the error message/data
         * @return {Promise.<Boolean, Error>} We will return false to ensure the violator is suppressed
         */
        function handleAutomaticViolatorFilterError(error) {
            ComponentsLogFactory.log("Automatic violator error: ", error);

            return Promise.resolve(false);
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

            _.forEach(function(promiseChainItem) {
                if(_.isObject(promiseChainItem) && _.isFunction(promiseChainItem.then)) {
                    promiseChainItem.catch(handleAutomaticViolatorFilterError);
                }
            });

            promiseChain.push(ViolatorModelFactory.getTrialTime(offerId));
            promiseChain.push(ViolatorModelFactory.getCatalog(offerId));
            promiseChain.push(ViolatorModelFactory.getEntitlement(offerId));
            promiseChain.push(ViolatorModelFactory.getClient(offerId));
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
        function getViolators(offerId, viewType) {
            return Promise.all([
                    getAutomaticViolators(offerId, viewType),
                    getProgrammedViolators(offerId, viewType)
                ])
                .then(_.spread(interleaveProgrammedAndAutomaticViolators))
                .then(filterCollectionBy(isWithinViewableDateRange))
                .then(filterCollectionBy(isNotDismissed(offerId)));
        }

        return {
            getViolators: getViolators,
            dismiss: dismiss,
            isDismissed: isDismissed,
            isWithinViewableDateRange: isWithinViewableDateRange,
            enddateReached: enddateReached,
            events: events,
            violatorType: violatorType,
            viewType: viewType,
            isValidViolatorData: isValidViolatorData
        };
    }

    /**
    * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
    * @param {object} SingletonRegistryFactory the factory which manages all the other factories
    */
    function GameViolatorFactorySingleton(ViolatorBaseGameFactory, ViolatorDlcFactory, ViolatorPlatformFactory, ViolatorLegacyTrialFactory, ViolatorVaultFactory, ViolatorProgrammedFactory, ViolatorModelFactory, ObjectHelperFactory, FunctionHelperFactory, ComponentsLogFactory, DateHelperFactory, Md5Factory, LocalStorageFactory, SingletonRegistryFactory) {
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