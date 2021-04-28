> Origin Component Packager



## Getting Started
This plugin requires Grunt `~0.4.0`

If you haven't used [Grunt](http://gruntjs.com/) before, be sure to check out the [Getting Started](http://gruntjs.com/getting-started) guide, as it explains how to create a [Gruntfile](http://gruntjs.com/sample-gruntfile) as well as install and use Grunt plugins. Once you're familiar with that process, you may install this plugin with this command:

Since this is a private library for use with origin build tools, in this project folder type:
```shell
npm link
```
Wherever you'd like to use this package, type

```shell
npm link origin-grunt-component-packager
```

Once the plugin has been installed, it may be enabled inside your Gruntfile with this line of JavaScript:

```js
grunt.loadNpmTasks('origin-grunt-component-packager');
```

### Getting the templates

There is a base CQ bundle package in **origin-grunt-component-packager/tasks/package**, please copy it to your project's build folder eg. build/cq5/package and configure it to suit your project. You can point to the base package in the configuration below.



### Example Config

```javascript
  grunt.initConfig({
    cqconfig: {
        "packageName": "origin-cq5-angular-components",
        "encoding": "utf-8",
        "packageDestinationPath": "tmp/cq5",

        "sidekickGroupName": "Web: ",
        "componentsAppsCrxPath": "/apps/originx/components/imported",
        "componentsDialogsCrxPath": "/apps/originx/components/imported/dialogs",
        "componentsDirectivesCrxPath": "/apps/originx/components/imported/directives",

        "defaultsAppsCrxPath": "/apps/originx-defaults",
        "defaultsDesignCrxPath": "/etc/designs/originx-defaults",
        "defaultsContentCrxPath": "/content/web-defaults",

        "devAppsCrxPath": "/apps/originx-dev",
        "devDesignCrxPath": "/etc/designs/originx-dev",
        "devContentCrxPath": "/content/web-dev"
    },
    clean: {
      test: ['tmp']
    },
    copy: {
        'cq5-base': {
            expand: true,
            dot: true,
            cwd: 'build/cq5/package',
            src: '**',
            dest: '<%= cqconfig.packageDestinationPath %>'
        }
    },
    replace: {
        paths: {
            options: {
                variables: {
                    "packageName": "<%= cqconfig.packageName %>",
                    "encoding": "<%= cqconfig.encoding %>",
                    "packageDestinationPath": "<%= cqconfig.packageDestinationPath %>",

                    "sidekickGroupName": "<%= cqconfig.sidekickGroupName %>",
                    "componentsAppsCrxPath": "<%= cqconfig.componentsAppsCrxPath %>",
                    "componentsDialogsCrxPath": "<%= cqconfig.componentsDialogsCrxPath %>",
                    "componentsDirectivesCrxPath": "<%= cqconfig.componentsDirectivesCrxPath %>",

                    "defaultsAppsCrxPath": "<%= cqconfig.defaultsAppsCrxPath %>",
                    "defaultsDesignCrxPath": "<%= cqconfig.defaultsDesignCrxPath %>",
                    "defaultsContentCrxPath": "<%= cqconfig.defaultsContentCrxPath %>",

                    "devAppsCrxPath": "<%= cqconfig.devAppsCrxPath %>",
                    "devDesignCrxPath": "<%= cqconfig.devDesignCrxPath %>",
                    "devContentCrxPath": "<%= cqconfig.devContentCrxPath %>"
                }
            },
            files: [{
                expand: true,
                dot: true,
                cwd: 'build/cq5/package/META-INF/vault',
                src: ['**'],
                dest: '<%= cqconfig.packageDestinationPath %>/META-INF/vault/'
            }]
        }
    },
    'package-cq5': {
        compile: {
            options: {
                "packageName": "<%= cqconfig.packageName %>",
                "encoding": "<%= cqconfig.encoding %>",
                "packageDestinationPath": "<%= cqconfig.packageDestinationPath %>",

                "sidekickGroupName": "<%= cqconfig.sidekickGroupName %>",
                "componentsAppsCrxPath": "<%= cqconfig.componentsAppsCrxPath %>",
                "componentsDialogsCrxPath": "<%= cqconfig.componentsDialogsCrxPath %>",
                "componentsDirectivesCrxPath": "<%= cqconfig.componentsDirectivesCrxPath %>",

                "defaultsAppsCrxPath": "<%= cqconfig.defaultsAppsCrxPath %>",
                "defaultsDesignCrxPath": "<%= cqconfig.defaultsDesignCrxPath %>",
                "defaultsContentCrxPath": "<%= cqconfig.defaultsContentCrxPath %>",

                "devAppsCrxPath": "<%= cqconfig.devAppsCrxPath %>",
                "devDesignCrxPath": "<%= cqconfig.devDesignCrxPath %>",
                "devContentCrxPath": "<%= cqconfig.devContentCrxPath %>"
            },
            files: [{
                cwd: 'src/directives',
                dest: '<%= cqconfig.packageDestinationPath %>/jcr_root',
                src: [
                    '**/*.js'
                ]
            }]
        }
    },
    compress: {
        zip: {
            options: {
                archive: function() {
                    return 'tmp/origin-cq5-angular-components.zip';
                }
            },
            files: [{
                expand: true,
                dot: true,
                cwd: '<%= cqconfig.packageDestinationPath %>',
                src: ['**/*']
            }]
        }
    }
  });

  grunt.loadNpmTasks('grunt-contrib-clean');
  grunt.loadNpmTasks('grunt-contrib-internal');
  grunt.loadNpmTasks('grunt-contrib-copy');
  grunt.loadNpmTasks('grunt-contrib-compress');
  grunt.loadNpmTasks('grunt-replace');
  grunt.loadNpmTasks('origin-grunt-component-packager');

Now add this to any defined task in the grunt.registerTask group eg.

grunt.registerTask('build', ['...', 'copy:cq5-base', 'replace:paths', 'package-cq5', 'compress:zip', '...'])
```



