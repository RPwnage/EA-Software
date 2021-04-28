#!/usr/bin/python

import commands, sys, boto3

# function used to find files of a specific type in a specific directory, and upload them to S3
# assumption: this can't fail with 'The specified bucket does not exist' because
# tech ops provisions any AWS account used to run DirtyCast with a dedicated S3 bucket
def listAndSendFiles(filetype, directory, outdir):
    print 'looking for',filetype,'files in', directory
    cmd = "ls " + directory
    files = commands.getoutput(cmd).split()
    for file in files:
        if file.find(filetype) != -1:
            src = directory + "/" + file
            s3key = app + "/" + podname + "/" + containerid + "/" + outdir + "/" + file
            print 'initiating upload of',src,'to S3 bucket',s3bucket,'with key',s3key
            s3.upload_file(src, s3bucket, s3key)

usage = "usage: backupToS3 -app=<application name>  -podname=<podname> -containerid=<container identification> -coredir=<location of core files> -logdir=<location of log files>"

if len(sys.argv) != 6 :
    print usage
    sys.exit()

for option in sys.argv[1:]:
    if option.find('-app=') != -1:
        app = option[5:]
    elif option.find('-podname=') != -1:
        podname = option[9:]
    elif option.find('-containerid=') != -1:
        containerid = option[13:]
    elif option.find('-coredir=') != -1:
        coredir = option[9:]
    elif option.find('-logdir=') != -1:
        logdir = option[8:]
    else:
        print usage
        sys.exit()

# create boto3 resource to access aws STS
sts = boto3.client('sts')

# retrieve aws account id for current cluster, and use it to build S3 bucket name
# this is necessary to workaround the fact that S3 bucket name can't be re-used between AWS accounts'
try:
    accountid = sts.get_caller_identity().get('Account')
    s3bucket = "dirtycast-runtime-" + accountid
except Exception as e:
    print "Aborting S3 upload because S3 bucket name could not be assembled from fetched AWS account. err=%s" % e
    sys.exit()

# create boto3 resource to access aws S3
s3 = boto3.client('s3')

# save core files to S3 bucket
listAndSendFiles('core.', coredir, "corefiles")

# save log files to S3 bucket
listAndSendFiles('.log', logdir, "logfiles")
