/**
* @jquery.profile.games.js
* Script used to retrieve and display information about a person's games.
*
* Loading users games, then using the games information to determine
* which games has achievements to build out the achievements widget.
* The boxart for each game is then loaded.
*
* This is called from the userProfile plugin
*
*/

;(function ($, undefined) {

    'use strict';

    $.fn.gamesList = function (options, parameters) {

        var settings = {
            profileUser : null,
            isCurrentUser : true,
            gamesList : $('#games-list')
        };

        // bindings
        var methods = {

            init : function () {

                $.extend(settings, options);

                // Create Games
                var userProductsQueryOperation = settings.profileUser.queryOwnedProducts();
                if (userProductsQueryOperation) {
                    methods.helpers.setSignals(userProductsQueryOperation);
                } else {
                    settings.gamesList.addClass("error");
                    methods.helpers.setInterval(userProductsQueryOperation);
                }

            }, //END init

            events : {
                
                gameCardClick : function (event) {
                    event.preventDefault();
                    clientNavigation.showGameDetails($(this).data('product-id'));
                },

                showStorePage : function (event) {
                    event.preventDefault();                    
                    clientNavigation.showStoreProductPage($(this).data("product-id"));
                }

            }, // END events

            helpers : {                

                getGameArt : function (index, imageUrls, productId, gamesList) {

                    if (index >= imageUrls.length) {
                        //settings.gamesBoxArt[ productId ] = '';
                        var $currentSelectedCard = $('.games-item', gamesList).filter(function () {  return $(this).data('product-id') === productId; });
                        $('.game-art', $currentSelectedCard).attr('src', '/profile/images/default-boxart.jpg');
                        return;
                    }
                    var img = new Image();
                    img.src = imageUrls[index];
                    img.onload = function () {
                        // if game found find achievement and game product to fill
                        // TODO: make cleaner
                        var $selectedCard = $('.games-item', gamesList).filter(function () {  return $(this).data('product-id') === productId; });
                        $('.game-art', $selectedCard).attr('src', imageUrls[index]);
                        return;
                    };
                    img.onerror = function () {
                        methods.helpers.getGameArt(index + 1, imageUrls, productId, gamesList);
                    };

                },

                getGames : function ( products ) {

                    var $gameItemTemplate = $('.games-item', settings.gamesList).detach(),
                        $gamesCommon = $('#games-common'),
                        $gamesOther = $('#games-other'),
                        $gameItem,
                        gamesMap = {},
                        commonGamesTotal = 0,
                        uncommonGamesTotal = 0,
                        sortedProducts;

                    // more efficient to create a key map of current entitlements
                    $.each(entitlementManager.topLevelEntitlements, function (index, entitlement) {
                        // ORSUBS-816: Don't show lesser entitlements
                        if(entitlement.isSuppressed === false) {
                            gamesMap[entitlement.id] = entitlement.id;
                        }
                    });

                    sortedProducts = methods.helpers.sortListAsc(products, 'title');

                    $.each(sortedProducts, function (index, product) {
                        
                        // Create Game Cards
                        $gameItem = $gameItemTemplate.clone();
                        $gameItem.data('product-id', product.productId);
                        $('.game-title', $gameItem).text(product.title);

                        if (gamesMap[product.productId]) {
                            $gamesCommon.append($gameItem);
                            commonGamesTotal++;
                            $gameItem.on( "click", methods.events.gameCardClick );
                        } else if (!settings.isCurrentUser) {
                            // add to strangers
                            $gamesOther.append($gameItem);
                            uncommonGamesTotal++;
                            $gameItem.on( "click", methods.events.showStorePage );
                        }

                        // Get boxart
                        methods.helpers.getGameArt(0, product.boxartUrls, product.productId);

                    });

                    settings.gamesList.removeClass("album-loading");
                    methods.helpers.sectionManager($gamesCommon, $gamesOther, commonGamesTotal, uncommonGamesTotal);
                },

                setInterval : function (userProductsQueryOperation) {
                    var intents = 0,
                        interval = setInterval(function() { 
                        userProductsQueryOperation = settings.profileUser.queryOwnedProducts();
                        if(userProductsQueryOperation) {
                            settings.gamesList.removeClass('error');
                            clearInterval(interval);
                            methods.helpers.setSignals(userProductsQueryOperation);
                        } else if(++intents === 5) {
                            clearInterval(interval);
                        }
                    }, 60000);   
                },

                sectionManager : function($gamesCommon, $gamesOther, commonGamesTotal, uncommonGamesTotal) {
                    
                    var $showAllRates = $('#games-showing .section-rates-showing-all'),
                        $showPartRates = $('#games-showing .section-rates-showing-part'),
                        totalGames = commonGamesTotal + uncommonGamesTotal;

                    if (commonGamesTotal === 0) {
                        $('.games-container').addClass('album-content-common-0');
                    } else if (commonGamesTotal >= 9) {
                        $('.games-container').addClass('album-content-common-3');
                    } else {
                        $('.games-container').addClass('album-content-common-' + Math.ceil(commonGamesTotal / 5));
                    }

                    $('.games-container .list-items').each(function (index, gameList) {
                        if ($('.album-content-item', gameList).length === 0) {
                            $(gameList).hide();
                        }
                    });

                    //TODO: needs to remove hidden class
                    if ((Math.ceil($('.album-content-item', $gamesOther).length / 5)
                        + Math.ceil($('.album-content-item', $gamesCommon).length / 5)) > 4) {
                        settings.gamesList.addClass('hide-list');
                    }

                    $('#showGamerate .rate, #games-total').text(totalGames);
                    $('#games-common .items-count-number').text(commonGamesTotal);
                    $('#games-other .items-count-number').text(uncommonGamesTotal);

                    $showAllRates.text($.formatVB($showAllRates.text(), totalGames, totalGames));
                    $showPartRates.text($.formatVB($showPartRates.text(), $.unique($('.games-container .album-content-item:visible')).length, totalGames));
                },

                setSignals : function (userProductsQueryOperation) {
                    userProductsQueryOperation.succeeded.connect(function (products) {
                        if(products.length > 0) {
                            methods.helpers.getGames(products);
                        } else {
                            settings.gamesList.hide();
                        }
                        settings.gamesList.removeClass().addClass("album full-list");
                    });

                    userProductsQueryOperation.failed.connect(function () {
                        if (httpStatusCode === 403) {
                            $('#games-list, #achievements-list').removeClass('error').addClass('private');
                        } else {
                            $('#games-list').addClass('error');
                            methods.helpers.setInterval(userProductsQueryOperation);
                        }
                    });

                    userProductsQueryOperation.queryError.connect(function (httpStatusCode) {
                        if (httpStatusCode === 403) {
                            $('#games-list, #achievements-list').removeClass('error').addClass('private');
                        }
                    });
                    userProductsQueryOperation.getUserProducts();
                },

                sortListAsc : function (list, param) {
                    return list.sort(function (a, b) {
                        if (a[param].toLowerCase() < b[param].toLowerCase()) { return -1; }
                        if (a[param].toLowerCase() > b[param].toLowerCase()) { return 1; }
                        return 0;
                    });
                }

            } // END helpers

        };// END: methods

        // initialization

        var publicMethods = ["init"];

        if (typeof options === 'object' || !options) {

            methods.init(); // initializes plugin

        } else if ($.inArray(options, publicMethods)) {
            // call specific methods with arguments

        }

    };  // END: $.gamesList = function () {

})(jQuery); // END: (function ($) {
