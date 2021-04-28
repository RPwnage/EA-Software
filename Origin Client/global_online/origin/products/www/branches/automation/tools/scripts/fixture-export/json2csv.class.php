<?php

class JSON2CSVutil{

	public $dataArray;

	function JSONfromFile($file){
		$this->dataArray = json_decode(file_get_contents($file), 1);
		return $this->dataArray;
	}

	function flatten2CSV($file){
		$fileIO = fopen($file, 'w+');

		$flatData = array('Component name', 'Attribute Name', 'English Value');
		fputcsv($fileIO, $flatData);

		foreach ($this->dataArray as $componentName => $items) {
			fputcsv($fileIO, [$componentName, '', '']);

			foreach ($items as $attributeName => $englishValue) {
				fputcsv($fileIO, ['', $attributeName ,  $englishValue]);
			}
		}

		fclose($fileIO);
	}

	function flattenjsonFile2csv($file, $destFile){
		$this->JSONfromFile($file);
		$this->flatten2CSV($destFile);
	}
}
