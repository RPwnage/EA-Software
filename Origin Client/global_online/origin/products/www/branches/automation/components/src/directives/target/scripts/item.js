/**
 * @file item.js
 */

(function() {
    'use strict';

    function originCqTargetingItem(CQTargetingFactory, CQTargetingStateChangeFactory) {
        var CQ_TARGETING_PREFIX = "cqTargeting";

        /**
         * put the targeting attributes into a JSON Object
         * @param  {object} incomingAttributes attributes coming from angular link function ($attr)
         * @return {object} an object that has key value pairs for the targeting attributes, passed in the CQTargetFactory.targetingItemMatches function
         */
        function buildCQTargetingAttributes(incomingAttributes) {
            var targetingAttributes = {};

            for (var attribute in incomingAttributes) {
                if (incomingAttributes.hasOwnProperty(attribute) && (attribute.indexOf(CQ_TARGETING_PREFIX) === 0)) {
                    //here we want the attribute name with the dashes for consistency
                    targetingAttributes[incomingAttributes.$attr[attribute]] = incomingAttributes[attribute];
                }
            }

            return targetingAttributes;
        }

        function originCqTargetingItemLink(scope, element, attr, ctrl, transclude) {
            /**
             * adds the element to the dom if there's a match
             */
            function addElementToDomIfMatch() {
                CQTargetingStateChangeFactory.setStateChangeTimer(attr.cqTargetingStarttime, attr.cqTargetingEndtime);
                if (CQTargetingFactory.targetingItemMatches(buildCQTargetingAttributes(attr))) {
                    transclude(function(clone) {
                        //insert a comment at the end the cq-target element
                        clone[clone.length++] = document.createComment(' end cq-targeting: ' + attr.originCqTargetingItem + ' ');

                        //add it to the dom
                        element.after(clone);
                    });
                }

            }

            /**
             * log the error
             * @param  {Error} error object
             */


            addElementToDomIfMatch();
        }

        return {
            transclude: 'element',
            priority: 600,
            restrict: 'A',
            $$tlb: true, //this is a private angular variable that we need that allows transclusion from the attribute even if the element has transclusion
            link: originCqTargetingItemLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCqTargetingItem
     * @restrict A
     * @element ANY
     * @scope
     * @description
     *
     * removes an element based on targeting
     *
     *
     */
    angular.module('origin-components')
        .directive('originCqTargetingItem', originCqTargetingItem);
}());