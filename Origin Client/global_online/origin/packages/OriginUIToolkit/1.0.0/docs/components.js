
"use strict";


$(document).ready(function()
{
    if (!navigator.userAgent.match(/webkit/i))
    {
        alert('This page requires a Webkit-based browser to function properly');
    }

    // Create a tooltip component

    // Create a callout component
    {
        $.otkCalloutCreate(
            {
                newTitle: "NEW",
                title: "VOICE CHAT AND GROUPS",
                content: [
                    {imageUrl:'images/icon-friend.png', text:'Call your Origin friends directly on their computers'},
                    {imageUrl:'images/icon-phone.png', text:'Group allow you to chat with a bunch of friends or guild mates all at once.'}
                ],
                bindedElement: $('#callout-parent1')
            });

        $.otkCalloutCreate(
            {
                newTitle: "NEW",
                title: "VOICE CHAT AND GROUPS",
                content: [
                    {text:'Call your Origin friends directly on their computers'},
                    {imageUrl:'images/icon-phone.png', text:'Group allow you to chat '}
                ],
                bindedElement: $('#callout-parent2')
            });

        $.otkCalloutCreate(
            {
                newTitle: "NEW",
                title: "VOICE CHAT AND GROUPS",
                content: [
                    {text:'Group allow you to chat with a bunch of friends or guild mates all at once.'}
                ],
                bindedElement: $('#callout-parent3')
            });

        $.otkCalloutCreate(
            {
                title: "VOICE CHAT AND GROUPS",
                bindedElement: $('#callout-parent4')
            });

        $.otkCalloutCreate(
            {
                newTitle: "NEW WITH A REALLY LONG TITLE AND NEW LOOK",
                title: "VOICE CHAT AND GROUPS AND A REALLY LONG TITLE WITH NEW LOOK",
                content: [
                    {text:'Call your Origin friends directly on their computers', imageUrl:'images/icon-friend.png'}
                ],
                bindedElement: $('#callout-parent5')
            });

        $('#premade-callout').otkCallout(
            {
                newTitle: "Modified Premade",
                title: "Modified premade description with an offset of -20,20",
                xOffset: -20,
                yOffset: 20,
                content: [
                    {text:'this was modified'},
                    {text:'line 2 modified'}
                ],
                bindedElement: $('#callout-parent6')
            });
    }

    // Create progress bar components
    {

        var byteFunc = function(bytes)
            {
                var KB = 1024,
                    MB = 1048576,
                    GB = 1073741824,
                    TB = 1099511627776;
                if (bytes < KB)
                {
                    return ("%1 B").replace('%1', bytes.toFixed(2));
                }
                else if (bytes < MB)
                {
                    return ("%1 KB").replace('%1', (bytes / KB).toFixed(2));
                }
                else if (bytes < GB)
                {
                    return ("%1 MB").replace('%1', (bytes / MB).toFixed(2));
                }
                else if (bytes < TB)
                {
                    return ("%1 GB").replace('%1', (bytes / GB).toFixed(2));
                }
                else
                {
                    return ("%1 TB").replace('%1', (bytes / TB).toFixed(2));
                }
            };


        var downloadProgressStrings =
            {
                timeRemaining: '%1 Remaining',
                percentNotation: '%1%',
                seconds: 'sec'
            }
        var $progressbarDetailed = $.otkProgressBarDetailedCreate($('#detailed-progress'),
            {
                title: "a long title and most likely has a soundtrack with a long name also",
                subtitle: "Painter's Nightmare Painter's Nightmare Painter's Nightmare Painter's Nightmare",
                bytesDownloaded: 100,
                totalFileSize: 100,
                bytesPerSecond: 100,
                progress: 0.1,
                secondsRemaining: '34:34:34',
                formatBytesFunction: byteFunc,
                strings: downloadProgressStrings
            });
        $progressbarDetailed.otkProgressBarDetailed("progressbar").otkProgressBar(
            {
                progress: .65,
                formatBytesFunction: byteFunc,
                state: 'State-Active'
            });


        $('#premade-progress').otkProgressBarDetailed(
            {
                title: "Edited title",
                subtitle: 'Edited sub title',
                //bytesDownloaded: 210,
                //totalFileSize: 210,
                //bytesPerSecond: 210,
                formatBytesFunction: byteFunc,
                strings: downloadProgressStrings
            });
        $('#premade-progress').otkProgressBarDetailed("progressbar").otkProgressBar(
            {
                progress: .30,
                formatBytesFunction: byteFunc,
                state: 'State-Indeterminate'
            });

        $.otkProgressBarCreate($('#progressbar-active'),
            {
                progress: .75,
                state: 'State-Active'
            });

        $.otkProgressBarCreate($('#progressbar-indeterminate'),
            {
                state: 'State-Indeterminate'
            });

        $.otkProgressBarCreate($('#progressbar-complete'),
            {
                progress: 1,
                state: 'State-Complete',
                text: 'Update Complete'
            });

        $.otkProgressBarCreate($('#progressbar-paused'),
            {
                progress: 0.6,
                state: 'State-Paused'
            });

        var $greenprogress = $.otkProgressBarCreate($('#progressbar-green'),
            {
                progress: 0,
                state: 'State-Active',
                color: 'Color-Blue'
            });
        $greenprogress.otkProgressBar("addProgressFlag", "playableAt", 0.1, "Playable", "Color-Green");

        $.otkProgressBarCreate($('#progressbar-green-indeterminate'),
            {
                progress: .75,
                state: 'State-Indeterminate',
                color: 'Color-Green'
            });
    }

    // Create a radio rating component
    {
        var $radiorating = $.otkRadioRatingCreate($('#radio-rating'),
            {
                lessLabel: "Less Likely and hate it rawr",
                moreLabel: "More Likely yay love it la la la",
                name: 'radio-rating',
                count: 11
            });

        var $radiorating = $.otkRadioRatingCreate($('#radio-rating1'),
            {
                lessLabel: "Less Likely and hate it rawr",
                moreLabel: "More Likely yay love it la la la",
                count: 11,
                name: 'radio-rating1',
                widthInherit: true
            });
    }

    // jQuery and jQuery UI versions
    $('body').prepend('<span class="small-print">jQuery version: ' + $.fn.jquery + ', jQuery UI version: ' + $.ui.version + '</span>');

    //
    // Create some contact items
    //
    {
        var $contact = $.otkContactCreate('<li>',
            {
                nicknameLabel: "CarlSpoon",
                realnameLabel: "Bob Loblaw",
                presenceLabel: "Online",
                presenceIndicator: "online",
                avatarPath: "https://lh3.googleusercontent.com/-l6hhlM_4x1A/AAAAAAAAAAI/AAAAAAAAAAA/6NTKD3n-Wug/s48-c-k/photo.jpg"
            });
        $('#contact-list').append($contact);
    }
    {
        var $contact = $.otkContactCreate('<li>',
            {
                nicknameLabel: "CrashOverride",
                realnameLabel: "Dade Murphy",
                presenceIndicator: "unavailable",
                avatarPath: "https://lh3.googleusercontent.com/-RcvtfuoErt8/AAAAAAAAAAI/AAAAAAAAAAA/zo6M0EUjpys/s48-c-k/photo.jpg"
            });
        $contact.otkContact("actionLink", "Accept", "#accept");
        $contact.otkContact("actionLink", "Reject", "#reject");
        // You can remove the action link(s) like this: $contact.eaContact("removeActionLinks");
        $('#contact-list').append($contact);
    }
    {
        var $contact = $.otkContactCreate('<li>',
            {
                nicknameLabel: "AcidBurn",
                realnameLabel: "Kate Libby",
                presenceIndicator: "unavailable",
                avatarPath: "https://lh3.googleusercontent.com/-c3Wq0wg8Q80/AAAAAAAAAAI/AAAAAAAAAAA/10h-AAHXFig/s48-c-k/photo.jpg"
            });
        $contact.otkContact("actionLink", "Cancel Friend Request", "#revoke");
        $('#contact-list').append($contact);
    }
    {
        var $contact = $.otkContactCreate('<li>',
            {
                nicknameLabel: "CerealKiller",
                realnameLabel: "Emmanuel Goldstein",
                presenceLabel: "Dragon Age(TM): Origins",
                presenceIndicator: "ingame",
                avatarPath: "https://lh3.googleusercontent.com/-70JuwFWyc98/AAAAAAAAAAI/AAAAAAAAAAA/QyI64VUlhJY/s48-c-k/photo.jpg"
            });
        $('#contact-list').append($contact);
    }
    {
        var $contact = $.otkContactCreate('<li>',
            {
                nicknameLabel: "OverflowTestOverflowTestOverflowTest",
                realnameLabel: "OverflowTestOverflowTestOverflowTest",
                presenceLabel: "NoAvatarNoAvatarNoAvatarNoAvatarNoAvatar",
                presenceIndicator: "online",
            });
        $('#contact-list').append($contact);
    }

    //
    // Create contact group items
    //

    // A selectable group item
    {
        var $group = $.otkContactGroupCreate('<section>',
            {
                label: "Offline",
                count: 0
            });

        $('#contact-group-item-select').append($group);
    }

    // Create a group item with some otk-contact items under it
    {
        var $group = $.otkContactGroupCreate('<section>',
            {
                label: "Online",
                count: 2
            });

        // Specify the toggle target as the contact list
        $group.attr('data-toggle-target', '#groupTarget');

        // Add the group to the page
        $('#contact-group-list').append($group);

        // Create a list of contacts for the group
        {
            var $contactList = $('<ul class="unstyled" id="groupTarget"></ul>');
            {
                var $contact = $.otkContactCreate('<li>',
                    {
                        nicknameLabel: "CerealKiller",
                        realnameLabel: "Emmanuel Goldstein",
                        presenceLabel: "Online",
                        presenceIndicator: "online",
                        avatarPath: "https://lh3.googleusercontent.com/-70JuwFWyc98/AAAAAAAAAAI/AAAAAAAAAAA/QyI64VUlhJY/s48-c-k/photo.jpg"
                    });
                $contactList.append($contact);
            }
            {
                var $contact = $.otkContactCreate('<li>',
                    {
                        nicknameLabel: "CrashOverride",
                        realnameLabel: "Dade Murphy",
                        presenceLabel: "Online",
                        presenceIndicator: "online",
                        avatarPath: "https://lh3.googleusercontent.com/-RcvtfuoErt8/AAAAAAAAAAI/AAAAAAAAAAA/zo6M0EUjpys/s48-c-k/photo.jpg"
                    });
                $contactList.append($contact);
            }

            // Add the contact list to the page, right under the group
            $('#contact-group-list').append($contactList);

        }
    }

    // Create some tabs
    {
        var $tabs = $.otkTabsCreate('<div>',
            {
                squareFirstTab: false,
                closeable: false,
                draggable: true,
                short: false
            });
        $tabs.otkTabs("appendTab", "Tab One", "");
        $tabs.otkTabs("appendTab", "Tab Two", "");
        $tabs.otkTabs("appendTab", "Tab Three", "");
        $tabs.otkTabs("appendTab", "Tab Four", "I have a subtitle");
        $('#tabArea1').append($tabs);
    }
    {
        var $tabs = $.otkTabsCreate('<div>',
            {
                squareFirstTab: true,
                closeable: false,
                draggable: false,
                short: false
            });
        $tabs.otkTabs("appendTab", "Tab One", "");
        $tabs.otkTabs("appendTab", "Tab Two", "");
        $tabs.otkTabs("appendTab", "Tab Three", "");
        $tabs.otkTabs("appendTab", "Tab Four", "I have a subtitle");
        $('#tabArea2').append($tabs);
    }
    {
        var $tabs = $.otkTabsCreate('<div>',
            {
                squareFirstTab: false,
                closeable: true,
                draggable: true,
                short: false
            });
        $tabs.otkTabs("appendTab", "Tab One", "");
        $tabs.otkTabs("appendTab", "Tab Two", "");
        $tabs.otkTabs("appendTab", "Tab Three", "");
        $tabs.otkTabs("appendTab", "Tab Four", "I have a subtitle");
        $('#tabArea3').append($tabs);
    }
    {
        var $tabs = $.otkTabsCreate('<div>',
            {
                squareFirstTab: false,
                closeable: true,
                draggable: true,
                short: true
            });
        $tabs.otkTabs("appendTab", "TAB ONE", "");
        $tabs.otkTabs("appendTab", "TAB TWO", "");
        $tabs.otkTabs("appendTab", "TAB THREE", "");
        $tabs.otkTabs("appendTab", "TAB FOUR", "");
        $('#tabArea4').append($tabs);
    }
    {
        var $tabs = $.otkTabsCreate('<div>',
            {
                squareFirstTab: true,
                closeable: true,
                draggable: true,
                short: true
            });
        $tabs.otkTabs("appendTab", "TAB ONE", "");
        $tabs.otkTabs("appendTab", "TAB TWO", "");
        $tabs.otkTabs("appendTab", "TAB THREE", "");
        $tabs.otkTabs("appendTab", "TAB FOUR", "");
        $('#tabArea4a').append($tabs);
    }
    {
        var $tabs = $.otkTabsCreate('<div>',
            {
                squareFirstTab: false,
                closeable: true,
                draggable: true,
                short: false
            });
        $tabs.otkTabs("appendTab", "Tab One", "");
        $tabs.otkTabs("appendTab", "Tab Two", "");
        $tabs.otkTabs("appendTab", "Tab Three", "");
        $tabs.otkTabs("appendTab", "Tab Four", "Tab with Subtitle");
        $('#tabArea5').append($tabs);
    }

    // Create some sliders
    {
        var $slider = $.otkSliderCreate('<div>',
            {
                value: .5,
                pointerDirection: "none"
            });
        $('#slider-test').append($slider);
    }
    {
        var $slider = $.otkSliderCreate('<div>',
            {
                value: .5,
                pointerDirection: "up"
            });
        $('#slider-test').append($slider);
    }
    {
        var $slider = $.otkSliderCreate('<div>',
            {
                value: .5,
                pointerDirection: "down"
            });
        $('#slider-test').append($slider);
    }

    // Attach the component code to our sample HTML contact so it's interactive
    $('#contact-html-test').otkContact();

    // Attach the component code to our sample HTML contact group so it's interactive
    $('#contact-group-html-test').otkContactGroup();

    // Attach the component code to our sample HTML contact group so it's interactive
    $('#count-badge1').otkCountBadge({ value: 1, autoHide: false });
    $('#count-badge2').otkCountBadge({ value: 1, autoHide: true });

    // Attach the component code to our tabs so it's interactive
    $('.otk-tabs').otkTabs();

    // Attach the component code to our tabs-carousel so it's interactive
    $('.otk-tabs-carousel').otkTabsCarousel();

    // Attach the component code to our sliders so they are interactive
    $('.otk-slider').otkSlider();

    // Create an "info" style otk-empty-list-callout
    {
        var $callout = $.otkEmptyListCalloutCreate('<section>',
            {
                titleLabel: "Gaming is better with Friends",
                descriptionLabel: "Build your Friends List so you can see what your friends are playing and chat with them from your game.",
                buttonLabel: "Find Friends",
                style: "info-style"
            });
        $('#empty-list-callout-info').append($callout);
    }

    // Create an "etched" style otk-empty-list-callout
    {
        var $callout = $.otkEmptyListCalloutCreate('<section>',
            {
                titleLabel: "Gaming is better with Friends",
                descriptionLabel: "Build your Friends List so you can see what your friends are playing and chat with them from your game.",
                buttonLabel: "Find Friends",
                style: "etched-style"
            });
        $('#empty-list-callout-etched').append($callout);
    }

    // Create an "plain" style otk-empty-list-callout
    {
        var $callout = $.otkEmptyListCalloutCreate('<section>',
            {
                titleLabel: "Gaming is better with Friends",
                descriptionLabel: "Build your Friends List so you can see what your friends are playing and chat with them from your game.",
                buttonLabel: "Find Friends",
                style: "plain-style"
            });
        $('#empty-list-callout-plain').append($callout);
    }

    // Create an otk-tabs-carousel
    {
        var $carousel = $.otkTabsCarouselCreate('<section>', {});
        $('#tabs-carousel-test').append($carousel);

        // Create a tab control to go inside the carousel
        var $tabs = $.otkTabsCreate('<div>',
            {
                squareFirstTab: false,
                closeable: true,
                draggable: true,
                short: false
            });
        $tabs.otkTabs("appendTab", "Tab One", "");
        $tabs.otkTabs("appendTab", "Tab Two", "");
        $tabs.otkTabs("appendTab", "Tab Three", "");
        $tabs.otkTabs("appendTab", "Tab Four", "");
        $tabs.otkTabs("appendTab", "Tab Five", "");
        $tabs.otkTabs("appendTab", "Tab Six", "");

        // Add the tabs to the carousel
        $carousel.otkTabsCarousel("setTabs", $tabs);
    }

    // Add a custom glyph to the first tab of the tab carousel
    {
        var $carousel = $('#tabs-carousel-test2'),
            $tabs = $carousel.otkTabsCarousel("tabs"),
            $tab = $tabs.otkTabs("tabByIndex", 0),
            $glyph = $.otkCustomGlyphCreate('<span>',
            {
                maskImageURL: "images/icon-mask.png",
                glyphColor: "#444444",
                shadowColor: "white",
                shadowOffsetX: 0,
                shadowOffsetY: 1,
                shadowOpacity: 1,
                autoSize: true
            });

            $tabs.otkTabs("setTabIcon1", $tab, $glyph);
    }

    // Create some custom glyphs
    {
        var $glyph = $.otkCustomGlyphCreate('<span>',
            {
                maskImageURL: "images/icon-mask.png",
                glyphColor: "#ffffff",
                shadowColor: "#000000",
                shadowOffsetX: 0,
                shadowOffsetY: 1,
                shadowOpacity: .5,
                autoSize: true
            });
        $('#custom-glyph-container').append($('<h5>Drop Shadow</h5>'));
        $('#custom-glyph-container').append($glyph);
    }
    {
        var $glyph = $.otkCustomGlyphCreate('<span>',
            {
                maskImageURL: "images/icon-mask.png",
                glyphColor: "#444444",
                shadowColor: "#ffffff",
                shadowOffsetX: 0,
                shadowOffsetY: 1,
                shadowOpacity: 1,
                autoSize: true
            });
        $('#custom-glyph-container').append($('<h5>"Etched" Effect</h5>'));
        $('#custom-glyph-container').append($glyph);
    }
    {
        var $glyph = $.otkCustomGlyphCreate('<span>',
            {
                maskImageURL: "images/icon-mask.png",
                glyphColor: "#444444",
                shadowOpacity: 0,
                autoSize: true
            });
        $('#custom-glyph-container').append($('<h5>No drop shadow effect</h5>'));
        $('#custom-glyph-container').append($glyph);
    }
    {
        var $glyphr = $.otkCustomGlyphCreate('<span>',
            {
                maskImageURL: "images/icon-mask.png",
                glyphColor: "Red",
                shadowColor: "#ffffff",
                shadowOffsetX: 0,
                shadowOffsetY: 1,
                shadowOpacity: 1,
                autoSize: false
            });
        var $glyphg = $.otkCustomGlyphCreate('<span>',
            {
                maskImageURL: "images/icon-mask.png",
                glyphColor: "Green",
                shadowColor: "#ffffff",
                shadowOffsetX: 0,
                shadowOffsetY: 1,
                shadowOpacity: 1,
                autoSize: false
            });
        var $glyphb = $.otkCustomGlyphCreate('<span>',
            {
                maskImageURL: "images/icon-mask.png",
                glyphColor: "Blue",
                shadowColor: "#ffffff",
                shadowOffsetX: 0,
                shadowOffsetY: 1,
                shadowOpacity: 1,
                autoSize: false
            });
        $('#custom-glyph-container').append($('<h5>Customized Colors</h5>'));
        $('#custom-glyph-container').append($glyphr);
        $('#custom-glyph-container').append($glyphg);
        $('#custom-glyph-container').append($glyphb);
    }

    // Level indicators (mini and normal)
    {
        var $miniLevel = $.otkMiniLevelIndicatorCreate('<div>',
            {
                numSegments: 6,
                orientation: "vertical"
            });
        $('#mini-level-indicator-test').append($miniLevel);

        var $miniLevelH = $.otkMiniLevelIndicatorCreate('<div>',
            {
                numSegments: 6,
                orientation: "horizontal"
            });
        $('#mini-level-indicator-test-h').append($miniLevelH);

        var $level = $.otkLevelIndicatorCreate('<div>',
            {
            });
        $('#level-indicator-test').append($level);

        var v=0;
        setInterval(function()
        {
            v -= .1;
            if (v < -.1){ v = Math.random(); }
            $miniLevel.otkMiniLevelIndicator("setValue", v);
            $miniLevelH.otkMiniLevelIndicator("setValue", v);
            $level.otkLevelIndicator("setValue", v);
        }, 50);
    }

    //add contact items for otk-medium-scrollbar
    var avatarList = [

        "https://lh3.googleusercontent.com/-l6hhlM_4x1A/AAAAAAAAAAI/AAAAAAAAAAA/6NTKD3n-Wug/s48-c-k/photo.jpg",
        "https://lh3.googleusercontent.com/-RcvtfuoErt8/AAAAAAAAAAI/AAAAAAAAAAA/zo6M0EUjpys/s48-c-k/photo.jpg",
        "https://lh3.googleusercontent.com/-c3Wq0wg8Q80/AAAAAAAAAAI/AAAAAAAAAAA/10h-AAHXFig/s48-c-k/photo.jpg",
        "https://lh3.googleusercontent.com/-70JuwFWyc98/AAAAAAAAAAI/AAAAAAAAAAA/QyI64VUlhJY/s48-c-k/photo.jpg"

    ];
    //crate 30 random users
    var max = 30;
    for(var i=0;i<max;i++){
        var itemStr= "<p>item " + i+"<p/>" ;
        var aName = "User "+i;
        var rand = Math.floor(Math.random()*4);
        var aPath = avatarList[rand]
        var $contact = $.otkContactCreate('<div>',
        {
            nicknameLabel: aName,
            realnameLabel: aName,
            presenceLabel: "Here!",
            presenceIndicator: "online",
            avatarPath: aPath
        });
        $contact.otkContact("actionLink", "Accept", "#accept");
        $contact.otkContact("actionLink", "Reject", "#reject");
        $("#contact-pane").find('.content').append($contact);
    }
    //end add contact items for otk-medium-scrollbar

});

