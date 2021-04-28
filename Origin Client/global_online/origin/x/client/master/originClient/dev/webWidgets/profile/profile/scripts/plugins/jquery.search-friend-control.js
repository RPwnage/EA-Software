/**
* @jquery.search-friend-control.js
*
* This is used by search control that is included in the
* sub-navigation that included in the profile and achievements frame.
*
*
*/

(function ($, undefined) {

    'use strict';

    $.fn.searchFriendControl = function (options, parameters) {

        var settings = {};

        // bindings
        var methods = {

            init : function () {

                $.extend(settings, options);

                $('#search-friend-control input').attr('placeholder', $('#search-friend-control .search-friend-control-placeholder').text());
                $('#search-friend-control button').on('click', methods.events.searchButtonClick);
                $('#search-friend-control input').on('keypress', methods.events.searchTextboxKeypress);

            }, //END init

            events : {
                searchButtonClick : function (event) {
                    event.preventDefault();
                    methods.helpers.createSearchUrl();
                },

                searchTextboxKeypress : function (event) {
                    if (event.keyCode === 13) {
                        methods.helpers.createSearchUrl();
                    }
                }


            }, // END events

            helpers : {
                createSearchUrl : function () {
                    var keywords = $('#search-friend-control input').val();
                    var regex = /(<([^>]+)>)/ig;
                    keywords = keywords.replace(regex, "");
                    keywords = encodeURIComponent(keywords.replace(/^\s\s*/, '').replace(/\s\s*$/, ''));
                    if (keywords !== '') {
                        window.location = '/search-friend.html?page=1&keyword=' + keywords;
                    }
                }

            } // END helpers

        };// END: methods

        // initialization
        var publicMethods = ['init'];

        if (typeof options === 'object' || !options) {

            methods.init(); // initializes plugin

        } else if ($.inArray(options, publicMethods)) {
            // call specific methods with arguments

        }

    };  // END: $.userSearch = function() {

})(jQuery); // END: (function( $ ){


$(document).ready(function () {
    'use strict';
    $.fn.searchFriendControl({});
});