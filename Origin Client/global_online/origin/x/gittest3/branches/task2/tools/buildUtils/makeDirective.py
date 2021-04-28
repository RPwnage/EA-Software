#!/usr/bin/python
import os
import re
import json
import argparse

OKGREEN = '\033[92m'
OKBLUE = '\033[104m'
ENDC = '\033[0m'
WARNING = '\033[93m'

parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter,description='''\
                This simple script will create the JavaScript and HTML for an element component
                defined by the name you provide.  Once executed the script  prompt will ask you for
                the name of your component, every capital means a new directory.  As such
                componenents are created using camelcase for example thisIsATest would create
                components:

                ../../this/is/a/scripts/test.js
                ../../this/is/a/views/test.js

                You will be prompted if you are replacing new files
            ''')

args = parser.parse_args()
#print args.accumulate(args.integers)

data = {
    'componentName': "forgotToAddAName",
    'src': '../../components/src/directives/',
    'seperator': '/'
}


def getParameters():
    cn = raw_input('What is your component name? ')
    return {
        'componentName': cn
    }


def createNames(componentName):
    upperFirst = lambda s: s[:1].upper() + s[1:] if s else ''
    return {
        "controllerName": 'Origin' + upperFirst(componentName) + 'Ctrl',
        "linkName": 'origin' + upperFirst(componentName) + 'Link',
        "directiveName": 'origin' + upperFirst(componentName)
    }


def createPaths(componentName, prePend, seperator):
    pathPieces = []
    startRe = re.compile("(^[a-z]*)")
    start = startRe.findall(componentName)
    regex = re.compile("([A-Z][a-z]*)")
    pieces = regex.findall(componentName)
    for i in start:
        pathPieces.append(i.lower())
    for j in pieces:
        pathPieces.append(j.lower())
    return {
        'filename': pathPieces.pop(),
        'javaPath': prePend + seperator.join(pathPieces) + '/scripts/',
        'htmlPath': prePend + seperator.join(pathPieces) + '/views/',
        'relativeHtmlPath': seperator.join(pathPieces) + '/views/',
        'relativeJavaPath': seperator.join(pathPieces) + '/scripts/',
        'directiveMarkup': '-'.join(pathPieces).lower()
    }


def writeFile(path, filename, extension, text):
    filePath = path + filename + extension
    if os.path.exists(filePath):
        ans = raw_input(
            "%sThis file already exisit %s! Do you want to Replace it? (Y/N)%s" % (WARNING, filePath, ENDC))
        if 'Y' != ans:
            return

    if not os.path.exists(path):
        os.makedirs(path)
    fd = open(path + filename + extension, 'w')
    fd.write(text)
    fd.close()
    print "%sCreated file %s%s" % (OKGREEN, filePath, ENDC)

data.update(getParameters())
data.update(createPaths(data['componentName'], data['src'], data['seperator']))
data.update(createNames(data['componentName']))

javascript = "\n\
/** \n\
 * @file %(relativeJavaPath)s%(filename)s.js\n\
 */ \n\
(function(){\n\
    'use strict';\n\
    /**\n\
    * The controller\n\
    */\n\
    function %(controllerName)s($scope) {\n\
    \n\
    }\n\
    /**\n\
    * The directive\n\
    */\n\
    function %(directiveName)s(ComponentsConfigFactory) {\n\
        /**\n\
        * The directive link\n\
        */\n\
        function %(linkName)s(scope, elem, attrs, ctrl) {\n\
            // Add Scope functions and call the controller from here\n\
        }\n\
        return {\n\
            restrict: 'E',\n\
            scope: {},\n\
            controller: '%(controllerName)s',\n\
            templateUrl: ComponentsConfigFactory.getTemplatePath('%(relativeHtmlPath)s%(filename)s.html'),\n\
            link: %(linkName)s\n\
        };\n\
    }\n\
    /**\n\
     * @ngdoc directive\n\
     * @name origin-components.directives:%(directiveName)s\n\
     * @restrict E\n\
     * @element ANY\n\
     * @scope\n\
     * @description\n\
     * @param {string=} \n\
     *\n\
     *\n\
     * @example\n\
     * <example module=\"origin-components\">\n\
     *     <file name=\"index.html\">\n\
     *     <origin-%(directiveMarkup)s-%(filename)s />\n\
     *     </file>\n\
     * </example>\n\
     */\n\
    angular.module('origin-components')\n\
        .controller('%(controllerName)s', %(controllerName)s)\n\
        .directive('%(directiveName)s', %(directiveName)s);\n\
}()); \n\
" % data

html = "<!-- YOUR HTML SHOULD GO HERE FOR %(directiveName)s --> " % data

writeFile(data['javaPath'], data['filename'], '.js', javascript)
writeFile(data['htmlPath'], data['filename'], '.html', html)
print "%sAll finished! You have been working hard take a break and grab a beer!%s" % (OKBLUE, ENDC)
