/**
 * @file store/sitestripe/scripts/wrapper.js
 */
(function () {
    'use strict';

    /**
     * The directive
     */
    function originStoreSitestripeWrapper($compile, SiteStripeFactory, ComponentsConfigFactory, AuthFactory) {

        var DELAY_MS = 600000; //10 minutes
        var LAST_MODIFIED_TOKEN = 'last-modified="';
        var LAST_MODIFIED_LENGTH = LAST_MODIFIED_TOKEN.length;
        var isTemplateDirty = false;
        var timerHandle;

        function originStoreSitestripeWrapperLink(scope, element) {
            /**
             * Sets a up a timer to periodically poll for template changes from CQ
             */
            function startTimerForTemplateChanges() {
                isTemplateDirty = false;
                clearTimeout(timerHandle);
                timerHandle = setTimeout(updateView, DELAY_MS);
            }

            /**
             * Cleans up events listeners and timeout handle.
             */
            function cleanUp() {
                // tells site stripes to unbind to prevent memory leak
                scope.$broadcast('siteStripeWrapper:cleanUp');
                AuthFactory.events.off('myauth:change', onAuthChanged);
                clearTimeout(timerHandle);
            }

            function listenForAuthChanges() {
                AuthFactory.events.on('myauth:change', onAuthChanged);
            }

            function onAuthChanged() {
                isTemplateDirty = true;
                updateView();
            }

            function filterOnlyDirty(template) {
                return isTemplateDirty || isLastModifiedDateChanged(template) ? template : null;
            }

            /**
             * Checks the template string to see if the lastmodified bit has changed.
             */
            function isLastModifiedDateChanged(template) {
                if (template) {
                    var indexOfLastModified = template.indexOf(LAST_MODIFIED_TOKEN);
                    if (indexOfLastModified >= 0) {
                        var lastModifiedDate = template.substring(indexOfLastModified + LAST_MODIFIED_LENGTH, template.indexOf('"', indexOfLastModified + LAST_MODIFIED_LENGTH));
                        if (lastModifiedDate && scope.lastModified !== lastModifiedDate) {
                            return true;
                        }
                    }
                }
                return false;
            }

            /**
             * Compiles and renders a new sitestripe template if template has changed or authentication has changed.
             */
            function updateView() {
                SiteStripeFactory.retrieveTemplate()
                    .then(filterOnlyDirty)
                    .then(render)
                    .then(startTimerForTemplateChanges);
            }

            function render(template) {
                if(template) {
                    cleanUp(); // running $compile will cause new sitestripe scopes to be created and link functions to be fired so it is important to clean up first to prevent memory leaks.
                    element.html($compile(template)(scope));
                    SiteStripeFactory.showAllSiteStripes();
                }
            }

            startTimerForTemplateChanges();
            listenForAuthChanges();
            scope.$on('$destroy', cleanUp);
        }

        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                lastModified: '@'
            },
            link: originStoreSitestripeWrapperLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/sitestripe/views/wrapper.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSitestripeWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {string} last-modified The date and time the mod page was last modified. Manual entry in this field will override the last-modified date returned by the renderer, leave blank to programmatically use the internal CQ last-modified date.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-sitestripe-wrapper
     *              last-modified="June 5, 2015 06:00:00 AM PDT">
     *          </origin-store-sitestripe-wrapper>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreSitestripeWrapper', originStoreSitestripeWrapper);
}());