$('body').on('click', '#btn-progressbar-green', function()
{
    var value = 0,
        interval;

    interval = window.setInterval(function() {
        if(value > 0.2)
        {
            $('#progressbar-green').otkProgressBar('toggleColorProgressFlag', "playableAt", false);
        }
        else if(value > 0.1)
        {
            $('#progressbar-green').otkProgressBar('toggleColorProgressFlag', "playableAt", true);
        }

        if(value > 1.0)
        {
            window.clearInterval(interval);
        }
        value += .003;
        $('#progressbar-green').otkProgressBar({progress: value});
        $('#progressbar-green').otkProgressBar('update');
    }, 110);
});

$('body').on('click', '.otk-contact', function()
{
    var $item = $(this);
    $item.toggleClass("selected");
});

$('body').on('click', '#otk-expand-collapse-indicator', function()
{
    var $item = $(this);
    $item.toggleClass("expanded");
});

$('body').on('click', 'section#contact-group-item-select section.otk-contact-group', function()
{
    var $item = $(this);
    $item.toggleClass("selected");
});

$('body').on('click', '.otk-hyperlink[href="#hyperlink"]', function()
{
    alert("Clicked on a hyperlink");
});

$('body').on('click', '.otk-contact .otk-hyperlink[href="#accept"]', function()
{
    alert("Clicked Accept");
});

