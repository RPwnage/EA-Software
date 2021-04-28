/**
 * @file home/discoverystory.js
 */
(function() {
    'use strict';

    function DiscoveryStoryFactory($timeout, UtilFactory, FeedFactory, PriorityEngineFactory, ComponentsCMSFactory, ComponentsLogFactory) {
        var myEvents = new Origin.utils.Communicator(),
            tileConfigReady = false,
            bucketConfigs = [],
            programmableIndex = 0,
            discoveryTilesConfigBase = {
                HT_01: {
                    gameDataFeedFunc: FeedFactory.getFriendsPlayingOwned,
                    feedId: 'og-friendsplaying',
                    requiresFriends: true
                },
                HT_02: {
                    gameDataFeedFunc: FeedFactory.getRecentlyReleased,
                    feedId: 'og-recentlyreleased'
                },
                HT_03: {
                    gameDataFeedFunc: FeedFactory.getNotRecentlyPlayed,
                    feedId: 'og-notplayedrecently'
                },
                HT_04: {
                    feedId: 'recfriends',
                    requiresRec: true,
                    numFeedDataUsed: 0
                },
                HT_05: {
                    gameDataFeedFunc: FeedFactory.getNeverPlayed,
                    feedId: 'og-neverplayed'
                },
                HT_06: {
                    feedId: 'newdlc',
                    gameDataFeedFunc: FeedFactory.getUnownedDLC,
                },
                HT_07: {
                    gameDataFeedFunc: FeedFactory.getPopularUnownedGames,
                    feedId: 'ug-friendsplaying',
                    requiresFriends: true
                },
                HT_21: {
                    feedId: 'og-recentlyplayed',
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
         * retrieves any game related feeds from FeedFactory
         */
        function buildGameDataFeeds(bucketId) {
            var bucket = bucketConfigs[bucketId].storyTypes;
            var usedProductIds = [];
            //all game data feeds are needed before we run the priority engine because it affects how many of each story type we can show.
            for (var p in bucket) {
                if (bucket.hasOwnProperty(p)) {
                    var c = bucket[p];
                    if (c.gameDataFeedFunc) {
                        var newLimit = -1;
                        var returnedData = c.gameDataFeedFunc(c);
                        //for the tiles that rely on feed information such as friends playing, we need to
                        //adjust the number of tiles we have available based on the retrieved information
                        c.feedData = [];

                        for (var i = 0; i < returnedData.length; i++) {
                            var productId = returnedData[i];
                            if (usedProductIds.indexOf(productId) < 0) {
                                c.feedData.push({
                                    offerId: productId
                                });
                            }
                            usedProductIds.push(productId);
                        }
                        newLimit = c.feedData.length;

                        //adjust the length if its smaller or there was previously no limit set
                        if (c.limit === -1 || newLimit < c.limit) {
                            c.limit = newLimit;
                        }
                    }
                }
            }
        }

        function processFeedError(error) {
            ComponentsLogFactory.error('[DiscoveryStoryFactory:getFeedsFromServer]', error.stack);
        }

        function processFeedResponse(type, feedResponses) {
            return function(response) {
                feedResponses[type] = response;
            };
        }

        /**
         * retrieves any feeds needed from the recommendation engine backend
         */
        function getFeedsFromServer(stats, bucket) {
            var feedResponses = {},
                feedsToRetrieve = [];

            //see which of our types get data from feeds
            for (var p in stats) {
                if (stats.hasOwnProperty(p)) {
                    var c = bucket[p];
                    if (c.requiresRec && c.feedId && stats[p].count > 0) {

                        //put this in for now untl we build out the real friends service
                        //we will retrieve the max amount of hardcoded feed data so that we can show the dismiss friends logic
                        if (p === 'HT_04') {
                            stats[p].count = 5;
                        }
                        feedsToRetrieve.push(Origin.feeds.retrieveStoryData(bucket[p].feedId, stats[p].base, stats[p].count, Origin.locale.locale()).then(processFeedResponse(p, feedResponses)).catch(processFeedError));
                    }
                }
            }

            return Promise.all(feedsToRetrieve).then(function() {
                return feedResponses;
            });
        }

        function assignToBucketFeeds(bucket, storyHeapInfo) {
            return function(responses) {
                for (var r in responses) {
                    if (responses.hasOwnProperty(r)) {
                        bucket[r].feedData = responses[r];
                    }
                }
                return {
                    config: bucket,
                    story: storyHeapInfo
                };
            };
        }

        /**
         * generates a list of discovery tile stories for a give bucket
         */
        function generateStoryList(bucketId) {
            var bucket = bucketConfigs[bucketId].storyTypes,
                storyHeapInfo = {};

            //add the game feed data to the bucket object
            buildGameDataFeeds(bucketId);

            //run the configs through the Priority Engine to generate a heap of stories
            storyHeapInfo = PriorityEngineFactory.generateList(bucket, 2000);

            return getFeedsFromServer(storyHeapInfo.stats, bucket).then(assignToBucketFeeds(bucket, storyHeapInfo));
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

                //if we got a valid property lets mark one of those feeds as used
                if (prop) {
                    tileConfig.numFeedDataUsed++;
                }

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
         * takes directives retrieved from CMS and adds attributes built from the feeds
         */
        function buildDirectivesWithAttributes(start, numTiles) {
            return function(storyInfo) {
                var directivesList = [],
                    bucket = storyInfo.config,
                    bucketStoryInfo = storyInfo.story,
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
                                //add cms directive datato the promise array
                                cmsRequestPromises.push(ComponentsCMSFactory.retrieveDiscoveryStoryData(tileInfo.feedId, offerId)
                                    .then(mixFeed, handleCMSRetrieveError).catch(handleCMSRetrieveError));
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

        function assignStoryToBucket(bucket) {
            return function(response) {
                bucket.stories = response.story;
                return response;
            };
        }

        function setBucketStoriesReady(bucket) {
            return function(response) {
                bucket.storiesReady = true;
                return response;
            };
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
                    return buildDirectivesWithAttributes(startTileIndex, numTilesToRequest)({
                            config: bucket.storyTypes,
                            story: bucket.stories
                        })
                        .then(setupStoryListNextIndex(bucketId, startTileIndex, numTilesToRequest));
                } else {
                    //otherwise we need to go generate the stories
                    return generateStoryList(bucketId)
                        .then(assignStoryToBucket(bucket))
                        .then(buildDirectivesWithAttributes(startTileIndex, numTilesToRequest))
                        .then(setupStoryListNextIndex(bucketId, startTileIndex, numTilesToRequest))
                        .then(setBucketStoriesReady(bucket));

                }

            };
        }

        function waitForFeedReady(bucketId) {
            //resolve for this promise happens in the generateStoriesAndDirectives function
            return new Promise(function(resolve, reject) {
                var feedFactoryReady = !Origin.auth.isLoggedIn(),
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

        Origin.events.on(Origin.events.AUTH_LOGGEDOUT, resetFlagsAndGlobals);

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
                            description: incomingConfig.description
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
            getNextRecFriend: function() {
                //TODO: revisit this when we build the actual backend
                //
                //find rec bucket, each story type can be in only one bucket
                var userid = null;
                for (var b in bucketConfigs) {
                    if (bucketConfigs.hasOwnProperty(b)) {
                        for (var t in bucketConfigs[b].storyTypes) {
                            //immutable ID for rec friends
                            if (t === 'HT_04') {
                                var tileConfig = bucketConfigs[b].storyTypes[t];
                                var index = tileConfig.numFeedDataUsed;
                                if (index < tileConfig.feedData.length) {
                                    tileConfig.numFeedDataUsed++;
                                    userid = tileConfig.feedData[index].userId;
                                }
                            }
                        }
                    }
                }

                return userid;
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
    function DiscoveryStoryFactorySingleton($timeout, UtilFactory, FeedFactory, PriorityEngineFactory, ComponentsCMSFactory, ComponentsLogFactory, SingletonRegistryFactory) {
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