Origin Downloader Diagnostics Tool

Created By: Ryan Binns

Initial Version - 10/08/2015

PURPOSE:

The Origin client downloader captures certain diagnostic data during the course of a download.  If a download fails, this diagnostic data is dumped to the client log file.

Using this diagnostic data dump, combined with a properly verified 'golden' copy of a game build (SHA1 verified, for example), we can 'reconstruct' or 'replay' the download as
it occurred for the Origin user.  By comparing the various chunks recorded CRC values versus the actual CRC values of the corresponding sections of the golden file, we can
determine whether data corruption originated from the CDN, or from a bug in the downloader itself.  (Zlib decompressor, etc)

Additionally, we can feed the reconstructed chunks through the unpack service itself, and 'replay' the download to see how it behaved.  If we fix a bug in the unpack service, we can
see whether that would have allowed the download to complete or not.

Last, for cases where we believe we have received bad data from the CDN, we can request this data directly from the CDN nodes, and then CRC that data.  Unfortunately, this feature
is limited in usefulness by the 'freshness' of your data.  CDN caches cycle out often, and you may find that it succeeds, even if it failed for the user.

EXAMPLE DIAGNOSTIC DATA:

**402    wrz 11 19:34:20.028 Warning   Origin::Downloader::UnpackStreamFile::DumpDiagnosticData                    4208      ################# Begin Downloader Diagnostic Data Dump #################
**403    wrz 11 19:34:20.029 Warning   Origin::Downloader::UnpackStreamFile::DumpDiagnosticData                    4208      Content$+ID~OFB-EAST_109552409++File~the_sims_4_pc_fg__ww_us_beta_658__pcbgmlrprodretaildip_110631020_5cd5466e50694b599df4f2e912a65c94.zip+Host$+Name~origin-a.akamaihd.net++IP~72.247.182.27+Ck$+Time~55f32bd6++Rng~ccb85ef7-ccc6fef7++CRC~2334a268+Ck$+Time~55f32bdd++Rng~ccc6fef7-cd35fef7++CRC~f98b9e9b+Ck$+Time~55f32be0++Rng~cd35fef7-cda2fef7++CRC~2d05356d+Ck$+Time~55f32be5++Rng~cda2fef7-ce2afef7++CRC~928e782a+Ck$+Time~55f32bf2++Rng~ce2afef7-ceda7ef7++CRC~35021efc+Ck$+Time~55f32bfc++Rng~ceda7ef7-cf487ef7++CRC~1ba0565e+Ck$+Time~55f32c03++Rng~cf487ef7-cfa77ef7++CRC~83df2e80+Ck$+Time~55f32c0b++Rng~cfa77ef7-d00f7ef7++CRC~c296d152+Ck$+Time~55f32c12++Rng~d00f7ef7-d0717ef7++CRC~97e734f5+Ck$+Time~55f32c19++Rng~d0717ef7-d0dafef7++CRC~b83c19e6+Ck$+Time~55f32c21++Rng~d0dafef7-d14a7ef7++CRC~c41eb375+Ck$+Time~55f32c29++Rng~d14a7ef7-d1bb7ef7++CRC~65de7cef+Ck$+Time~55f32c60++Rng~d1bb7ef7-d1bbfef7++CRC~6237ca5d+Ck$+Time~55f32c94++Rng~d1bbfef7-d1ebfef7++CRC~51eb84e+Ck$+Time~55f32cab++Rng~d1ebfef7-d1ec96f7++CRC~dcb0055+Ck$+Time~55f32cab++Rng~d1ec96f7-d1f116f7++CRC~6793d3a3+
**404    wrz 11 19:34:20.029 Warning   Origin::Downloader::UnpackStreamFile::DumpDiagnosticData                    4208      Content$+ID~OFB-EAST_109552409++File~the_sims_4_pc_fg__ww_us_beta_658__pcbgmlrprodretaildip_110631020_5cd5466e50694b599df4f2e912a65c94.zip+Host$+Name~origin-a.akamaihd.net++IP~72.247.182.27+Ck$+Time~55f32cac++Rng~d1f116f7-d1f596f7++CRC~8f73b2ed+Ck$+Time~55f32cba++Rng~d1f596f7-d1fa16f7++CRC~de62f421+Ck$+Time~55f32cbb++Rng~d1fa16f7-d1fe96f7++CRC~6b5e53e3+Ck$+Time~55f32cbb++Rng~d1fe96f7-d20316f7++CRC~545799a9+
**405    wrz 11 19:34:20.029 Warning   Origin::Downloader::UnpackStreamFile::DumpDiagnosticData                    4208      ################# End Downloader Diagnostic Data Dump #################

USAGE:

Please see the 'input.txt' file provided in the sampledata folder.  This tool takes only one parameter, which is this input file.  The input file is conceptually similar to an INI file.

Provide a path to game builds with ZIPFILES.
List corresponding CDN URLs with CDNURL (Optional, tool will attempt to scrape from logs).

Typically, your input data (INPUTFILES) is a folder containing Origin client logs, for example from an OER.

After the tool runs, find the summary at the end of the output file.

EXAMPLE SUMMARY:

******* Summary *******
Data dump # 0  had  2243  chunk records. (Input lines  2761  -  2913 )
Used local zip file:  "fifa_16_g4_pc_patch__ww_x0_ww_patch0_concept_17__final273107_e8a83b43d59945f1b811c2655a9ca488.zip"
Span corresponded to package file:  "data_graphic3_extra.big"  ( 195788909  of  1402453821  bytes)
 -  0  CRC failures.
 -  0  Discontinuities.
 - Unpack Replay FAILED: Error  262149 : -5

<snip>

Data dump # 4  had  20  chunk records. (Input lines  16219  -  16222 )
Used local zip file:  "the_sims_4_pc_fg__ww_us_beta_658__pcbgmlrprodretaildip_110631020_5cd5466e50694b599df4f2e912a65c94.zip"
Span corresponded to package file:  "Data/Client/ClientFullBuild3.package"  ( 88782970  of  1060652280  bytes)
 -  1  CRC failures.
 -  0  Discontinuities.
Used CDN URL:  "http://origin-a.akamaihd.net/eamaster/s/shift/the_sims/the_sims_4/fg_-_ww_us/the_sims_4_pc_fg__ww_us_beta_658__pcbgmlrprodretaildip_110631020_5cd5466e50694b599df4f2e912a65c94.zip"
 -  0  CDN errors.

The basic interpretation of this data is that for dump #0, there is likely a bug in Zlib since there were no CRC failures noted, and the replayed download failed.  (In fact, this is the case)  For
dump #4, the CDN likely returned bad data, since there is 1 CRC failure.  However, the CDN is no longer hosting bad data at that IP, so the CDN check passed successfully.


FUTURE TOOL IMPROVEMENTS:

The Origin client also sends these data dumps through telemetry.  The tool could be improved if it were adapted to parse these from telemetry (TSV) files also.  Note that the telemetry system changes certain
'reserved' characters, so some work would need to be performed on the parser to make this work.