$('body').on('click', '.otk-contact .otk-hyperlink[href="#reject"]', function()
{
    alert("Clicked Reject");
});

$('body').on('click', '.otk-contact .otk-hyperlink[href="#revoke"]', function()
{
    alert("Clicked on Cancel Friend Request");
});

// Close tabs when the tab's close button is clicked
$('body').on('tabCloseClicked', '.otk-tabs', function(e, $tab)
{
    $(this).otkTabs("closeTab", $tab);
});

// Tab selection changed
$('body').on('currentTabChanged', '.otk-tabs', function(e, $tab)
{
    console.log("Current tab changed");
});

// Closed a tab
$('body').on('tabClosed', '.otk-tabs', function(e, $tab)
{
    console.log("Tab closed");
});

// Notified a tab
$('body').on('tabNotified', '.otk-tabs', function(e, $tab)
{
    console.log("Tab notified");
});

// Number of tabs changed
$('body').on('numTabsChanged', '.otk-tabs', function(e)
{
    console.log("Number of tabs changed");
});

// Tab drag start
$('body').on('tabDragStart', function(e)
{
    console.log("A tab has started drag");
});

// Tab dropped
$('body').on('tabDropped', function(e)
{
    console.log("A tab was dropped");
});

// Count Badge controls
$('body').on('click', '#increment-count-badge', function(e)
{
    $('#count-badge1').otkCountBadge("increment");
    $('#count-badge2').otkCountBadge("increment");
});
$('body').on('click', '#clear-count-badge', function(e)
{
    $('#count-badge1').otkCountBadge("clear");
    $('#count-badge2').otkCountBadge("clear");
});

