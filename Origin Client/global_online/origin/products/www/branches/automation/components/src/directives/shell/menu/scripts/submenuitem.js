/* jshint ignore:start */
/**
 * Disposition map
 * @enum {string}
 */
var DispositionEnumeration = {
    "hide": "hide",
    "logout": "logout"
};

/**
 * For context map
 * @enum {string}
 */
var ForEnumeration = {
    "client": "client",
    "web": "web"
};

var MenuCategoryEnumeration = {
    "store" : "store",
    "browse" : "browse",
    "freegames" : "freegames",
    "categorypage" : "categorypage",
    "dealcenter" : "dealcenter",
    "originaccess" : "originaccess",
    "gamelibrary" : "gamelibrary",
    "settings" : "settings",
    "search" : "search",
    "profile" : "profile",
    "home": "home",
    "homeloggedin" : "homeloggedin",
    "error" : "error"
};
/* jshint ignore:end */
/**
 * @ngdoc directive
 * @name origin-components.directives:originShellSubmenuItem
 * @restrict E
 * @element ANY
 * @scope
 * @description
 *
 * menu item
 * @param {LocalizedString} title-str text show on link
 * @param {DispositionEnumeration} disposition a link behavior
 * @param {ForEnumeration} for the behavior's context
 * @param {Url} href the link it will try to open
 * @param {MenuCategoryEnumeration} section section the menu item belongs to. Must be set to enable menu hightlighting
 * @example
 * <example module="origin-components">
 *     <file name="index.html">
 *         <origin-shell-menu-item>
 *             <origin-shell-menu-item-link title-str="Home" href="#">
 *                 <origin-shell-submenu-item title="Browse Games" section="browse" href="store/freegames">
 *                 </origin-shell-submenu-item>
 *             </origin-shell-menu-item-link>
 *         </origin-shell-menu-item>
 *     </file>
 * </example>
 *
 */
