|--------------|
|              |
| spa-setup.sh |
|              |
|--------------|


./ spa-setup.sh (should run this option on the first sync from P4)
	Runs both the bowerlink and builddepend options together

./spa-setup.sh clean 
	Deletes the npm install and bower install folders and deletes the sym links

./spa-setup.sh bowerlink
	Just creates the symlink between the toolkit/jssdk/components and the bower_components folder of the SPA

./spa-setup.sh builddepend
	Just runs npm install and bower install for the toolkit/jsddk/components/SPA 
