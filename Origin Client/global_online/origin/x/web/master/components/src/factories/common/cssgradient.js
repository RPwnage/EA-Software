/**
 * @file common/cssgradient.js
 */
(function() {
    'use strict';

    var DIRECTION_LEFT_TO_RIGHT = 'left to right',
        DIRECTION_RIGHT_TO_LEFT = 'right to left',
        DIRECTION_TOP_TO_BOTTOM = 'top to bottom',
        DIRECTION_BOTTOM_TO_TOP = 'bottom to top';

    function CSSGradientFactory() {
        /**
         * Return a linear gradient for Safari 5.1-6, Chrome 10+
         *
         * @param  {String} startColor      A string representation of an rgba color eg rgba(30, 38, 44, 0)
         * @param  {Number} startPosPercent Where to start the gradient at as a percentage
         * @param  {String} endColor        A string representation of an rgba color eg rgba(30, 38, 44, 0)
         * @param  {Number} endPosPercent   Where to end the gradient at as a percentage
         * @param  {String} direction       (optional) gradient direction: 'left to right', 'right to left', 'top to bottom', 'bottom to top'
         * @return {object}                 the css to be applied as an object possibly containing array.
         */
        function oldWebkitLinearGradient(startColor, startPosPercent, endColor, endPosPercent, direction) {
            var startingPoint;

            switch (direction) {
                case DIRECTION_TOP_TO_BOTTOM:
                    startingPoint = 'top';
                    break;
                case DIRECTION_BOTTOM_TO_TOP:
                    startingPoint = 'bottom';
                    break;
                case DIRECTION_RIGHT_TO_LEFT:
                    startingPoint = 'right';
                    break;
                case DIRECTION_LEFT_TO_RIGHT:
                    startingPoint = 'left';
                    break;
                default:
                    startingPoint = 'left';
            }

            return '-webkit-linear-gradient(' + startingPoint + ', color-stop(' + startColor +' ' + startPosPercent + '%), color-stop(' + endColor + ' ' + endPosPercent + '%))';
        }

        /**
         * Return a linear gradient for IE9 and down
         *
         * @param  {String} startColor      A string representation of an rgba color eg rgba(30, 38, 44, 0)
         * @param  {Number} startPosPercent Where to start the gradient at as a percentage
         * @param  {String} endColor        A string representation of an rgba color eg rgba(30, 38, 44, 0)
         * @param  {Number} endPosPercent   Where to end the gradient at as a percentage
         * @param  {String} direction       (optional) gradient direction: 'left to right', 'right to left', 'top to bottom', 'bottom to top'
         * @return {object}                 the css to be applied as an object possibly containing array.
         */
        function oldIeLinearGradient(startColor, startPosPercent, endColor, endPosPercent, direction) {
            var gradientType;

            switch (direction) {
                case DIRECTION_TOP_TO_BOTTOM:
                case DIRECTION_BOTTOM_TO_TOP:
                    gradientType = '2';
                    break;
                default:
                    gradientType = '1';
            }

            return "progid:DXImageTransform.Microsoft.gradient(startColorstr='" + endColor + "', endColorstr='" + startColor + "', GradientType=" + gradientType + ")";
        }

        /**
         * Return a standard linear gradient declaration: IE10, Firefox 16+, Opera 12.10+, Safari 7+, Chrome 26+
         *
         * @param  {String} startColor      A string representation of an rgba color eg rgba(30, 38, 44, 0)
         * @param  {Number} startPosPercent Where to start the gradient at as a percentage
         * @param  {String} endColor        A string representation of an rgba color eg rgba(30, 38, 44, 0)
         * @param  {Number} endPosPercent   Where to end the gradient at as a percentage
         * @param  {String} direction       (optional) gradient direction: 'left to right', 'right to left', 'top to bottom', 'bottom to top'
         * @return {object}                 the css to be applied as an object possibly containing array.
         */
        function standardLinearGradient(startColor, startPosPercent, endColor, endPosPercent, direction) {
            var startingPoint;

            switch (direction) {
                case DIRECTION_TOP_TO_BOTTOM:
                    startingPoint = 'to bottom';
                    break;
                case DIRECTION_BOTTOM_TO_TOP:
                    startingPoint = 'to top';
                    break;
                case DIRECTION_RIGHT_TO_LEFT:
                    startingPoint = 'to left';
                    break;
                default:
                    startingPoint = 'to right';
            }

            return 'linear-gradient(' + startingPoint + ', ' + startColor + ' ' + startPosPercent + '%, ' + endColor + ' ' + endPosPercent + '%)';
        }

        /**
         * Return a linear gradient
         *
         * @param  {String} startColor      A string representation of an rgba color eg rgba(30, 38, 44, 0)
         * @param  {Number} startPosPercent Where to start the gradient at as a percentage
         * @param  {String} endColor        A string representation of an rgba color eg rgba(30, 38, 44, 0)
         * @param  {Number} endPosPercent   Where to end the gradient at as a percentage
         * @param  {String} direction       (optional) gradient direction: 'left to right', 'right to left', 'top to bottom', 'bottom to top'
         * @return {object}                 the css to be applied as an object possibly containing array.
         */
        function createLinearGradient(startColor, startPosPercent, endColor, endPosPercent, direction) {
            return  {
                "background-repeat": "repeat-x",
                "background-image": [
                    oldWebkitLinearGradient(startColor, startPosPercent, endColor, endPosPercent, direction),
                    standardLinearGradient(startColor, startPosPercent, endColor, endPosPercent, direction)
                ],
                "filter": oldIeLinearGradient(startColor, startPosPercent, endColor, endPosPercent, direction)
            };
        }

        /**
         * Creates the gradient fade-out effect
         *
         * @param  {String} color A string representation of the color to fade out eg rgba(30, 38, 44, 0)
         * @param  {String} direction      effect direction: 'left to right', 'right to left', 'top to bottom', 'bottom to top'
         * @return {object}                the css to be applied as an object possibly containing array.
         */
        function createGradientFadeout(color, direction) {
            return createLinearGradient('rgba(0, 0, 0, 0)', '0', color, '70', direction);
        }

        return {
            createLinearGradient: createLinearGradient,
            createGradientFadeout: createGradientFadeout
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function CSSGradientFactorySingleton(LocFactory, ComponentsLogFactory, $http, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('CSSGradientFactory', CSSGradientFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.CSSGradientFactory

     * @description
     *
     * UtilFactory
     */
    angular.module('origin-components')
        .factory('CSSGradientFactory', CSSGradientFactorySingleton);

})();