#!/bin/bash

# This is the script the Jenkins can use to build the SPA.

WEB_DIR=$WORKSPACE/origin/sandboxes/${BRANCH}/web
APP_DIR=$WEB_DIR/app
BUILD_DIR=$APP_DIR/build
TARGET_DIR=$BUILD_DIR/target
export VERSION=`cat $APP_DIR/version-linux.txt`.$P4_CHANGELIST
export MAVEN_WEBAPP_PROJ_DIR=/opt/apps/maven-repo/maven/com/ea/origin/x/sandboxes/${BRANCH}/originwebapp
export MAVEN_WEBAPP_DIR=$MAVEN_WEBAPP_PROJ_DIR/$VERSION
export MAVEN_DOCS_PROJ_DIR=/opt/apps/maven-repo/maven/com/ea/origin/x/sandboxes/${BRANCH}/otkdocs
export MAVEN_DOCS_DIR=$MAVEN_DOCS_PROJ_DIR/$VERSION
export MAVEN_HOST=maven.dm.origin.com

cd $WEB_DIR/tools
sudo chmod 777 setup
./setup --clean --jenkins --release

cd $WEB_DIR/automation/mockup
npm install
cd $WEB_DIR/jssdk
npm install
bower install
grunt
grunt start-mockup functional-test terminate-mockup --force

mkdir -p $BUILD_DIR/
sudo rm -rf $TARGET_DIR/
mkdir $TARGET_DIR/
cd $APP_DIR/dist

# add info to the build.info file:
BUILDINFO_FILE="$APP_DIR/dist/build.info"

TS=$(curl -u originautomation:Origin@11 --insecure --silent -H "Accept: application/json" "$JENKINS_URL/job/${JOB_NAME}/${BUILD_NUMBER}/api/json?tree=timestamp" | tr -d "{|}|\"" | cut -d":" -f2 | cut -c -10)
BUILD_TS=$(TZ=America/Vancouver date -d@$TS)

BUILD_CAUSE=$(curl -u originautomation:Origin@11 --insecure --silent -H \"Accept: application/json\" "$JENKINS_URL/job/${JOB_NAME}/${BUILD_NUMBER}/api/json?tree=actions\[causes\[shortDescription\]\]" | cut -d ":" -f4 | cut -d "\"" -f2)

if test ! -f $BUILDINFO_FILE; then
    touch $BUILDINFO_FILE
    sudo chmod 664 $BUILDINFO_FILE
else
    echo -n "" > $BUILDINFO_FILE
    if test $? -eq 0; then
        echo "spa_build_version=${VERSION}" >> $BUILDINFO_FILE
        echo "spa_build_number=${BUILD_NUMBER}" >> $BUILDINFO_FILE
        echo "spa_build_branch=${BRANCH}" >> $BUILDINFO_FILE
        echo "spa_build_cl=${P4_CHANGELIST}" >> $BUILDINFO_FILE
        echo "spa_build_ts=${BUILD_TS}" >> $BUILDINFO_FILE
        echo "spa_build_cause=${BUILD_CAUSE}" >> $BUILDINFO_FILE
        echo "spa_build_log_url=$JENKINS_URL/view/OriginX/job/originx-spa-branch-build/${BUILD_NUMBER}/consoleText" >> $BUILDINFO_FILE
    else
        echo "WARNING: Updating build.info failed!"
        exit 1
    fi
fi

sudo tar --exclude=**/node_modules -hcpf $TARGET_DIR/originwebapp-$VERSION.tar .
# sudo zip -r $TARGET_DIR/originwebapp.zip .

cd $TARGET_DIR
gzip -9 originwebapp-$VERSION.tar

echo 'CurrentSPAVersion $VERSION'

#create the version dir
sudo ssh -i /home/originautomation/keys/mavenrepo.pem ubuntu@maven.dm.origin.com mkdir -p $MAVEN_WEBAPP_DIR

# copy to maven.dm.origin.com
sudo scp -i ~/keys/mavenrepo.pem originwebapp-$VERSION.tar.gz ubuntu@maven.dm.origin.com:$MAVEN_WEBAPP_DIR/

# update the latest version file
#Set the latest build ID to the permanent link page:
cd $WORKSPACE
sudo ssh -q -i /home/originautomation/keys/mavenrepo.pem ubuntu@maven.dm.origin.com "echo $VERSION >$MAVEN_WEBAPP_PROJ_DIR/latest"

# build the docs
cd $WEB_DIR/uitoolkit
npm install
bower install
grunt dist-docs

cd $WEB_DIR/uitoolkit/docs
sudo tar --exclude=**/node_modules -hcpf $TARGET_DIR/otkdocs-$VERSION.tar .

cd $TARGET_DIR
gzip -9 otkdocs-$VERSION.tar

#create the version dir
sudo ssh -i /home/originautomation/keys/mavenrepo.pem ubuntu@maven.dm.origin.com mkdir -p $MAVEN_DOCS_DIR

# copy to maven.dm.origin.com
sudo scp -i ~/keys/mavenrepo.pem otkdocs-$VERSION.tar.gz ubuntu@maven.dm.origin.com:$MAVEN_DOCS_DIR/

# update the latest version file
#Set the latest build ID to the permanent link page:
cd $WORKSPACE
sudo ssh -q -i /home/originautomation/keys/mavenrepo.pem ubuntu@maven.dm.origin.com "echo $VERSION >$MAVEN_DOCS_PROJ_DIR/latest"

# trigger the deploy of the build if sandbox name is specified
if [ "$SANDBOX" != "none" ]; then
   curl "$JENKINS_URL/view/All/job/originx-dev-sandbox-deploy-SPA/buildWithParameters?token=deployspa&SANDBOX=$SANDBOX&SPA_BUILD_BRANCH=$BRANCH&SPA_PACKAGE_VERSION=$VERSION" --insecure --user originautomation:Origin@11
fi