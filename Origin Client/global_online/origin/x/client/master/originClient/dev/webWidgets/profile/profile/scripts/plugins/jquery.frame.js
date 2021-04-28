/**
* @jquery.frame.js
* Script that is used to control the height of the containing frame
* so that a scrollbar is presented properly. This is used to acccount
* for the upper sub-navigation that required for many pages.
*
* This is used throughout all the pages in the widget
*
*
*/
;(function ($, undefined) {

    "use strict";

    $.fn.frame = function (options, parameters) {

        var settings = {
            navHeight : 0,
            wrapper : null
        };

        var methods = {

            init : function () {

                $.extend(settings, options);

                settings.navHeight = $(".nav-wrapper").height() +
                    parseInt($(".nav-wrapper").css("padding-top"), 10) +
                    parseInt($(".nav-wrapper").css("padding-bottom"), 10);

                var frameHeight = $(window).height() - settings.navHeight;
                settings.wrapper = $(".outer-wrapper > div.wrapper")
                                       .height(frameHeight);

                $(window).on("resize", function () {
                    var currFrameHeight = $(window).height() - settings.navHeight;
                    settings.wrapper.height(currFrameHeight);
                });

            } //END init
        };// END: methods

        // initialization

        var publicMethods = [ "init" ];

        if (typeof options === 'object' || !options) {

            methods.init(); // initializes plugin

        } else if ($.inArray(options, publicMethods)) {
            // call specific methods with arguments

        }

    };  // END: $.frame = function () {


})(jQuery);
 // END: (function ($ ){


$(document).ready(function () {
    "use strict";
    $.fn.frame({});
});