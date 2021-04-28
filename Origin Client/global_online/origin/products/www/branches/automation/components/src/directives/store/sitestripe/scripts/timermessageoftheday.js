/**
 * @file /store/sitestripe/scripts/Timermessageoftheday.js
 */
(function () {
    'use strict';

    /* jshint ignore:start */

    /**
     * Sets the layout of timer in this site stripe.
     * @readonly
     * @enum {string}
     */
    var TimerPositionEnumeration = {
        "Left": "left",
        "Right": "right"
    };

    /**
     * Hide the timer when it expires.
     * @readonly
     * @enum {string}
     */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    var ImagePositionEnumeration = {
        "left": "left",
        "center": "center"
    };

    var FontColorEnumeration = {
        "light" : "#FFFFFF",
        "dark" : "#1E262C"
    };

    var CONTEXT_NAMESPACE = 'origin-store-sitestripe-timermessageoftheday',
        IMAGE_CLASS_CENTER = 'origin-store-sitestripe-messageoftheday-image-center',
        IMAGE_CLASS_LEFT = 'origin-store-sitestripe-messageoftheday-image-left';

    /**
     * The controller
     */
    function OriginStoreSitestripeTimermessageofthedayCtrl($scope, DirectiveScope, NavigationFactory) {
        // enable timer-specific template components
        $scope.timer = true;
        $scope.timerVisible = true;
        $scope.scopeResolved = false;

        function update() {
            if ($scope.newtitle) {
                $scope.titleStr = $scope.newtitle;
            }

            if ($scope.newsubtitle) {
                $scope.subtitle = $scope.newsubtitle;
            }

            if ($scope.hidetimer === 'true') {
                $scope.timerVisible = false;
            }
        }

        // update sitestripe messages if scope has resolved before event, 
        // if scope has not resolved, wait for it, then update messages
        $scope.$on('timerReached', function () {
            if($scope.scopeResolved) {
                update();
            } else {
                var endTimer = $scope.$watch('scopeResolved', function(newValue, oldValue) {
                    if(newValue !== oldValue) {
                        update();
                        endTimer();
                    }
                });
            }
        });

        function onScopeResolved() {
            $scope.scopeResolved = true;

            if(!$scope.goaldate && $scope.ocdPath) {
                var stopWatching = $scope.$watch('model.releaseDate', function(newValue) {
                    if(_.isDate(newValue)) {
                        $scope.releaseDate = $scope.model.releaseDate.toString();
                        stopWatching();
                    }
                });
            }

            function getHref() {
                if (!$scope.href) {
                    return  $scope.ocdPath ? 'store' + $scope.ocdPath : null;
                }
                return $scope.href;
            }

            $scope.href = getHref();

            $scope.goTo = function(event, href){
                event.preventDefault();
                NavigationFactory.openLink(href);
            };

            $scope.imagePositionClass = $scope.imagePosition === ImagePositionEnumeration.center ? IMAGE_CLASS_CENTER : IMAGE_CLASS_LEFT;
            $scope.hoverClass = $scope.fontcolor === FontColorEnumeration.dark ? 'close-transition-dark' : 'close-transition-light';
        }

        DirectiveScope.populateScopeWithOcdAndCatalog($scope, CONTEXT_NAMESPACE, $scope.ocdPath)
            .then(onScopeResolved);
    }

    /**
     * The directive
     */
    function originStoreSitestripeTimermessageoftheday(ComponentsConfigFactory, SiteStripeFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                uniqueSitestripeId: '@',
                titleStr: '@',
                subtitle: '@',
                image: '@',
                logo: '@',
                href: '@',
                edgecolor: '@',
                hidetimer: '@',
                goaldate: '@',
                newtitle: '@',
                newsubtitle: '@',
                timerposition: '@',
                fontcolor: '@',
                ocdPath: '@',
                ctid: '@',
                imagePosition: '@',
                showOnPage: '@',
                hideOnPage: '@'
            },
            controller: OriginStoreSitestripeTimermessageofthedayCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/sitestripe/views/messageoftheday.html'),
            link: SiteStripeFactory.siteStripeLinkFn(CONTEXT_NAMESPACE)
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSitestripeTimermessageoftheday
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {string} unique-sitestripe-id a unique id to identify this site stripe from others. Must be specified to enable site stripe dismissal.
     * @param {LocalizedString} title-str The title for this site stripe.
     * @param {LocalizedString} subtitle The title for this site stripe.
     * @param {ImageUrl|OCD} image The background image for this site stripe.
     * @param {ImageUrl|OCD} logo The logo image for this site stripe.
     * @param {string} href The URL for this site stripe to link to.
     * @param {string} edgecolor The background color for this site stripe (Hex: #000000).
     * @param {FontColorEnumeration} fontcolor The font color of the site stripe, light or dark.
     * @param {BooleanEnumeration} hidetimer Hide the timer when it expires?
     * @param {DateTime} goaldate The timer end date and time. (UTC time)
     * @param {LocalizedString} newtitle The new title to switch to after the timer expires (Optional).
     * @param {LocalizedString} newsubtitle The new subtitle to switch to after the timer expires (Optional).
     * @param {TimerPositionEnumeration} timerposition Sets the layout of the timer.
     * @param {OcdPath} ocd-path OCD Path
     * @param {string} ctid Campaign Targeting ID
     * @param {ImagePositionEnumeration} image-position left or center. Default to left
     * @param {string} show-on-page Urls to show this sitestripe on (comma separated)
     * @param {string} hide-on-page Urls to hide this sitestripe on (comma separated)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-sitestripe-wrapper
     *         last-modified="June 5, 2015 06:00:00 AM PDT">
     *         <origin-store-sitestripe-timermessageoftheday
     *             title-str="some title"
     *             subtitle="some subtitle"
     *             image="//somedomain/someimage.jpg"
     *             logo="//somedomain.com/someimage.jpg"
     *             href="//somedomain.com"
     *             edgecolor="#000000"
     *             hidetimer="true"
     *             goaldate="2015-06-02 09:10:30 UTC"
     *             newtitle="some new title"
     *             newsubtitle="some new subtitle"
     *             timerposition="left"
     *             ocd-path="/battlefield/battlefield-4/standard-edition">
     *         </origin-store-sitestripe-timermessageoftheday>
     *     </origin-store-sitestripe-wrapper>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreSitestripeTimermessageoftheday', originStoreSitestripeTimermessageoftheday);
}());
