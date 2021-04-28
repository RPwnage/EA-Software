 var dictionary;
 var locale = 'en-us';
 angular.module('origin-components')
     .run(function(LocFactory) {
         LocFactory.init('en-us', dictionary);
     })


 $.get('https://cms.x.origin.com/' + locale + '/content/components/translations/components.json', function(data) {
     dictionary = data;
 })