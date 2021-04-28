Echo to file
echo -n buildNum= > property.txt 

Returns last build
curl "https://ebisu-build01.eac.ad.ea.com:83/hudson/job/Origin-Sonar-Voice-Server/api/xml?xpath=//lastBuild/number/text()" --insecure --user originautomation:Origin@11 -o output.txt

Concatenate
cat property.txt output.txt > version.properties
