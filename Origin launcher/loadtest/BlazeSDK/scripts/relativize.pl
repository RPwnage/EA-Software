# Relativize the paths in a vcproj file
# Afterwards replaces inputFile with outputFile
# Args: sdkRoot vcprojFile

if($#ARGV < 1) {
    print "Usage: perl relativize.pl packageRoot vcprojFile";
    exit 1;
}

my $sdkRoot = $ARGV[0];
my $vcprojFile = $ARGV[1];
my $tempFile1 = "$vcprojFile.temp";
my $tempFile2 = "$vcprojFile.temp2";

my $DRIVE = "";
my $PACKAGES = "";
my $PACKAGE = "";
my $VERSION = "";

if($sdkRoot =~ /^(.*)\\(.*)\\(.*)\\(.*)/) {
    $DRIVE = $1;
    $PACKAGES = $2;
    $PACKAGE = $3;
    $VERSION = $4;
}

if($PACKAGE !~ /^BlazeSDK$/ || $PACKAGES !~ /^packages$/) {
    print "Bad sdkRoot!  Must be DRIVE/packages/BlazeSDK/VERSION\n";
    exit 1;
}

if($vcprojFile !~ /Pyro\.csproj$/) {

# 1. Replace C:\packages\BlazeSDK\mainline with relative path
`C:\\cygwin\\bin\\sed s/$DRIVE\\\\$PACKAGES\\\\$PACKAGE\\\\$VERSION/..\\\\..\\\\../g < $vcprojFile > $tempFile1`;

# 2. Replace C:\packages with relative path
`C:\\cygwin\\bin\\sed s/$DRIVE\\\\$PACKAGES/..\\\\..\\\\..\\\\..\\\\../g < $tempFile1 > $tempFile2`;

} else{

# 1. Replace C:\packages\BlazeSDK\mainline with relative path
`C:\\cygwin\\bin\\sed s/$DRIVE\\\\$PACKAGES\\\\$PACKAGE\\\\$VERSION/..\\\\..\\\\..\\\\../g < $vcprojFile > $tempFile1`;

# 2. Replace C:\packages with relative path
`C:\\cygwin\\bin\\sed s/$DRIVE\\\\$PACKAGES/..\\\\..\\\\..\\\\..\\\\..\\\\../g < $tempFile1 > $tempFile2`;

}

# 3. Replace ..\..\..\..\..\..\Program Files\Microsoft Visual Studio 8 with %VSHOME%
`C:\\cygwin\\bin\\sed s/..\\\\..\\\\..\\\\..\\\\..\\\\..\\\\Program/$DRIVE\\\\Program/g < $tempFile2 > $tempFile1`;
`C:\\cygwin\\bin\\sed \"s/$DRIVE\\\\\\\\Program Files\\\\\\\\Microsoft Visual Studio 8/\$(VSHOME)/g\" < $tempFile1 > $tempFile2`;

`move /Y $tempFile2 $vcprojFile`;
`del /F $tempFile1`;

exit 0;
