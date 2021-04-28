
/**
 * Factory to be used to communicate with the youtube and handle youtube logic
 * @file youtube.js
 */
(function() {
    'use strict';
    var VIDEO_THUMBNAIL = "//img.youtube.com/vi/%videoId%/%offset%.jpg";
    var VIDEO_URL = "//www.youtube.com/embed/{videoId}?wmode=opaque&rel=0&enablejsapi=1&iv_load_policy=3&showinfo=0&modestbranding=1&hl={hl}";
   	var PLAYER_ID_PREFIX = 'youTubePlayer';
    var PLAYER_SELECTOR = '.youtube-video-target';
    var YOUTUBE_API = '//www.youtube.com/iframe_api';
    var YOUTUBE_ID = 'youtubeScript';
    var YOUTUBE_READY_EVENT= 'youtubeReady';

    var DEFAULT_OPTIONS = {
        'wmode': 'opaque',
        'rel': 0,
        'enablejsapi': 1,
        'iv_load_policy': 3,
        'showinfo': 0,
        'html5': 1,
        'modestbranding': 1,
        'hl': 'en'
    };

    function YoutubeFactory($httpParamSerializerJQLike, UtilFactory) {
        var events = new Origin.utils.Communicator();

        function initYoutubeVideo(playerId, options, videoId, onPlayerReady, width, height) {
            var player = new window.YT.Player(playerId, {
                videoId: videoId,
                playerVars: mergeOptions(options),
                width: width,
                height: height,
                events: {
                    'onReady': onPlayerReady
                }
            });
            return player;
        }

        function isScriptLoaded() {
            return (document.getElementById(YOUTUBE_ID) !== null);
        }

        function isYoutubeReady() {
            return (typeof(window.YT) !== 'undefined');
        }

        function fireYoutubeReady() {
            events.fire(YOUTUBE_READY_EVENT);
        }

        function createScriptElement(){
            var youtubeScript = document.createElement('script');
            youtubeScript.src = YOUTUBE_API;
            youtubeScript.id = YOUTUBE_ID;
            return youtubeScript;
        }

        function loadYoutubeScript() {
            if(isScriptLoaded()) {
                if(isYoutubeReady()) {
                    fireYoutubeReady();
                }
            } else{
                var youtubeScript = createScriptElement();
                var firstScriptTag = document.getElementsByTagName('script')[0];
                firstScriptTag.parentNode.insertBefore(youtubeScript, firstScriptTag);
                window.onYouTubeIframeAPIReady = fireYoutubeReady;
            }
        }

        /**
         * builds the YouTube stil image Url based on videoId
         * @return {string} url of youtube still image to embed.
         */
        function getThumbnailImage(videoId, offset) {
            offset = offset || 0;

            return VIDEO_THUMBNAIL.replace("%videoId%", videoId).replace("%offset%", offset);
        }

        function getStillImage(videoId) {
            return VIDEO_THUMBNAIL.replace("%videoId%", videoId).replace("%offset%", 'maxresdefault');
        }

        function createPlayerId() {
            return PLAYER_ID_PREFIX + UtilFactory.guid();
        }

        function getPlayer(elem) {
            return angular.element(angular.element(elem).find(PLAYER_SELECTOR));
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
            return Origin.locale.locale().toLowerCase().substring(0,2);
        }

        function mergeOptions(options) {
            return angular.extend({}, options, DEFAULT_OPTIONS);
        }

        function optionsToString(options) {
            return $httpParamSerializerJQLike(options);
        }

        return {
            getPlayer: getPlayer,
            getStillImage: getStillImage,
            getYoutubeUrl: getYoutubeUrl,
            createPlayerId: createPlayerId,
            initYoutubeVideo: initYoutubeVideo,
            loadYoutubeScript: loadYoutubeScript,
            events: events,
            onReady: YOUTUBE_READY_EVENT,
            mergeOptions: mergeOptions,
            getThumbnailImage: getThumbnailImage
        };

    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function YoutubeFactorySingleton($httpParamSerializerJQLike, UtilFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('YoutubeFactory', YoutubeFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.YoutubeFactory
     * @description
     *
     * YoutubeFactory
     */
    angular.module('origin-components')
        .factory('YoutubeFactory', YoutubeFactorySingleton);
}());