// Append tab controls
$('body').on('click', '#append-tab', function(e)
{
    var $tabComponent = $('#tabArea3').find('.otk-tabs');

    $tabComponent.otkTabs("appendTab", "New Tab", "", "#");
});
$('body').on('click', '#append-tab-select', function(e)
{
    var $tabComponent = $('#tabArea3').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("appendTab", "New Tab", "", "#");

    $tabComponent.otkTabs("selectTab", $tab);
});

// Notify tab controls
$('body').on('click', '#increment-tab-1', function(e)
{
    var $tabComponent = $('#tabArea3').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 0);

    $tabComponent.otkTabs("notifyTab", $tab);
});
$('body').on('click', '#increment-tab-2', function(e)
{
    var $tabComponent = $('#tabArea3').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 1);

    $tabComponent.otkTabs("notifyTab", $tab);
});
$('body').on('click', '#increment-tab-3', function(e)
{
    var $tabComponent = $('#tabArea3').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 2);

    $tabComponent.otkTabs("notifyTab", $tab);
});
$('body').on('click', '#increment-tab-4', function(e)
{
    var $tabComponent = $('#tabArea3').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 3);

    $tabComponent.otkTabs("notifyTab", $tab);
});

// Tab icon controls
$('body').on('click', '#set-icon-1-tab-1', function(e)
{
    var $tabComponent = $('#tabArea5').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 0);

    $tabComponent.otkTabs("setTabIcon1", $tab, $("<img src='images/voip-tab-icon.png'></img>"));
});
$('body').on('click', '#set-icon-2-tab-1', function(e)
{
    var $tabComponent = $('#tabArea5').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 0);

    $tabComponent.otkTabs("setTabIcon2", $tab, $('<div class="otk-presence-indicator inset" data-presence="online"></div>'));
});
$('body').on('click', '#clear-icon-1-tab-1', function(e)
{
    var $tabComponent = $('#tabArea5').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 0);

    $tabComponent.otkTabs("clearTabIcon1", $tab);
});
$('body').on('click', '#clear-icon-2-tab-1', function(e)
{
    var $tabComponent = $('#tabArea5').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 0);

    $tabComponent.otkTabs("clearTabIcon2", $tab);
});

