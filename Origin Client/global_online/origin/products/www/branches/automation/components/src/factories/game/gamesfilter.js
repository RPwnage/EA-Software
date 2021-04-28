/**
 * @file game/gamefilters.js
 */

(function() {
    'use strict';


    var STORAGE_KEY = 'origin-game-library-filters:',
        NORMAL_FILTER = false,
        INVERTED_FILTER = true,
        HIGH_PRIORITY = 10000,
        //MEDIUM_PRIORITY = 1000,
        LOW_PRIORITY = 0;


    function GamesFilterFactory($filter, ObjectHelperFactory, LocalStorageFactory, ResultFilterFactory, SubscriptionFactory, GameRefiner, GamesDataFactory, PlatformRefiner, DateHelperFactory, EntitlementStateRefiner) {
        // Shortcuts
        var filterService = $filter('filter'),
            length = ObjectHelperFactory.length,
            transformWith = ObjectHelperFactory.transformWith,
            subtractDays = DateHelperFactory.subtractDays,

            // Initialize filters
            gameFilterValues = LocalStorageFactory.get(STORAGE_KEY, {}),
            gameFilters = {
                displayName: {filterType: NORMAL_FILTER, value: ''},
                isPlayedLastWeek: {filterType: NORMAL_FILTER, value: false},
                isFavorite: {filterType: NORMAL_FILTER, value: false},
                isDownloading: {filterType: NORMAL_FILTER, value: false},
                isHidden: {filterType: INVERTED_FILTER, value: false},
                isInstalled: {filterType: NORMAL_FILTER, value: false},
                isNonOrigin: {filterType: NORMAL_FILTER, value: false},
                isSubscription: {filterType: NORMAL_FILTER, value: false},
                isPurchasedGame: {filterType: NORMAL_FILTER, value: false},
                isSuppressed: {filterType: INVERTED_FILTER, value: false},
                isPcGame: {filterType: NORMAL_FILTER, value: false},
                isMacGame: {filterType: NORMAL_FILTER, value: false}
            },
            resultFilterFactoryInstance = ResultFilterFactory.getInstance(gameFilters);

        // force game isSuppressed to be disabled
        gameFilterValues.isSuppressed = false;
        resultFilterFactoryInstance.setAllFilterValues(gameFilterValues);

        /**
         * Removes suppressed filter when isPurchasedGame filter is active.
         * @param filterValues available filters
         */
        function showSuppressedGamesIfPurchasedFilterActive(filterValues) {
            // isPurchased is a special case based on the PRD.
            // It causes isSuppressed to be ignored to show games that
            // were purchased but suppressed by Origin Access subscription
            if (filterValues.isPurchasedGame) {
                delete filterValues.isSuppressed;
            }
        }

        /**
         * Applies active filters to the given collection of models.
         * @param {object} collection - collection to filter
         * @param {object} filterValues - filterValues from ResultFilterFactory
         * @return {object}
         */
        function applyFilterValues(collection, filterValues) {
            showSuppressedGamesIfPurchasedFilterActive(filterValues);
            LocalStorageFactory.set(STORAGE_KEY, resultFilterFactoryInstance.getAllFilterValues());

            if (length(filterValues) > 0) {
                collection = filterService(collection, filterValues);
            }

            return collection;
        }

        /**
         * Applies active filters to the given collection of models.
         * @param {object} collection - collection to filter
         * @return {object}
         */
        function applyFilters(collection) {
            var filterValues = resultFilterFactoryInstance.getApplicableFilterValues();
            return applyFilterValues(collection, filterValues);
        }

        /**
         * Applies default filters to the given collection of models.
         * @param {object} collection - collection to filter
         * @return {object} filtered collection
         */
        function applyDefaultFilters(collection) {
            var defaultFilterValues = resultFilterFactoryInstance.getDefaultFilterValues();
            return applyFilterValues(collection, defaultFilterValues);
        }

        /**
         * Counts matching objects for a filter.
         * @param {string} filterName - filter names correspond to game model property names
         * @param {object} collection - collection to filter
         * @return {integer}
         */
        function getCountForFilter(filterName, collection) {
            var filterObject = {};

            filterObject[filterName] = true;
            if (_.isString(gameFilters[filterName].value)) {
                filterObject[filterName] = gameFilters[filterName].value;
            }

            // when calculating the count, don't include suppressed games.
            // but the purchased filter includes suppressed games so
            // include them in the count for purchased games
            if (filterName !== 'isPurchasedGame') {
                filterObject.isSuppressed = false;
            }

            return filterService(collection, filterObject).length;
        }


        /**
         * Attempts to apply subscription suppression to the given edition array.
         * If the user has an active subscription and a subscription offer, all lesser offers will be suppressed.
         * @param {string} editions - the edition array
         * @param {Number} bestReleasedGameIndex - the index from which to start suppression
         */
        function applySubscriptionSuppression(editions, bestReleasedGameIndex) {
            var isSubscriptionActive = SubscriptionFactory.isActive(),
                applySuppression = false;

            _.forEach(editions, function(edition) {
                if (edition.isSubscription && isSubscriptionActive) {
                    applySuppression = true;
                    return false;
                }
            });

            if (applySuppression) {
                _.forEach(editions, function(edition, i) {
                    // If we found a subscription entitlement, suppress anything lesser ranked.
                    // Never suppress title at index 0.  Avoid suppressing anything until we've found a released title.
                    if (i > bestReleasedGameIndex) {
                        edition.isSuppressed = true;
                    }
                });
            }
        }

        /**
         * Attempts to apply trial suppression to the given edition array.
         * If the user has both a trial offer and the assoc. base game, the trial will be suppressed.
         * @param {string} editions - the edition array
         * @param {Number} bestReleasedGameIndex - the index from which to start suppression
         */
        function applyTrialSuppression(editions, bestReleasedGameIndex) {
            var applySuppression = false;

            _.forEach(editions, function(edition) {
                if (edition.isTrial || edition.isEarlyAccess) {
                    applySuppression = true;
                    return false;
                }
            });

            if (applySuppression) {
                _.forEach(editions, function(edition, i) {
                    // If we found a trial, suppress only the trial.
                    // Never suppress title at index 0.  Avoid suppressing anything until we've found a released title.
                    if (i > bestReleasedGameIndex && (edition.isTrial || edition.isEarlyAccess)) {
                        edition.isSuppressed = true;
                    }
                });
            }
        }

        /**
         * Attempts to apply preorder suppression to the given edition array.
         * If the user has both a preorder and a non-preorder entitlement, the preorder will be suppressed.
         * @param {string} editions - the edition array
         */
        function applyPreorderSuppression(editions) {
            var foundPreorder = false,
                foundOwned = false;

            _.forEach(editions, function(edition) {
                if (edition.isPreorder) {
                    foundPreorder = true;
                } else {
                    foundOwned = true;
                }
            });

            if (foundPreorder && foundOwned) {
                _.forEach(editions, function(edition) {
                    // If we found a preorder entitlement, suppress only the preorder.
                    if (edition.isPreorder) {
                        edition.isSuppressed = true;
                    }
                });
            }
        }

        /**
         * Hide games that should not be available.  This modifies the collection directly.
         *
         * The collection is grouped by gameNameFacetKey (ex. dragon-age-inquisition) first.
         * The sort order is gameEditionTypeFacetKeyRankDesc first (ex. 3000 for standard-edition), then
         * gameDistributionSubType (ex. Gameplay Timer Trail - Game Time).  Uses GAME_DISTRIBUTION_SUBTYPE
         * to assign the string value of gameDistributionSubType a numeric value.
         *
         * @param {object} collection - collection to filter
         */
        function hideLesserEditions(collection) {
            var gameMap = {};

            // create gameMap dictionary of arrays
            // organize game with multiple editions together
            collection.map(function(game) {
                // Alpha/beta/demo are independent editions per PRD
                if (!game.isAlphaBetaDemo) {
                    if (_.isArray(gameMap[game.gameNameFacetKey])) {
                        gameMap[game.gameNameFacetKey].push(game);
                    } else {
                        gameMap[game.gameNameFacetKey] = [game];
                    }
                    // sort array, higher edition will be zero item
                    gameMap[game.gameNameFacetKey].sort(sortGameEditions);
                }
            });

            // hide anything but the best edition
            // or if installed
            _.forEach(gameMap, function(editions) {
                var bestReleasedGameIndex = editions.length;

                // Find index of best released title
                _.forEach(editions, function(edition, i) {
                    if (PlatformRefiner.isReleasedOnAnyPlatform()(edition)) {
                        bestReleasedGameIndex = i;
                        return false;
                    }
                });

                // Once a valid best match has been found, suppress any lesser editions.
                // These are the scenarios we need to handle:
                // 1. If user owns subscription offer(s), we should suppress any lesser offer(s).
                // 2. If user owns early access offer before the base game release date, we should not suppress either offer.
                // 3. If user owns early access offer after the base game release date, the early access offer should be suppressed.
                // 4. If user owns a trial and the matching base game, the trial should be suppressed.
                // 5. If user has a lapsed subscription, subscription suppression should not occur.
                // 6. If the user has a preorder entitlement and a non-preorder entitlement for the same content, we should suppress the preorder.
                applySubscriptionSuppression(editions, bestReleasedGameIndex);
                applyTrialSuppression(editions, bestReleasedGameIndex);
                applyPreorderSuppression(editions);
            });
            return collection;
        }

        /**
         * Sort game editions
         *
         * Uses based on gameEditionTypeFacetKeyRankDesc and gameDistributionSubType.
         * gameEditionTypeFacetKeyRankDesc is a number, but gameDistributionSubType
         * is mapping defined by production.
         *
         * @param {object} a, b - collection objects to sort
         * @return {object} -1 if a < b, 0 if a == b, 1 if a > b
         */
        function sortGameEditions(a,b) {
            var subscriptionExpired = !SubscriptionFactory.isActive();
            var aVal = a.gameEditionTypeFacetKeyRankDesc || LOW_PRIORITY,
                bVal = b.gameEditionTypeFacetKeyRankDesc || LOW_PRIORITY;
            if (aVal === bVal) {
                // sort by secondary key gameDistributionSubType
                aVal = GameRefiner.getSubTypePriority(a.gameDistributionSubType);
                bVal = GameRefiner.getSubTypePriority(b.gameDistributionSubType);
            }

            // expired subscriptions priority are reduced
            if (subscriptionExpired) {
                aVal -= a.isSubscription ? HIGH_PRIORITY : LOW_PRIORITY;
                bVal -= b.isSubscription ? HIGH_PRIORITY : LOW_PRIORITY;
            }

            // higher number is first
            return bVal - aVal;
        }

        /**
         * Counts matching objects for all the filters: active or inactive.
         * @param {object} collection - collection to filter
         * @return {object}
         */
        function calculateCounts(collection) {

            _.forEach(resultFilterFactoryInstance.getFilters(), function(filterObject, filterName) {
                filterObject.count = getCountForFilter(filterName, collection);
                // Active filter that's lost all matching items should be de-activated
                if (resultFilterFactoryInstance.isActive(filterObject) && filterObject.count === 0) {
                    resultFilterFactoryInstance.deactivate(filterObject);
                }
            });

            return collection;
        }

        /**
         * Create game filter model from the catalogData
         * @param {object} catalogData - catalog data to turn into filter model
         * @return {object}
         */
        var createGameModel = transformWith({
            'offerId': 'offerId',
            'displayName': ['i18n', 'displayName'],
            'gameNameFacetKey': 'gameNameFacetKey',
            'gameEditionType': 'gameEditionType',
            'platformFacetKey' : 'platformFacetKey',
            'platforms' : 'platforms',
            'gameEditionTypeFacetKeyRankDesc': 'gameEditionTypeFacetKeyRankDesc',
            'gameDistributionSubType': 'gameDistributionSubType',
            'vault': 'vault',
            'masterTitleId': 'masterTitleId'
        });

        /**
         * Update the model with extra information used to filter the data
         * @param {object} gameModel - model returned from createGameModel
         * @return {object} updated game model with extra properties
         */
        function updateGameModelExtraProperties(model) {

            model.isPcGame = PlatformRefiner.isAvailableOnPlatform('PCWIN')(model);
            model.isMacGame = PlatformRefiner.isAvailableOnPlatform('MAC')(model);
            model.isFavorite = GamesDataFactory.isFavorite(model.offerId);
            model.isHidden = GamesDataFactory.isHidden(model.offerId);
            model.isPlayedLastWeek = model.lastPlayedTime && (model.lastPlayedTime >= (subtractDays(new Date(), 7).getTime()));
            model.isSuppressed = false;
            model.isNonOrigin = GamesDataFactory.isNonOriginGame(model.offerId);
            model.isSubscription = GamesDataFactory.isSubscriptionEntitlement(model.offerId);
            model.isTrial = GamesDataFactory.isTrial(model.offerId);
            model.isEarlyAccess = GamesDataFactory.isEarlyAccess(model.offerId);
            model.isPurchasedGame = !model.isSubscription && !(model.isTrial || model.isEarlyAccess);
            model.isDownloading = GamesDataFactory.isDownloading(model.offerId);
            model.isInstalled = GamesDataFactory.isInstalled(model.offerId);
            model.isAlphaBetaDemo = GamesDataFactory.isAlphaBetaDemo(model.offerId);
            model.isPreorder = EntitlementStateRefiner.isPreorder(GamesDataFactory.getEntitlement(model.offerId));

            return model;
        }

        return angular.extend({
            applyFilters: applyFilters,
            applyDefaultFilters: applyDefaultFilters,
            hideLesserEditions: hideLesserEditions,
            getCountForFilter: getCountForFilter,
            calculateCounts: calculateCounts,
            createGameModel: createGameModel,
            updateGameModelExtraProperties: updateGameModelExtraProperties
        }, resultFilterFactoryInstance);

    }

    angular.module('origin-components')
        .factory('GamesFilterFactory', GamesFilterFactory);
}());
