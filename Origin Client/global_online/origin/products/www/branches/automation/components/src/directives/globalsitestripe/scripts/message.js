
/**
 * @file globalsitestripe/scripts/message.js
 */
(function(){
    'use strict';

    /* jshint ignore:start */
    /**
     * MessageSitestripeTypeEnumeration - enumeration of icon types
     * @enum {string}
     */
    var MessageSitestripeTypeEnumeration = {
        "passive": "passive",
        "warning": "warning",
        "error": "error"
    };

    /* jshint ignore:end */

    /**
     * BooleanEnumeration - a specialized type of enumeration that returns "true" if affirmative/checked-off
     * @enum {string}
     */
    var BooleanEnumeration = {
        "true": "true"
    };

    var LOCALSTORAGE_KEY = 'gss-msg:';

    var CONTEXT_NAMESPACE = 'origin-global-sitestripe-message';

    function OriginGlobalSitestripeMessageCtrl($scope, GlobalSitestripeFactory, NavigationFactory, LocalStorageFactory, UtilFactory) {
        function resolveBoolean(boolean) {
            return boolean === true || (_.isString(boolean) && boolean.toLowerCase() === BooleanEnumeration.true);
        }

        function resolveNumber(number, alt) {
            if (_.isNumber(number)) {
                return number;
            } else if (_.isString(number)) {
                return Number(number);
            }

            return alt;
        }

        function resolveCommaDeliminatedString(string) {
            return  !_.isUndefined(string) && _.isString(string) ? string.split(',') : [];
        }

        $scope.ctaIsInternal = resolveBoolean($scope.ctaIsInternal);
        $scope.cta2IsInternal = resolveBoolean($scope.cta2IsInternal);
        $scope.permanent = resolveBoolean($scope.permanent);
        $scope.priority = resolveNumber($scope.priority, 0);
        $scope.winFilter = resolveBoolean($scope.winFilter);
        $scope.macFilter = resolveBoolean($scope.macFilter);
        $scope.webFilter = resolveBoolean($scope.webFilter);
        $scope.greaterVersionFilter = resolveNumber($scope.greaterVersionFilter, Number.MAX_VALUE);
        $scope.lesserVersionFilter = resolveNumber($scope.lesserVersionFilter, Number.MIN_VALUE);
        $scope.betaFilter = resolveBoolean($scope.betaFilter);
        $scope.showOnStates = resolveCommaDeliminatedString($scope.showOnStates);
        $scope.hideOnStates = resolveCommaDeliminatedString($scope.hideOnStates);

        function getLocalStorageKey() {
            function getIfDefined (string) {
                return !_.isUndefined(string) ? string : "";
            }

            return LOCALSTORAGE_KEY.concat(getIfDefined($scope.message))
                .concat(getIfDefined($scope.reason))
                .concat(getIfDefined($scope.cta))
                .concat(getIfDefined($scope.cta2))
                .concat(getIfDefined($scope.lastModified));
        }

        this.closeCallback = function() {
            LocalStorageFactory.set(getLocalStorageKey(), false);
            GlobalSitestripeFactory.hideStripe($scope.$id);
        };

        function openHref(href, internal) {
            if (internal) {
                NavigationFactory.internalUrl(href);
            } else {
                NavigationFactory.externalUrl(href, true);
            }
        }

        this.ctaCallback = function() {
            openHref($scope.strings.ctaHref, $scope.ctaIsInternal);
        };

        this.getCtaUrl = function(){
            return NavigationFactory.getAbsoluteUrl($scope.strings.ctaHref);
        };

        this.cta2Callback = function() {
            openHref($scope.strings.cta2Href, $scope.cta2IsInternal);
        };

        this.getCta2Url = function(){
            return NavigationFactory.getAbsoluteUrl($scope.strings.cta2Href);
        };

        $scope.strings = {
            ctaHref: UtilFactory.getLocalizedStr($scope.ctaHref, CONTEXT_NAMESPACE, 'cta-href'),
            cta2Href: UtilFactory.getLocalizedStr($scope.cta2Href, CONTEXT_NAMESPACE, 'cta2-href')
        };


        if (LocalStorageFactory.get(getLocalStorageKey()) !== false) {
            GlobalSitestripeFactory.showStripe(
                $scope.$id,
                $scope.priority,
                this.closeCallback,
                $scope.message,
                $scope.reason,
                $scope.cta,
                this.ctaCallback,
                $scope.icon,
                $scope.cta2,
                this.cta2Callback,
                $scope.permanent,
                $scope.winFilter,
                $scope.macFilter,
                $scope.webFilter,
                $scope.greaterVersionFilter,
                $scope.lesserVersionFilter,
                $scope.betaFilter,
                $scope.showOnStates,
                $scope.hideOnStates,
                this.getCtaUrl,
                this.getCta2Url
            );
        }
    }

    function originGlobalSitestripeMessage() {
        return {
            restrict: 'E',
            scope: {
                priority: '@',
                message: '@',
                reason: '@',
                cta: '@',
                ctaHref: '@',
                ctaIsInternal: '@',
                cta2: '@',
                cta2Href: '@',
                cta2IsInternal: '@',
                icon: '@',
                permanent: '@',
                winFilter: '@',
                macFilter: '@',
                webFilter: '@',
                greaterVersionFilter: '@',
                lesserVersionFilter: '@',
                betaFilter: '@',
                showOnStates: '@',
                hideOnStates: '@',
                ctid: '@'
            },
            controller: 'OriginGlobalSitestripeMessageCtrl'
        };
    }
     /**
     * @ngdoc directive
     * @name origin-components.directives:originGlobalSitestripeMessage
     * @restrict E
     * @element ANY
     * @scope
     * @param {Number} priority Priority of the stripe (displayed high to low). Valid priority are 900 and lower.
     * @param {LocalizedString} message the bold main title of the message
     * @param {LocalizedString} reason the subtext with details
     * @param {LocalizedString} cta text
     * @param {string} cta-href cta link url
     * @param {BooleanEnumeration} cta-is-internal if true the cta link will be opened in the SPA
     * @param {LocalizedString} cta2 text
     * @param {string} cta2-href cta2 link url
     * @param {BooleanEnumeration} cta2-is-internal if true the cta link will be opened in the SPA
     * @param {MessageSitestripeTypeEnumeration} icon icon to use (passive, warning, error)
     * @param {BooleanEnumeration} permanent if true the stripe cannot be closed
     * @param {BooleanEnumeration} win-filter if true do not show on windows
     * @param {BooleanEnumeration} mac-filter if true do not show on mac
     * @param {BooleanEnumeration} web-filter if true do not show on web
     * @param {Number} greater-version-filter if version is greater or equal do not show
     * @param {Number} lesser-version-filter  if version is lesser or equal do not show
     * @param {BooleanEnumeration} beta-filter if version is beta do not show
     * @param {string} show-on-states - show if current url is in list (use comma deliminated string like: "/discover,/store,/game-library")
     * @param {string} hide-on-states - hide if current url is in list (use comma deliminated string like: "/discover,/store,/game-library")
     * @param {string} last-modified - The date and time the mod page was last modified. Manual entry in this field will override the last-modified date returned by the renderer, leave blank to programmatically use the internal CQ last-modified date.
     * @param {String} ctid Campaign Targeting ID
     * @description
     * Global site stripe message with filtering
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-global-sitestripe-message
     *             message='dev message'
     *             reason='dev reason'
     *             cta='dev cta'
     *             cta-href='http://ctaLink'
     *             icon='warning'
     *             pcWinFilter='false'
     *             macFilter='false'
     *             webFilter='false'
     *             greaterVersionFilter='100016891'
     *             lesserVersionFilter='100016893'
     *             betaFilter='false'>
     *         </origin-global-sitestripe-message>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginGlobalSitestripeMessageCtrl', OriginGlobalSitestripeMessageCtrl)
        .directive('originGlobalSitestripeMessage', originGlobalSitestripeMessage);
}());
