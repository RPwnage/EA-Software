
/**
 * @file /scripts/style.js
 */
(function(){
    'use strict';
    /**
    * This directive is used to cover a use case that the ngStyle directive does not cover, if the
    * CSS you need to apply has the same key but multiple values. For example you may want to
    * apply CSS for background image gradient this currently requires vendor prefixes on the CSS
    * keys but not the values. This new directive can take objects of the form
    * var CSS = {key1: [value1, value2, value3], key2: value4, key3: [value5, value6]}
    */
    function originStyle(ObjectHelperFactory) {
        var forEach = ObjectHelperFactory.forEach;

        /**
        * The directive link
        */
        function originStyleLink(scope, elem, attrs) {

            /**
             * Loop over the set of CSS and apply them
             * @param  object newStyles a collection of css styles and values
             */
            function applyStyleSet(newStyles) {
                forEach(applyStyle, newStyles);
            }

            /**
             * For the provide property apply all the style
             * @param  array|string style    the value for the css property
             * @param  string property a CSS property like display
             */
            function applyStyle(style, property){
                if(style instanceof Array) {
                    for(var val in style) {
                        if (property.hasOwnProperty(val)) {
                            elem.css(property, style[val]);
                        }
                    }
                } else {
                    elem.css(property, style);
                }
            }

            // Watch the value origin-style for css values
            scope.$watch(attrs.originStyle, function(newStyleSet) {
                applyStyleSet(newStyleSet);
            });
        }
        return {
            restrict: 'A',
            link: originStyleLink
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originStyle
     * @restrict A
     * @element ANY
     * @scope
     * @description
     * @param {string=} name desc
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *
     *     <div origin-style="{background:[-webkit-color(blue),'-moz-color(blue)'], color:red}"> <?div>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStyle', originStyle);
}());
