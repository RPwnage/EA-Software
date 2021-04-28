/**
 * @file home/priorityengine.js
 */
(function() {
    'use strict';

    function PriorityEngineFactory() {

        var priorityDepot = [],
            positionDepot = [],
            storyHeap = [],
            storyStats = {};

        /**
         * The object used to house the information for a single story type
         */
        function StoryConfigObject(incoming, type) {
            this.priority = incoming.priority;
            this.diminish = incoming.diminish;
            this.limit = incoming.limit;
            this.id = type;
            this.numStories = 0;

            //check limits
            if ((this.limit >= 0) && (this.numStories >= this.limit)) {
                this.priority = 0;
            }
        }

        StoryConfigObject.prototype.update = function() {
            //decrease priority by diminish value
            this.priority -= this.diminish;
            this.numStories++;

            if ((this.priority < 0) || ((this.limit >= 0) && (this.numStories >= this.limit))) {
                this.priority = 0;
            }            
        };


        function comparePriority(a, b) {
            return b.priority - a.priority;
        }

        function comparePosition(a, b) {
            return a.position - b.position;
        }

        /**
         * Separate the passed in configs into two depots:
         * one that uses priority
         * one that uses position
         */
        function initPriorityEngine(incomingStoryTypes) {
            storyHeap = [];
            priorityDepot = [];
            positionDepot = [];
            storyStats = {};

            for (var types in incomingStoryTypes) {
                if (incomingStoryTypes.hasOwnProperty(types)) {
                    //if there is a priority value add it to priority depot
                    if (incomingStoryTypes[types].priority) {
                        priorityDepot.push(new StoryConfigObject(incomingStoryTypes[types], types));

                    } else //check if it has a position property and add to position depot
                    if (incomingStoryTypes[types].hasOwnProperty('position')) {
                        positionDepot.push({
                            id: types,
                            position: incomingStoryTypes[types].position
                        });
                    }

                    //intialize the counts for each story type
                    storyStats[types] = {
                        id: types,
                        base: 0,
                        count: 0
                    };
                }
            }

            positionDepot.sort(comparePosition);
        }

        /**
         * Adds a story that uses priority to the storyHeap
         */
        function addPriorityStory() {
            if (priorityDepot.length) {
                var storyType = priorityDepot[0];
                if (storyType.priority) {
                    storyHeap.push({
                        index: storyType.numStories,
                        priority: storyType.priority,
                        id: storyType.id
                    });
                    storyStats[storyType.id].count++;
                    storyType.update();
                }

                priorityDepot = priorityDepot.sort(comparePriority);

                return (priorityDepot[0].priority);
            } else {
                return false;
            }
        }

        /**
         * Adds a story that uses position on the story heap
         */
        function addPositionStory(currentPos) {
            while (positionDepot.length && (currentPos === positionDepot[0].position)) {
                storyHeap.push({
                    index: 0,
                    id: positionDepot[0].id,

                });
                positionDepot.shift();
            }
            return true;
        }

        /**
         * Adds a story to the story heap
         */
        function addStory() {
            var currentPos = storyHeap.length + 1;
            //see if there are any values in the positionDepot or if the front of the position depot matches the current position
            if (!positionDepot.length || (positionDepot[0].position !== currentPos)) {
                return addPriorityStory();
            } else {
                return addPositionStory(currentPos);
            }
        }

        /**
         * generates the story heap
         */
        function buildInteralList() {
            var i = 0;

            //set a max number of stories
            var maxStories = 5000;
            storyHeap = [];
            //sort once for initial depot
            priorityDepot = priorityDepot.sort(comparePriority);

            for (i = 0; i < maxStories; i++) {
                //lets keep adding stories till we hit the max or run out of available stories
                if (!addStory()) {
                    break;
                }
            }
            return storyHeap;
        }

        return {
            generateList: function(storyConfig) {
                //setup story type config objects
                initPriorityEngine(storyConfig);

                //generate list of stories from config
                buildInteralList();
                return {
                    list: storyHeap,
                    stats: storyStats
                };
            },
        };

    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function PriorityEngineFactorySingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('PriorityEngineFactory', PriorityEngineFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.PriorityEngineFactory
     
     * @description
     *
     * PriorityEngineFactory
     */
    angular.module('origin-components')
        .factory('PriorityEngineFactory', PriorityEngineFactorySingleton);
}());