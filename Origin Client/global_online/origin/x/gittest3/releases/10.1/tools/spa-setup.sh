#!/bin/bash
#setup

function bower_links()
{
    echo "|---------------------|"
    echo "|                     |"
    echo "| SETTING BOWER LINKS |"
    echo "|                     |"
    echo "|---------------------|"

    cd ../jssdk
    bower link
    cd ../i18n
    bower link
    cd ../components
    bower link
    cd ../uitoolkit
    bower link
    cd ../app
    bower link origin-ui-toolkit
    bower link origin-jssdk
    bower link origin-i18n
    bower link origin-components
    cd ../automation/minispa
    bower link origin-ui-toolkit
    bower link origin-jssdk
    bower link origin-i18n
    bower link origin-components
    cd ..

    echo "|-------------------------------|"
    echo "|                               |"
    echo "| SETTING BOWER LINKS COMPLETED |"
    echo "|                               |"
    echo "|-------------------------------|"
}

function clean_npm_bower_folders()
{
    echo "|--------------------------|"
    echo "|                          |"
    echo "| CLEAN NODE/BOWER FOLDERS |"
    echo "|                          |"
    echo "|--------------------------|"

    cd ../jssdk
    sudo rm -rf node_modules
    echo "deleting ../jssdk/node_modules"
    sudo rm -rf src/bower_components
    echo "deleting ../jssdk/bower_components"
    cd ../i18n
    sudo rm -rf node_modules
    echo "deleting ../i18n/node_modules"
    sudo rm -rf src/bower_components
    echo "deleting ../i18n/bower_components"
    cd ../components
    sudo rm -rf node_modules
    echo "deleting ../components/node_modules"
    sudo rm -rf src/bower_components
    echo "deleting ../components/bower_components"
    cd ../uitoolkit
    sudo rm -rf node_modules
    echo "deleting ../uitoolkit/node_modules"
    sudo rm -rf src/bower_components
    echo "deleting ../uitoolkit/bower_components"
    cd ../app
    sudo rm -rf node_modules
    echo "deleting ../app/node_modules"
    sudo rm -rf src/bower_components
    echo "deleting ../app/bower_components"
    cd ../automation/minispa
    sudo rm -rf node_modules
    echo "deleting ../automation/minispa/node_modules"
    sudo rm -rf dist/bower_components
    echo "deleting ../automation/minispa/dist/bower_components"
    cd ..
    
    echo "|-----------------------------------|"
    echo "|                                   |"
    echo "| CLEAN NODE/BOWER FOLDERS COMPLETED|"
    echo "|                                   |"
    echo "|-----------------------------------|"
}

function build_bower_dep()
{
    echo "|------------------------------|"
    echo "|                              |"
    echo "| NPM/BOWER DEPENDENCIES START |"
    echo "|                              |"
    echo "|------------------------------|"

    cd ../jssdk
    sudo npm install
    bower install
    grunt
    cd ../i18n
    sudo npm install
    bower install
    grunt
    cd ../components
    sudo npm install
    bower install
    grunt
    cd ../uitoolkit
    sudo npm install
    bower install
    grunt --force
    cd ../app
    sudo npm install
    bower install
    cd ../automation/minispa
    sudo npm install
    bower install
    cd ..

    echo "|----------------------------|"
    echo "|                            |"
    echo "| NPM/BOWER DEPENDENCIES END |"
    echo "|                            |"
    echo "|----------------------------|"
}



if [ "$1" == "clean" ]; then
    clean_npm_bower_folders
elif [ "$1" == "bowerlink" ]; then
    bower_links
elif [ "$1" == "builddepend" ]; then
    build_bower_dep
else
    bower_links
    build_bower_dep
fi
