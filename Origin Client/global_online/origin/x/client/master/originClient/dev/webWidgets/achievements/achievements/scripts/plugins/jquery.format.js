/**
* @jquery.format.js
* A consolidation of all the different string formating
* functions that account for word replacement that are used in HAL.
*
* This is used throughout all the pages in the widget
*
*
*/
;(function ($, undefined) {

    "use strict";

    $.extend({

        // formatVB( "%1 %2", "Hello", "World");
        formatVB : function (string) {
            var args = arguments,
                arglen = arguments.length - 1,
                pattern = new RegExp("%([1-" + arglen + "])", "g");
            return string.replace(pattern, function (match, index) {
                return args[index];
            });
        },

        // formatC( "%s %s", "Hello", "World");
        // equivalent to sprintf
        formatC: function (string) {
            var args = arguments,
                pattern = new RegExp("%s", "g"),
                counter = 1;
            return string.replace(pattern, function (match, index) {
                return args[counter++];
            });
        },

        // formatPython( "{0} {1}", "Hello", "World");
        formatPython : function (string) {
            var args = arguments,
                arglen = arguments.length - 2,
                pattern = new RegExp("{([0-" + arglen + "])}", "g");
            return string.replace(pattern, function (match, index) {
                return args[parseInt(index, 10) + 1];
            });
        }

    });
})(jQuery);