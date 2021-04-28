
/**
 * @file globalsitestripe/scripts/sitestripe.js
 */
(function(){
    'use strict';

    function OriginGlobalSitestripeCtrl($scope, GlobalSitestripeFactory, ComponentsConfigHelper, ScopeHelper) {
        $scope.stripe = undefined;
        $scope.visible = false;
        $scope.reasonHtml = "";
        $scope.isOigContext = angular.isDefined(ComponentsConfigHelper.getOIGContextConfig());

        $scope.isPassive = function() {
            return !$scope.isWarning() && !$scope.isError();
        };

        $scope.isWarning = function() {
            return $scope.stripe && $scope.stripe.icon === "warning";
        };

        $scope.isError = function() {
            return $scope.stripe && $scope.stripe.icon === "error";
        };

        $scope.isDoubleCta = function() {
            return $scope.stripe && $scope.stripe.cta2 && $scope.stripe.cta2.length ? true : false;
        };

        $scope.hideMessage = function() {
            if ($scope.stripe.closeCallback) {
                $scope.stripe.closeCallback();
            }
        };

        $scope.btnAction = function($event) {
            $event.preventDefault();
            if ($scope.stripe.ctaCallback) {
                $scope.stripe.ctaCallback();
            }
        };
        
        $scope.getBtnUrl = function(){
            if ($scope.stripe && $scope.stripe.getCtaUrl){
                return $scope.stripe.getCtaUrl();
            }
        };

        $scope.btnAction2 = function($event) {
            $event.preventDefault();
            if ($scope.stripe.cta2Callback) {
                $scope.stripe.cta2Callback();
            }
        };

        $scope.getBtn2Url = function(){
            if ($scope.stripe && $scope.stripe.getCta2Url){
              return $scope.stripe.getCta2Url();
            }
        };

        $scope.generateReasonHtml = function() {
            return "";
        };

        this.setData = function(stripe) {
            if (!_.isUndefined(stripe)) {
                $scope.stripe = stripe;
                $scope.visible = true;
                $scope.generateReasonHtml();
            } else {
                $scope.stripe = null;
                $scope.visible = false;
            }

            ScopeHelper.digestIfDigestNotInProgress($scope);
        };

        this.getHeight = function(callback) {
            if ($scope.getHeight) {
                $scope.getHeight(callback);
            } else {
                callback(0);
            }
        };

        GlobalSitestripeFactory.addStripeArea(this.setData, this.getHeight);
    }

    function originGlobalSitestripe($compile, $timeout, ComponentsConfigFactory, LocFactory, AuthFactory, GlobalSitestripeFactory) {
        function originGlobalSitestripeLink(scope, elem) {
            var reasonElement = elem.find('.l-origin-sitestripe-reason'),
                timeoutHandle;

            function getHeight() {
                if (scope.visible) {
                    var element = elem.find(".origin-sitestripe");
                    if (element.length !== 0) {
                        return element[0].getBoundingClientRect().height;
                    }
                }

                return 0;
            }

            scope.getHeight = function(callback) {
                if(!_.isFunction(callback)) {
                    return 0;
                }

                timeoutHandle=$timeout(function() { callback(getHeight()); });
            };

            scope.generateReasonHtml = function() {
                var ctaHtml = '<a style="cursor: pointer;" ng-click="btnAction($event)" ng-href="{{::getBtnUrl()}}" class="otka origin-tealium-global-sitestripe-link" ng-bind="stripe.cta"></a>',
                    cta2Html = '<a style="cursor: pointer;" ng-click="btnAction2($event)" ng-href="{{::getBtn2Url()}}" class="otka origin-tealium-global-sitestripe-link" ng-bind="stripe.cta2"></a>';

                var reason = scope.stripe.reason ? LocFactory.substitute(scope.stripe.reason, { '%cta%': ctaHtml, '%cta2%': cta2Html }) : "";
                var reasonHtml = LocFactory.substitute('<span>%reasonHtml%</span>', {'%reasonHtml%': reason });
                reasonElement.empty().append($compile(reasonHtml)(scope));
            };

            function onDestroy() {
                $timeout.cancel(timeoutHandle);
            }

            function updateTemplate(template) {
                reasonElement.html($compile(template)(scope));
            }

            function onAuthChange() {
                GlobalSitestripeFactory.retrieveTemplate()
                .then(updateTemplate)
                .then(GlobalSitestripeFactory.updateFilters);
            }

            AuthFactory.events.on('myauth:change', onAuthChange);
            scope.$on('$destroy', onDestroy);
        }
        return {
            restrict: 'E',
            scope: true,
            controller: 'OriginGlobalSitestripeCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('globalsitestripe/views/sitestripe.html'),
            link: originGlobalSitestripeLink
        };
    }
     /**
     * @ngdoc directive
     * @name origin-components.directives:originGlobalSitestripe
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * Rendering area for a global site stripe
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-global-sitestripe>
     *         </origin-global-sitestripe>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginGlobalSitestripeCtrl', OriginGlobalSitestripeCtrl)
        .directive('originGlobalSitestripe', originGlobalSitestripe);
}());
