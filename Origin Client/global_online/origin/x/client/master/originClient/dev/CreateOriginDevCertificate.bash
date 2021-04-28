#!/bin/bash

# Does the certificate already exist?
(security find-certificate -c OriginDev | grep "OriginDevCA") > /dev/null 2>&1

if [ $? == 0 ]; then
	echo "Found an existing OriginDev certificate"
	exit 0
fi

# ================================================================================================================

# Remove any previous setup
rm ./OriginDev.*
rm ./OriginDevCA.*

# Create default input files and OpenSSL configuration file
cat > ./OriginDevCA.defaults << EOF
US
Florida
Orlando
EA
Origin
OriginDevCA
OriginMacSCRUM@ea.com
EOF

cat > ./OriginDev.defaults << EOF
US
Florida
Orlando
EA
Origin
OriginDev
OriginMacSCRUM@ea.com


EOF

cat > ./OriginDevCA.conf << EOF
[ ca ]
default_ca = OriginDevCA

[ crl_ext ]
# issuerAltName=issuer:copy  #this would copy the issuer name to altname
authorityKeyIdentifier=keyid:always

[ OriginDevCA ]
new_certs_dir = /tmp
unique_subject = no
certificate = ./OriginDevCA.cer
database = ./OriginDevCA.index
private_key = ./OriginDevCA.key
serial = ./OriginDevCA.serial
default_days = 365
default_md = sha1
policy = OriginDevCA_policy
x509_extensions = OriginDevCA_extensions

[ OriginDevCA_policy ]
commonName = supplied
stateOrProvinceName = supplied
countryName = supplied
emailAddress = optional
organizationName = supplied
organizationalUnitName = optional

[ OriginDevCA_extensions ]
basicConstraints = CA:false
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid:always
keyUsage = digitalSignature,keyEncipherment
extendedKeyUsage = codeSigning
#crlDistributionPoints = URI:http://path.to.crl/myca.crl
EOF

# ================================================================================================================

# Setup self-signed CA
openssl req -newkey rsa:2048 -days 3650 -x509 -nodes -out ./OriginDevCA.cer -keyout ./OriginDevCA.key < ./OriginDevCA.defaults

# Create serial/index files
echo -n | tee ./OriginDevCA.index > /dev/null 2>&1
echo -n 0aaa | tee ./OriginDevCA.serial > /dev/null 2>&1

# Create code signing request
openssl req -newkey rsa:2048 -nodes -out ./OriginDev.csr -keyout ./OriginDev.key < ./OriginDev.defaults

# Sign the CSR with our CA
openssl ca -batch -config ./OriginDevCA.conf -notext -in ./OriginDev.csr -out ./OriginDev.cer

# Create PKCS12 to allow push to Keychain Access
#openssl pkcs12 -export -in ./OriginDev.cer -inkey ./OriginDev.key -out ./OriginDev.pfx

# Convert to DER format to import into Keychain Access - didn't use -inform -outform for previous commands because I was getting errors
openssl x509 -in ./OriginDevCA.cer -inform PEM -out ./OriginDevCA.crt -outform DER
openssl rsa  -in ./OriginDevCA.key -inform PEM -out ./OriginDevCA.csa -outform DER

# Register certificate with Keychain Access
security add-trusted-cert ./OriginDevCA.crt
security add-certificates ./OriginDevCA.crt
certtool i ./OriginDev.cer r=./OriginDev.key

# Clean up our mess
rm OriginDev.*
rm OriginDevCA.*
