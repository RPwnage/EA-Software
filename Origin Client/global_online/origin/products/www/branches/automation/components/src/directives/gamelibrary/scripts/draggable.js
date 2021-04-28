/**
 * @file gamelibrary/draggable.js
 */
(function() {
    'use strict';

    var CLS_DROPTARGET = 'origin-gameslist-droptarget',
        CLS_GAMESLIST = 'origin-gameslist',
        CLS_GAMESLIST_ITEM = 'origin-gameslist-item',
        CLS_GAMESLIST_PROXYELEMENT = 'origin-gameslist-proxyelement',
        CLS_GAME_TILE_IMAGE = 'origin-gametile-img',

        // constants
        TARGET_FAVORITE = 'gamelibrary-favoritegames',
        TARGET_OTHER = 'gamelibrary-othergames',
        PROXY_PADDING = 10;

    function OriginGamelibraryDraggableCtrl(GamesDataFactory) {

        var offerId = '';

        /**
        * Set the offer id of the current game we are
        * dragging.
        * @param {String} _offerId
        * @method setOfferId
        * @return {void}
        */
        this.setOfferId = function(_offerId) {
            offerId = _offerId;
        };

        /**
        * Set the game as a favorite
        * @return {void}
        * @method favoriteGame
        */
        this.favoriteGame = function() {
            GamesDataFactory.addFavoriteGame(offerId);
        };

        /**
        * Unfavorite the game
        * @return {void}
        * @method unfavoriteGame
        */
        this.unfavoriteGame = function() {
            GamesDataFactory.removeFavoriteGame(offerId);
        };

    }


    function originGamelibraryDraggable(UtilFactory) {

    	function originGamelibraryDraggableLink(scope, element, attrs, ctrl) {

    		var body = angular.element(document.querySelector('body')),
    			currentGamesList,
                currentGamesListItem,
                gamesLists,
                proxy;

            /**
            * Get the closest games list item
            * @param {Node} node
            * @return {Node}
            * @method getGamesListItem
            */
    		function getGamesListItem(node) {
    			return UtilFactory.getParentByClassName(node, CLS_GAMESLIST_ITEM);
    		}

            /**
            * Get the closest games list
            * @param {Node} node
            * @return {Node}
            * @method getParentGamesList
            */
    		function getParentGamesList(node) {
                return UtilFactory.getParentByClassName(node, CLS_GAMESLIST);
    		}

            /**
            * Get the offer id from the game list item
            * @param {Node} gamesListItem
            * @return {String}
            * @method getOfferId
            */
            function getOfferId(gamesListItem) {
                return gamesListItem.find('origin-game-tile')[0].getAttribute('offerid');
            }

            /**
            * Get the games lists and get them from cache if they are cached
            * @return {Object}
            * @method getGamesLists
            */
            function getGamesLists() {
                if (!gamesLists) {
                    gamesLists = {
                        'target': angular.element(document.querySelector('[data-gameslist-id="' + attrs.draggableTarget + '"]')),
                        'current': angular.element(document.querySelector('[data-gameslist-id="' + attrs.id + '"]'))
                    };
                }
                return gamesLists;
            }

            /**
            * Get the transition type between the new games list
            * and the previous games list that we are dragging from
            * @param {dropGamesList}
            * @return {String}
            * @method getTransitionType
            */
            function getTransitionType(dropGamesList) {

                //not a valid drop target, so return early
                if (!(dropGamesList && dropGamesList.length)) {
                    return '';
                }

                var gamesLists = getGamesLists();
                if (angular.equals(dropGamesList[0], gamesLists.target[0])) {
                    if (attrs.draggableTarget === TARGET_FAVORITE) {
                        return 'favorite';
                    } else if (attrs.draggableTarget === TARGET_OTHER) {
                        return 'unfavorite';
                    } else {
                        return ''; // this should never happen
                    }
                } else {
                    return '';
                }
            }

            /**
            * Create the drag proxy element if we need it
            * @return {void}
            * @method createGameProxy
            */
            function createGameProxy() {
                var img = currentGamesListItem.find('.' + CLS_GAME_TILE_IMAGE),
                    _proxy = document.createElement('img');
                _proxy.src = img[0].getAttribute('src');
                _proxy.alt = '';
                proxy = angular.element(_proxy);
                proxy.addClass(CLS_GAMESLIST_PROXYELEMENT);
                body.append(proxy);
            }

            /**
            * Set the position of the proxy element
            * @param {Number} x
            * @param {Number} y
            * @method setProxyPosition
            */
            function setProxyPosition(x, y) {
                proxy.css({
                    'left': (x + PROXY_PADDING) + 'px',
                    'top': (y + PROXY_PADDING) + 'px'
                });
            }

            /**
            * Reset the proxy element
            * @return {void}
            * @method resetProxy
            */
            function resetProxy() {
                if (!!proxy) {
                    proxy.remove();
                    proxy = null;
                }
            }

            /**
            * Remove the classes from all the drop targets.
            * @return {void}
            * @method resetDropTarget
            */
            function resetDropTargets() {
                angular.element(document.querySelectorAll('.' + CLS_DROPTARGET))
                    .removeClass(CLS_DROPTARGET);
            }

            /**
            * Determine what game we are trying to drag and then
            * add event listeners for dragging the game.
            * @param {Event} e
            * @return {void}
            * @method onMouseDown
            */
    		function onMouseDown(e) {
    			var targ = angular.element(e.target),
                    gamesListItem = getGamesListItem(targ);
                if (!!gamesListItem) {
    				e.preventDefault();
                    currentGamesList = getParentGamesList(targ);
                    currentGamesListItem = gamesListItem;
                    ctrl.setOfferId(getOfferId(gamesListItem));
    				body.on('mousemove', onMouseMove);
    				body.on('mouseup', onMouseUp);
    			}
    		}

            /**
            * Move the proxy element and highlight the games list that we are hovering over
            * (I wonder if we can do this with CSS instead??)
            * @param {Event} e
            * @return {void}
            * @method onMouseMove
            */
    		function onMouseMove(e) {
                var targ = angular.element(e.target),
    				gamesList = getParentGamesList(targ);
                if (!proxy) {
                    createGameProxy();
                }
                setProxyPosition(e.pageX, e.pageY);
                resetDropTargets();
                if (gamesList) {
    				gamesList.addClass(CLS_DROPTARGET);
    			}
    			e.preventDefault();
    		}

            /**
            * Determine where we should drop the game and then
            * execute the appropriate function based on the bucket transition
            * @param {Event} e
            * @return {void}
            * @method onMouseUp
            */
    		function onMouseUp(e) {
                var targ = angular.element(e.target),
                    dropGamesList = getParentGamesList(targ),
                    transitionType = getTransitionType(dropGamesList);

                resetDropTargets();
                resetProxy();

                if (transitionType === 'favorite') {
                    ctrl.favoriteGame();
                } else if (transitionType === 'unfavorite') {
                    ctrl.unfavoriteGame();
                }

                body.off('mousemove', onMouseMove);
                body.off('mouseup', onMouseUp);
                e.preventDefault();
    		}

    		element.on('mousedown', onMouseDown);
    	}

        return {
            restrict: 'A',
            controller: 'OriginGamelibraryDraggableCtrl',
            link: originGamelibraryDraggableLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryDraggable
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * game library anonymous container
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-gameslist></origin-gamelibrary-gameslist>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGamelibraryDraggableCtrl', OriginGamelibraryDraggableCtrl)
        .directive('originGamelibraryDraggable', originGamelibraryDraggable);

}());