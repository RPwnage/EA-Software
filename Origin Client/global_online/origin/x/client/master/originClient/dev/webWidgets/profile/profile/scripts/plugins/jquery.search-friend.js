/**
* @jquery.search-friend.js
* Script that reads the query parameters and passes the search
* keywords to Origin Social to search for friends. Include pagination
* parameters
*
* This is used only in the search-friend.html
*
*
*/

;(function ($, undefined) {

    "use strict";

    $.fn.searchFriend = function (options, parameters) {

        var settings = {
            resultTemplate : null,
            currentPage : 1,
            resultsPerPage : 20,
            keywords : null,
            textPrivate : ''
        };

        // bindings
        var methods = {

            init : function () {

                $.extend(settings, options);
                settings.currentPage = parseInt(($.getQueryString('page') || 1), 10);
                settings.keywords = decodeURIComponent($.getQueryString('keyword'));
                $('#friend-list .keywords, #no-friend-result .keywords').text(settings.keywords);
                $('.text-only-pc-users-searchable').text($('.text-only-pc-users-searchable').text().replace('${CLIENT_NAME}', 'Origin'));
                settings.textPrivate = $('#text-string-private').text();

                $('body').on('click', '#friend-list tfoot a', methods.events.pageNavigationClick);
                $('body').on('click', '#friend-list tbody a', methods.events.pageProfileClick);
                $('body').on('click', '.link-profile',  methods.events.profileLinkClick);
                $('body').on('click', '.link-achievements', methods.events.achievementsLinkClick);

                methods.helpers.stringSubstitution();

                var mySearchFriendsInstance = originSocial.searchFriends(settings.keywords, settings.currentPage);
                mySearchFriendsInstance.succeeded.connect(function(results) {
                    methods.events.drawResults(results);
                });
                mySearchFriendsInstance.failed.connect(function() {
                    methods.events.drawNoResults();
                });

                mySearchFriendsInstance.execute();

            }, //END init

            events : {
                drawResults : function (results) {
                    var maxPage = 0;
                    var startResult = (settings.currentPage - 1) * settings.resultsPerPage + 1;
                    var endResult =  settings.currentPage * settings.resultsPerPage;
                    var totalResult = 0;

                    var $resultTemplate = $('.friend-list-result-template').removeClass('friend-list-result-template').detach();

                    // Hide the Loader
                    $('#friend-list.search-loading').removeClass('search-loading');

                    // The number of results that is actually returned is N+1.
                    // This is because the total number of search results is returned as the last item in the array.
                    if ( results && results.length == 2 ){
                        window.location = '/index.html?id=' + results[0].userId;
                        return;
                    }

                    $(results).each(function (index, result) {
                        if (result.totalCount) {

                            $('.friend-list-count span').text($.formatC($('.friend-list-count').text(), startResult, Math.min(endResult, result.totalCount), result.totalCount)).show();
                            totalResult = result.totalCount;
                            maxPage = Math.ceil(totalResult / settings.resultsPerPage);

                        } else {

                            var fullName = '';
                            if (result.fullName === 'ebisu_client_private') {
                                fullName = settings.textPrivate;
                            } else {
                                fullName = result.fullName;
                            }

                            var $resultRow = $resultTemplate.clone();
                            $resultRow
                                .find('.friend-list-result-avatar img').attr('src', result.avatarUrl).end()
                                .find('.friend-list-result-origin-id a').text(result.nickname).data('id', result.userId).end()
                                .find('.friend-list-result-name').text(fullName).end();
                            $('#friend-list tbody').append($resultRow);
                        }
                    });

                    $('#friend-list tfoot').toggle(totalResult > 20);


                    // draw navigation
                    var counter = -5;
                    var selectedPageClass = '';
                    while (counter < 6) {
                        if (settings.currentPage + counter > 0  && settings.currentPage + counter <= Math.ceil(totalResult / settings.resultsPerPage)) {
                            selectedPageClass = '';
                            if (counter === 0) { selectedPageClass = 'selected'; }
                            $('tfoot .navigation-pages').append('<a href="#" class="' + selectedPageClass + '"  data-page="' + (settings.currentPage + counter) + '" >' + (settings.currentPage + counter) + '</a> ');
                        }
                        counter++;
                    }

                    $('.navigation-first').data('page', 1);
                    $('.navigation-prev').data('page', Math.max(1, settings.currentPage - 1));
                    $('.navigation-next').data('page', Math.min(maxPage, settings.currentPage + 1));
                    $('.navigation-last').data('page', maxPage);

                    if (settings.currentPage === 1) { $(".navigation-first, .navigation-prev").addClass("selected"); }
                    if (settings.currentPage === maxPage) { $(".navigation-last, .navigation-next").addClass("selected"); }

                },

                drawNoResults : function () {
                    $('#friend-list').hide();
                    $('#no-friend-result').show();
                },

                pageNavigationClick : function (event) {
                    event.preventDefault();

                    var keywords = $("#search-friend-control input").val();
                    keywords = encodeURIComponent(settings.keywords.replace(/^\s\s*/, '').replace(/\s\s*$/, ''));
                    if (keywords !== "") {
                        window.location = '/search-friend.html?page=' + $(this).data('page') + '&keyword=' + keywords;
                    }

                },

                pageProfileClick : function (event) {
                    event.preventDefault();
                    window.location = '/index.html?id=' + $(this).data('id');
                },

                profileLinkClick : function (event) {
                    event.preventDefault();
                    clientNavigation.showMyProfile();
                },

                achievementsLinkClick : function (event) {
                    event.preventDefault();
                    clientNavigation.showAchievements();
                }



            }, // END events

            helpers : {

                stringSubstitution : function () {

                    // %s
                    $('#no-friend-result-heading-text').text($.formatC($('#no-friend-result-heading-text').text(), settings.keywords));

                }

            } // END helpers

        };// END: methods

        // initialization

        var publicMethods = [ "init" ];

        if (typeof options === 'object' || !options) {

            methods.init(); // initializes plugin

        } else if ($.inArray(options, publicMethods)) {
            // call specific methods with arguments

        }

    };  // END: $.userSearch = function() {

})(jQuery); // END: (function( $ ){


$(document).ready(function () {
    "use strict";
    $.fn.searchFriend({});
});