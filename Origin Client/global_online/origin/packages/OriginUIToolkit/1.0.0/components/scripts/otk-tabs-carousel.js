
!function ($) {

    "use strict";

    // CONSTANTS
    var constants = new (function()
    {
        var animationOverflowMenuOpenMs = 100,
            animationOverflowMenuCloseMs = 100,
            carouselScrollMultiplier = 25,
            overflowMenuScrollMultiplier = 20;

        this.animationOverflowMenuOpenMs = function() { return animationOverflowMenuOpenMs; };
        this.animationOverflowMenuCloseMs = function() { return animationOverflowMenuCloseMs; };
        this.carouselScrollMultiplier = function() { return carouselScrollMultiplier; };
        this.overflowMenuScrollMultiplier = function() { return overflowMenuScrollMultiplier; };

    })();

    // OPTIONAL HTML CONTENT BUILDER

    $.otkTabsCarouselCreate = function (element, options)
    {
        var $element = $(element), $html;
        $element.addClass("otk-tabs-carousel");
        $html = $(
                '<div class="carousel">' +
                    '<div class="endcap left">' +
                        '<button class="scroll-left"></button>' +
                    '</div>' +
                    '<div class="tabs-content">' +
                    // *** an otk-tabs component goes here ***
                    '</div>' +
                    '<div class="endcap right">' +
                        '<button class="scroll-right"></button>' +
                        '<button class="overflow"></button>' +
                    '</div>' +
                    // bottom edge div is dynamically added here
                '</div>' +
                '<div class="overflow-menu">' +
                    '<div class="scroll-up scroll-button"></div>' +
                    '<div class="scroll-area">' +
                        '<ul class="items"></ul>' +
                    '</div>' +
                    '<div class="scroll-down scroll-button"></div>' +
                '</div>'
                );
        $element.append($html);
        $element.otkTabsCarousel(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkTabsCarousel",
    {
        // Default options.
        options:
        {
        },
 
        _create: function()
        {
            //
            // See if there are already values for our defaults
            //

            // If there is already an otk-tab element hosted, init it here
            if (this.tabs().length)
            {
                this.tabs().otkTabs();
            }

            // Add the bottom edge div
            this.element.find('.carousel').append($('<div class="bottom-edge"></div>'));

            // Listen for when the window is resized. We'll need to update ourselves.
            $( window ).resize(function()
            {
                this._resize();
            }.bind(this));

            // Listen for when the number of tabs has changed so we can update accordingly
            this.element.on('numTabsChanged', '.otk-tabs', function(evt)
            {
                this._update(false);
            }.bind(this));

            // Listen for when a tab has been notified so we can update accordingly
            this.element.on('tabNotified', '.otk-tabs', function(evt, $tab)
            {
                // TODO - we may want to dynamically update the overflow menu here :-)
                // For now we just update the overflow menu button state
                this._updateScrollButtons();
            }.bind(this));

            // Listen for when the selected tabs has changed so we can update accordingly
            this.element.on('currentTabChanged', '.otk-tabs', function(evt)
            {
                this._ensureSelectedTabVisible(true);
            }.bind(this));

            // Listen tabs dragging start so we can keep them scrolled into view
            this.element.on('tabDragStart', '.otk-tabs', function(evt, $tab)
            {
                this.ensureTabVisible($tab, true);
            }.bind(this));

            // Listen tabs being dragged so we can keep them scrolled into view
            this.element.on('tabDragged', '.otk-tabs', function(evt, $tab)
            {
                this.ensureTabVisible($tab, false);
            }.bind(this));

            // Listen tabs being dropped so we can keep them scrolled into view
            this.element.on('tabDropped', '.otk-tabs', function(evt, $tab)
            {
                this.ensureTabVisible($tab, true);
            }.bind(this));

            // Listen for the left scroll button
            this.element.on('mousedown', 'button.scroll-left', function(evt)
            {
                this.element.data("scrollDir", 1);
                this._doScrolling();
            }.bind(this));
            this.element.on('mouseup', 'button.scroll-left', function(evt)
            {
                this.element.data("scrollDir", 0);
            }.bind(this));
            this.element.on('mouseout', 'button.scroll-left', function(evt)
            {
                this.element.data("scrollDir", 0);
            }.bind(this));

            // Listen for the right scroll button
            this.element.on('mousedown', 'button.scroll-right', function(evt)
            {
                this.element.data("scrollDir", -1);
                this._doScrolling();
            }.bind(this));
            this.element.on('mouseup', 'button.scroll-right', function(evt)
            {
                this.element.data("scrollDir", 0);
            }.bind(this));
            this.element.on('mouseout', 'button.scroll-right', function(evt)
            {
                this.element.data("scrollDir", 0);
            }.bind(this));

            this.element.on('mousewheel', function(evt)
            {
                evt.stopPropagation();
                evt.preventDefault();
                if (this.isOverflowed())
                {
                    this._scrollBy(evt.deltaY*constants.carouselScrollMultiplier());
                }
            }.bind(this));

            // Listen for the overflow menu button and overflow menu mouse events
            this.element.on('mousedown', 'button.overflow', function(evt)
            {
                var $menuButton = this.element.find("button.overflow");

                evt.stopPropagation();

                if ($menuButton.hasClass("menu-open"))
                {
                    this._hideOverflowMenu();
                }
                else if ($menuButton.hasClass("menu-closing") === false)
                {
                this._showOverflowMenu();
                }

            }.bind(this));

            this.element.on('mousewheel', 'button.overflow', function(evt)
            {
                evt.stopPropagation();
                evt.preventDefault();
            }.bind(this));

            this.element.on('mouseenter', '.overflow-menu', function(evt)
            {
                this._showOverflowMenu();
            }.bind(this));
            
            this.element.on('mousewheel', '.overflow-menu', function(evt)
            {
                evt.stopPropagation();
                evt.preventDefault();
                this._scrollOverflowMenuBy(evt.deltaY * constants.overflowMenuScrollMultiplier());
            }.bind(this));

            // Check initial overflow state
            if (this.isOverflowed())
            {
                this.element.addClass("overflowed");
            }
            else
            {
                this.element.removeClass("overflowed");
            }

            this._update(true);
        },

         _setOption: function( key, value )
        {
            this.options[ key ] = value;
            this._update(true);
        },

        _update: function(keepSelectedTabVisible)
        {
            this._resize(keepSelectedTabVisible);
        },

        // The component has resized
        _resize: function(keepSelectedTabVisible)
        {
            var overflowed = this.isOverflowed(),
                wasOverflowed = this.element.hasClass("overflowed");

            if (overflowed && !wasOverflowed)
            {
                // We just became overflowed
            }
            else if (!overflowed && wasOverflowed)
            {
                // We just stopped being overflowed

                // Reset the tab scroll offset to 0
                this._animateScrollOffset(0, true);
            }

            // Save our state for next time so we can detect transitions
            if (overflowed)
            {
                this.element.addClass("overflowed");
            }
            else
            {
                this.element.removeClass("overflowed");
            }

            if (overflowed && keepSelectedTabVisible)
            {
                // When resizing the selected tab should always be kept in-view
                this._ensureSelectedTabVisible(true);
            }

            this._hideOverflowMenu();
            this._updateScrollButtons();
        },

        // Make sure the selected tab is visible
        _ensureSelectedTabVisible: function(animated)
        {
            var $selectedTab = this.tabs().otkTabs("selectedTab");
            if ($selectedTab && $selectedTab.length)
            {
                this.ensureTabVisible($selectedTab, animated);
            }
        },

        _updateScrollButtons: function()
        {
            var $tabs = this.tabs(),
                currentScrollOffset = parseInt($tabs.css('left'), 10),
                $scrollLeftButton = this.element.find('button.scroll-left'),
                $scrollRightButton = this.element.find('button.scroll-right'),
                $menuButton = this.element.find("button.overflow"),
                overflowedTabs = this.overflowedTabs(),
                notificationCount = 0;

            // See if we can scroll left
            if (this._clampedScrollValue(currentScrollOffset+1) <= currentScrollOffset)
            {
                // We can't scroll left
                $scrollLeftButton.attr("disabled", true);
            }
            else
            {
                // We can scroll left
                $scrollLeftButton.attr("disabled", false);
            }

            // See if we can scroll right
            if (this._clampedScrollValue(currentScrollOffset-1) >= currentScrollOffset)
            {
                // We can't scroll right
                $scrollRightButton.attr("disabled", true);
            }
            else
            {
                // We can scroll right
                $scrollRightButton.attr("disabled", false);
            }

            // See if there are tabs with notifications offscreen
            overflowedTabs.map(function($tab)
            {
                // If we don't need the actual count we should break once we find ANY tab with a non-zero count
                notificationCount += $tabs.otkTabs("notificationCount", $tab);
            });

            if (notificationCount > 0)
            {
                $menuButton.addClass("notified");
            }
            else
            {
                $menuButton.removeClass("notified");
            }

        },

        _animateScrollOffset: function(offset)
        {
            this.tabs().stop().animate({left: offset + 'px'}, 125, function()
            {
                this._updateScrollButtons();
            }.bind(this));
        },

        _doScrolling: function()
        {
            var $tabs = this.tabs(),
                scrollDir = this.element.data("scrollDir"),
                currentScrollOffset = parseInt($tabs.css('left'), 10),
                newOffset;

            if (scrollDir !== 0)
            {
                newOffset = currentScrollOffset + (scrollDir * 20); // scroll 20px at a time
                newOffset = this._clampedScrollValue(newOffset);
                if (newOffset !== currentScrollOffset)
                {
                    this.tabs().stop().animate({left: newOffset + 'px'}, 25, function()
                    {
                        this._updateScrollButtons();
                        scrollDir = this.element.data("scrollDir");
                        if (scrollDir !== 0)
                        {
                            // Recursive
                            // Keep scrolling if the left or right scroll buttons are still down
                            this._doScrolling();
                        }
                    }.bind(this));
                }
            }
        },

        _clampedScrollValue: function(value)
        {
            var $tabs = this.tabs(),
                tabsWidth = $tabs.width(),
                $tabContentArea = this.element.find('.tabs-content'),
                tabContentAreaWidth = $tabContentArea.innerWidth(),
                tabContentDifference = tabContentAreaWidth - tabsWidth,
                minOffset,
                maxOffset;

            if (tabContentDifference < 0)
            {
                minOffset = tabContentDifference;
                maxOffset = 0;
            }
            else
            {
                minOffset = 0;
                maxOffset = tabContentDifference;
            }

            if (value < minOffset)
                return minOffset;
            else if (value > maxOffset)
                return maxOffset;

            return value;
        },

        _scrollBy: function(value)
        {
            if (value !== 0)
            {
                var $tabs = this.tabs(),
                    currentScrollOffset = parseInt($tabs.css('left'), 10),
                    newOffset = currentScrollOffset + value;

                newOffset = this._clampedScrollValue(newOffset);
                if (newOffset !== currentScrollOffset)
            {
                    this.tabs().stop().css({left: newOffset + 'px'});
                    this._updateScrollButtons();
            }
            }
        },

        _showOverflowMenu: function()
        {
            var self = this,
                $menu = this.element.find(".overflow-menu"),
                $menuButton = this.element.find("button.overflow"),
                $tabs = this.tabs(),
                $allTabs = $tabs.otkTabs("allTabs"), selectedTabIndex,
                $itemList = this.element.find(".overflow-menu .items"),
                iconColumn1Width, iconColumn2Width,
                $listItemTemplate,
                windowBottomPx, menuHeightPx, menuBottomPx, menuOverflowPx,
                $scrollArea = $menu.find(".scroll-area"), scrollAreaHeightPx;

            if ($menuButton.hasClass("menu-open")) { return; } // already open

            // Clear out any previous list items
            $itemList.empty();

            iconColumn1Width = 0;
            iconColumn2Width = 0;

            // Reset the scroll offset
            $itemList.css("top", 0);

            // Go through our list of overflowed tabs and create list items for each of them
            $listItemTemplate = $('<li><div class="icon1"></div><div class="icon2"></div><span class="title"></span></li>'),
            selectedTabIndex = $tabs.otkTabs("selectedTabIndex");
            $allTabs.each(function(index)
            {
                var $tab = $(this),
                    text = $tabs.otkTabs("tabTitle", $tab),
                    subtitle = $tabs.otkTabs("tabSubtitle", $tab),
                    $item = $listItemTemplate.clone(),
                    notificationCount = $tabs.otkTabs("notificationCount", $tab),
                    $badge,
                    $tabIcon1, $tabIcon2,
                    $title = $item.find(".title");

                // Set the title text from the tab
                $title.text(text);

                // Mark item corresponding to the tab as selected
                if (selectedTabIndex === index)
                {
                    $item.addClass("selected");
                }

                // See if the tab has an icon1
                $tabIcon1 = $tabs.otkTabs("tabIcon1", $tab);
                if ($tabIcon1.length)
                {
                    if ($tabIcon1.width() > iconColumn1Width)
                    {
                        iconColumn1Width = $tabIcon1.width();
                    }
                    $item.find(".icon1").append($tabIcon1.clone());
                }

                // See if the tab has an icon2
                $tabIcon2 = $tabs.otkTabs("tabIcon2", $tab);
                if ($tabIcon2.length)
                {
                    if ($tabIcon2.width() > iconColumn2Width)
                    {
                        iconColumn2Width = $tabIcon2.width();
                    }
                    $item.find(".icon2").append($tabIcon2.clone());
                }
                else if (subtitle)
                {
                    // If we have a subtitle but no icon2, take up the space of the icon2 column
                    $item.addClass("colspan2");
                }

                // Create and append the count badge and set its value to the same as the tab's
                $badge = $.otkCountBadgeCreate('<div>', { value: notificationCount, autoHide: true });
                $item.append($badge);

                // Add the new item to our list
                $item.data("tab", $tab); // Save a reference to the tab object this item represents. Is this bad?
                $itemList.append($item);

                // Listen for the item being clicked
                $item.on('mousedown', function(evt)
                {
                    var $tab = $(this).data("tab");

                    evt.stopPropagation();

                    // Select the tab
                    $tabs.otkTabs("selectTab", $tab);

                    // Scroll the tab into view
                    self.ensureTabVisible($tab, true);

                    // Close the menu
                    self._hideOverflowMenu();
                });

                // Close the menu if the user clicks anywhere besides the menu
                $('body').on('mousedown', function(evt)
                {
                    self._hideOverflowMenu();
                });
            });

            // Size the icon columns
            $itemList.find("li > .icon1").css("width", iconColumn1Width + "px");
            $itemList.find("li > .icon2").css("width", iconColumn2Width + "px");
            if (iconColumn1Width === 0)
            {
                $itemList.find("li > .icon1").css({"margin-left": "0", "margin-right": "0" });
            }
            else
            {
                $itemList.find("li > .icon1").css({"margin-left": "4px", "margin-right": "4px" });
            }
            if (iconColumn2Width === 0)
            {
                $itemList.find("li > .icon2").css({"margin-left": "0", "margin-right": "0" });
            }
            else
            {
                $itemList.find("li > .icon2").css({"margin-left": "4px", "margin-right": "4px" });
            }

            // If we have a subtitle but no icon2, take up the space of the icon2 column
            $itemList.find("li.colspan2 > .icon2").hide();
            $itemList.find("li.colspan2 .title").css("margin-left", "4px");

            // Assume we aren't overflowed
            $scrollArea.css("max-height", "");
            $menu.removeClass("overflowed");

            // Update the buttons to NOT have the overflow UI
            self._updateScrollOverflowMenuButtons();

            // Now check to see if we have enough menu items to overflow
            // We want to make sure the bottom of the overflow menu does not extend off the bottom edge of the window
            windowBottomPx = parseInt($(window).scrollTop()) + parseInt($(window).height()) - 4; // - 4 for a small gutter at the bottom of the window
            menuHeightPx = parseInt($menu.outerHeight(true));
            menuBottomPx = parseInt($menu.css("top")) + parseInt(this.element.offset().top) + menuHeightPx;
            if (menuBottomPx > windowBottomPx)
            {
                // We're overflowed - enable showing the scrolling UI
                $menu.addClass("overflowed");

                // We need to temporarily show the menu in order to get the correct heights.
                // We'll hide it again when we're done and allow the slideDown animation to happen
                // I'm setting the opacity to 0 temporarily to try and make sure we don't get a visual glitch here
                $menu.css("opacity", "0");
                $menu.show();

                // Update the buttons to have the overflow UI now that we know we're overflowed
                // as this will make the menu slightly taller to account for the overflow buttons
                self._updateScrollOverflowMenuButtons();

                // Get the menu height and bottom position again now that we have overflow UI visible
                menuHeightPx = parseInt($menu.outerHeight(true));
                scrollAreaHeightPx = parseInt($scrollArea.height());
                menuBottomPx = parseInt($menu.css("top")) + parseInt(this.element.offset().top) + menuHeightPx;
                menuOverflowPx = menuBottomPx - windowBottomPx;

                // Set a max-height on the scrollable area that will place the bottom of the menu at the bottom of the window
                $scrollArea.css("max-height", scrollAreaHeightPx - menuOverflowPx + "px");

                // Update the buttons one last time now that we're overflowed and have the proper max-height set on the scrollable area
                self._updateScrollOverflowMenuButtons();

                // Try to center the selected tab in the scroll area
                self._centerSelectedTabInOverflowMenu();
                
                // Lastly, hide the menu again now that we have the values we need
                $menu.hide();
                $menu.css("opacity", "");

                // Scroll the menu items when hovered over the scroll areas
                $menu.on('mouseenter', '.scroll-up', function(evt)
                {
                    if (!this.menuScrollTimer)
                    {
                        // Start scrolling
                        this.menuScrollTimer = setInterval(function() { self._scrollOverflowMenuBy(2 * constants.overflowMenuScrollMultiplier()); }, 25);
                    }
                });
                $menu.on('mouseleave', '.scroll-up', function(evt)
                {
                    // Stop scrolling
                    if (!!this.menuScrollTimer)
                    {
                        clearInterval(this.menuScrollTimer);
                        delete this.menuScrollTimer;
                    }
                    self._updateScrollOverflowMenuButtons();
                });
                $menu.on('mousewheel', '.scroll-up', function(evt)
                {
                    evt.stopPropagation();
                    evt.preventDefault();
                });
                $menu.on('mouseenter', '.scroll-down', function(evt)
                {
                    if (!this.menuScrollTimer)
                    {
                        // Start scrolling
                        this.menuScrollTimer = setInterval(function() { self._scrollOverflowMenuBy(-2 * constants.overflowMenuScrollMultiplier()); }, 25);
                    }
                });
                $menu.on('mouseleave', '.scroll-down', function(evt)
                {
                    // Stop scrolling
                    if (!!this.menuScrollTimer)
                    {
                        clearInterval(this.menuScrollTimer);
                        delete this.menuScrollTimer;
                    }
                    self._updateScrollOverflowMenuButtons();
                });
                $menu.on('mousewheel', '.scroll-down', function(evt)
                {
                    evt.stopPropagation();
                    evt.preventDefault();
                });
            }

            // Show the menu
            $menu.slideDown(constants.animationOverflowMenuOpenMs());

            $menuButton.addClass("menu-open");
        },

        _hideOverflowMenu: function()
        {
            var $menu = this.element.find(".overflow-menu"),
                $menuButton = this.element.find("button.overflow");

            if ($menuButton.hasClass("menu-open"))
            {
                $menuButton.addClass("menu-closing");
                $menu.slideUp(constants.animationOverflowMenuCloseMs(), function()
                {
                    $menuButton.removeClass("menu-closing");
                });
                $menuButton.removeClass("menu-open");
                $(document).off('click');
            }
        },

        _updateScrollOverflowMenuButtons: function()
        {
            var $menu = this.element.find(".overflow-menu"),
                $itemList = this.element.find(".overflow-menu .items"),
                itemListHeight = $itemList.height(),
                availableHeight = $menu.find(".scroll-area").height(),
                currentScrollOffset = parseInt($itemList.css('top'), 10);

            if (currentScrollOffset === 0)
            {
                $menu.find(".scroll-up").attr('disabled', 'disabled');
            }
            else
            {
                $menu.find(".scroll-up").removeAttr('disabled');
            }
            if ((currentScrollOffset + itemListHeight) === availableHeight)
            {
                $menu.find(".scroll-down").attr('disabled', 'disabled');
            }
            else
            {
                $menu.find(".scroll-down").removeAttr('disabled');
            }
        },

        _scrollOverflowMenuBy: function(value)
        {
            if (value !== 0)
        {
            var $menu = this.element.find(".overflow-menu"),
                $itemList = this.element.find(".overflow-menu .items"),
                itemListHeight = parseInt($itemList.height()),
                availableHeight = parseInt($menu.find(".scroll-area").height()),
                currentScrollOffset = parseInt($itemList.css('top'), 10);

                var newOffset = currentScrollOffset + (value);

                // Keep it in range
                if (newOffset > 0) { newOffset = 0; }
                if ((newOffset + itemListHeight) < availableHeight) { newOffset = availableHeight - itemListHeight; }

                // Update the scroll button states
                // Don't call _updateScrollOverflowMenuButtons() because we already have all the info we need here
                if (newOffset === 0)
                {
                    $menu.find(".scroll-up").attr('disabled', 'disabled');
                }
                else
                {
                    $menu.find(".scroll-up").removeAttr('disabled');
                }
                if ((newOffset + itemListHeight) === availableHeight)
                {
                    $menu.find(".scroll-down").attr('disabled', 'disabled');
                }
                else
                {
                    $menu.find(".scroll-down").removeAttr('disabled');
                }

                if (newOffset !== currentScrollOffset)
                {
                    $itemList.css({top: newOffset + 'px'});
                }
            }
        },

        _centerSelectedTabInOverflowMenu: function()
        {
            var $menu = this.element.find(".overflow-menu"),
                $itemList = this.element.find(".overflow-menu .items"),
                itemListHeight = parseInt($itemList.height()),
                availableHeight = parseInt($menu.find(".scroll-area").height()),
                $selectedItem = this.element.find(".overflow-menu .selected"),
                selectedItemOffset = parseInt($selectedItem.position().top),
                newOffset;
                
            // Center the selected tab
            newOffset = (availableHeight / 2) - selectedItemOffset;

            // Keep it in range
            if (newOffset > 0) { newOffset = 0; }
            if ((newOffset + itemListHeight) < availableHeight) { newOffset = availableHeight - itemListHeight; }

            // Update the scroll button states
            // Don't call _updateScrollOverflowMenuButtons() because we already have all the info we need here
            if (newOffset === 0)
            {
                $menu.find(".scroll-up").attr('disabled', 'disabled');
            }
            else
            {
                $menu.find(".scroll-up").removeAttr('disabled');
            }
            if ((newOffset + itemListHeight) === availableHeight)
            {
                $menu.find(".scroll-down").attr('disabled', 'disabled');
            }
            else
            {
                $menu.find(".scroll-down").removeAttr('disabled');
            }

            $itemList.css("top", newOffset + "px");
        },
        
        // *** START PUBLIC INTERFACE ***

        // Returns the otk-tabs component for this carousel
        //
        // NOTE: this does NOT return a collection of actual tabs, just the otk-tabs component
        //
        // To get a collection of the actual tabs, do this:
        //  this.tabs().otkTabs("allTabs");
        //
        tabs: function()
        {
            return this.element.find('.otk-tabs');
        },

        // Returns an array of all tabs that are at least partially scrolled out of view
        overflowedTabs: function()
        {
            var tabArray = new Array();

            if (this.isOverflowed())
            {
                var $tabs = this.tabs(),
                    $allTabs = $tabs.otkTabs("allTabs"),
                    $tabContentArea, tabContentAreaWidth,
                    currentScrollOffset;

                currentScrollOffset = parseInt($tabs.css('left'), 10);

                $tabContentArea = this.element.find('.tabs-content');
                tabContentAreaWidth = $tabContentArea.innerWidth();

                $allTabs.each(function(index)
                {
                    var $tab = $(this),
                        tabLeft, tabRight, tabWidth, tabMarginLeft;

                    tabMarginLeft = parseInt($tab.css("margin-left"), 10);
                    tabWidth = $tabs.otkTabs("tabWidth", $tab);
                    tabLeft = $tab.position().left + tabMarginLeft + currentScrollOffset;
                    tabRight = tabLeft + tabWidth;

                    // This tab is at least partially scrolled out of view
                    // Add it to the array
                    if ((tabLeft < 0) || (tabRight > tabContentAreaWidth))
                    {
                        tabArray.push($tab);
                    }
                });
            }

            return tabArray;
        },

        hideOverflowMenu: function()
        {
            this._hideOverflowMenu();
        },

        // Inserts the specified tabs component into the carousel
        setTabs: function($tabs)
        {
            var $tabContentArea = this.element.find('.tabs-content');

            if ($tabs.hasClass('otk-tabs') === false) { return; }

            // Remove anything that's there now
            $tabContentArea.empty();

            // Add the new tabs
            $tabContentArea.append($tabs);

            this._update(true);
        },

        // Returns true if the there is not enough room to display all tabs, else returns false
        isOverflowed: function()
        {
            if (this.tabs().width() > this.element.find('.tabs-content').width())
            {
                return true;
            }
            else
            {
                return false;
            }
        },

        // Scrolls the specified tab into view if it is not already
        ensureTabVisible: function($tab, animated)
        {
            var tabLeft, tabRight, tabWidth, tabMarginLeft,
                $tabs = this.tabs(),
                $tabContentArea, tabContentAreaWidth,
                newOffset, currentScrollOffset;

            if ($tabs.otkTabs("tabCount") === 0) { return; }
            if (($tab === null) || ($tab.length === 0)) { return; }
            if (this.isOverflowed() === false) { return; }

            currentScrollOffset = parseInt($tabs.css('left'), 10);
            newOffset = currentScrollOffset;

            $tabContentArea = this.element.find('.tabs-content');
            tabContentAreaWidth = $tabContentArea.innerWidth();

            tabMarginLeft = parseInt($tab.css("margin-left"), 10);
            tabWidth = $tabs.otkTabs("tabWidth", $tab);
            tabLeft = $tab.position().left + tabMarginLeft + currentScrollOffset;
            tabRight = tabLeft + tabWidth;

            // Scroll left or right as needed to in order to keep the specified tab in-view
            if (tabLeft < 0)
            {
                newOffset = (-tabLeft) + currentScrollOffset;
            }
            else if (tabRight > tabContentAreaWidth)
            {
                newOffset = (tabContentAreaWidth - tabRight) + currentScrollOffset;
            }
            // else - the tab is already completely in-view

            // Make sure we don't scroll too far left or right
            newOffset = this._clampedScrollValue(newOffset);

            // Animate or simply move to the new offset
            if (newOffset !== currentScrollOffset)
            {
                if (animated)
                {
                    this._animateScrollOffset(newOffset);
                }
                else
                {
                    this.tabs().stop().css("left", newOffset + 'px');
                    this._updateScrollButtons();
                }
            }

            // TODO - COVER THE EDGE CASE WHERE THE TAB IS WIDER THAN THE CAROUSEL

        },

        advanceTabFocus: function()
        {
            this.tabs().otkTabs("advanceTabFocus");
            this._ensureSelectedTabVisible(true);
        },

        rewindTabFocus: function()
        {
            this.tabs().otkTabs("rewindTabFocus");
            this._ensureSelectedTabVisible(true);
        },


    });

}(jQuery);

