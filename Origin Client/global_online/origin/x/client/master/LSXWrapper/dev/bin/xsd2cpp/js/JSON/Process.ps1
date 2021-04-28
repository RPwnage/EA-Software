# Add the PS files you want to Process here. 
# Add a $true for whether you want to iterate through the schemas
# Add a $false if you just want to Process one time.

$Generator.Process("Source.ps1", $true)
$Generator.Process("Documentation.ps1", $true)
