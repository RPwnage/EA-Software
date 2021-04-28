#!/usr/bin/env php
<?php
/**
 * Import translations for Origin X project into a local database
 * So Excel can form a template HAL document.
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

$config = [
    'mysql_username' => 'root',
    'mysql_password' => 'admin',
    'mysql_dsn' => 'mysql:host=localhost;port=3306;dbname=xlocalization',
    'cq5_host' => 'https://qa.preview.cms.origin.com',
    'cq5_endpoint' => '/content/translation-tools/translator/jcr:content.handler.getInfos',
    'cq5_username' => 'admin',
    'cq5_password' => 'notadmin',
    'path' => '/content/web',
    'max_results' => 1000
];

// get the translations for a given path from CQ5's localization service
$exporter = new \CQ5TranslationExporter($config);
$doc = $exporter->export($config['path'], $config['max_results']);
if(isset($doc['rows']) && is_array($doc['rows']))
{
    echo sprintf("\nFound %d Translation Strings\n", count($doc['rows']));

    // insert applicable translations into the local mysql
    $pdo = new \PDO($config['mysql_dsn'], $config['mysql_username'], $config['mysql_password']);
    $importer = new \MySQLTranslationImporter($pdo);
    $importer->import($doc);
    echo sprintf("\nImported %d new translations\n", $importer->getCount());
}
elseif($doc === false)
{
    echo sprintf("\nResponse malformed for %s\n", $config['path']);
}
else
{
    echo sprintf("\nNo translations found for %s\n", $config['path']);
}

echo "\nImport Process Complete.\n";

/**
 * CQ5TranslationExporter
 */
class CQ5TranslationExporter
{
    protected $config = [
        'cq5_host' => '',
        'cq5_endpoint' => '',
        'cq5_username' => '',
        'cq5_password' => ''
    ];

    /**
     * Constructor.
     *
     * @param array $config the service configuration
     */
    public function __construct(array $config)
    {
        $this->config = $config;
    }

    /**
     * Get fresh data from CQ5's transaltion service
     *
     * @param string $path a path to search for translations
     * @param integer maximum number of results to fetch
     *
     * @return array or false
     */
    public function export($path='/content/web', $rp=1000)
    {
        $parameters = [
            'path'=>$path,
            'page'=>'1',
            'rp'=>$rp,
            'sortname'=>'id',
            'sortorder'=>'asc',
            'query'=>'',
            'qtype'=>'key',
            'ignoretags'=>'false'
        ];

        $ch = curl_init(sprintf('%s%s?%s', $this->config['cq5_host'], $this->config['cq5_endpoint'], http_build_query($parameters)));
        curl_setopt($ch, CURLOPT_USERPWD, sprintf('%s:%s', rawurlencode($this->config['cq5_username']), rawurlencode($this->config['cq5_password'])));
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_VERBOSE, 1);
        curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, 0);
        $response = curl_exec($ch);
        curl_close($ch);

        return json_decode($response, true);
    }
}

/**
 * MySQLTranslationImporter
 */
class MySQLTranslationImporter
{
    protected $pdo;

    /**
     * Constructor.
     *
     * @param object $pdo Instance of PDO
     */
    public function __construct(\PDO $pdo)
    {
        $this->pdo = $pdo;
    }

    /**
     * This is the import process, clean the old values and replace with new ones from
     * the provided docs.
     *
     * @param array $doc an array serialized version of the input JSON from localizer
     */
    public function import(array $doc = array())
    {
        $this->mysqlClean();
        $this->mysqlCreateTable();
        $this->mysqlHydrate($doc);
    }

    /**
     * Get a row count of the new database
     *
     * @return integer row count
     */
    public function getCount()
    {
        $result = $this->pdo->query('SELECT COUNT(*) as count FROM xi18n', \PDO::FETCH_ASSOC);
        return (int) $result->fetch()['count'];
    }

    /**
     * Truncate the table
     */
    protected function mysqlClean()
    {
        $this->pdo->query('DROP TABLE xi18n');
    }

    /**
     * Create table and schema
     */
    protected function mysqlCreateTable()
    {
        $this->pdo->query('create table xi18n(
            `Key` text,
            `English Text` text,
            `Context` text,
            `Max Length` text,
            `String Id` text,
            `SPA_ES` text,
            `GER_DE` text,
            `FRE_FR` text,
            `ITA_IT` text,
            `DUT_NL` text,
            `DAN_DK` text,
            `SWE_SE` text,
            `POR_PT` text,
            `POR_BR` text,
            `POL_PL` text,
            `NOR_NO` text,
            `KOR_KR` text,
            `SPA_MX` text,
            `RUS_RU` text,
            `FIN_FI` text,
            `CHT_CN` text,
            `THA_TH` text,
            `JPN_JP` text,
            `FRA_CA` text
            )');
    }

    /**
     * Import the new document
     *
     * @param array $doc The new localization array
     */
    protected function mysqlHydrate(array $doc=array())
    {
        if(isset($doc['rows']) && is_array($doc['rows']) && count($doc['rows']) > 0)
        {
            $command = 'INSERT INTO xi18n VALUES ';
            $values = [];

            foreach($doc['rows'] as $row)
            {
                // Specifially look for previous polish translations to exclude
                // These are fixed indexes (sad):
                //   0: English
                //   104: Polish
                if(isset($row['cell'][0]) && strlen($row['cell'][0]) > 0 && isset($row['cell'][104]) && empty($row['cell'][104]))
                {
                    $values[] = sprintf('("", "%s", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "")', $row['cell'][0]);
                }
            }

            $statement = $command . join(',', $values);
            $this->pdo->query($statement);
        }
    }
}