$('body').on('click', '#set-icon-1-tab-2', function(e)
{
    var $tabComponent = $('#tabArea5').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 1);

    $tabComponent.otkTabs("setTabIcon1", $tab, $("<img src='images/voip-tab-icon.png'></img>"));
});
$('body').on('click', '#set-icon-2-tab-2', function(e)
{
    var $tabComponent = $('#tabArea5').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 1);

    $tabComponent.otkTabs("setTabIcon2", $tab, $('<div class="otk-presence-indicator inset" data-presence="online"></div>'));
});
$('body').on('click', '#clear-icon-1-tab-2', function(e)
{
    var $tabComponent = $('#tabArea5').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 1);

    $tabComponent.otkTabs("clearTabIcon1", $tab);
});
$('body').on('click', '#clear-icon-2-tab-2', function(e)
{
    var $tabComponent = $('#tabArea5').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 1);

    $tabComponent.otkTabs("clearTabIcon2", $tab);
});

$('body').on('click', '#set-icon-1-tab-3', function(e)
{
    var $tabComponent = $('#tabArea5').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 2);

    $tabComponent.otkTabs("setTabIcon1", $tab, $("<img src='images/voip-tab-icon.png'></img>"));
});
$('body').on('click', '#set-icon-2-tab-3', function(e)
{
    var $tabComponent = $('#tabArea5').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 2);

    $tabComponent.otkTabs("setTabIcon2", $tab, $('<div class="otk-presence-indicator inset" data-presence="online"></div>'));
});
$('body').on('click', '#clear-icon-1-tab-3', function(e)
{
    var $tabComponent = $('#tabArea5').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 2);

    $tabComponent.otkTabs("clearTabIcon1", $tab);
});
$('body').on('click', '#clear-icon-2-tab-3', function(e)
{
    var $tabComponent = $('#tabArea5').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 2);

    $tabComponent.otkTabs("clearTabIcon2", $tab);
});

