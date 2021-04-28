
!function ($) {

    "use strict";

    // CONSTANTS
    var constants = new (function()
    {
        var animationMs = 125,
            appendAnimationMs = 150,
            closeAnimationMs = 125,
            dislodgeThresholdPx = 10;

        this.animationMs = function() { return animationMs; };
        this.appendAnimationMs = function() { return appendAnimationMs; };
        this.closeAnimationMs = function() { return closeAnimationMs; };
        this.dislodgeThresholdPx = function() { return dislodgeThresholdPx; };

    })();

    // OPTIONAL HTML CONTENT BUILDER

    $.otkTabsCreate = function (element, options)
    {
        var $element = $(element), $html;
        $element.addClass("otk-tabs");
        $html = $(
            '<ul></ul>'
            // bottom edge div is dynamically added here
            );
        $element.append($html);
        $element.otkTabs(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkTabs",
    {
        // Default options.
        options:
        {
            squareFirstTab: false,
            closeable: false,
            draggable: false,
            short: false
        },
 
        _create: function()
        {
            var self = this;

            if (this.element.hasClass('square-first-tab'))
            {
                this.options.squareFirstTab = true;
            }

            if (this.element.hasClass('closeable'))
            {
                this.options.closeable = true;
            }

            if (this.element.hasClass('draggable'))
            {
                this.options.draggable = true;
            }

            if (this.element.hasClass('short'))
            {
                this.options.short = true;
            }

            {
                // GO THROUGH ANY EXISTING TAB <LI> ITEMS AND LOOK
                // FOR POSSIBLE COUNT BADGES, THEN INIT THEM IF FOUND
                var $existingTabs = this.allTabs();
                if ($existingTabs.length)
                {
                    // This is jacked
                    // For some reason statically declared tab items have incorrect spacing on them
                    // If we pull them out of the list and put them back it works fine. >:-(
                    // If we use float:left on the tab <li> elements (instead of display:inline-block)
                    // we don't have this problem but that causes the tabs to wrap to the next line when
                    // when they overflow, which we don't want.
                    $existingTabs.remove();
                    this.element.find('ul').append($existingTabs);

                    // Init the count badges
                    $existingTabs.each(function(index)
                    {
                        if ($(this).has('.otk-count-badge').length)
                        {
                            var $badge = $(this).find('.otk-count-badge');
                            $badge.otkCountBadge({autoHide: true});
                        }
                    });
                }
            }

            // Add the bottom edge div
            this.element.append($('<div class="bottom-edge"></div>'));

            // Select a new current tab on click
            this.element.on('click', 'ul > li', function(evt)
            {
                var $tab = $(this);

                if (evt.button !== 0) { return; }

                evt.preventDefault();

                self.selectTab($tab);
            });

            // Start tab dragging on mousedown
            this.element.on('mousedown', 'ul > li', function(evt)
            {
                var $tab = $(this);

                if (evt.button !== 0) { return; }

                evt.preventDefault();

                if (self.options.draggable === false) { return; }

                self._startDrag(self, $tab, evt);
            });

            // Handle clicking the optional close button
            this.element.on('click', 'ul > li .otk-close-button', function(evt)
            {
                var $this = $(this),
                    $tab = $this.closest('li');

                if (evt.button !== 0) { return; }

                evt.stopPropagation();
                evt.preventDefault();

                // Indicate that the user clicked the close button
                self.element.trigger("tabCloseClicked", [ $tab ]);
            });

            if ((this.selectedTab() === null) && (this.tabCount() > 0))
            {
                // There isn't a selected tab - select the first one
                this.selectTabByIndex(0);
            }

            this._update();
        },

        _setOption: function( key, value )
        {
            this.options[ key ] = value;
            this._update();
        },

        _update: function()
        {
            this._stackElements();

            if (this.options.squareFirstTab)
            {
                this.options.draggable = false; // No dragging allowed with square first tabs
                this.element.addClass("square-first-tab");
            }
            else
            {
                this.element.removeClass("square-first-tab");
            }

            if (this.options.closeable)
            {
                this.element.addClass("closeable");
            }
            else
            {
                this.element.removeClass("closeable");
            }

            if (this.options.draggable)
            {
                this.element.addClass("draggable");
            }
            else
            {
                this.element.removeClass("draggable");
            }

            if (this.options.short)
            {
                this.element.addClass("short");
            }
            else
            {
                this.element.removeClass("short");
            }

        },

        // Properly updates the z-index of each element appropriately
        _stackElements: function()
        {
            var $tabs = this.allTabs(),
                tabCount = this.tabCount(),
                $currentTab = this.element.find('ul > li.current'),
                $bottomEdge = this.element.find('.bottom-edge');

            // Update the z-orders to properly layer the overlapping parts of the tabs
            // ## This seems to lag when I click an item. Evaluate for faster ways of doing this.
            $tabs.each(function(index)
            {
                $(this).css('z-index', (tabCount-index));
            });

            // Layer the bottom edge above all tabs except the selected one
            $bottomEdge.css('z-index', tabCount);

            // Make the current tab topmost
            $currentTab.css('z-index', tabCount+1);
        },

        _newEmptyTab: function()
        {
            var $tab, $badge;
            
            $tab = $('<li>' +
                        '<div class="contents">' +
                            // icon 1 gets dynamically added here
                            // icon 2 gets dynamically added here
                            '<div class="labels">' +
					            '<span class="title"></span>' +
                            '</div>' +
                        '</div>' +
                        // The close button gets dynamically added here
                        // The notification badge gets dynamically added here
                    '</li>');

            // Create and append a close button if closeable
            if (this.options.closeable)
            {
                var $closeButton = $('<button class="otk-close-button small"></button>');
                $tab.append($closeButton);
            }

            return $tab;
        },

        _numberOfTabsChanged: function()
        {
            // Trigger that the number of tabs has changed
            this.element.trigger("numTabsChanged");
        },

        // Handle tab dragging / reordering
        _startDrag: function(self, $tab, ev)
        {
            var $tabContainer, handleMoveEvent, baseX, stopDrag,
                initialScrollX,
                animateTabSwap, hasDislodged = false;

            $tabContainer = $tab.closest('ul');

            initialScrollX = parseInt(self.element.css("left"));

            baseX = ev.pageX;

            animateTabSwap = function($otherTab, startLeftPx)
            {
                $otherTab.css('left', startLeftPx + 'px');
                $otherTab.animate({left: '0'}, constants.animationMs(), function()
                {
                    $otherTab.css('left', '');
                });
            };

            handleMoveEvent = function(ev)
            {
                var deltaX, $previousTab, currentScrollX,
                    $nextTab, previousTabWidth, nextTabWidth;

                deltaX = (ev.pageX - baseX);

                //
                // FIX THIS
                // This isn't perfect because as the carousel scrolls,
                // new mousemove events don't get generated. So the
                // dragged tab will be out of position until the next
                // mousemouse event happens. To reduce this effect
                // I've turned off scrolling animation while dragging.
                //
                // The tab carousel scrolls the tab content by changing the "left" style
                currentScrollX = parseInt(self.element.css("left"));
                deltaX += initialScrollX-currentScrollX;
                //
                //

                if (ev.which !== 1)
                {
                    // Not holding the LMB
                    stopDrag();
                    return;
                }

                for($previousTab = $tab.prev();
                    $previousTab.length;
                    $previousTab = $tab.prev())
                {
                    previousTabWidth = $previousTab.width();

                    if (deltaX > (-previousTabWidth / 2))
                    {
                        // We're done
                        break;
                    }
            
                    hasDislodged = true;
            
                    // Swap with the previous tab
                    $tab.insertBefore($previousTab);

                    deltaX += previousTabWidth;
                    baseX -= previousTabWidth;

                    animateTabSwap($previousTab, -($tab.width()));
                }

                for($nextTab = $tab.next();
                    $nextTab.length;
                    $nextTab = $tab.next())
                {
                    nextTabWidth = $nextTab.width();

                    if (deltaX < ((nextTabWidth / 2)+1)) // +1 to keep it from fighting with the code above if you're right on the edge
                    {
                        break;
                    }

                    hasDislodged = true;
            
                    $tab.insertAfter($nextTab);

                    deltaX -= nextTabWidth;
                    baseX += nextTabWidth;
            
                    animateTabSwap($nextTab, $tab.width());
                }

                if (hasDislodged || (Math.abs(deltaX) > constants.dislodgeThresholdPx()))
                {
                    if (!hasDislodged)
                    {
                        self.element.trigger("tabDragStart", [ $tab ]);
                    }

                    hasDislodged = true;

                    $tab.addClass('dragging')
                        .css({
                            'z-index': self.tabCount() + 2,
                            'opacity': '.7',
                            'left': deltaX + 'px'
                        });


                    // BE VERY CAREFUL ABOUT HOW YOU CONSUME THIS EVENT!!!!!
                    // IT IS CALLED EVERY TIME A TAB IS DRAGGED BY EVEN A SINGLE PIXEL
                    // AND IS CALLED REPEATEDLY AS THE TAB CONTINUES TO BE DRAGGED
                    self.element.trigger("tabDragged", [ $tab ]);
                }
            };

            stopDrag = function()
            {
                // Kill our z-index, top and position
                $tab.animate({'left': 0, 'opacity': 1}, constants.animationMs(), function()
                {
                    $tab.removeClass('dragging')
                        .css({
                            'left': '',
                            'opacity': ''
                        });


                    if (hasDislodged)
                    {
                        // Trigger that this tab has been dropped
                        self.element.trigger("tabDropped", [ $tab ]);
                    }
                });

                $tabContainer.off('mousemove', handleMoveEvent);
                $tabContainer.off('mouseleave', stopDrag);
                $('body').off('mouseup', stopDrag);

                // Restack everything
                self._stackElements();
            };

            $tabContainer.on('mousemove', handleMoveEvent);
            $tabContainer.on('mouseleave', stopDrag);
            $('body').on('mouseup', stopDrag);
        },

        // *** BEGIN PUBLIC INTERFACE ***

        // Returns the number of tab items in the otk-tabs component
        tabCount: function()
        {
            return this.allTabs().length;
        },

        // Selects the tab passed as a parameter
        selectTab: function($tab)
        {
            if ($tab.hasClass('current')) { return; }
        
            // Update the current tab
            $tab.siblings('.current').removeClass('current');
            $tab.addClass('current');

            // Clear the count badge if present since this tab has been selected
            if ($tab.has('.otk-count-badge').length)
            {
                var $badge = $tab.find('.otk-count-badge');
                $badge.otkCountBadge("clear");
            }

            // Trigger that the current tab has changed
            this.element.trigger("currentTabChanged", [ $tab ]);

            // Update ourselves on selection of a new tab
            this._update();
        },

        // Select a tab at the given index
        selectTabByIndex: function(index)
        {
            var $tab = this.tabByIndex(index);
            this.selectTab($tab);
        },

        // Returns the selected tab or null if no tab is selected
        selectedTab: function()
        {
            if (this.element.has('ul > li.current').length)
            {
                return this.element.find('ul > li.current');
            }
            else // JSLint flags this ("Unnecessary 'else' after disruption"). Why?
            {
                // There is no selected tab
                return null;
            }
        },

        // Returns the index of the selected tab or -1 if no tab is selected
        selectedTabIndex: function()
        {
            if (this.element.has('ul > li.current').length)
            {
                var $selectedTab = this.element.find('ul > li.current');
                return $selectedTab.index();
            }
            else // JSLint flags this ("Unnecessary 'else' after disruption"). Why?
            {
                // There is no selected tab
                return -1;
            }
        },

        // Returns the index of the specified tab or -1 if not found
        tabIndex: function($tab)
        {
            return $tab.index();
        },

        // Adds a standard tab
        // Returns the new tab element
        appendTab: function(title, subtitle)
        {
            // Create it
            var $tabList = this.element.find('ul'),
                $tab = this._newEmptyTab();

            this.setTabTitle($tab, title);
            this.setTabSubtitle($tab, subtitle);

            // Add it
            $tabList.append($tab);

            // Fade it in
            $tab.css({opacity: '0'});
            $tab.animate({opacity: '1'}, constants.appendAnimationMs(), function()
            {
                $tab.css('opacity', '');
            });

            // Number of tabs has changed
            this._numberOfTabsChanged();

            if (this.selectedTab() === null)
            {
                // There isn't a selected tab - select this one
                this.selectTab($tab);
            }

            this._update();

            return $tab;
        },

        // Close the specified tab
        closeTab: function($tab)
        {
            var self = this,
                $nextTab,
                closedTabWidth = $tab.width(),
                closedTabIndex = this.tabIndex($tab),
                selectedTabIndex = this.selectedTabIndex(),
                $selectedTab = this.selectedTab(),
                tabCount;

            // Remove the closed tab
            $nextTab = $tab.next();
            $tab.remove();

            // Animate any the tabs after the one being closed to the left
            for(;
                $nextTab.length;
                $nextTab = $nextTab.next())
            {
                $nextTab.stop().css({left: closedTabWidth + 'px'});
                $nextTab.animate({left: '0'}, constants.closeAnimationMs(), "swing", function()
                {
                    this.css({left: ''});
                }.bind($nextTab));
            }

            // Trigger that a tab has been closed
            self.element.trigger("tabClosed", [ $tab ]);

            // Number of tabs has changed
            self._numberOfTabsChanged();

            if (selectedTabIndex === closedTabIndex)
            {
                // If we closed the selected tab, elect a new one
                tabCount = self.tabCount();
                if (tabCount > 0)
                {
                    if (selectedTabIndex < tabCount)
                    {
                        self.selectTabByIndex(selectedTabIndex);
                    }
                    else if (selectedTabIndex > 0)
                    {
                        self.selectTabByIndex(selectedTabIndex-1);
                    }
                }
            }

            // Update ourselves
            self._update();
        },

        // Returns the tab at the specified index or null if the tab is not found
        tabByIndex: function(index)
        {
            var tabCount = this.tabCount(), $tabs, $tab, $badge;

            if ((tabCount === 0) || (index < 0) || (index >= tabCount)) { return null; }

            $tabs = this.allTabs();

            $tab = $($tabs[index]);

            return $tab;
        },

        // Returns the notification count for the specified tab
        notificationCount: function($tab)
        {
            var $badge = $tab.find(".otk-count-badge");
            if ($badge.length === 0) { return 0; }
            return $badge.otkCountBadge("value");
        },

        // Set the notification count for the specified tab
        setNotificationCount: function($tab, count)
        {
            var $badge;

            // Don't notify the selected tab
            if ($tab.hasClass('current')) { return; }

            // If we don't have a notification badge element create and append one here
            if ($tab.find(".otk-count-badge").length === 0)
            {
                // Create an append the count badge
                $badge = $.otkCountBadgeCreate('<div>', { value: 0, autoHide: true });
                $tab.append($badge);
            }
            else
            {
                $badge = $tab.find(".otk-count-badge");
            }

            $badge.otkCountBadge("setValue", count);

            // Signal that this tab has been notified (used by otk-tabs-carousel)
            if (count !== 0)
            {
                this.element.trigger("tabNotified", [ $tab ]);
            }
        },

        // Notifies the specified tab
        notifyTab: function($tab)
        {
            var $badge;

            // Don't notify the selected tab
            if ($tab.hasClass('current')) { return; }

            // If we don't have a notification badge element create and append one here
            if ($tab.find(".otk-count-badge").length === 0)
            {
                // Create an append the count badge
                $badge = $.otkCountBadgeCreate('<div>', { value: 0, autoHide: true });
                $tab.append($badge);
            }
            else
            {
                $badge = $tab.find(".otk-count-badge");
            }

            $badge.otkCountBadge("increment");

            // Signal that this tab has been notified (used by otk-tabs-carousel)
            this.element.trigger("tabNotified", [ $tab ]);
        },

        // Get the tab title
        tabTitle: function($tab)
        {
            return $tab.find('.title').text();
        },

        // Set the tab title
        setTabTitle: function($tab, title)
        {
            $tab.find('.title').text(title);
        },

        // Get the tab subtitle
        tabSubtitle: function($tab)
        {
            return $tab.find('.subtitle').text;
        },

        // Set the tab subtitle
        setTabSubtitle: function($tab, subtitle)
        {
            // No subtitles for short tabs
            if (this.options.short === true){ return; }

            if (subtitle)
            {
                // Make sure we have a subtitle element.
                // If not, create and append it now
                if ($tab.find('.subtitle').length === 0)
                {
                    $tab.find('.labels').append($('<span class="subtitle"></span>'));
                }
                $tab.find('.subtitle').text(subtitle);
            }
            else
            {
                // Remove the subtitle element so the title centers vertically
                if ($tab.find('.subtitle').length !== 0)
                {
                    $tab.find('.subtitle').remove();
                }
            }
        },

        // Gets the icon at position 1 (if one exists)
        tabIcon1: function($tab)
        {
            var $iconContainer = $tab.find('.icon1'),
                $icon = $iconContainer.children('*');
            return $icon;
        },

        // Applies an icon at position 1
        setTabIcon1: function($tab, $icon)
        {
            var $iconContainer;

            // No icons for short tabs
            if (this.options.short === true){ return; }

            // If we don't already have an icon here, create and append it now
            if ($tab.find('.icon1').length === 0)
            {
                $tab.find('.contents').prepend($('<div class="icon1"></div>'));
            }
            $iconContainer = $tab.find('.icon1');

            // If there is existing icon content get rid of it
            $iconContainer.empty();

            $iconContainer.append($icon);
        },

        // Removes the icon at position 1
        clearTabIcon1: function($tab)
        {
            if ($tab.find('.icon1').length !== 0)
            {
                $tab.find('.icon1').remove();
            }
        },

        // Gets the icon at position 2 (if one exists)
        tabIcon2: function($tab)
        {
            var $iconContainer = $tab.find('.icon2'),
                $icon = $iconContainer.children('*');
            return $icon;
        },

        // Applies an icon at position 2
        setTabIcon2: function($tab, $icon)
        {
            var $iconContainer;

            // No icons for short tabs
            if (this.options.short === true){ return; }

            // If we don't already have an icon here, create and append it now
            if ($tab.find('.icon2').length === 0)
            {
                if ($tab.find('.icon1').length !== 0)
                {
                    // It needs to go after icon1
                    $tab.find('.icon1').after($('<div class="icon2"></div>'));
                }
                else
                {
                    $tab.find('.contents').prepend($('<div class="icon2"></div>'));
                }
            }
            $iconContainer = $tab.find('.icon2');

            // If there is existing icon content get rid of it
            $iconContainer.empty();

            $iconContainer.append($icon);
        },

        // Removes the icon at position 2
        clearTabIcon2: function($tab)
        {
            if ($tab.find('.icon2').length !== 0)
            {
                $tab.find('.icon2').remove();
            }
        },

        // Returns a collection containing all <li> tab elements in the tab component
        allTabs: function()
        {
            return this.element.find('ul > li');
        },

        // Returns the actual visible width of the specified tab, adjusted for margins, freakyness, etc
        tabWidth: function($tab)
        {
            //return $tab.width();
            return $tab.outerWidth(false);
        },

        advanceTabFocus: function()
        {
            var index = this.selectedTabIndex(),
                tabCount = this.tabCount();
            ++index;
            if (index >= tabCount)
            {
                index = 0;
            }
            this.selectTabByIndex(index);
        },

        rewindTabFocus: function()
        {
            var index = this.selectedTabIndex(),
                tabCount = this.tabCount();
            --index;
            if (index < 0)
            {
                index = tabCount-1;
            }
            this.selectTabByIndex(index);
        },

    });

}(jQuery);

