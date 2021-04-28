/**
 * @file gamelibrary/ogdheader.js
 */
(function() {

    'use strict';

    var CONTEXT_NAMESPACE = 'origin-gamelibrary-ogd-header',
        VIOLATOR_CONTAINER_ELEMENT_CLASSNAME = '.origin-ogd-messages',
        VIDEO_CONTAINER_ELEMENT_CLASSNAME = '.origin-ogd-backgroundvideo',
        INLINE_VIOLATOR_COUNT = 2,
        VIOLATOR_CONTEXT = 'importantinfo',
        HEADER_MODEL_NAME = 'header',
        VIOLATOR_LOAD_DELAY_MS = 900,
        VIDEO_LOAD_DELAY_MS = 800;

    function originGamelibraryOgdHeader(ComponentsConfigFactory, ObjectHelperFactory, OgdHeaderObserverFactory, ViolatorObserverFactory, AppCommFactory, $compile, UtilFactory, ScopeHelper) {
        function originGamelibraryOgdHeaderLink(scope, element) {
            var messages = {
                    gamemessages: _.partial(UtilFactory.getLocalizedStr, scope.gamemessages, CONTEXT_NAMESPACE, 'gamemessages', _),
                    viewmoremessagesctaCallback: _.partial(UtilFactory.getLocalizedStr, scope.viewmoremessagescta, CONTEXT_NAMESPACE, 'viewmoremessagescta', _,  _),
                    closemessagescta: UtilFactory.getLocalizedStr(scope.closemessagescta, CONTEXT_NAMESPACE, 'closemessagescta')
                };

            scope.nonorigingameLoc = UtilFactory.getLocalizedStr(scope.nonorigingame, CONTEXT_NAMESPACE, 'nonorigingame');

            /**
             * Close the ogd slideout
             */
            scope.closeOgd = function() {
                AppCommFactory.events.fire('uiRouter:go', 'app.game_gamelibrary');
            };

            /**
             * Given new model information clear and reraw the new dom elements
             * @param  {Object} model the violator model data
             */
            function updateViolatorDomFromModel(model) {
                if(!model) {
                    return;
                }

                var violatorContainer = element.find(VIOLATOR_CONTAINER_ELEMENT_CLASSNAME);

                violatorContainer.empty();

                _.forEach(model.inlineMessages, function(inlineMessage) {
                    violatorContainer.append($compile(inlineMessage)(scope));
                });

                if(model.readmore) {
                    scope.readmore = model.readmore;
                    violatorContainer.append($compile(model.readmoreText)(scope));
                }

                ScopeHelper.digestIfDigestNotInProgress(scope);
            }

            /**
             * Add video backdrop to the header
             */
            function addVideoBackgroundDom(backgroundVideoId) {
                if(!backgroundVideoId) {
                    return;
                }

                var videoContainer = element.find(VIDEO_CONTAINER_ELEMENT_CLASSNAME);

                videoContainer.append($compile(UtilFactory.buildTag('origin-background-video', {'video-id': backgroundVideoId}))(scope));

                ScopeHelper.digestIfDigestNotInProgress(scope);
            }

            /**
             * Deferred loading of the violators to speed up the OGD slide out
             */
            function getViolatorObserver() {
                ViolatorObserverFactory.getObserver(scope.offerId, VIOLATOR_CONTEXT, INLINE_VIOLATOR_COUNT, messages, scope, UtilFactory.unwrapPromiseAndCall(updateViolatorDomFromModel));
            }

            var delayedAddVideoBackground = _.debounce(addVideoBackgroundDom, VIDEO_LOAD_DELAY_MS),
                delayedGetViolatorObserver = _.debounce(getViolatorObserver, VIOLATOR_LOAD_DELAY_MS);

            /**
             * Clean up event listeners
             */
            function destroyEventListeners() {
                delayedAddVideoBackground.cancel();
                delayedGetViolatorObserver.cancel();
            }

            scope.$on('$destroy', destroyEventListeners);
            scope.$watchOnce([HEADER_MODEL_NAME, 'backgroundVideoId'].join('.'), delayedAddVideoBackground);

            OgdHeaderObserverFactory.getObserver(scope.offerId, CONTEXT_NAMESPACE, scope, HEADER_MODEL_NAME);
            delayedGetViolatorObserver();
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                offerId: '@offerid',
                viewmoremessagescta: '@',
                closemessagescta: '@',
                gamemessages: '@',
                nonorigingame: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdheader.html'),
            link: originGamelibraryOgdHeaderLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgdHeader
     * @restrict E
     * @element ANY
     * @scope
     * @param {OfferId} offerid the offer id
     * @param {LocalizedString} viewmoremessagescta view more violator messages pluralized string. use %messages% as the count placeholder
     * @param {LocalizedString} closemessagescta the close button text for the extra masssages dialog
     * @param {LocalizedString} gamemessages the extra mesages dialog heading. use %game% as the game name placeholder
     * @param {LocalizedString} gamename a game name overide
     * @param {LocalizedString} nonorigingame the non origin game banner text used to differentiate non origin games from EA ones
     * @param {Video} backgroundvideoid the background video id to use
     * @param {ImageUrl} backgroundimage a large, static background image asset in case there is no video merchandised
     * @param {ImageUrl} gamelogo a game logo to overlay on the background image
     * @param {ImageUrl} gamepremiumlogo the premium status icon to use on the header
     * @description
     *
     * Owned game details slide out header section (note some of the variables below are used in models/ogdheader.js)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-ogd>
     *             <origin-gamelibrary-ogd-header
     *              offerid="OFB-EAST:50885"
     *              viewmoremessagescta="{0} End of Messages | {1} +1 other message | ]0,+Inf] +%messages% other messages"
     *              closemessagescta="Close"
     *              gamemessages="%game% Messages"
     *              gamename="Dragon Age!"
     *              nonorigingame="Non-Origin Game"
     *              backgroundvideoid="FC5hh0YMTwc"
     *              backgroundimage="http://example.com/static-bg.jpg"
     *              gamelogo="https://example.com/logo.jpg"
     *              gamepremiumlogo="https://example.com/game-logo.jpg">
     *             </origin-gamelibrary-ogd-header>
     *         </origin-gamelibrary-ogd>
     *     </file>
     * </example>
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originGamelibraryOgdHeader', originGamelibraryOgdHeader);
}());