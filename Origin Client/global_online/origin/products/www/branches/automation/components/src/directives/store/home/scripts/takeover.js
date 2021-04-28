/**
 * @file store/home/takeover.js
 */
(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-home-takeover',
        TAKEOVER_WRAPPER_SELECTOR = '.l-origin-store-takeover-wrapper';

    /**
     * A list of text colors
     * @readonly
     * @enum {string}
     */
    var ColorsEnumeration = {
        "dark": "dark",
        "light": "light"
    };

    var BgColorMapping = {
        "dark": "#ffffff",
        "light": "#141b20"
    };

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function originStoreHomeTakeoverCtrl($scope, OcdPathFactory) {
        if ($scope.ocdPath) {
            OcdPathFactory.get($scope.ocdPath).attachToScope($scope, 'model');
        } else {
            $scope.model = {};
        }
    }

    function originStoreHomeTakeover(ComponentsConfigFactory, UtilFactory, CSSUtilFactory, PriceInsertionFactory, OriginTakeoverHelper, DirectiveScope, PdpUrlFactory, AuthFactory, NavigationFactory) {

        function originStoreHomeTakeoverLink(scope, element, attrs, originTakeoverPageController) {
            var $sitestripes = angular.element('.origin-store-sitestripe-wrapper'),
                isOnline = Origin.client.onlineStatus.isOnline();

            function setPageContentVisibility(contentVisibility, requireDigest) {
                scope.isDismissedOrHidden = contentVisibility;
                if (originTakeoverPageController) {
                    if (contentVisibility) {
                        originTakeoverPageController.showPageContent(requireDigest);
                        $sitestripes.show();
                    } else {
                        originTakeoverPageController.hidePageContent(requireDigest);
                        $sitestripes.hide();
                    }
                }
            }

            /**
             * Handles online status change to show and hide takeover
             * @param authObj
             */
            function onlineStatusHandler(authObj) {
                if (authObj.isOnline) {
                    setPageContentVisibility(false, true);
                } else {
                    setPageContentVisibility(true, true);
                }
            }

            function init() {
                scope.isDismissedOrHidden = OriginTakeoverHelper.isTakeoverDismissed(scope.takeoverId);
                scope.textColor = scope.textColor || ColorsEnumeration.light;
                scope.backgroundColor = scope.backgroundColor || BgColorMapping.light;

                scope.strings = {
                    primaryText: UtilFactory.getLocalizedStr(scope.primaryText, CONTEXT_NAMESPACE, 'primary-text'),
                    secondaryText: UtilFactory.getLocalizedStr(scope.secondaryText, CONTEXT_NAMESPACE, 'secondary-text')
                };

                PriceInsertionFactory
                    .insertPriceIntoLocalizedStr(scope, scope.strings, scope.keyMessage, CONTEXT_NAMESPACE, 'keyMessage');

                scope.hasLogo = (typeof scope.logo === 'string' && scope.logo.length > 0);
                scope.isDarkTextColor = (scope.textColor === ColorsEnumeration.dark);

                if (!scope.backgroundColor) {
                    scope.backgroundColor = BgColorMapping[scope.textColor];
                }

                scope.backgroundColor = CSSUtilFactory.setBackgroundColorOfElement(scope.backgroundColor, element.find(TAKEOVER_WRAPPER_SELECTOR), CONTEXT_NAMESPACE);

                scope.dismissTakeoverAndOpenLink = function (e, href) {
                    e.preventDefault();
                    e.stopPropagation();
                    OriginTakeoverHelper.dismissTakeover(scope.takeoverId);
                    setPageContentVisibility(true);
                    NavigationFactory.openLink(href);
                };

                scope.gotoPrimaryLinkAndDismissTakeover = function (e, href) {
                    e.preventDefault();
                    e.stopPropagation();
                    OriginTakeoverHelper.dismissTakeover(scope.takeoverId);
                    if (href) {
                        NavigationFactory.openLink(href);
                    } else {
                        PdpUrlFactory.goToPdp(scope.model);
                    }
                };

                if (!scope.isDismissedOrHidden && isOnline) {
                    setPageContentVisibility(false);
                    Origin.telemetry.events.fire('originTakeoverVisible');
                } else {
                    setPageContentVisibility(true);
                }

                AuthFactory.events.on('myauth:clientonlinestatechanged', onlineStatusHandler);

                if (scope.primaryUrl) {
                    scope.primaryAbsoluteUrl = NavigationFactory.getAbsoluteUrl(scope.primaryUrl);
                    scope.isPrimaryLinkExternal = UtilFactory.isAbsoluteUrl(scope.primaryUrl);
                } else {
                    scope.$watchOnce('model', function () {
                        scope.primaryAbsoluteUrl = PdpUrlFactory.getPdpUrl(scope.model, true);
                    });
                }
            }

            scope.$on('$destroy', function () {
                $sitestripes.show();
                AuthFactory.events.off('myauth:clientonlinestatechanged', onlineStatusHandler);
            });

            DirectiveScope.populateScope(scope, CONTEXT_NAMESPACE, scope.ocdPath).then(init);

            scope.secondaryAbsoluteUrl = NavigationFactory.getAbsoluteUrl(scope.secondaryUrl);
        }

        return {
            restrict: 'E',
            scope: {
                ocdPath: '@',
                primaryUrl: '@',
                secondaryUrl: '@',
                primaryText: '@',
                secondaryText: '@',
                image: '@',
                logo: '@',
                titleStr: '@',
                keyMessage: '@',
                videoId: '@',
                matureContent: '@',
                videoDescription: '@',
                textColor: '@',
                backgroundColor: '@',
                takeoverId: '@',
                ctid: '@'
            },
            require: '^?originTakeoverPage',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/home/views/takeover.html'),
            controller: originStoreHomeTakeoverCtrl,
            link: originStoreHomeTakeoverLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreHomeTakeover
     * @restrict E
     * @element ANY
     * @scope
     * @description Static promo module
     * @param {OcdPath} ocd-path OCD Path
     * @param {LocalizedString} primary-text Text for the first button. E.G. Learn more
     * @param {LocalizedString} secondary-text Text for the second button. E.G., Continue to store
     * @param {Url} primary-url Url for the primary button, if not provided, will use OCD path to goto PDP
     * @param {Url} secondary-url Url for the second button
     * @param {ImageUrl} image The background image for this module.
     * @param {ImageUrl} logo The game logo
     * @param {ColorsEnumeration} text-color The text color scheme: dark or light
     * @param {LocalizedString} title-str The game title override
     * @param {Video} video-id The youtube video ID of the video for this promo (Optional).
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {string} takeover-id takeover id. Must be unique for each unique takeover. We use this to track dismissal
     * @param {LocalizedString} video-description The text for the video cta
     * @param {LocalizedString} key-message The key message override
     * @param {string} background-color Hex value of the background color
     * @param {string} ctid Campaign Targeting ID
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-home-takeover
     *             image="https://docs.x.origin.com/proto/images/store/template_heroic_.png"
     *             logo="https://docs.x.origin.com/proto/images/store/template_heroic_logo.png"
     *             title-str="Dragon Age: Inquisition"
     *             key-message="Immerse Yourself in the Ultimate Star Wars™ Experience"
     *             ocd-path="/star-wars/star-wars-battlefront/standard-edition"
     *             text-color="light"
     *             background-color="rgba(30, 38, 44, 1)">
     *         </origin-store-home-takeover>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('originStoreHomeTakeoverCtrl', originStoreHomeTakeoverCtrl)
        .directive('originStoreHomeTakeover', originStoreHomeTakeover);
}());
