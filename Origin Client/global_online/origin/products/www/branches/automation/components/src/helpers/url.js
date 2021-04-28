/**
 * @file url.js
 */
(function() {
    'use strict';

    function UrlHelper(ComponentsConfigHelper, Md5Factory, ObjectHelperFactory, $state) {

        // Set up map locale -> epilepsy warning URL
        var specialCaseLocaleMap = {},
            LOCALE_GREEK = 'el_GR',
            LOCALE_SLOVAK = 'sl_SK',
            LOCALE_CHINESE_TAIWAN = 'zh_TW',
            LOCALE_CHINESE = 'zh_CN',
            LOCALE_PORTUGESE_BRAZIL = 'pt_BR',
            LOCALE_SPANISH_MEXICO = 'es_MX',
            USER_PROFILE_ROUTE_STATE_NAME = 'app.profile.user';

        specialCaseLocaleMap[LOCALE_GREEK] = ComponentsConfigHelper.getUrl('greekEpilepsyWarningUrl');
        specialCaseLocaleMap[LOCALE_SLOVAK] = ComponentsConfigHelper.getUrl('slovakEpilepsyWarningUrl');
        specialCaseLocaleMap[LOCALE_CHINESE_TAIWAN] = ComponentsConfigHelper.getUrl('chineseEpilepsyWarningUrl');
        specialCaseLocaleMap[LOCALE_CHINESE] = ComponentsConfigHelper.getUrl('chineseEpilepsyWarningUrl');
        specialCaseLocaleMap[LOCALE_PORTUGESE_BRAZIL] = ComponentsConfigHelper.getUrl('portugeseEpilepsyWarningUrl');
        specialCaseLocaleMap[LOCALE_SPANISH_MEXICO] = ComponentsConfigHelper.getUrl('spanishEpilepsyWarningUrl');


        /**
        * remove a string from the beginning of a string
        * @param {String} str - the string to remove the characters from
        * @param {String} trimStr - the string to remove from the beginning of the string
        * @return {String}
        */
        function trimQueryString(str, trimStr) {
            if (str.indexOf(trimStr) === 0) {
                str = str.slice(trimStr.length);
            }
            return str;
        }

        /**
        * Convert a query string to an object
        * @param {String} str - query string (minus the leading ?)
        * @return {Object}
        */
        function convertQueryStrToObj(str) {
            var obj = {},
                pairs;

            if (_.isString(str) && str.length > 0) {
                pairs = str.split('&');
                _.forEach(pairs, function(pair) {
                    pair = pair.split('=');
                    obj[pair[0]] = decodeURIComponent(pair[1]);
                });
            }
            return obj;
        }

        /**
        * Add to a query string and trim the first characters to normalize things
        * @param {String} originalQueryString - the original string to add parameters to
        * @param {String} queryStringToAdd - the query string to add to the original string
        * @return {String}
        */
        function addToQueryString(originalQueryString, queryStringToAdd) {
            var mergedQueryObj = _.merge(
                    convertQueryStrToObj(trimQueryString(originalQueryString, '?')),
                    convertQueryStrToObj(trimQueryString(queryStringToAdd, '?'))
                ),
                newQueryStr = [];
            _.forEach(Object.keys(mergedQueryObj), function(key) {
                newQueryStr.push(key + '=' + mergedQueryObj[key]);
            });
            return newQueryStr.join('&');
        }

        /**
        * Construct a full url with the url and query params
        * @param {String} url - the path
        * @param {String} queryParams - the query params
        * @return {String} the url
        */
        function buildAbsoluteUrl(path, queryParams) {
            var sep = (path.indexOf('?') >= 0) ? '&' : '?';
            return [
                getWindowOrigin(),
                path.indexOf('/') === 0 ? path : '/' + path,
                queryParams.indexOf(sep) === 0 ? queryParams : sep + queryParams
            ].join('');
        }

        /**
         * Build a URL string from an endpoint and an object of parameters.
         *
         * @param  {string} endpoint Base URL (host + URI)
         * @param  {object} params   Parameters in key value format
         * @return {string}
         * @method buildParameterizedUrl
         */
        function buildParameterizedUrl(endpoint, params) {
            var paramArray = [],
                parameterized;

            _.forEach(params, function (n, key) {
                if (typeof(params[key]) !== 'undefined' && params[key] !== null) {
                    paramArray.push(encodeURIComponent(key) + '=' + encodeURIComponent(params[key]));
                }
            });

            parameterized = paramArray.join('&');

            if (parameterized.length > 0) {
                endpoint += ((endpoint.indexOf('?') === -1) ? '?' : '&') + parameterized;
            }

            return endpoint;
        }

        /**
         * Relace the keys with the value in the string.
         * @param  {string} str The target string
         * @param  {object} obj The keys are replaced with the values
         * @return {string}     The string with the new values added in.
         */
        function replaceValues(str, obj) {
            _.forEach(obj, function(value, key) {
                var sub = new RegExp(key,"g");
                str = str.replace(sub, value);
            });
            return str;
        }

         /**
         * Get a full populated URL from the configs
         * @param  {sting} id            The config id
         * @param  {string} endpoint     The endpoint you want
         * @param  {object} replacements The values you want to be replaced and what to replace them with {'replaceMe': 'newValue'}
         * @return {string}              The url you requested
         */
        function buildConfigUrl(id, endpoint, replacements) {
            var configObj = ComponentsConfigHelper.getUrl(id);
            var url = configObj.origin;

            if(endpoint){
                url += configObj.endpoints[endpoint];
            }

            if(replacements) {
                url = replaceValues(url, replacements);
            }

            return url;
        }

        function buildAchievementImageUrl(achievementSetId) {
            var achievementSetIdMD5 = Md5Factory.md5(achievementSetId + "overviewAchievement");
            return ComponentsConfigHelper.getUrl('achievementTileImageUrl').replace('{achievementSetId}', achievementSetId).replace('{achievementSetIdMD5}', achievementSetIdMD5);
        }

        /**
          * builds the YouTube Url based on videoId
          * @return {string} url of youtube video to embed.
          */
        function buildYouTubeEmbedUrl(videoId) {
            var hl = Origin.locale.locale().toLowerCase().substring(0, 2);
            var replaceObj = {
                '{videoId}': videoId,
                '{hl}': hl
            };
            return replaceValues(ComponentsConfigHelper.getUrl('youTubeEmbedUrl'),replaceObj);
        }

        /**
          * builds the Twitch Url based on channel
          * @return {string} url of twitch channel to embed.
          */
        function buildTwitchChannelEmbedUrl(channel, autoplay) {
            var replaceObj = {
                '{channel}': channel,
                '{autoplay}': autoplay
            };
            return replaceValues(ComponentsConfigHelper.getUrl('twitchChannelUrl'),replaceObj);
        }

        /**
          * builds the Twitch Url based on video
          * @return {string} url of twitch video to embed.
          */
        function buildTwitchVideoEmbedUrl(video, autoplay) {
            var replaceObj = {
                '{video}': video,
                '{autoplay}': autoplay
            };
            return replaceValues(ComponentsConfigHelper.getUrl('twitchVideoUrl'),replaceObj);
        }

        /**
          * builds the Twitch Url based on clip
          * @return {string} url of twitch clip to embed.
          */
        function buildTwitchClipEmbedUrl(clip, autoplay) {
            var replaceObj = {
                '{clip}': clip,
                '{autoplay}': autoplay
            };
            return replaceValues(ComponentsConfigHelper.getUrl('twitchClipUrl'),replaceObj);
        }

        /**
         * builds the epilepsy warning URL based on the locale/language
         * @param  {string} locale The user's locale
         * @return {string} url of epilepsy warning
         */
        function buildEpilepsyWarningUrl(locale) {
            var lang = locale.split('_')[0].toUpperCase();

            return (specialCaseLocaleMap[locale] ?  specialCaseLocaleMap[locale] :
                ComponentsConfigHelper.getUrl('epilepsyWarningUrl')
                    .replace('{locale}', locale)
                    .replace('{lang}', lang));
        }

        /**
         * Return Origin client download link specific to your platform (or undefined if no link for your platform)
         * @return {string|undefined} download link
         */
        function getClientDownloadLink() {
            return ObjectHelperFactory.getProperty(Origin.utils.os())(ComponentsConfigHelper.getUrl('clientDownload'));
        }

        /**
         * Returns the origin of the current window. Ex. the origin of https://local.app.origin.com/?cmsstage=preview/store/pdp/ is https://local.app.origin.com
         * @return {string} Window origin
         */
        function getWindowOrigin() {
            var origin;

            //IE10 doesn't support window.location.origin
            if (!angular.isDefined(window.location.origin)) {
                origin = window.location.protocol + '//' + window.location.host;
            } else {
                origin = window.location.origin;
            }

            return origin;
        }

        /**
         * Extract url search params by name
         * @param name
         * @param path (optional)
         */
        function extractSearchParamByName(name, path){
            var url = path || window.location.search,
                regex = new RegExp('[?&]' + name + '=([^&;]+?)(&|#|;|$)', 'i'),
                results = regex.exec(url);

            //The regex returns matched string 'name=value' and the captured string after '='
            //So we are checking if the array is >= 2
            if (results && angular.isArray(results) && results.length >= 2){
                return decodeURIComponent(results[1].replace(/\+/g, ' '));
            }
        }

        function buildLocaleUrl(locale) {
            var href = window.location.href,
                winOrigin = getWindowOrigin(),
                currentLocale = Origin.locale.locale().toLowerCase().replace("_", "-"),
                country = Origin.locale.threeLetterCountryCode().toLowerCase();
            if (href.indexOf(currentLocale) > -1) {
                return href.replace(currentLocale, locale);
            } else {
                return href.replace(winOrigin, winOrigin + "/" + country + "/" + locale);
            }
        }

        function buildUserProfileUrl(userId){
            return $state.href(USER_PROFILE_ROUTE_STATE_NAME, {
                id: userId
            });
        }

        return {
            buildConfigUrl: buildConfigUrl,
            buildParameterizedUrl: buildParameterizedUrl,
            buildYouTubeEmbedUrl: buildYouTubeEmbedUrl,
            buildTwitchChannelEmbedUrl: buildTwitchChannelEmbedUrl,
            buildTwitchVideoEmbedUrl: buildTwitchVideoEmbedUrl,
            buildTwitchClipEmbedUrl: buildTwitchClipEmbedUrl,
            getAchievementImageUrl: buildAchievementImageUrl,
            getEpilepsyWarningUrl: buildEpilepsyWarningUrl,
            getClientDownloadLink: getClientDownloadLink,
            getWindowOrigin: getWindowOrigin,
            extractSearchParamByName: extractSearchParamByName,
            addToQueryString: addToQueryString,
            buildAbsoluteUrl: buildAbsoluteUrl,
            convertQueryStrToObj: convertQueryStrToObj,
            buildLocaleUrl: buildLocaleUrl,
            buildUserProfileUrl: buildUserProfileUrl
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function UrlHelperSingleton(ComponentsConfigHelper, Md5Factory, ObjectHelperFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('UrlHelper', UrlHelper, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.UrltHelper

     * @description
     *
     * URL Helper
     */
    angular.module('origin-components')
        .factory('UrlHelper', UrlHelperSingleton);
}());
