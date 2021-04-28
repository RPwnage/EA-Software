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
 /* jshint ignore:end */
/**
 * @ngdoc directive
 * @name origin-components.directives:originShellPopoutmenuItem
 * @restrict E
 * @element ANY
 * @scope
 * @description
 *
 * menu item
 * @param {LocalizedString} title text show on link
 * @param {DispositionEnumeration} disposition a link behavior
 * @param {ForEnumeration} for the behavior's context
 * @param {string} section The section of the app this button links to. Used for highlighting.
 * @param {Url} href the link it will try to open
 * @example
 * <example module="origin-components">
 *     <file name="index.html">
 *         <origin-shell-menu-item>
 *             <origin-shell-menu-item-link titlestr="Home" href="#">
 *                 <origin-shell-submenu-item title="Browse Games" section="store.search" href="#/store/freegames">
 *                 <origin-shell-popoutmenu-item title="Downloadable Content" section="store.search" href="#/home"></origin-shell-popoutmenu-item>
 *                 </origin-shell-submenu-item>
 *             </origin-shell-menu-item-link>
 *         </origin-shell-menu-item>
 *     </file>
 * </example>
 *
 */