$('body').on('click', '#set-icon-1-tab-4', function(e)
{
    var $tabComponent = $('#tabArea5').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 3);

    $tabComponent.otkTabs("setTabIcon1", $tab, $("<img src='images/voip-tab-icon.png'></img>"));
});
$('body').on('click', '#set-icon-2-tab-4', function(e)
{
    var $tabComponent = $('#tabArea5').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 3);

    $tabComponent.otkTabs("setTabIcon2", $tab, $('<div class="otk-presence-indicator inset" data-presence="online"></div>'));
});
$('body').on('click', '#clear-icon-1-tab-4', function(e)
{
    var $tabComponent = $('#tabArea5').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 3);

    $tabComponent.otkTabs("clearTabIcon1", $tab);
});
$('body').on('click', '#clear-icon-2-tab-4', function(e)
{
    var $tabComponent = $('#tabArea5').find('.otk-tabs'),
        $tab = $tabComponent.otkTabs("tabByIndex", 3);

    $tabComponent.otkTabs("clearTabIcon2", $tab);
});

// Slider has been changed
$('body').on('valueChanged', '.otk-slider', function(e, value)
{
    console.log("Slider value = " + parseInt(value*100) + "%");
});

// Slider is being changed
$('body').on('valueChanging', '.otk-slider', function(e, value)
{
//    console.log("Slider value = " + parseInt(value*100) + "%");
});

$('body').on('click', '#rating-button', function(e)
{
     alert($('#radio-rating1').otkRadioRating("value"));
});


$('body').on('click', '.otk-toggle-pressed', function(e)
{
    $(this).toggleClass('pressed');
});
