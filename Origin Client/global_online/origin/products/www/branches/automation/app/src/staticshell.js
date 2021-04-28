(function() {
    'use strict';
    var staticShell = OriginKernel.configs['dist/configs/app-static-shell.json'],
        hideStaticShell = window.location.href.indexOf('hidestaticshell=true') > -1;
    if (staticShell && !hideStaticShell) {
        var el = document.querySelector('.staticshell'),
            localizedShell = staticShell[originLocaleApi.getLocale()];
        if (el && localizedShell) {
            el.innerHTML = localizedShell;
        }
    }
}());