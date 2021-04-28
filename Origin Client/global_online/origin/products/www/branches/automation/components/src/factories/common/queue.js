/**
 * @file common/queue.js
 *
 */
(function () {
    'use strict';

    function QueueFactory() {

        var queues = {};
        var followers = {};
        var followerId = 0;

        /**
         * Crates a queue & follower
         * @param queueName
         */
        function createQueue(queueName) {
            if (!queues[queueName]) {
                queues[queueName] = [];
                followers[queueName] = [];
            }
        }

        /**
         * does Queue have any elements
         * @param queueName
         * @returns {*|boolean}
         */
        function hasElement(queueName) {
            return (queues[queueName] && queues[queueName].length > 0);
        }

        function hasFollower(queueName) {
            return followers[queueName].length > 0;
        }

        function getQueue(queueName) {
            return queues[queueName];
        }

        /**
         *
         * @param queueName
         * @param object
         */
        function enqueue(queueName, object) {
            createQueue(queueName);
            getQueue(queueName).push(object);
            notifyFollowers(queueName);
        }

        /**
         *
         * @param queueName
         * @returns {*}
         */
        function dequeue(queueName) {
            if (hasElement(queueName)) {
               return getQueue(queueName).shift();
            }
        }

        /**
         *
         * @param queueName
         * @returns {*}
         */
        function dequeueAll(queueName) {
            if (hasElement(queueName)) {
                var returnValue = [].concat(getQueue(queueName));
                queues[queueName] = [];
                return returnValue;
            }
        }

        /**
         * Notify followers of a change
         * @param queueName
         */
        function notifyFollowers(queueName) {
            if (hasFollower(queueName)) {
                var object = dequeue(queueName);
                _.forEach(followers[queueName], function (listener) {
                    listener.callback(object);
                });
            }
        }

        function notifyFollowerImmediately(queueName, listener) {
            var correctContent = dequeueAll(queueName);
            var callback = listener.callback;
            _.forEach(correctContent, function(content) {
                callback(content);
            });
        }

        /**
         * removes a follower from follower's list
         * @param queueName
         * @param listenerToDelete
         */
        function unfollowQueue(queueName, listenerToDelete) {
            _.remove(followers[queueName], function (listener) {
                return listenerToDelete.id === listener.id;
            });
        }

        /**
         * Creates a handle to unfollow a enqueue
         * @param queueName
         * @param listener
         * @returns {Function}
         */
        function unfollowQueueHandler(queueName, listener) {
            return function () {
                unfollowQueue(queueName, listener);
            };
        }

        /**
         * Follow changes in a enqueue (enqueue and dequeue events)
         * @param queueName
         * @param callback
         * @returns {Function} function to unfollow enqueue
         */
        function followQueue(queueName, callback) {
            createQueue(queueName);
            var listener = {id: followerId++, callback: callback};
            followers[queueName].push(listener);
            //notify this listener of all the content in the directive.
            notifyFollowerImmediately(queueName, listener);
            return unfollowQueueHandler(queueName, listener);
        }

        return {
            enqueue: enqueue,
            followQueue: followQueue
        };

    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function QueueFactorySingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('QueueFactory', QueueFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.QueueFactory

     * @description Queue factory that queues events/objects until a listener starts listening to that queue.
     * Once a listener registers itself, all object will be consumed by that listener.
     *
     * Listener will consume (dequeue) any new object that is queued.
     *
     * QueueFactory
     */
    angular.module('origin-components')
        .factory('QueueFactory', QueueFactorySingleton);

})();