 var dictionary;
 var locale = 'en-us';
 angular.module('origin-components')
     .run(function(LocFactory, AuthFactory, COMPONENTSCONFIG) {
         COMPONENTSCONFIG.overrides = {
             env: 'dev',
             cmsstage:'preview'
         };
         Origin.utils.replaceTemplatedValuesInConfig(COMPONENTSCONFIG);

         LocFactory.init();
         AuthFactory.init({
             "testForLoginUrl": "https://accounts.int.ea.com/connect/auth?client_id=ORIGIN_PC&response_type=token&redirect_uri=nucleus:rest&prompt=none",
             "loginUrl": "https://accounts.int.ea.com/connect/auth?response_type=token&scope=basic.identity+basic.persona+signin+offline&client_id=ORIGIN_PC&locale={loginlocale}",
             "logoutUrl": "https://accounts.int.ea.com/connect/logout?client_id=ORIGIN_PC",
             "logInOutHtml": "/views/login.html"
         });
     })
