> Origin Locale



## Getting Started

This library is used to read the application URL and extract the locale information from it.

for example:

https://www.origin.com/fr-fr/#/home

the library will extract the "fr-fr" string and provide some helpful methods for creating new urls and some common transformations and mapping required to match marketing language coding.


## Building the library

To build the bower dependency, change to this project's directory and run the following commands

```shell
cd tools/bower/origin-locale
npm install
bower install
grunt
```

this will create a bower library for use in your project. Since this is an internal component, you can bower link it to your application layer

```shell
cd tools/bower/origin-locale
sudo bower link
cd path/to/app
bower link origin-locale
```

## Using the library

in your application's index file near the beginning of the javascript includes, include this script. It will add the library to your globals so other parts of the application can be initialized with it.

```
<script src="bower_components/origin-locale/dist/origin-locale.js" type="text/javascript"></script>
```

In your javascript contexts, you'll have access to a global OriginLocale object.

```javascript
var locale = OriginLocale.parse("https://www.origin.com/en-gb/#/store/pdp/DR%3A225064100/info", 'en-us');

methods - you can have a look at the tests for more use cases
expect(locale.getLocale())
    .toBe('en-gb');
expect(locale.getCasedLocale())
    .toBe('en_GB');
expect(locale.getLanguageCode())
    .toBe('en');
expect(locale.getCountryCode())
    .toBe('gb');
expect(locale.getThreeLetterCountryCode())
    .toBe('gbr');
expect(locale.getThreeLetterLanguageCode())
    .toBe('eng');
expect(locale.getLocaleAlternates().length)
    .toBeGreaterThan(1);
expect(locale.getCurrencyCode())
    .toBe('gbp');
expect(locale.createUrl('https://www.origin.com/en-gb#/store/pdp/DR%3A225064100/info', 'fr'))
    .toBe('https://www.origin.com/fr-gb#/store/pdp/DR%3A225064100/info');
```

## Technical Documentation on Locale

https://confluence.ea.com/display/EBI/Managing+Locale+State+in+the+Application