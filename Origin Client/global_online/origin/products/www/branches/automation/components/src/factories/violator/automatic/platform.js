/**
 * @file violator/automatic/platform.js
 */
(function(){

    'use strict';

    function ViolatorPlatformFactory(FunctionHelperFactory, ObjectHelperFactory, ViolatorModelFactory, BeaconFactory, PlatformRefiner, GameRefiner, EntitlementStateRefiner) {
        var and = FunctionHelperFactory.and;

        /**
         * Test for an upcoming release date for the current platform
         * @param {string} platform the platform to test
         * @return {Function}
         */
        function releasesOn(platform) {
            /**
             * Test for an upcoming release date for the current platform up to a year in advance
             * @param {string} offerId the offerId to test
             * @return {Promise.<Boolean, Error>} Resolves to a boolean
             */
            return function(offerId) {
                return ViolatorModelFactory.getCatalog(offerId)
                    .then(PlatformRefiner.isReleaseDateImminentInDays(platform, 365));
            };
        }

        /**
         * Test for a preload date for the current platform up to a year in advance
         * @param {string} platform the platform to test
         * @return {Function}
         */
        function preloadOn(platform) {
            /**
             * @param {string} offerId the offerId to test
             * @return {Promise.<Boolean, Error>} Resolves to a boolean
             */
            return function(offerId) {
                return ViolatorModelFactory.getCatalog(offerId)
                    .then(PlatformRefiner.isPreloadImminentInDays(platform, 365));
            };
        }

        /**
         * Test preload available status
         * @param {string} platform the platform to test
         * @return {Function}
         */
        function preloadAvailable(platform) {
            /**
             * @param {string} offerId the offerId to test
             * @return {Promise.<Boolean, Error>} Resolves to a boolean
             */
            return function(offerId) {
                return Promise.all([
                        ViolatorModelFactory.getCatalog(offerId),
                        ViolatorModelFactory.getClient(offerId),
                        ViolatorModelFactory.getEntitlement(offerId)
                            .then(EntitlementStateRefiner.isPreorder)
                    ])
                    .then(_.spread(PlatformRefiner.isPreloadable(platform)));
            };
        }

        /**
         * Test if release date is recent (last 5 days)
         * @return {Function}
         */
        function justReleased(platform) {
            /**
             * @param {string} offerId the offerId to test
             * @return {Promise.<Boolean, Error>} Resolves to a boolean
             */
            return function(offerId) {
                return ViolatorModelFactory.getCatalog(offerId)
                    .then(PlatformRefiner.isRecentlyReleasedInDays(platform, 5));
            };
        }

        /**
         * This violator should only appear if the conditions are met:
         * The game is NOT a hybrid game
         * the game is available on the selected platform
         * the user is NOT on that platform at the moment
         * @param {string} platform the platform name
         * @return {Function}
         */
        function isCompatibleSinglePlatform(platform) {
            return function (catalogInfo, isInstallableOnPlatform) {
                if(GameRefiner.isHybridGame(catalogInfo)) {
                    return false;
                }

                return !isInstallableOnPlatform && PlatformRefiner.isAvailableOnPlatform(platform)(catalogInfo);
            };
        }

        /**
         * This violator should only appear if preannouncement display date is set,
         * the release date is not in the next year
         * the game is not released
         * @param {object}
         * @return {Function}
         */
        function showPreAnnouncementDisplayDate(platform) {
            return function (catalogInfo) {
                var isReleased = PlatformRefiner.isReleased(platform)(catalogInfo),
                    releaseImminent = PlatformRefiner.isReleaseDateImminentInDays(platform,365)(catalogInfo),
                    hasDisplayDate = !_.isEmpty(_.get(catalogInfo,['i18n','preAnnouncementDisplayDate']));
                return (!isReleased && !releaseImminent && hasDisplayDate);
            };
        }

        /**
         * This violator should only appear if preannouncement display date is set,
         * the release date is not in the next year
         * the game is not released
         * @param {object}
         */
        function preAnnouncementDisplayDate(platform) {
            return function(offerId) {
                return ViolatorModelFactory.getCatalog(offerId)
                    .then(showPreAnnouncementDisplayDate(platform));
            };
        }

         /**
         * This violator should only appear if the conditions are met:
         * the game is a hybrid game
         * the user is NOT on either a mac or pc at the moment
         * @param {object} catalogInfo the catalogInfo
         * @param {Boolean} isInstallable the user is on a platform that supports Origin
         * @return {Function}
         */
        function isCompatibleHybridPlatform(catalogInfo, isInstallable) {
            return !isInstallable && GameRefiner.isHybridGame(catalogInfo);
        }

        /**
         * Test if the game is installable on the supplied platform and that the offer is available on that platform
         * @param {string} platform the platform to test
         * @return {Function}
         */
        function compatibleSinglePlatform(platform) {
            /**
             * @param {string} offerId the offerId to test
             * @return {Promise.<Boolean, Error>} Resolves to a boolean
             */
            return function(offerId) {
                return Promise.all([
                    ViolatorModelFactory.getCatalog(offerId),
                    BeaconFactory.isInstallableOnPlatform(platform)
                ]).then(_.spread(isCompatibleSinglePlatform(platform)));
            };
        }

        /**
         * Test if the game is both a Hybrid Game and the client is completely unsupported (iOS & *nix devices)
         * @param {string} offerId the offerId to test
         * @return {Promise.<Boolean, Error>} Resolves to a boolean
         */
        function incompatibleHybridPlatform(offerId) {
            return Promise.all([
                ViolatorModelFactory.getCatalog(offerId),
                BeaconFactory.isInstallable()
            ]).then(_.spread(isCompatibleHybridPlatform));
        }

        /**
         * Test if the game has been killed off from the catalog and it's use end date is within 90 days
         * @param {string} platform the platform to test
         * @return {Function}
         */
        function expiresOn(platform) {
            /**
             * @param {string} offerId the offerId to test
             * @return {Promise.<Boolean, Error>} Resolves to a boolean
             */
            return function(offerId) {
                return ViolatorModelFactory.getCatalog(offerId)
                    .then(PlatformRefiner.isExpiryImminentInDays(platform, 90));
            };
        }

        /**
         * Test if the game is expired
         * @param {string} platform the platform to test
         * @return {Function}
         */
        function expired(platform) {
            /**
             * @param {string} offerId the offerId to test
             * @return {Promise.<Boolean, Error>} Resolves to a boolean
             */
            return function(offerId) {
                return ViolatorModelFactory.getCatalog(offerId)
                    .then(PlatformRefiner.isExpired(platform));
            };
        }

        /**
         * Test if a game's preorder has failed for the selected platform
         * @param {string} platform the platform to test
         * @return {Function}
         */
        function preorderFailed(platform) {
            /**
             * @param {string} offerId the offerId to test
             * @return {Promise.<Boolean, Error>} Resolves to a boolean
             */
            return function(offerId) {
                return ViolatorModelFactory.getCatalogAndEntitlementData(offerId)
                    .then(_.spread(EntitlementStateRefiner.isFailedPreorder(platform)));
            };
        }

        /**
         * Test if a user has preloaded game, but the game isn't released yet
         * @param {string} platform the platform to test
         * @return {Function}
         */
        function preloaded(platform) {
            /**
             * @param {string} offerId the offerId to test
             * @return {Promise.<Boolean, Error>} Resolves to a boolean
             */
            return function(offerId) {
                return Promise.all([
                        ViolatorModelFactory.getCatalog(offerId),
                        ViolatorModelFactory.getClient(offerId)
                    ])
                    .then(_.spread(PlatformRefiner.isPreloaded(platform)));
            };
        }

        /**
         * Test if a game obtained from the vault is withing 90 days of its end date
         * @param {string} platform the platform to test
         * @return {Function}
         */
        function vaultGameExpiresOn(platform) {
            /**
             * @param {string} offerId the offerId to test
             * @return {Promise.<Boolean, Error>} Resolves to a boolean
             */
            return function(offerId) {
                return Promise.all([
                        ViolatorModelFactory.getCatalog(offerId)
                            .then(PlatformRefiner.isVaultGameExpiryImminentInDays(platform, 90)),
                        ViolatorModelFactory.getSubscriptionActive(),
                        ViolatorModelFactory.getClientOnline(),
                        ViolatorModelFactory.getEntitlement(offerId)
                            .then(EntitlementStateRefiner.isInSubscription)
                    ])
                    .then(ObjectHelperFactory.assertAllValuesTruthy);
            };
        }

        /**
         * Test if a game is the wrong architecture AND the correct platform
         */ 
        function isWrongArchitecture(platform) {            
            return function(clientInfo,catalogInfo) {
                return PlatformRefiner.isAvailableOnPlatform(platform)(catalogInfo) && EntitlementStateRefiner.isWrongArchitecture(clientInfo);
            };
        }
        /**
         * Test wrong architecture status
         * @param {string} platform the current platform
         * @return {Promise.<Boolean, Error>} Resolves to a boolean
         */
        function wrongArchitecture(platform) {
            /**
             * @param {string} offerId the offerId to test
             * @return {Promise.<Boolean, Error>} Resolves to a boolean
             */
            return function (offerId) {
                return Promise.all([
                        ViolatorModelFactory.getClient(offerId),
                        ViolatorModelFactory.getCatalog(offerId)
                    ])
                    .then(_.spread(isWrongArchitecture(platform)));
            };
        }

        /**
         * Test if a game obtained from the vault has passed its use end date
         * @param {string} platform the platform to test
         * @return {Function}
         */
        function vaultGameExpired(platform) {
            /**
             * @param {string} offerId the offerId to test
             * @return {Promise.<Boolean, Error>} Resolves to a boolean
             */
            return function(offerId) {
                return Promise.all([
                        ViolatorModelFactory.getCatalog(offerId)
                            .then(PlatformRefiner.isVaultGameExpired(platform)),
                        ViolatorModelFactory.getSubscriptionActive(),
                        ViolatorModelFactory.getClientOnline(),
                        ViolatorModelFactory.getEntitlement(offerId)
                            .then(EntitlementStateRefiner.isInSubscription)
                    ])
                    .then(ObjectHelperFactory.assertAllValuesTruthy);
            };
        }


        /**
         * Test if a game obtained from the vault has passed its use end date
         * @param {string} platform the platform to test
         * @return {Function}
         */
        function hasVaultMacAlternative(platform) {
            /**
             * @param {string} offerId the offerId to test
             * @return {Promise.<Boolean, Error>} Resolves to a boolean
             */
            return function(offerId) {
                return Promise.all([
                    ViolatorModelFactory.getCatalog(offerId),
                    ViolatorModelFactory.getEntitlement(offerId)
                        .then(and([EntitlementStateRefiner.isInSubscription, EntitlementStateRefiner.isActive])),
                    ViolatorModelFactory.getSubscriptionActive()
                ])
                .then(_.spread(PlatformRefiner.hasVaultMacAlternative(platform)));
            };
        }

        return {
            releasesOn: releasesOn,
            preloadOn: preloadOn,
            preloadAvailable: preloadAvailable,
            justReleased: justReleased,
            compatibleSinglePlatform: compatibleSinglePlatform,
            incompatibleHybridPlatform: incompatibleHybridPlatform,
            expiresOn: expiresOn,
            expired: expired,
            preorderFailed: preorderFailed,
            preAnnouncementDisplayDate: preAnnouncementDisplayDate,
            preloaded: preloaded,
            vaultGameExpiresOn: vaultGameExpiresOn,
            vaultGameExpired: vaultGameExpired,
            wrongArchitecture: wrongArchitecture,
            hasVaultMacAlternative: hasVaultMacAlternative
        };
    }

    /**
    * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
    * @param {object} SingletonRegistryFactory the factory which manages all the other factories
    */
    function ViolatorPlatformFactorySingleton(FunctionHelperFactory, ObjectHelperFactory, ViolatorModelFactory, BeaconFactory, PlatformRefiner, GameRefiner, EntitlementStateRefiner, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ViolatorPlatformFactory', ViolatorPlatformFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ViolatorPlatformFactory

     * @description
     *
     * ViolatorPlatformFactory
     * Tools for evaluating the user's operating system platform and available client interactions
     */
    angular.module('origin-components')
        .factory('ViolatorPlatformFactory', ViolatorPlatformFactorySingleton);
})();