/**
 * @file achievements/achievement.js
 */
(function() {
    'use strict';

    function AchievementFactory(ComponentsLogFactory, ObjectHelperFactory, GamesDataFactory) {
        //events triggered from this service:
        //  achievements:achievementGranted
        //  achievements:achievementSetUpdated

        var myEvents = new Origin.utils.Communicator(),
            achievementPortfolios = {},
            pendingAchievementPortfolios = {};

        /**
         * @class AchievementGame
         * An object representing a game with achievement support, including a list of all relevant achievements.
         */
         function AchievementGame(name, achievements, id) {
            this.name = name;
            this.achievements = achievements;
            this.id = id || '';
         }

        AchievementGame.prototype.totalAchievements = function() {
            return Object.keys(this.achievements).length;
        };

        AchievementGame.prototype.grantedAchievements = function() {
            var total = 0;
            for (var id in this.achievements) {
                if (this.achievements[id].cnt > 0) {
                    total++;
                }
            }
            return total;
        };

        AchievementGame.prototype.totalXp = function() {
            var total = 0;
            for (var id in this.achievements) {
                total += this.achievements[id].xp;
            }
            return total;
        };

        AchievementGame.prototype.grantedXp = function() {
            var total = 0;
            for (var id in this.achievements) {
                if (this.achievements[id].cnt > 0) {
                    total += this.achievements[id].xp;
                }
            }
            return total;
        };

        AchievementGame.prototype.totalRp = function() {
            var total = 0;
            for (var id in this.achievements) {
                total += this.achievements[id].rp;
            }
            return total;
        };

        /**
         * @class AchievementSet
         * A container for achievements under the same set id.
         */
        function AchievementSet(personaId, achievementSetId) {
            this.personaId = personaId;
            this.achievementSetId = achievementSetId;
            this.compatibleIds = getCompatibleAchievementSetIds(achievementSetId);
            this.platform = '';
            this.achievements = {};
            this.baseGame = null;
            this.expansions = {};
        }

        function getCompatibleAchievementSetIds(achievementSetId) {
            var idList = [],
                keys = achievementSetId.split('_');
            do
            {
                idList.push(keys.join('_'));
                keys.pop();
            }
            while(keys.length > 2 || (keys.length > 1 && keys[0] !== 'META'));
            return idList;
        }

        // Returns true if the two achievement set ids are compatible.
        function achievementSetIdsAreCompatible(id1, id2) {
            var compatibleIdList1 = getCompatibleAchievementSetIds(id1);
            var compatibleIdList2 = getCompatibleAchievementSetIds(id2);
            if (compatibleIdList2.indexOf(id1) !== -1 || compatibleIdList1.indexOf(id2) !== -1) {
                return true;
            }
            return false;
        }

        AchievementSet.prototype.refresh = function() {
            return Origin.achievements.userAchievements(this.personaId, this.achievementSetId, Origin.locale.locale())
                .then(this.onRefreshSucceeded.bind(this))
                .catch(this.onRefreshFailed.bind(this));
        };

        AchievementSet.prototype.update = function(data) {
            this.platform = data.platform;
            this.achievements = this.parseAchievements(data);

            var sortedAchievements = this.sortAchievementsByGame(this.achievements);
            this.baseGame = new AchievementGame(data.name, sortedAchievements[''],this.achievementSetId);
            for (var i = 0; i < data.expansions.length; i++) {
                var exp = data.expansions[i];
                var expAchievements = sortedAchievements.hasOwnProperty(exp.id) ? sortedAchievements[exp.id] : {};
                this.expansions[exp.id] = new AchievementGame(exp.name, expAchievements, exp.id);
            }

            myEvents.fire('achievements:achievementSetUpdated', {
                personaId: this.personaId,
                achievementSetId: this.achievementSetId
            });
        };

        AchievementSet.prototype.onRefreshSucceeded = function(response) {
            this.update(response);
            return this;
        };

        AchievementSet.prototype.onRefreshFailed = function(error) {
            ComponentsLogFactory.error('AchievementFactory: set refresh failed for ' + this.personaId + ' : ' + this.achievementSetId, error);
            return null;
        };

        AchievementSet.prototype.parseAchievements = function(response) {
            var parsedAchievements = {};
            for (var id in response.achievements) {
                var raw = response.achievements[id],
                    parsed = {};

                // Preserve only the relevant attributes to reduce memory usage.
                var relevantAttrs = ['achievedPercentage', 'cnt', 'desc', 'e', 'icons', 'howto', 'img', 'name', 'p', 'rp', 'rp_t', 't', 'u', 'xp', 'xp_t', 'xpack'];
                var filterFunc = ObjectHelperFactory.pick(relevantAttrs);
                parsed = filterFunc(raw);

                // If the achievement has not been completed, set the time to -1 (server defaults it to current time).
                if (parsed.cnt === 0) {
                    parsed.u = -1;
                }

                // Add achievement ID to parsed achievement for ease of access.
                parsed.id = id;

                //Unique identifier for ng-repeat (track by this)
                //This way, if an achievement goes from unachieved to achieved, the affected achievements redraw
                parsed.rindex = id + '_' + parsed.cnt;

                parsedAchievements[id] = parsed;
            }
            return parsedAchievements;
        };

        AchievementSet.prototype.sortAchievementsByGame = function(achievements) {
            var sorted = {};
            for (var id in achievements) {
                var ach = achievements[id];
                var expansionId = ach.hasOwnProperty('xpack') ? ach.xpack.id : '';
                if (!sorted.hasOwnProperty(expansionId)) {
                    sorted[expansionId] = {};
                }
                sorted[expansionId][ach.id] = ach;
            }
            return sorted;
        };

        AchievementSet.prototype.totalAchievements = function() {
            return Object.keys(this.achievements).length;
        };

        AchievementSet.prototype.grantedAchievements = function() {
            var total = 0;
            for (var id in this.achievements) {
                if (this.achievements[id].cnt > 0) {
                    total++;
                }
            }
            return total;
        };

        AchievementSet.prototype.totalXp = function() {
            var total = 0;
            for (var id in this.achievements) {
                total += this.achievements[id].xp;
            }
            return total;
        };

        AchievementSet.prototype.grantedXp = function() {
            var total = 0;
            for (var id in this.achievements) {
                if (this.achievements[id].cnt > 0) {
                    total += this.achievements[id].xp;
                }
            }
            return total;
        };

        AchievementSet.prototype.totalRp = function() {
            var total = 0;
            for (var id in this.achievements) {
                total += this.achievements[id].rp;
            }
            return total;
        };

        /**
         * @class AchievementPortfolio
         * A container for a user's achievement sets
         */
        function AchievementPortfolio(personaId) {
            this.personaId = personaId;
            this.achievementSets = {};
            this.pendingAchievementSets = {};
            this.hidden = false;
        }

        function getCatalogInfo() {
            return GamesDataFactory.getCatalogInfo(GamesDataFactory.baseEntitlementOfferIdList());
        }

        function determineOwnedAchievementSets(catalogInfo) {
            var currentPlatform = Origin.utils.os(),
                achievementSet = '',
                achievementSets = [],
                transformed = {},
                found;
            for (var offerId in catalogInfo) {
                transformed = ObjectHelperFactory.transform({
                    'achievementSet' : ['platforms', currentPlatform, 'achievementSet'],
                    'achievementSetPCWIN' : ['platforms', 'PCWIN', 'achievementSet']
                }, catalogInfo[offerId]);
                achievementSet = transformed.achievementSet;
                if (!transformed.achievementSet || transformed.achievementSet.length === 0) {
                    achievementSet = transformed.achievementSetPCWIN;
                }
                if (achievementSet && achievementSet.length > 0) {
                    // Check if we already have a compatible achievement set id in achievementSets
                    found = false;
                    for (var id in achievementSets) {
                        if (achievementSetIdsAreCompatible(achievementSets[id], achievementSet)) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        achievementSets.push(achievementSet);
                    }
                }
            }
            return achievementSets;
        }

        AchievementPortfolio.prototype.refresh = function() {
            if (this.personaId === Origin.user.personaId()) {
                return GamesDataFactory.waitForInitialRefreshCompleted()
                    .then(getCatalogInfo)
                    .then(determineOwnedAchievementSets)
                    .then(this.refreshAchievementSets.bind(this))
                    .catch(this.onRefreshFailed.bind(this));
            } else {
                return Origin.achievements.userAchievementSets(this.personaId, Origin.locale.locale())
                    .then(this.onRefreshSucceeded.bind(this))
                    .catch(this.onRefreshFailed.bind(this));
            }
        };

        AchievementPortfolio.prototype.findAchievementSet = function(achievementSetId) {
            var set = null;
            for (var id in this.achievementSets) {
                set = this.achievementSets[id];
                if (set.compatibleIds.indexOf(achievementSetId) >= 0) {
                    return set;
                }
            }
            return null;
        };

        AchievementPortfolio.prototype.refreshAchievementSet = function(achievementSetId) {
            var set = this.findAchievementSet(achievementSetId);
            if (!set) {
                // If we are in the process of fetching this set, return the existing promise.
                if (this.pendingAchievementSets.hasOwnProperty(achievementSetId)) {
                    return this.pendingAchievementSets[achievementSetId];
                }

                set = new AchievementSet(this.personaId, achievementSetId);
                var promise = set.refresh().then(this.onAchievementSetCreated.bind(this));
                this.pendingAchievementSets[this.personaId] = promise;
                return promise;
            }
            return set.refresh();
        };

        AchievementPortfolio.prototype.refreshAchievementSets = function(achievementSetIds) {
            var promises = [];
            for (var i = 0; i < achievementSetIds.length; i++) {
                promises.push(this.refreshAchievementSet(achievementSetIds[i]));
            }
            return Promise.all(promises);
        };

        AchievementPortfolio.prototype.onAchievementSetCreated = function(set) {
            if (set) {
                delete this.pendingAchievementSets[set.achievementSetId];
                this.achievementSets[set.achievementSetId] = set;
            }
            return set;
        };

        AchievementPortfolio.prototype.onRefreshSucceeded = function(response) {
            var promises = [];
            for (var achievementSetId in response) {
                // Ignore any dev/test/meta achievement sets.
                if (achievementSetId.indexOf('CHALLENGES_') === 0 || achievementSetId.indexOf('META_') === 0) {
                    continue;
                }
                promises.push(this.refreshAchievementSet(achievementSetId));
            }
            return Promise.all(promises);
        };

        AchievementPortfolio.prototype.onRefreshFailed = function(error) {
            ComponentsLogFactory.log('AchievementFactory: portfolio refresh failed for ' + this.personaId + "(" + error.status + ")");
            if (this.personaId !== Origin.user.personaId() && error.status === Origin.defines.http.ERROR_403_FORBIDDEN) {
                this.hidden = true;
            } else {
                // if the error is not 403 and its friend/strangers profile we need to reject the promise
                return Promise.reject(error);
            }
        };

        AchievementPortfolio.prototype.totalAchievements = function() {
            var total = 0;
            for (var achievementSetId in this.achievementSets) {
                total += this.achievementSets[achievementSetId].totalAchievements();
            }
            return total;
        };

        AchievementPortfolio.prototype.grantedAchievements = function() {
            var total = 0;
            for (var achievementSetId in this.achievementSets) {
                total += this.achievementSets[achievementSetId].grantedAchievements();
            }
            return total;
        };

        AchievementPortfolio.prototype.totalXp = function() {
            var total = 0;
            for (var achievementSetId in this.achievementSets) {
                total += this.achievementSets[achievementSetId].totalXp();
            }
            return total;
        };

        AchievementPortfolio.prototype.grantedXp = function() {
            var total = 0;
            for (var achievementSetId in this.achievementSets) {
                total += this.achievementSets[achievementSetId].grantedXp();
            }
            return total;
        };

        AchievementPortfolio.prototype.totalRp = function() {
            var total = 0;
            for (var achievementSetId in this.achievementSets) {
                total += this.achievementSets[achievementSetId].totalRp();
            }
            return total;
        };

        /**
         * AchievementFactory methods
         */
        function createPortfolio(personaId) {
            // If we are in the process of fetching this portfolio, return the existing promise.
            if (pendingAchievementPortfolios.hasOwnProperty(personaId)) {
                return pendingAchievementPortfolios[personaId];
            }

            var portfolio = new AchievementPortfolio(personaId);
            var promise = portfolio.refresh().then(function() {
                delete pendingAchievementPortfolios[personaId];
                achievementPortfolios[personaId] = portfolio;
                return portfolio;
            })
            .catch(function(error){
                delete pendingAchievementPortfolios[personaId];
                ComponentsLogFactory.error('[Origin-Achievements-Create-Portfolio] error', error);
            });
            pendingAchievementPortfolios[personaId] = promise;
            return promise;
        }

        function progress(personaId) {
            if (!personaId) {
                personaId = Origin.user.personaId();
            }
            return Origin.achievements.userAchievementPoints(personaId);
        }

        function getAchievementPortfolio(personaId, achievementSetIds) {
            if (!personaId) {
                personaId = Origin.user.personaId();
            }

            var promises = [],
                promise = null;
            if (!achievementPortfolios.hasOwnProperty(personaId)) {
                promise = createPortfolio(personaId);
                promises.push(promise);
            }

            // If the caller specified a list of achievementSetIds, query for those that are not already part of the portfolio.
            // If not, just return all sets.
            if (achievementSetIds && achievementSetIds.length > 0) {
                for (var i = 0; i < achievementSetIds.length; i++) {
                    promise = getAchievementSet(personaId, achievementSetIds[i]);
                    promises.push(promise);
                }
            }
            return Promise.all(promises).then(function() {
                return achievementPortfolios[personaId];
            });
        }

        function getAchievementSet(personaId, achievementSetId) {
            if (!personaId) {
                personaId = Origin.user.personaId();
            }

            if (achievementPortfolios.hasOwnProperty(personaId)) {
                var set = achievementPortfolios[personaId].findAchievementSet(achievementSetId);
                if (set && set.compatibleIds.indexOf(achievementSetId) >= 0) {
                    return Promise.resolve(set);
                }
            }

            return refreshAchievementSet(personaId, achievementSetId);
        }

        function refreshAchievementSet(personaId, achievementSetId) {
            if (!personaId) {
                personaId = Origin.user.personaId();
            }

            if (!achievementPortfolios.hasOwnProperty(personaId)) {
                return createPortfolio(personaId).then(function(portfolio) {
                    return portfolio.refreshAchievementSet(achievementSetId);
                });
            }
            return achievementPortfolios[personaId].refreshAchievementSet(achievementSetId);
        }

        function publishedAchievementOffers() {
            return Origin.achievements.achievementSetReleaseInfo(Origin.locale.locale());
        }

        /*
         * Called after dirtybits fires AND the resulting achievement set has been refreshed
         * @param payload the payload of the inital
         */
        function onAchievementSetChangedAndRefreshed(payload) {
            return function(achievementSet) {
                myEvents.fire('achievements:achievementGranted', {
                    personaId: payload.pid,
                    achievementSet: achievementSet,
                    achievementIndex: payload.achid
                });
            };
        }

        /*
         * Called after dirtybits fires
         * @param payload the payload of the inital
         */
        function onAchievementGranted(payload) {
            if (achievementPortfolios.hasOwnProperty(payload.pid)) {
                //We must refresh our data first before alerting listeners of the change
                achievementPortfolios[payload.pid].refreshAchievementSet(payload.prodid)
                    .then(onAchievementSetChangedAndRefreshed(payload));
            }
        }

        Origin.events.on(Origin.events.DIRTYBITS_ACHIEVEMENTS, onAchievementGranted);

        return {
            events: myEvents,

            /**
             * Get the short form progress for a user.
             * @param {string} The user's persona ID.  If undefined the logged in user is used.
             * @return {Promise<achievementProgressObject>} A promise that on success will return the Achievement Progression for that user.
             */
            progress: progress,

            /**
             * Get all the achievement sets that the user has achievements for. If personaId is undefined the logged in user is used.
             * @param personaId {string} The persona id of the user you want the achievement sets for.
             * @param achievementSetIds {[string]} Optional array of achievement set Ids. Specifying the achievement set ids will allow for the achievement factory to gather information
             *        about achievements for games that have achievements but the user hasn't achieved any yet.
             * @return {Promise} A promise that on success will return the achievement portfolio for the user.  The achievment portfolio is an object that contains achievement sets.
             */
            getAchievementPortfolio: getAchievementPortfolio,

            /**
             * Get a specific achievement set for a persona.
             * @param personaId {string} The persona id of the user you want the achievement set for.
             * @param achievementSetId {string} The id of the achievement set to look for.
             * @return {Promise} A promise that on success will return the requested achievement set for the requested persona.
             */
            getAchievementSet: getAchievementSet,

            /**
             * Refreshes current user achievement Set. Call this function from the dirtybit handler, when an achievement event comes in.
             * @param personaId {string} The persona id to refresh the achievement set for.
             * @param achievementSetId {string} The achievement Set Id the needs to be refreshed.
             */
            refreshAchievementSet: refreshAchievementSet,

            /**
             * Get a list of all the published achievement offers. This is a cross correlation between the achievement service released products, and the catalog information.
             * @return {Promise<publishedAchievementProductObject[]>} A promise that on success with a list of offers indexed by achievementSetId that have achievements and
             * are published both in the catalog and the achievement service.
             */
            publishedAchievementOffers: publishedAchievementOffers,
            determineOwnedAchievementSets: determineOwnedAchievementSets
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function AchievementFactorySingleton(ComponentsLogFactory, ObjectHelperFactory, GamesDataFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('AchievementFactory', AchievementFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.AchievementFactory

     * @description
     *
     * AchievementFactory
     */
    angular.module('origin-components')
        .factory('AchievementFactory', AchievementFactorySingleton);
}());
