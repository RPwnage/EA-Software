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

#Simply removes the SNAPSHOT from the version name
def normalize_version(version):
	n_version = version.split("-")[0]
	return n_version
	
#Given the version, timestamp, and build number this method constructs the name
#of the installer file
def construct_filename(artifact_id, version, timestamp, build_number):
	filename = artifact_id + "-" + version + "-" + timestamp + "-" + build_number + "-" + artifact_id + ".zip"
	return filename

#Given the url and filename constructs the file path for the installer
def construct_filepath(url, filename):
	file_path = url + "/" + filename
	return file_path

#Downloads the installer from the given url with the given version timestamp
#and build_number
def getFilename(url, artifact_id, version, timestamp, build_number):
	#remove SNAPSHOT from version
	version = normalize_version(version)
	filename = construct_filename(artifact_id, version, timestamp, build_number)
	filepath = construct_filepath(url, filename)
	return filename

#Construct the installer repo url
def construct_url(repo_snapshot_url, group_id, artifact_id, version):
	group_dir = group_id.replace(".", "/");
	url = repo_snapshot_url + "/" + group_dir + "/" + artifact_id + "/" + version
	return url

#Extract timestamp and build_number from maven-metadata.xml
def extract_latest_info(url):
	metadata = "maven-metadata.xml"
	file = urllib.urlopen(url + "/" + metadata);
	dom = minidom.parse(file)
	timestamp = dom.getElementsByTagName('timestamp')[0].firstChild.data
	build_number = dom.getElementsByTagName('buildNumber')[0].firstChild.data
	return timestamp, build_number

def extract_zip(the_zipfile, dest):
	zip = zipfile.ZipFile(the_zipfile)

	for name in zip.namelist():
		zip.extract(name, dest)
	zip.close()
	
def get_origin_from_url(origin_url, destination):
	filename = os.path.basename(origin_url)
	urllib.urlretrieve(origin_url,filename)
	extract_zip(filename, destination)
	
def main():
	try:
		optList, args = getopt.getopt(sys.argv[1:], 'r:g:a:v:d:o:u:d:',['get'])
	except getopt.GetoptError:
		usage()
		sys.exit(1)

	destination = "."
	origin_url = ""
	getBuild = False
	
	for opt, val in optList:
		if opt == "-r":
			repo_snapshot_url = val
		elif opt == "-u":
			origin_url = val
		elif opt == "-g":
			group_id = val
		elif opt == "-a":
			artifact_id = val
		elif opt == "-v":
			version = val
		elif opt == "-o":
			outputFile = val
		elif opt == "-d":
			destination = val
		elif opt == "--get":
			getBuild = True
		else:
			assert False, "Unhandled Option"
	
	if (origin_url == ""):
		url = construct_url(repo_snapshot_url, group_id, artifact_id, version)
		timestamp, build_number = extract_latest_info(url)
	
		filename = getFilename(url, artifact_id, version, timestamp, build_number)
	
		fullPath = url + "/" + filename
		
		print fullPath
		f = open(outputFile,'w+')
		f.write(fullPath)
		os.putenv("LATEST_ORIGIN_URL",fullPath)
		f.close()
		if (getBuild):
			get_origin_from_url(fullPath, destination)
	else:
		get_origin_from_url(origin_url, destination)
	
	return 0 
	
def usage():
	print "Usage: get_latest_origin_url <options>"
	print "-r: repository url"
	print "-g: artifact group eg. com.ea.core"
	print "-a: artifact id. eg. OriginInstaller"
	print "-v: artifact version. eg. 8.3.0-SNAPSHOT" 
	print "-o: output file. eg. latestOriginURL.properties" 
	print "-u: URL to retrieve from. If this is specified, this will use the URL provided instead of getting it."
	print "-d: destination where the output file should go. Default is current dir." 
	print "--get: if this is specified, get the build and unzip it too." 
 

main()