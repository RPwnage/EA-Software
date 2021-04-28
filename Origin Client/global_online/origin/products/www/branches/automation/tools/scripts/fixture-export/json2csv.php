<?php
require_once('json2csv.class.php');
$JSON2CSV = new JSON2CSVutil;

if(isset($argv[1])){
	$shortopts = "f::";  // Required value
	$longopts  = array("file::","dest::");
	$arguments = getopt($shortopts, $longopts);

	if(isset($arguments["dest"])){
		$filepath = $arguments["dest"];
	}
	else{
		$filepath = "JSON2.CSV";
	}

	$JSON2CSV->flattenjsonFile2csv($arguments["file"], $filepath);
}
