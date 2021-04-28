/**
 * Factory to be used to communicate with the youtube and handle youtube logic
 * @file youtube.js
 */
(function () {
    'use strict';
    var VIDEO_THUMBNAIL = "//img.youtube.com/vi/%videoId%/%offset%.jpg";
    var VIDEO_URL = "//www.youtube.com/embed/{videoId}?wmode=opaque&rel=0&enablejsapi=1&iv_load_policy=3&showinfo=0&modestbranding=1&hl={hl}";
    var PLAYER_ID_PREFIX = 'youtubePlayer';
    var YOUTUBE_API = '//www.youtube.com/iframe_api';
    var YOUTUBE_ID = 'youtubeScript';
    var YOUTUBE_CLASS = 'origin-youtube-video-target';

    var DEFAULT_OPTIONS = {
        wmode: 'opaque',
        enablejsapi: 1,
        'iv_load_policy': 3,
        html5: 1,
        hl: 'en',
        autoplay: 0,
        autohide: 1,
        loop: 1,
        controls: 0,
        modestbranding: 0,
        playsinline: 1,
        rel: 0,
        showinfo: 0
    };

    // YT API events reference at https://developers.google.com/youtube/iframe_api_reference#onStateChange
    var YOUTUBE_PLAYER_STATES = {
        UNSTARTED: -1,
        ENDED: 0,
        PLAYING: 1,
        PAUSED: 2,
        BUFFERING: 3,
        CUED: 5
    };

    var callbackQueue = [];

    function YoutubeFactory($httpParamSerializerJQLike, UtilFactory) {

        var youtubeScriptLoadInProgress = false;

        function createScriptElement() {
            var youtubeScript = document.createElement('script');
            youtubeScript.src = YOUTUBE_API;
            youtubeScript.id = YOUTUBE_ID;
            return youtubeScript;
        }

        function isScriptLoaded() {
            return (document.getElementById(YOUTUBE_ID) !== null || isYoutubeReady());
        }

        function isYoutubeReady() {
            return angular.isDefined(window.YT) && angular.isDefined(window.YT.Player);
        }

        /**
         * Queues promises waiting to create a youtube player
         * @param callback Promise.resolve
         */
        function queuePromiseUntilYoutubeScriptLoads(callback) {
            callbackQueue.push(callback);
        }

        /**
         * Injects youtube script into dom
         */
        function loadYoutubeScript() {
            var youtubeScript = createScriptElement();
            var firstScriptTag = document.getElementsByTagName('script')[0];
            firstScriptTag.parentNode.insertBefore(youtubeScript, firstScriptTag);
        }

        /**
         * Prepares youtube script.
         * @returns {Promise}
         */
        function prepareYoutubeScript() {

            return new Promise(function (resolve) {
                if (isScriptLoaded()) { //if youtube script loaded
                    if (isYoutubeReady()) { //if youtube api ready
                        resolve(); //we have everything we need, resolve so we can create a player.
                    } else { //script has loaded but youtube isn't ready (onYouTubeIframeAPIReady hasn't fired)
                        //queue the promise until onYouTubeIframeAPIReady fires.
                        queuePromiseUntilYoutubeScriptLoads(resolve);
                    }
                } else {
                    if (!youtubeScriptLoadInProgress) {
                        //to avoid race condition and loading youtube script twice
                        //set this flag to true to prevent other components from loading youtube script
                        youtubeScriptLoadInProgress = true;
                        //load the script
                        loadYoutubeScript();
                        //queue the promise until onYouTubeIframeAPIReady is fired.
                        queuePromiseUntilYoutubeScriptLoads(resolve);

                        window.onYouTubeIframeAPIReady = function () {
                            //notify all promises that youtube is ready so they can create a player.
                            _.forEach(callbackQueue, function(callback) {
                                callback();
                            });
                            //clear the queue
                            callbackQueue = [];
                        };
                    } else {
                        queuePromiseUntilYoutubeScriptLoads(resolve);
                    }

                }
            });
        }

        /**
         * Adds a unique id to the element
         * @param elem
         * @returns {string} unique id
         */
        function createUniqueId(elem) {
            if (!elem.hasClass(YOUTUBE_CLASS)) {
                elem.addClass(YOUTUBE_CLASS);
            }
            var playerId = elem.attr('id');
            if (!playerId) {
                playerId = createPlayerId();
                elem.attr('id', playerId);
            }

            return playerId;
        }

        /**
         * Updates player options playlist with video ID, this is to make loop works
         *
         * @param  {object} playerOptions
         * @param {string} videoId
         * @returns {object}
         */
        function updateOptionPlaylist(playerOptions, videoId){
            if (playerOptions.loop === 1){
                playerOptions.playlist = videoId;
            }
            return playerOptions;
        }

        /**
         * Creates a youtube player. Note that this function assumes youtube api is ready.
         * @param playerId
         * @param videoId
         * @param playerOptions
         * @param width
         * @param height
         * @returns {Promise} a promise that wraps around youtube player
         */
        function createPlayer(playerId, videoId, playerOptions, width, height, playerEvents) {

            return new Promise(function (resolve) {
                new window.YT.Player(playerId, {
                    videoId: videoId,
                    playerVars: updateOptionPlaylist(mergeOptions(playerOptions), videoId),
                    width: width,
                    height: height,
                    events: mergeEvents({
                        'onReady': function (event) {
                            resolve(event.target);
                        }
                    }, playerEvents)
                });
            });
        }

        /**
         * builds the YouTube stil image Url based on videoId
         * @return {string} url of youtube still image to embed.
         */
        function getThumbnailImage(videoId, offset) {
            offset = offset || 0;
            return videoId ? VIDEO_THUMBNAIL.replace('%videoId%', videoId).replace('%offset%', offset) : null;
        }

        function getStillImage(videoId) {
            return videoId ? VIDEO_THUMBNAIL.replace('%videoId%', videoId).replace('%offset%', 'maxresdefault') : null;
        }

        /**
         * Generates a unique youtube player id
         * @returns {string}
         */
        function createPlayerId() {
            return PLAYER_ID_PREFIX + UtilFactory.guid();
        }

        /**
         * builds the YouTube Url based on videoId
         * @return {string} url of youtube video to embed.
         */
        function getYoutubeUrl(videoId, options) {
            var url = VIDEO_URL + optionsToString(options);
            return url.replace('{videoId}', videoId).replace('{hl}', getLanguage());
        }

        function getLanguage() {
            return Origin.locale.locale().toLowerCase().substring(0, 2);
        }

        function mergeOptions(options) {
            return _.extend({}, DEFAULT_OPTIONS, options);
        }

        function mergeEvents(defaultEvents, events) {
            // the extend() is ordered in this way to intentionally overwrite an 'onReady' property in the 'events' argument
            return _.extend({}, events, defaultEvents);
        }

        function optionsToString(options) {
            return $httpParamSerializerJQLike(options);
        }

        /**
         * Loads all script necessary to play a youtube video
         * @param element DOM element. Note that youtube will remove this DOM element and replaces it with an iframe
         * @param videoId youtube video id
         * @param playerOptions
         * @param playerWidth width pass 100 for 100%
         * @param playerHeight height pass 100 for 100%
         * @returns {Promise}
         */
        function loadYoutubePlayer(element, videoId, playerOptions, playerWidth, playerHeight, playerEvents) {
            var playerId = createUniqueId(element);
            playerOptions = playerOptions || {playlist: videoId};
            return prepareYoutubeScript().then(function () {
                return createPlayer(playerId, videoId, playerOptions, playerWidth, playerHeight, playerEvents);
            });
        }

        /**
         * Add additional pinEvent Data
         * @param status
         * @returns {{en: string, type: string, service: string, format: string, msg_id: string, status: string, content: string, destination_name: string}
         */
        function packPinEventData(eventData) {
            return {
                'en': 'message',
                'type': 'in-game',
                'service': 'youtube',
                'format': 'video',
                'msg_id': eventData.videoId,
                'status': eventData.status,
                'content': eventData.source,
                'destination_name': eventData.destinationName
            };
        }

        /**
         * Send telemetry to GA/Pin when video starts playing
         */
        function sendStartVideoTelemetryEvent(eventData) {
            // Send telemetry when video started playing
            Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_TILE_VIDEO_STARTED, {
                videoId: eventData.videoId,
                additionalPinEvents: packPinEventData(eventData)
            });
        }

        /**
         * Send telemetry to GA/Pin when video is stopped or modal is closed with the current video time.
         */
        function sendStopVideoTelemetryEvent(eventData) {
            // send telemetry when user closed the dialog
            Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_TILE_VIDEO_ENDED, {
                videoId: eventData.videoId,
                playedTime: eventData.playedTime,
                additionalPinEvents: packPinEventData(eventData)
            });
        }

        return {
            PLAYER_STATES: YOUTUBE_PLAYER_STATES,
            getThumbnailImage: getThumbnailImage,
            getStillImage: getStillImage,
            getYoutubeUrl: getYoutubeUrl,
            loadYoutubePlayer: loadYoutubePlayer,
            sendStartVideoTelemetryEvent: sendStartVideoTelemetryEvent,
            sendStopVideoTelemetryEvent: sendStopVideoTelemetryEvent
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.YoutubeFactory
     * @description
     *
     * YoutubeFactory
     */
    angular.module('origin-components')
        .factory('YoutubeFactory', YoutubeFactory);
}());
