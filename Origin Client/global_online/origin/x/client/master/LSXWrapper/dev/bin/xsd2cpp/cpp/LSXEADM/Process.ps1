# Add the PS files you want to Process here. 
# Add a $true for whether you want to iterate through the schemas
# Add a $false if you just want to Process one time.

$Generator.Process("Header.ps1", $true)
$Generator.Process("Source.ps1", $true)
$Generator.Process("EnumHeader.ps1", $true)
$Generator.Process("EnumStringHeader.ps1", $true)
$Generator.Process("EnumStringSource.ps1", $true)
$Generator.Process("ReaderHeader.ps1", $true)
$Generator.Process("ReaderSource.ps1", $true)

if(-not $Generator.ReadOnly)
{
	$Generator.Process("WriterHeader.ps1", $true)
	$Generator.Process("WriterSource.ps1", $true)
}

# $Generator.CopyCommonFiles()

$Generator.Process("PostBuildStep.ps1", $false)