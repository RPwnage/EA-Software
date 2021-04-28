'''
Created on December 16, 2010

@author: dgog
'''

import urllib
import sys
import zipfile
import getopt
import os

from xml.dom import minidom

#Given the version, timestamp, and build number this method constructs the name
#of the installer file
def construct_filename(artifact_id, version, classifier):
	filename = artifact_id + "-" + version + "-" + classifier + ".zip"
	return filename

#Given the url and filename constructs the file path for the installer
def construct_filepath(url, filename, version):
	file_path = url + "/" + version + "/" + filename
	return file_path

#Downloads the installer from the given url with the given version timestamp
#and build_number
def getFilename(url, artifact_id, version, classifier):
	#remove SNAPSHOT from version
	filename = construct_filename(artifact_id, version, classifier)
	filepath = construct_filepath(url, filename, version)
	return filename

#Construct the installer repo url
def construct_url(repo_snapshot_url, group_id, artifact_id):
	group_dir = group_id.replace(".", "/");
	url = repo_snapshot_url + "/" + group_dir + "/" + artifact_id
	return url

#Extract timestamp and build_number from maven-metadata.xml
def extract_latest_info(url, artifact_id, classifier):
	metadata = "maven-metadata.xml"
	file = urllib.urlopen(url + "/" + metadata);
	dom = minidom.parse(file)
	versions = dom.getElementsByTagName('version')
	versionNum = latestVersion = 0
	for version in versions:
		newVersionNum = "".join(version.firstChild.data.split("."))
		try:
			if int(newVersionNum) > int(versionNum):
				versionString = version.firstChild.data
				filename = getFilename(url, artifact_id, versionString, classifier)
				fullPath = url + "/" + versionString + "/" + filename
				if (urllib.urlopen(fullPath)):
					versionNum = newVersionNum
					latestVersion = versionString
		except ValueError:
			pass
		
	return str(latestVersion)

def extract_zip(the_zipfile, dest):
	try:
		zip = zipfile.ZipFile(the_zipfile)
		for name in zip.namelist():
			zip.extract(name, dest)
		zip.close()
	except Exception as exc:
		print  "Zipfile exception :"  + str(exc)

	
def main():
	try:
		optList, args = getopt.getopt(sys.argv[1:], 'r:g:a:d:o:u:d:c:')
	except getopt.GetoptError:
		usage()
		sys.exit(1)

	destination = "."
	origin_url = ""
	group_id = ""
	artifact_id = ""
	
	for opt, val in optList:
		if opt == "-r":
			repo_snapshot_url = val
		elif opt == "-u":
			origin_url = val
		elif opt == "-g":
			group_id = val
		elif opt == "-c":
			classifier = val
		elif opt == "-a":
			artifact_id = val
		elif opt == "-o":
			outputFile = val
		elif opt == "-d":
			destination = val
		else:
			assert False, "Unhandled Option"
	
	if (origin_url == ""):
		url = construct_url(repo_snapshot_url, group_id, artifact_id)
		version = extract_latest_info(url)
	
		filename = getFilename(url, artifact_id, version, classifier)
	
		fullPath = url + "/" + filename
		
		print fullPath
		f = open(outputFile,'w+')
		f.write("latestAPIURL=" + fullPath)
		os.putenv("LATEST_API_URL",fullPath)
		f.close()
	elif (origin_url == "0"):
		url = construct_url(repo_snapshot_url, group_id, artifact_id)
		version = extract_latest_info(url, artifact_id, classifier)
	
		filename = getFilename(url, artifact_id, version, classifier)
	
		fullPath = url + "/" + version + "/" + filename
		
		filename = os.path.basename(fullPath)
		url = os.path.dirname(fullPath)
		urllib.urlretrieve(fullPath,filename)
		extract_zip(filename, destination)
	else:
		filename = os.path.basename(origin_url)
		url = os.path.dirname(origin_url)
		urllib.urlretrieve(origin_url,filename)
		extract_zip(filename, destination)

	return 0 
	
def usage():
	print "Usage: get_latest_api_url <options>"
	print "-r: repository url"
	print "-g: artifact group eg. com.ea.online.ebisu"
	print "-a: artifact id. eg. avatar"
	print "-c: artifact classifier. eg. apiref"
	print "-o: output file. eg. latestAPIURL.properties" 
	print "-u: URL to retrieve from. If a URL is specified, this will use the URL provided instead of getting it. If 0 is specified, it will download the URL that it generates."
	print "-d: destination where the output file should go. Default is current dir." 
 

main()