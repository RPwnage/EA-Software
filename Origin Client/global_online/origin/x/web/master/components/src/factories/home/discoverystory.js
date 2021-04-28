/**
 * @file home/discoverystory.js
 */
/* jshint latedef:nofunc */
(function() {
    'use strict';

    function DiscoveryStoryFactory($timeout, UtilFactory, FeedFactory, PriorityEngineFactory, ComponentsCMSFactory, GamesDataFactory, ComponentsLogFactory, AuthFactory, ArrayHelperFactory, ObjectHelperFactory) {
        var myEvents = new Origin.utils.Communicator(),
            tileConfigReady = false,
            bucketConfigs = [],
            programmableIndex = 0,
            discoveryTilesConfigBase = {
                HT_01: {
                    gameDataFeedFunc: FeedFactory.getFriendsPlayingOwned,
                    feedId: 'origin-home-discovery-tile-ogfriendsplaying',
                    requiresFriends: true
                },
                HT_02: {
                    gameDataFeedFunc: FeedFactory.getRecentlyReleased,
                    feedId: 'origin-home-discovery-tile-ogrecentlyreleased'
                },
                HT_03: {
                    gameDataFeedFunc: FeedFactory.getNotRecentlyPlayed,
                    feedId: 'origin-home-discovery-tile-ognotplayedrecently'
                },
                HT_04: {
                    feedId: 'recfriends',
                    dataFeedFunc: FeedFactory.getRecommendedFriends,
                    postStoryGenFeedModFunc: applyFriendsRandomization,
                    numFeedDataUsed: 0
                },
                HT_05: {
                    gameDataFeedFunc: FeedFactory.getNeverPlayed,
                    feedId: 'origin-home-discovery-tile-ogneverplayed'
                },
                HT_06: {
                    feedId: 'origin-home-discovery-tile-newdlc',
                    gameDataFeedFunc: FeedFactory.getUnownedDLC,
                },
                HT_07: {
                    gameDataFeedFunc: FeedFactory.getPopularUnownedGames,
                    feedId: 'origin-home-discovery-tile-ugfriendsplaying',
                    requiresFriends: true
                },
                HT_21: {
                    feedId: 'origin-home-discovery-tile-ogrecentlyplayed',
                    gameDataFeedFunc: FeedFactory.getRecentlyPlayedNoFriends
                }
            },

            //look up table to the possible directives
            ctaConfigBase = {
                'url-internal': {
                    directive: '<origin-cta-loadurlinternal></origin-cta-loadurlinternal>'
                },
                'url-external': {
                    directive: '<origin-cta-loadurlexternal></origin-cta-loadurlexternal>'
                },
                'play-video': {
                    directive: '<origin-cta-playvideo></origin-cta-playvideo>'
                },
                'purchase': {
                    directive: '<origin-cta-purchase></origin-cta-purchase>'
                },
                'direct-acquistion': {
                    directive: '<origin-cta-directacquisition></origin-cta-directacquisition>'
                },
                'pdp': {
                    directive: '<origin-cta-pdp></origin-cta-pdp>'
                },
                'download-install-play': {
                    directive: '<origin-cta-downloadinstallplay></origin-cta-downloadinstallplay>'
                }
            };

        function resetFlagsAndGlobals() {
            bucketConfigs = [];
            programmableIndex = 0;
            tileConfigReady = false;
        }

        function addToStoryTypeConfig(curStoryType, bucketId, storyId) {
            if (typeof bucketConfigs[bucketId] === 'undefined') {
                bucketConfigs[bucketId] = {
                    storiesReady: false,
                    requiresFriends: false,
                    storyTypes: {}
                };
            }

            if (curStoryType.requiresFriends) {
                bucketConfigs[bucketId].requiresFriends = true;
            }

            bucketConfigs[bucketId].storyTypes[storyId] = curStoryType;
        }

        /**
         * adjust the number of stories available if the feed provides less that the asked number from CQ
         * @param  {object} storyTypeConfig configuration info for the story type
         */
        function adjustStoriesAvailable(storyTypeConfig) {
            var newLimit = storyTypeConfig.feedData.length;

            //adjust the length if its smaller or there was previously no limit set
            if (storyTypeConfig.limit === -1 || newLimit < storyTypeConfig.limit) {
                storyTypeConfig.limit = newLimit;
            }
        }

        /**
         * populate the feed data for story types that deal with game information
         * @param  {object} storyTypeConfig configuration info for the story type
         * @param  {string[]} usedProductIds  an array of product ids alredy used
         */
        function populateGameFeedData(storyTypeConfig, usedProductIds) {
            if (storyTypeConfig.gameDataFeedFunc) {
                var gameDataFeed = storyTypeConfig.gameDataFeedFunc(storyTypeConfig);

                storyTypeConfig.feedData = gameDataFeed.filter(function(gameDataFeedItem) {
                    var keep = (usedProductIds.indexOf(gameDataFeedItem.offerId) < 0);

                    if (keep) {
                        usedProductIds.push(gameDataFeedItem.offerId);
                    }

                    return keep;
                });

                adjustStoriesAvailable(storyTypeConfig);
            }
        }

        /**
         * populate the feed data for story types that deal with non-game information
         * @param  {object} storyTypeConfig configuration info for the story type
         */
        function populateOtherFeedData(storyTypeConfig) {
            if (storyTypeConfig.dataFeedFunc) {
                storyTypeConfig.feedData = storyTypeConfig.dataFeedFunc(storyTypeConfig);
                adjustStoriesAvailable(storyTypeConfig);
                storyTypeConfig.numFeedDataUsed = storyTypeConfig.limit;
            }
        }

        /**
         * this function runs on every story type we have for a give bucket
         * @param  {string[]} usedProductIds an array of used product ids
         * @return {function}                a function that takes in a storytype config object
         */
        function buildDataFeedForStoryType(usedProductIds) {
            return function(storyTypeConfig) {
                populateGameFeedData(storyTypeConfig, usedProductIds);
                populateOtherFeedData(storyTypeConfig);
            };
        }

        /**
         * generates a list of discovery tile stories for a give bucket
         */
        function generateStoryList(bucketId) {
            var bucket = bucketConfigs[bucketId].storyTypes,
                usedProductIds = [];

            //add the game feed data to the bucket object
            ObjectHelperFactory.forEach(buildDataFeedForStoryType(usedProductIds), bucket);

            bucketConfigs[bucketId].stories = PriorityEngineFactory.generateList(bucket, 2000);
            //run the configs through the Priority Engine to generate a heap of stories
            return Promise.resolve(bucketConfigs[bucketId]);
        }


        function handleCMSRetrieveError(error) {
            ComponentsLogFactory.error('[DiscoveryStoryFactory:buildDirectivesWithAttributes - retrieveDiscoveryStoryData]', error.stack);
        }

        function buildCTAElement(ctaConfig) {
            //take the string directive and convert it to an element
            var ctaDirectiveElement = angular.element(ctaConfigBase[ctaConfig.actionid].directive);
            if (ctaConfig) {
                //add cta config properties as attributes to the actual cta directive
                for (var p in ctaConfig) {
                    if (ctaConfig.hasOwnProperty(p)) {
                        ctaDirectiveElement.attr(p, ctaConfig[p]);
                    }
                }
            }

            return ctaDirectiveElement;
        }

        function appendCTAElementList(tileDirectiveElement, ctaList) {
            //loop through the cta ekenents and append to tile element
            if (ctaList && ctaList.length) {
                for (var i = 0; i < ctaList.length; i++) {
                    tileDirectiveElement.append(buildCTAElement(ctaList[i]));
                }
            }
        }

        function mixFeedInformationToDirectiveAttributes(heapPos, listId, listIndex, bucket, start, directivesList) {
            //here we track the response so that we can associated the feed data
            //with the data thats coming from the CMS. If we batch files in the future
            //we may need to revisit this
            return function(response) {

                var tileConfig = bucket[listId],
                    prop = tileConfig.feedData[listIndex],
                    directiveElement = angular.element(response.xml).children(),
                    index = heapPos - start;



                //mix in the properties from the recommendation feed
                for (var p in prop) {
                    if (prop.hasOwnProperty(p)) {
                        directiveElement.attr(p, prop[p]);
                    }
                }

                //if there was cta information available with the config, lets append it as a child to the
                //tile directive element
                appendCTAElementList(directiveElement, tileConfig.cta);

                //we assign to a specifc index instead of using push so we can preserve the order. we can't guarantee that
                //we will receive the responses in the same order we requested
                directivesList[index] = directiveElement[0];

                return directivesList;
            };
        }

        /**
         * returns an object to be passed in to UtilFactory.buildTag
         * @param  {object} chosenStory contains the offerId and direective name
         * @return {object}             returns an array of an object with the directive name and a data object that will become attributes
         */
        function setupDirectivesObject(directiveName, offerId) {
            return function(dataObject) {
                dataObject.offerId = offerId;
                return [{
                    name: directiveName,
                    data: dataObject
                }];
            };
        }

        /**
         * added directive to the return list
         */
        function addToDirectivesList(directivesList, index) {
            return function(tag) {
                directivesList[index] = angular.element(tag)[0];
            };
        }

        /**
         * retrieves the OCD Info, then builds a directive with the OCD Info as attributes
         * @param  {object} chosenStory contains the offerId and directive name
         */
        function buildDirectiveWithOCD(directiveName, offerId, index, directivesList) {
            return GamesDataFactory.getOcdDirectiveByOfferId(directiveName, offerId, Origin.locale.locale())
                .then(setupDirectivesObject(directiveName, offerId))
                .then(UtilFactory.buildTag)
                .then(addToDirectivesList(directivesList, index));
        }

        /**
         * takes directives retrieved from CMS and adds attributes built from the feeds
         */
        function buildDirectivesWithAttributes(start, numTiles, bucketConfig) {
            return function() {
                var directivesList = [],
                    bucket = bucketConfig.storyTypes,
                    bucketStoryInfo = bucketConfig.stories,
                    cmsRequestPromises = [],
                    i = 0,
                    list = bucketStoryInfo.list,
                    count = numTiles,
                    end = start + numTiles,
                    promise = null;


                //if the number of tiles we are requesting is greated than the number of stories, adjust it
                if (start + numTiles > list.length) {
                    numTiles = list.length - start;
                    count = numTiles;
                    end = start + numTiles;
                }

                //if the list is empty just check for completion so we resolve with empty list
                if (end === 0) {
                    promise = Promise.resolve([]);
                } else {
                    for (i = start; i < end; i++) {
                        var offerId = null,
                            tileIndex = list[i].index,
                            tileInfo = bucket[list[i].id],
                            mixFeed = null;

                        //lets make sure we have valid offer Id
                        if (tileInfo.feedData && tileIndex < tileInfo.feedData.length && tileInfo.feedData[tileIndex].offerId) {
                            offerId = tileInfo.feedData[tileIndex].offerId;
                        }

                        //lets go get the directive from CMS
                        if (tileInfo.feedData && tileIndex < tileInfo.feedData.length) {
                            mixFeed = mixFeedInformationToDirectiveAttributes(i, list[i].id, tileIndex, bucket, start, directivesList);

                            //if it isn't a local directive we go an retrieve it from CMS
                            if (!tileInfo.directive) {
                                //if we have an offerid get it from OCD
                                if (offerId) {
                                    cmsRequestPromises.push(buildDirectiveWithOCD(tileInfo.feedId, offerId, i - start, directivesList));
                                } else {
                                    //add cms directive datato the promise array
                                    cmsRequestPromises.push(ComponentsCMSFactory.retrieveDiscoveryStoryData(tileInfo.feedId, offerId)
                                        .then(mixFeed, handleCMSRetrieveError).catch(handleCMSRetrieveError));

                                }
                            } else {
                                //add local directive data to the promise array
                                cmsRequestPromises.push(Promise.resolve(mixFeed({
                                    xml: tileInfo.directive
                                })));
                            }
                        }
                    }

                    promise = Promise.all(cmsRequestPromises).then(function() {
                        return directivesList;
                    });
                }

                return promise;
            };
        }

        /**
         * handles the response from buildDirectivesWithAttributes
         */
        function setupStoryListNextIndex(bucketId, startTileIndex, numTilesToRequest) {
            return function(directivesList) {
                var next = startTileIndex + numTilesToRequest;
                if (next >= bucketConfigs[bucketId].stories.list.length) {
                    next = -1;
                }
                return {
                    directives: directivesList,
                    nextToShow: next
                };

            };
        }

        /**
         * marks the bucket stories as ready
         */
        function setBucketStoriesReady(bucket) {
            return function(response) {
                bucket.storiesReady = true;
                return response;
            };
        }

        /**
         * applies a randomization to the friends, taking the top X*2(where X is the number of initial tiles shown)
         * and randomizing the order
         *
         * @param  {object} stats  generated story information
         * @param  {object} config storyType configuration information
         */
        function applyFriendsRandomization(stats, config) {
            var topMatches = [],
                numToApplyRandom = stats.count * 2,
                seed = (new Date()).setHours(0, 0, 0, 0) / 100000; //use the day as a starting point for hte seed

            //a simple pseudo random number generator, we don't need it to be super random
            //just somthing basic
            //
            //the constants generate a relatively distributed list of numbers from a given seed
            function seededRandom() {
                seed = (seed * 9301 + 49297) % 233280;
                return (seed / 233280);
            }

            //split off the top X friends
            topMatches = config.feedData.splice(0, numToApplyRandom);

            //randomize it
            ArrayHelperFactory.shuffle(topMatches, seededRandom);

            config.feedData = topMatches.concat(config.feedData);
        }

        /**
         * run after the stories are generated for each story type
         * we run any logic here that requires knowing how many stories we are going to  show
         * @param  {object} bucketConfig bucket information
         */
        function postGenerationFeedModifications(bucketConfig) {
            ObjectHelperFactory.forEach(function(storyStat, storyTypeId) {
                var storyTypeConfig = bucketConfig.storyTypes[storyTypeId];
                if (storyTypeConfig.postStoryGenFeedModFunc) {
                    storyTypeConfig.postStoryGenFeedModFunc(storyStat, storyTypeConfig);
                }
            }, bucketConfig.stories.stats);

            return bucketConfig;
        }
        /**
         * grabs game feed data and generates a story list via the PriorityEngine if needed
         * then generates the directives by making requests to CMS and merging it with feed information
         */
        function generateStoriesAndDirectives(bucketId, startTileIndex, numTilesToRequest) {
            return function() {
                var bucket = bucketConfigs[bucketId];

                if (!startTileIndex) {
                    startTileIndex = 0;
                }
                if (!numTilesToRequest) {
                    //temp make this 3 so we can show feature while we get the CQ5 hooked up, eventually it will default to the whole list size
                    numTilesToRequest = 3; //bucket.stories.list.length;
                }
                if (bucket.storiesReady) {
                    //if we've already generated stories from the priority engine, just go and retrive the directives from CMS
                    return buildDirectivesWithAttributes(startTileIndex, numTilesToRequest, bucket)()
                        .then(setupStoryListNextIndex(bucketId, startTileIndex, numTilesToRequest));
                } else {
                    return generateStoryList(bucketId)
                        .then(postGenerationFeedModifications)
                        .then(buildDirectivesWithAttributes(startTileIndex, numTilesToRequest, bucket))
                        .then(setupStoryListNextIndex(bucketId, startTileIndex, numTilesToRequest))
                        .then(setBucketStoriesReady(bucket));

                }

            };
        }

        function waitForFeedReady(bucketId) {
            //resolve for this promise happens in the generateStoriesAndDirectives function
            return new Promise(function(resolve, reject) {
                var feedFactoryReady = !AuthFactory.isAppLoggedIn(),
                    feedFactoryReadyEvent,
                    timeoutPromise;

                //we listen for different events so that buckets that don't have friend
                //dependencies can load faster
                if (bucketConfigs[bucketId].requiresFriends) {
                    feedFactoryReady = feedFactoryReady || FeedFactory.isOwnedGamesFriendsPlayingReady();
                    feedFactoryReadyEvent = 'story:ownedGamesFriendsInfoReady';
                } else {
                    feedFactoryReady = feedFactoryReady || FeedFactory.isOwnedGameInfoReady();
                    feedFactoryReadyEvent = 'story:ownedGameInfoReady';
                }

                //if the feed factory is not ready, lets listen for the event
                if (!feedFactoryReady) {


                    //give it 30 seconds and time out
                    timeoutPromise = $timeout(function() {
                        reject(new Error('[DiscoveryStoryFactory:waitForFeedReady timedout]'));
                    }, 30000, false);

                    FeedFactory.events.once(feedFactoryReadyEvent, function() {
                        $timeout.cancel(timeoutPromise);
                        resolve();
                    });
                } else {
                    resolve();
                }

            });
        }

        function onAuthChange(loginObj) {
            if (loginObj && !loginObj.isLoggedIn) {
                resetFlagsAndGlobals();
            }
        }

        AuthFactory.events.on('myauth:change', onAuthChange);

        return {
            events: myEvents,
            reset: function() {
                resetFlagsAndGlobals();
            },

            updateAutoStoryTypeConfig: function(bucketId, storyId, incomingConfig) {
                var curStoryType = {};
                Origin.utils.mix(curStoryType, discoveryTilesConfigBase[storyId]);
                Origin.utils.mix(curStoryType, incomingConfig);
                curStoryType.priority = Number(curStoryType.priority);
                curStoryType.diminish = Number(curStoryType.diminish);
                curStoryType.limit = Number(curStoryType.limit);

                addToStoryTypeConfig(curStoryType, bucketId, storyId);

            },
            updateProgramStoryTypeConfig: function(bucketId, incomingConfig) {
                var curStoryType = {
                        diminish: 0,
                        limit: 1,
                        directive: '<origin-home-discovery-tile><origin-home-discovery-tile-programmable></origin-home-discovery-tile-programmable></origin-home-discovery-tile>',
                        feedData: [{
                            image: incomingConfig.image,
                            description: incomingConfig.description,
                            authorname: incomingConfig.authorname,
                            authorhref: incomingConfig.authorhref,
                            authordescription: incomingConfig.authordescription,
                            requestedfriendtype: incomingConfig.friendslisttype,
                            friendstext: incomingConfig.friendstext,
                            friendspopouttitle: incomingConfig.friendspopouttitle,
                            offerid: incomingConfig.offerid,
                            showprice: incomingConfig.showprice
                        }]
                    },
                    storyId = 'PT_' + programmableIndex;
                programmableIndex++;

                Origin.utils.mix(curStoryType, incomingConfig);
                switch (incomingConfig.placementtype) {
                    case 'priority':
                        curStoryType.priority = Number(incomingConfig.placementvalue);
                        break;
                    case 'position':
                        curStoryType.position = Number(incomingConfig.placementvalue);
                        break;
                }


                addToStoryTypeConfig(curStoryType, bucketId, storyId);

            },
            getNumBuckets: function() {
                return bucketConfigs.length;
            },
            /**
             * get the next recommended friend
             * @return {object} retName an object returning the next friend id and mutual friends
             */
            getNextRecFriend: function() {
                var friendInfo = null,
                    i = 0;

                for (i = 0; i < bucketConfigs.length; i++) {
                    //find the bucket that has the recommended friend (there is only one)
                    var tileConfig = bucketConfigs[i].storyTypes.HT_04;
                    if (tileConfig) {
                        //if we found it and there is still data left, lets get the next one otherwise return a null object indicating theres no more
                        var index = tileConfig.numFeedDataUsed;
                        if (index < tileConfig.feedData.length) {
                            tileConfig.numFeedDataUsed++;
                            friendInfo = tileConfig.feedData[index];
                            break;
                        }
                    }
                }

                return friendInfo;
            },
            /**
             * @ngdoc service
             * @name origin-components.factories.DiscoveryStoryFactory
             * @param {string} bucketId identifier for the bucket we want the tiles for
             * @param {string} startTileIndex index of story we want to start with
             * @param {string} numTilesToRequest number of tiles we want
             * @description
             *
             * function returns a set of tile directives with attributes
             */
            getTileDirectivesForBucket: function(bucketId, startTileIndex, numTilesToRequest) {
                return waitForFeedReady(bucketId)
                    .then(generateStoriesAndDirectives(bucketId, startTileIndex, numTilesToRequest));


            }
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function DiscoveryStoryFactorySingleton($timeout, UtilFactory, FeedFactory, PriorityEngineFactory, ComponentsCMSFactory, GamesDataFactory, ComponentsLogFactory, AuthFactory, ArrayHelperFactory, ObjectHelperFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('DiscoveryStoryFactory', DiscoveryStoryFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.DiscoveryStoryFactory

     * @description
     *
     * DiscoveryStoryFactory
     */
    angular.module('origin-components')
        .factory('DiscoveryStoryFactory', DiscoveryStoryFactorySingleton);
}());