### Configuring the CQ paths

To change a CQ bundle configuration, edit the following lines in the gruntfile

```
    cqconfig: {
        "packageName": "origin-cq5-angular-components",
        "encoding": "utf-8",
        "packageDestinationPath": "tmp/cq5",

        "sidekickGroupName": "Web: ",
        "componentsAppsCrxPath": "/apps/originx/components/imported",
        "componentsDialogsCrxPath": "/apps/originx/components/imported/dialogs",
        "componentsDirectivesCrxPath": "/apps/originx/components/imported/directives",

        "defaultsAppsCrxPath": "/apps/originx-defaults",
        "defaultsDesignCrxPath": "/etc/designs/originx-defaults",
        "defaultsContentCrxPath": "/content/web-defaults",

        "devAppsCrxPath": "/apps/originx-dev",
        "devDesignCrxPath": "/etc/designs/originx-dev",
        "devContentCrxPath": "/content/web-dev"
    },
```

Package Name: the CQ bundle name

Encoding: the XML encoding scheme

Package Destination path: The location where the bundle will be generated on disk

Sidekick Group name: this prefix will be added to the sidekick to group elements from a common project

Components Apps Crx Path: the base CRX tree location for generated components and dialogs

Components Dialogs Crx Path: the CRX tree location for generated default dialogs

Components Directives Crx Path: the CRX tree location for generated generated components

Defaults Apps Crx Path: the CRX tree location for generated defaults apps components

Defaults Design Crx Path: the CRX tree location for generated defaults etc components

Defaults Content Crx Path: the CRX tree location for generated defaults content components

Dev Apps Crx Path: he CRX tree location for generated dev apps components

Dev Design Crx Path: the CRX tree location for generated dev apps components

Dev Content Crx Path: the CRX tree location for generated dev apps components



### Javascript/JSDoc tokens

Given that our application and component documentation system relies on ngdoc for up to date code specifications, we can use this information to reduce maintenance of other systems by packaging the components for other dependent systems like the CMS, Quality Assurance tooling and documentation systems.

This tool interprets valid JSdoc dockblocks for the following origin specific conventions. Grunt can use this information to build dependent tools.

**Directive Name**
```
@name origin-components.directives:originHomeRecommendedActionTrial
```
From the name JSDOC tag, follow the above convention to name the directive module:

* The parser derives the  originHomeRecommendedActionTrial (camelcase) and the element name origin-home-recommended-action-trial (hyphenated)
* the parser ignores the origin-components.directives documentation categorization hint.
* Our custom types/symbols are annotated in ManlyCase.

**Localized Strings**
```
@param {LocalizedString} title - the title string
```
To inidcate the string value should be sent for localization and that it should use the loc system in the application, use the custom type LocalizedString

**Non-Localized Strings**
```
@param {string} href the URL to the trial PDP
```
Param tags of type string are not sent for localization

**Numbers**
```
@param {number} priority - the priority value
```
Param tags of type number indicate the expected value is a javascript number type

**Image**
```
@param {ImageUrl} image the url of the image
```
Use the type ImageUrl to indicate a path to a web ready image is the required format for the field. this type is case sensitive.

**Enumerations**
```
@param {TrialTypeEnumeration} trialtype a list of tile configurations
```
the param tag of a custom type suffixed with **Enumeration** will inicate that there is a limited list of enumerations expected for the field.
The list of enumerations shall be added as a var to the javascript code as a variable of the same type name. Since this field will be parsed as JSON,
please use double quotes to represent strings:
```
/**
 * TrialTypeEnumeration list of allowed types
 * @enum {string}
 */
var TrialTypeEnumeration = {
    "foo": "bar"
}
```

**Date**
```
@param {StartDate} startdate the startdate of the trial
```
the param tag of a custom type suffixed with **Date** inicates that the field should contain a formatted date. The parser expects a format definition
in the javascript code in the following format. Note: it should be ignored by jshint if it is not directly referenced by the code.
```
/**
 * StartDate display format
 */
/* jshint ignore:start */
var StartDate = {
    "format": "Y-m-d"
};
/* jshint ignore:end */
```
For a list of format parameters, please see: [The Date Formatting Documentation](http://docs.adobe.com/docs/en/cq/5-6/widgets-api/index.html?class=Date)

**Time**
```
@param {StartTime} starttime the start time of the trial
```
the param tag of a custom type suffixed with **Time** inicates that the field should contain a formatted time. The parser expects a format definition
in the javascript code in the following format. Note: it should be ignored by jshint if it is not directly referenced by the code.
```
/**
 * StartTime display format
 * @see http://docs.adobe.com/docs/en/cq/5-6/widgets-api/index.html?class=Date
 */
/* jshint ignore:start */
var StartTime = {
    "format": "H:i:s"
};
/* jshint ignore:end */
```
For a list of format parameters, please see: [The Time Formatting Documentation](http://docs.adobe.com/docs/en/cq/5-6/widgets-api/index.html?class=Date)



### About Templates

The packaging tool will build a number of support files into CQ5 to help make development and merchandising easier.
```
defaults

The defaults workspace is the location where merchandisers can add default information for each component. The packager will set up a page in the workspace with only a single component allowed per page.
```

```
dev

When developing new functionality it can be a huge benefit to model the layout in a free form template without GEO rules. The Deve workspace allows for such modelling. This allows every directive in the vocabulary to be used. This workspace should not be exposed to the web.
```