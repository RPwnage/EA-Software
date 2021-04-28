/*H********************************************************************************/
/*!
    \File lobbylang.h

    \Description
        Country and Language Code Information

    \Notes
        This module provides country and language codes for both client and 
        server code in DirtySDK.
        2-letter language codes are currently available here:
        http://en.wikipedia.org/wiki/ISO_639
        2-letter country codes are currently available here:
        http://en.wikipedia.org/wiki/ISO_3166

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 11/30/2004 (jfrank) First Version
*/
/********************************************************************************H*/

#ifndef _lobbylang_h
#define _lobbylang_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

//! Commonly used locality values
#define LOBBYAPI_LOCALITY_UNKNOWN                       ('zzZZ')
#define LOBBYAPI_LOCALITY_EN_US                         ('enUS')
#define LOBBYAPI_LOCALITY_BLANK                         ('\0\0\0\0')
#define LOBBYAPI_LOCALITY_WILDCARD                      ('****')

#define LOBBYAPI_LOCALITY_DEFAULT                       LOBBYAPI_LOCALITY_EN_US
#define LOBBYAPI_LOCALITY_DEFAULT_STR                   "enUS"
#define LOBBYAPI_LOCALITY_UNKNOWN_STR                   "zzZZ"

//! Non-specific, commonly used country and language codes
#define LOBBYAPI_LANGUAGE_UNKNOWN                       ('zz')
#define LOBBYAPI_COUNTRY_UNKNOWN                        ('ZZ')
#define LOBBYAPI_LANGUAGE_WILDCARD                      ('**')
#define LOBBYAPI_COUNTRY_WILDCARD                       ('**')
#define LOBBYAPI_LANGUAGE_UNKNOWN_STR                   ("zz")
#define LOBBYAPI_COUNTRY_UNKNOWN_STR                    ("ZZ")

//! Languages
#define LOBBYAPI_LANGUAGE_AFAN_OROMO                    ('om')
#define LOBBYAPI_LANGUAGE_ABKHAZIAN                     ('ab')
#define LOBBYAPI_LANGUAGE_AFAR                          ('aa')
#define LOBBYAPI_LANGUAGE_AFRIKAANS                     ('af')
#define LOBBYAPI_LANGUAGE_ALBANIAN                      ('sq')
#define LOBBYAPI_LANGUAGE_AMHARIC                       ('am')
#define LOBBYAPI_LANGUAGE_ARABIC                        ('ar')
#define LOBBYAPI_LANGUAGE_ARMENIAN                      ('hy')
#define LOBBYAPI_LANGUAGE_ASSAMESE                      ('as')
#define LOBBYAPI_LANGUAGE_AYMARA                        ('ay')
#define LOBBYAPI_LANGUAGE_AZERBAIJANI                   ('az')
#define LOBBYAPI_LANGUAGE_BASHKIR                       ('ba')
#define LOBBYAPI_LANGUAGE_BASQUE                        ('eu')
#define LOBBYAPI_LANGUAGE_BENGALI                       ('bn')
#define LOBBYAPI_LANGUAGE_BHUTANI                       ('dz')
#define LOBBYAPI_LANGUAGE_BIHARI                        ('bh')
#define LOBBYAPI_LANGUAGE_BISLAMA                       ('bi')
#define LOBBYAPI_LANGUAGE_BRETON                        ('br')
#define LOBBYAPI_LANGUAGE_BULGARIAN                     ('bg')
#define LOBBYAPI_LANGUAGE_BURMESE                       ('my')
#define LOBBYAPI_LANGUAGE_BYELORUSSIAN                  ('be')
#define LOBBYAPI_LANGUAGE_CAMBODIAN                     ('km')
#define LOBBYAPI_LANGUAGE_CATALAN                       ('ca')
#define LOBBYAPI_LANGUAGE_CHINESE                       ('zh')
#define LOBBYAPI_LANGUAGE_CORSICAN                      ('co')
#define LOBBYAPI_LANGUAGE_CROATIAN                      ('hr')
#define LOBBYAPI_LANGUAGE_CZECH                         ('cs')
#define LOBBYAPI_LANGUAGE_DANISH                        ('da')
#define LOBBYAPI_LANGUAGE_DUTCH                         ('nl')
#define LOBBYAPI_LANGUAGE_ENGLISH                       ('en')
#define LOBBYAPI_LANGUAGE_ESPERANTO                     ('eo')
#define LOBBYAPI_LANGUAGE_ESTONIAN                      ('et')
#define LOBBYAPI_LANGUAGE_FAEROESE                      ('fo')
#define LOBBYAPI_LANGUAGE_FIJI                          ('fj')
#define LOBBYAPI_LANGUAGE_FINNISH                       ('fi')
#define LOBBYAPI_LANGUAGE_FRENCH                        ('fr')
#define LOBBYAPI_LANGUAGE_FRISIAN                       ('fy')
#define LOBBYAPI_LANGUAGE_GALICIAN                      ('gl')
#define LOBBYAPI_LANGUAGE_GEORGIAN                      ('ka')
#define LOBBYAPI_LANGUAGE_GERMAN                        ('de')
#define LOBBYAPI_LANGUAGE_GREEK                         ('el')
#define LOBBYAPI_LANGUAGE_GREENLANDIC                   ('kl')
#define LOBBYAPI_LANGUAGE_GUARANI                       ('gn')
#define LOBBYAPI_LANGUAGE_GUJARATI                      ('gu')
#define LOBBYAPI_LANGUAGE_HAUSA                         ('ha')
#define LOBBYAPI_LANGUAGE_HEBREW                        ('he')
#define LOBBYAPI_LANGUAGE_HINDI                         ('hi')
#define LOBBYAPI_LANGUAGE_HUNGARIAN                     ('hu')
#define LOBBYAPI_LANGUAGE_ICELANDIC                     ('is')
#define LOBBYAPI_LANGUAGE_INDONESIAN                    ('id')
#define LOBBYAPI_LANGUAGE_INTERLINGUA                   ('ia')
#define LOBBYAPI_LANGUAGE_INTERLINGUE                   ('ie')
#define LOBBYAPI_LANGUAGE_INUPIAK                       ('ik')
#define LOBBYAPI_LANGUAGE_INUKTITUT                     ('iu')
#define LOBBYAPI_LANGUAGE_IRISH                         ('ga')
#define LOBBYAPI_LANGUAGE_ITALIAN                       ('it')
#define LOBBYAPI_LANGUAGE_JAPANESE                      ('ja')
#define LOBBYAPI_LANGUAGE_JAVANESE                      ('jw')
#define LOBBYAPI_LANGUAGE_KANNADA                       ('kn')
#define LOBBYAPI_LANGUAGE_KASHMIRI                      ('ks')
#define LOBBYAPI_LANGUAGE_KAZAKH                        ('kk')
#define LOBBYAPI_LANGUAGE_KINYARWANDA                   ('rw')
#define LOBBYAPI_LANGUAGE_KIRGHIZ                       ('ky')
#define LOBBYAPI_LANGUAGE_KIRUNDI                       ('rn')
#define LOBBYAPI_LANGUAGE_KOREAN                        ('ko')
#define LOBBYAPI_LANGUAGE_KURDISH                       ('ku')
#define LOBBYAPI_LANGUAGE_LAOTHIAN                      ('lo')
#define LOBBYAPI_LANGUAGE_LATIN                         ('la')
#define LOBBYAPI_LANGUAGE_LATVIAN_LETTISH               ('lv')
#define LOBBYAPI_LANGUAGE_LINGALA                       ('ln')
#define LOBBYAPI_LANGUAGE_LITHUANIAN                    ('lt')
#define LOBBYAPI_LANGUAGE_MACEDONIAN                    ('mk')
#define LOBBYAPI_LANGUAGE_MALAGASY                      ('mg')
#define LOBBYAPI_LANGUAGE_MALAY                         ('ms')
#define LOBBYAPI_LANGUAGE_MALAYALAM                     ('ml')
#define LOBBYAPI_LANGUAGE_MALTESE                       ('mt')
#define LOBBYAPI_LANGUAGE_MAORI                         ('mi')
#define LOBBYAPI_LANGUAGE_MARATHI                       ('mr')
#define LOBBYAPI_LANGUAGE_MOLDAVIAN                     ('mo')
#define LOBBYAPI_LANGUAGE_MONGOLIAN                     ('mn')
#define LOBBYAPI_LANGUAGE_NAURU                         ('na')
#define LOBBYAPI_LANGUAGE_NEPALI                        ('ne')
#define LOBBYAPI_LANGUAGE_NORWEGIAN                     ('no')
#define LOBBYAPI_LANGUAGE_OCCITAN                       ('oc')
#define LOBBYAPI_LANGUAGE_ORIYA                         ('or')
#define LOBBYAPI_LANGUAGE_PASHTO_PUSHTO                 ('ps')
#define LOBBYAPI_LANGUAGE_PERSIAN                       ('fa')
#define LOBBYAPI_LANGUAGE_POLISH                        ('pl')
#define LOBBYAPI_LANGUAGE_PORTUGUESE                    ('pt')
#define LOBBYAPI_LANGUAGE_PUNJABI                       ('pa')
#define LOBBYAPI_LANGUAGE_QUECHUA                       ('qu')
#define LOBBYAPI_LANGUAGE_RHAETO_ROMANCE                ('rm')
#define LOBBYAPI_LANGUAGE_ROMANIAN                      ('ro')
#define LOBBYAPI_LANGUAGE_RUSSIAN                       ('ru')
#define LOBBYAPI_LANGUAGE_SAMOAN                        ('sm')
#define LOBBYAPI_LANGUAGE_SANGRO                        ('sg')
#define LOBBYAPI_LANGUAGE_SANSKRIT                      ('sa')
#define LOBBYAPI_LANGUAGE_SCOTS_GAELIC                  ('gd')
#define LOBBYAPI_LANGUAGE_SERBIAN                       ('sr')
#define LOBBYAPI_LANGUAGE_SERBO_CROATIAN                ('sh')
#define LOBBYAPI_LANGUAGE_SESOTHO                       ('st')
#define LOBBYAPI_LANGUAGE_SETSWANA                      ('tn')
#define LOBBYAPI_LANGUAGE_SHONA                         ('sn')
#define LOBBYAPI_LANGUAGE_SINDHI                        ('sd')
#define LOBBYAPI_LANGUAGE_SINGHALESE                    ('si')
#define LOBBYAPI_LANGUAGE_SISWATI                       ('ss')
#define LOBBYAPI_LANGUAGE_SLOVAK                        ('sk')
#define LOBBYAPI_LANGUAGE_SLOVENIAN                     ('sl')
#define LOBBYAPI_LANGUAGE_SOMALI                        ('so')
#define LOBBYAPI_LANGUAGE_SPANISH                       ('es')
#define LOBBYAPI_LANGUAGE_SUDANESE                      ('su')
#define LOBBYAPI_LANGUAGE_SWAHILI                       ('sw')
#define LOBBYAPI_LANGUAGE_SWEDISH                       ('sv')
#define LOBBYAPI_LANGUAGE_TAGALOG                       ('tl')
#define LOBBYAPI_LANGUAGE_TAJIK                         ('tg')
#define LOBBYAPI_LANGUAGE_TAMIL                         ('ta')
#define LOBBYAPI_LANGUAGE_TATAR                         ('tt')
#define LOBBYAPI_LANGUAGE_TEGULU                        ('te') // TODO: deprecate v9
#define LOBBYAPI_LANGUAGE_THAI                          ('th')
#define LOBBYAPI_LANGUAGE_TIBETAN                       ('bo')
#define LOBBYAPI_LANGUAGE_TIGRINYA                      ('ti')
#define LOBBYAPI_LANGUAGE_TONGA                         ('to')
#define LOBBYAPI_LANGUAGE_TSONGA                        ('ts')
#define LOBBYAPI_LANGUAGE_TURKISH                       ('tr')
#define LOBBYAPI_LANGUAGE_TURKMEN                       ('tk')
#define LOBBYAPI_LANGUAGE_TWI                           ('tw')
#define LOBBYAPI_LANGUAGE_UIGUR                         ('ug') // TODO: deprecate v9
#define LOBBYAPI_LANGUAGE_UKRAINIAN                     ('uk')
#define LOBBYAPI_LANGUAGE_URDU                          ('ur')
#define LOBBYAPI_LANGUAGE_UZBEK                         ('uz')
#define LOBBYAPI_LANGUAGE_VIETNAMESE                    ('vi')
#define LOBBYAPI_LANGUAGE_VOLAPUK                       ('vo')
#define LOBBYAPI_LANGUAGE_WELCH                         ('cy') // TODO: deprecate v9
#define LOBBYAPI_LANGUAGE_WOLOF                         ('wo')
#define LOBBYAPI_LANGUAGE_XHOSA                         ('xh')
#define LOBBYAPI_LANGUAGE_YIDDISH                       ('yi')
#define LOBBYAPI_LANGUAGE_YORUBA                        ('yo')
#define LOBBYAPI_LANGUAGE_ZHUANG                        ('za')
#define LOBBYAPI_LANGUAGE_ZULU                          ('zu')

// Languages: corrections
#define LOBBYAPI_LANGUAGE_TELUGU                        ('te')
#define LOBBYAPI_LANGUAGE_UIGHUR                        ('ug')
#define LOBBYAPI_LANGUAGE_WELSH                         ('cy')

// Languages: added on Mar-25-2011 according to ISO 639-1
#define LOBBYAPI_LANGUAGE_BOSNIAN                       ('bs')
#define LOBBYAPI_LANGUAGE_DIVEHI                        ('dv')
#define LOBBYAPI_LANGUAGE_IGBO                          ('ig')
#define LOBBYAPI_LANGUAGE_LUXEMBOURGISH                 ('lb')
#define LOBBYAPI_LANGUAGE_YI                            ('ii')
#define LOBBYAPI_LANGUAGE_NORWEGIAN_BOKMAL              ('nb')
#define LOBBYAPI_LANGUAGE_NORWEGIAN_NYNORSK             ('nn')
#define LOBBYAPI_LANGUAGE_SAMI                          ('se')

// Default language
#define LOBBYAPI_LANGUAGE_DEFAULT                       LOBBYAPI_LANGUAGE_ENGLISH
#define LOBBYAPI_LANGUAGE_DEFAULT_STR                   "en"

//! Countries
#define LOBBYAPI_COUNTRY_AFGHANISTAN                                        ('AF')
#define LOBBYAPI_COUNTRY_ALBANIA                                            ('AL')
#define LOBBYAPI_COUNTRY_ALGERIA                                            ('DZ')
#define LOBBYAPI_COUNTRY_AMERICAN_SAMOA                                     ('AS')
#define LOBBYAPI_COUNTRY_ANDORRA                                            ('AD')
#define LOBBYAPI_COUNTRY_ANGOLA                                             ('AO')
#define LOBBYAPI_COUNTRY_ANGUILLA                                           ('AI')
#define LOBBYAPI_COUNTRY_ANTARCTICA                                         ('AQ')
#define LOBBYAPI_COUNTRY_ANTIGUA_BARBUDA                                    ('AG')
#define LOBBYAPI_COUNTRY_ARGENTINA                                          ('AR')
#define LOBBYAPI_COUNTRY_ARMENIA                                            ('AM')
#define LOBBYAPI_COUNTRY_ARUBA                                              ('AW')
#define LOBBYAPI_COUNTRY_AUSTRALIA                                          ('AU')
#define LOBBYAPI_COUNTRY_AUSTRIA                                            ('AT')
#define LOBBYAPI_COUNTRY_AZERBAIJAN                                         ('AZ')
#define LOBBYAPI_COUNTRY_BAHAMAS                                            ('BS')
#define LOBBYAPI_COUNTRY_BAHRAIN                                            ('BH')
#define LOBBYAPI_COUNTRY_BANGLADESH                                         ('BD')
#define LOBBYAPI_COUNTRY_BARBADOS                                           ('BB')
#define LOBBYAPI_COUNTRY_BELARUS                                            ('BY')
#define LOBBYAPI_COUNTRY_BELGIUM                                            ('BE')
#define LOBBYAPI_COUNTRY_BELIZE                                             ('BZ')
#define LOBBYAPI_COUNTRY_BENIN                                              ('BJ')
#define LOBBYAPI_COUNTRY_BERMUDA                                            ('BM')
#define LOBBYAPI_COUNTRY_BHUTAN                                             ('BT')
#define LOBBYAPI_COUNTRY_BOLIVIA                                            ('BO')
#define LOBBYAPI_COUNTRY_BOSNIA_HERZEGOVINA                                 ('BA')
#define LOBBYAPI_COUNTRY_BOTSWANA                                           ('BW')
#define LOBBYAPI_COUNTRY_BOUVET_ISLAND                                      ('BV')
#define LOBBYAPI_COUNTRY_BRAZIL                                             ('BR')
#define LOBBYAPI_COUNTRY_BRITISH_INDIAN_OCEAN_TERRITORY                     ('IO')
#define LOBBYAPI_COUNTRY_BRUNEI_DARUSSALAM                                  ('BN')
#define LOBBYAPI_COUNTRY_BULGARIA                                           ('BG')
#define LOBBYAPI_COUNTRY_BURKINA_FASO                                       ('BF')
#define LOBBYAPI_COUNTRY_BURUNDI                                            ('BI')
#define LOBBYAPI_COUNTRY_CAMBODIA                                           ('KH')
#define LOBBYAPI_COUNTRY_CAMEROON                                           ('CM')
#define LOBBYAPI_COUNTRY_CANADA                                             ('CA')
#define LOBBYAPI_COUNTRY_CAPE_VERDE                                         ('CV')
#define LOBBYAPI_COUNTRY_CAYMAN_ISLANDS                                     ('KY')
#define LOBBYAPI_COUNTRY_CENTRAL_AFRICAN_REPUBLIC                           ('CF')
#define LOBBYAPI_COUNTRY_CHAD                                               ('TD')
#define LOBBYAPI_COUNTRY_CHILE                                              ('CL')
#define LOBBYAPI_COUNTRY_CHINA                                              ('CN')
#define LOBBYAPI_COUNTRY_CHRISTMAS_ISLAND                                   ('CX')
#define LOBBYAPI_COUNTRY_COCOS_KEELING_ISLANDS                              ('CC')
#define LOBBYAPI_COUNTRY_COLOMBIA                                           ('CO')
#define LOBBYAPI_COUNTRY_COMOROS                                            ('KM')
#define LOBBYAPI_COUNTRY_CONGO                                              ('CG')
#define LOBBYAPI_COUNTRY_COOK_ISLANDS                                       ('CK')
#define LOBBYAPI_COUNTRY_COSTA_RICA                                         ('CR')
#define LOBBYAPI_COUNTRY_COTE_DIVOIRE                                       ('CI')
#define LOBBYAPI_COUNTRY_CROATIA                                            ('HR')
#define LOBBYAPI_COUNTRY_CUBA                                               ('CU')
#define LOBBYAPI_COUNTRY_CYPRUS                                             ('CY')
#define LOBBYAPI_COUNTRY_CZECH_REPUBLIC                                     ('CZ')
#define LOBBYAPI_COUNTRY_DENMARK                                            ('DK')
#define LOBBYAPI_COUNTRY_DJIBOUTI                                           ('DJ')
#define LOBBYAPI_COUNTRY_DOMINICA                                           ('DM')
#define LOBBYAPI_COUNTRY_DOMINICAN_REPUBLIC                                 ('DO')
#define LOBBYAPI_COUNTRY_EAST_TIMOR                                         ('TP')
#define LOBBYAPI_COUNTRY_ECUADOR                                            ('EC')
#define LOBBYAPI_COUNTRY_EGYPT                                              ('EG')
#define LOBBYAPI_COUNTRY_EL_SALVADOR                                        ('SV')
#define LOBBYAPI_COUNTRY_EQUATORIAL_GUINEA                                  ('GQ')
#define LOBBYAPI_COUNTRY_ERITREA                                            ('ER')
#define LOBBYAPI_COUNTRY_ESTONIA                                            ('EE')
#define LOBBYAPI_COUNTRY_ETHIOPIA                                           ('ET')
#define LOBBYAPI_COUNTRY_EUROPE_SSGFI_ONLY                                  ('EU')
#define LOBBYAPI_COUNTRY_FALKLAND_ISLANDS                                   ('FK')
#define LOBBYAPI_COUNTRY_FAEROE_ISLANDS                                     ('FO')
#define LOBBYAPI_COUNTRY_FIJI                                               ('FJ')
#define LOBBYAPI_COUNTRY_FINLAND                                            ('FI')
#define LOBBYAPI_COUNTRY_FRANCE                                             ('FR')
#define LOBBYAPI_COUNTRY_FRANCE_METROPOLITAN                                ('FX')
#define LOBBYAPI_COUNTRY_FRENCH_GUIANA                                      ('GF')
#define LOBBYAPI_COUNTRY_FRENCH_POLYNESIA                                   ('PF')
#define LOBBYAPI_COUNTRY_FRENCH_SOUTHERN_TERRITORIES                        ('TF')
#define LOBBYAPI_COUNTRY_GABON                                              ('GA')
#define LOBBYAPI_COUNTRY_GAMBIA                                             ('GM')
#define LOBBYAPI_COUNTRY_GEORGIA                                            ('GE')
#define LOBBYAPI_COUNTRY_GERMANY                                            ('DE')
#define LOBBYAPI_COUNTRY_GHANA                                              ('GH')
#define LOBBYAPI_COUNTRY_GIBRALTAR                                          ('GI')
#define LOBBYAPI_COUNTRY_GREECE                                             ('GR')
#define LOBBYAPI_COUNTRY_GREENLAND                                          ('GL')
#define LOBBYAPI_COUNTRY_GRENADA                                            ('GD')
#define LOBBYAPI_COUNTRY_GUADELOUPE                                         ('GP')
#define LOBBYAPI_COUNTRY_GUAM                                               ('GU')
#define LOBBYAPI_COUNTRY_GUATEMALA                                          ('GT')
#define LOBBYAPI_COUNTRY_GUINEA                                             ('GN')
#define LOBBYAPI_COUNTRY_GUINEA_BISSAU                                      ('GW')
#define LOBBYAPI_COUNTRY_GUYANA                                             ('GY')
#define LOBBYAPI_COUNTRY_HAITI                                              ('HT')
#define LOBBYAPI_COUNTRY_HEARD_AND_MC_DONALD_ISLANDS                        ('HM')
#define LOBBYAPI_COUNTRY_HONDURAS                                           ('HN')
#define LOBBYAPI_COUNTRY_HONG_KONG                                          ('HK')
#define LOBBYAPI_COUNTRY_HUNGARY                                            ('HU')
#define LOBBYAPI_COUNTRY_ICELAND                                            ('IS')
#define LOBBYAPI_COUNTRY_INDIA                                              ('IN')
#define LOBBYAPI_COUNTRY_INDONESIA                                          ('ID')
#define LOBBYAPI_COUNTRY_INTERNATIONAL_SSGFI_ONLY                           ('II')
#define LOBBYAPI_COUNTRY_IRAN                                               ('IR')
#define LOBBYAPI_COUNTRY_IRAQ                                               ('IQ')
#define LOBBYAPI_COUNTRY_IRELAND                                            ('IE')
#define LOBBYAPI_COUNTRY_ISRAEL                                             ('IL')
#define LOBBYAPI_COUNTRY_ITALY                                              ('IT')
#define LOBBYAPI_COUNTRY_JAMAICA                                            ('JM')
#define LOBBYAPI_COUNTRY_JAPAN                                              ('JP')
#define LOBBYAPI_COUNTRY_JORDAN                                             ('JO')
#define LOBBYAPI_COUNTRY_KAZAKHSTAN                                         ('KZ')
#define LOBBYAPI_COUNTRY_KENYA                                              ('KE')
#define LOBBYAPI_COUNTRY_KIRIBATI                                           ('KI')
#define LOBBYAPI_COUNTRY_KOREA_DEMOCRATIC_PEOPLES_REPUBLIC_OF               ('KP')
#define LOBBYAPI_COUNTRY_KOREA_REPUBLIC_OF                                  ('KR')
#define LOBBYAPI_COUNTRY_KUWAIT                                             ('KW')
#define LOBBYAPI_COUNTRY_KYRGYZSTAN                                         ('KG')
#define LOBBYAPI_COUNTRY_LAO_PEOPLES_DEMOCRATIC_REPUBLIC                    ('LA')
#define LOBBYAPI_COUNTRY_LATVIA                                             ('LV')
#define LOBBYAPI_COUNTRY_LEBANON                                            ('LB')
#define LOBBYAPI_COUNTRY_LESOTHO                                            ('LS')
#define LOBBYAPI_COUNTRY_LIBERIA                                            ('LR')
#define LOBBYAPI_COUNTRY_LIBYAN_ARAB_JAMAHIRIYA                             ('LY')
#define LOBBYAPI_COUNTRY_LIECHTENSTEIN                                      ('LI')
#define LOBBYAPI_COUNTRY_LITHUANIA                                          ('LT')
#define LOBBYAPI_COUNTRY_LUXEMBOURG                                         ('LU')
#define LOBBYAPI_COUNTRY_MACAU                                              ('MO')
#define LOBBYAPI_COUNTRY_MACEDONIA_THE_FORMER_YUGOSLAV_REPUBLIC_OF          ('MK')
#define LOBBYAPI_COUNTRY_MADAGASCAR                                         ('MG')
#define LOBBYAPI_COUNTRY_MALAWI                                             ('MW')
#define LOBBYAPI_COUNTRY_MALAYSIA                                           ('MY')
#define LOBBYAPI_COUNTRY_MALDIVES                                           ('MV')
#define LOBBYAPI_COUNTRY_MALI                                               ('ML')
#define LOBBYAPI_COUNTRY_MALTA                                              ('MT')
#define LOBBYAPI_COUNTRY_MARSHALL_ISLANDS                                   ('MH')
#define LOBBYAPI_COUNTRY_MARTINIQUE                                         ('MQ')
#define LOBBYAPI_COUNTRY_MAURITANIA                                         ('MR')
#define LOBBYAPI_COUNTRY_MAURITIUS                                          ('MU')
#define LOBBYAPI_COUNTRY_MAYOTTE                                            ('YT')
#define LOBBYAPI_COUNTRY_MEXICO                                             ('MX')
#define LOBBYAPI_COUNTRY_MICRONESIA_FEDERATED_STATES_OF                     ('FM')
#define LOBBYAPI_COUNTRY_MOLDOVA_REPUBLIC_OF                                ('MD')
#define LOBBYAPI_COUNTRY_MONACO                                             ('MC')
#define LOBBYAPI_COUNTRY_MONGOLIA                                           ('MN')
#define LOBBYAPI_COUNTRY_MONTSERRAT                                         ('MS')
#define LOBBYAPI_COUNTRY_MOROCCO                                            ('MA')
#define LOBBYAPI_COUNTRY_MOZAMBIQUE                                         ('MZ')
#define LOBBYAPI_COUNTRY_MYANMAR                                            ('MM')
#define LOBBYAPI_COUNTRY_NAMIBIA                                            ('NA')
#define LOBBYAPI_COUNTRY_NAURU                                              ('NR')
#define LOBBYAPI_COUNTRY_NEPAL                                              ('NP')
#define LOBBYAPI_COUNTRY_NETHERLANDS                                        ('NL')
#define LOBBYAPI_COUNTRY_NETHERLANDS_ANTILLES                               ('AN')
#define LOBBYAPI_COUNTRY_NEW_CALEDONIA                                      ('NC')
#define LOBBYAPI_COUNTRY_NEW_ZEALAND                                        ('NZ')
#define LOBBYAPI_COUNTRY_NICARAGUA                                          ('NI')
#define LOBBYAPI_COUNTRY_NIGER                                              ('NE')
#define LOBBYAPI_COUNTRY_NIGERIA                                            ('NG')
#define LOBBYAPI_COUNTRY_NIUE                                               ('NU')
#define LOBBYAPI_COUNTRY_NORFOLK_ISLAND                                     ('NF')
#define LOBBYAPI_COUNTRY_NORTHERN_MARIANA_ISLANDS                           ('MP')
#define LOBBYAPI_COUNTRY_NORWAY                                             ('NO')
#define LOBBYAPI_COUNTRY_OMAN                                               ('OM')
#define LOBBYAPI_COUNTRY_PAKISTAN                                           ('PK')
#define LOBBYAPI_COUNTRY_PALAU                                              ('PW')
#define LOBBYAPI_COUNTRY_PANAMA                                             ('PA')
#define LOBBYAPI_COUNTRY_PAPUA_NEW_GUINEA                                   ('PG')
#define LOBBYAPI_COUNTRY_PARAGUAY                                           ('PY')
#define LOBBYAPI_COUNTRY_PERU                                               ('PE')
#define LOBBYAPI_COUNTRY_PHILIPPINES                                        ('PH')
#define LOBBYAPI_COUNTRY_PITCAIRN                                           ('PN')
#define LOBBYAPI_COUNTRY_POLAND                                             ('PL')
#define LOBBYAPI_COUNTRY_PORTUGAL                                           ('PT')
#define LOBBYAPI_COUNTRY_PUERTO_RICO                                        ('PR')
#define LOBBYAPI_COUNTRY_QATAR                                              ('QA')
#define LOBBYAPI_COUNTRY_REUNION                                            ('RE')
#define LOBBYAPI_COUNTRY_ROMANIA                                            ('RO')
#define LOBBYAPI_COUNTRY_RUSSIAN_FEDERATION                                 ('RU')
#define LOBBYAPI_COUNTRY_RWANDA                                             ('RW')
#define LOBBYAPI_COUNTRY_SAINT_KITTS_AND_NEVIS                              ('KN')
#define LOBBYAPI_COUNTRY_SAINT_LUCIA                                        ('LC')
#define LOBBYAPI_COUNTRY_SAINT_VINCENT_AND_THE_GRENADINES                   ('VC')
#define LOBBYAPI_COUNTRY_SAMOA                                              ('WS')
#define LOBBYAPI_COUNTRY_SAN_MARINO                                         ('SM')
#define LOBBYAPI_COUNTRY_SAO_TOME_AND_PRINCIPE                              ('ST')
#define LOBBYAPI_COUNTRY_SAUDI_ARABIA                                       ('SA')
#define LOBBYAPI_COUNTRY_SENEGAL                                            ('SN')
#define LOBBYAPI_COUNTRY_SEYCHELLES                                         ('SC')
#define LOBBYAPI_COUNTRY_SIERRA_LEONE                                       ('SL')
#define LOBBYAPI_COUNTRY_SINGAPORE                                          ('SG')
#define LOBBYAPI_COUNTRY_SLOVAKIA_                                          ('SK') // TODO: deprecate v9
#define LOBBYAPI_COUNTRY_SLOVENIA                                           ('SI')
#define LOBBYAPI_COUNTRY_SOLOMON_ISLANDS                                    ('SB')
#define LOBBYAPI_COUNTRY_SOMALIA                                            ('SO')
#define LOBBYAPI_COUNTRY_SOUTH_AFRICA                                       ('ZA')
#define LOBBYAPI_COUNTRY_SOUTH_GEORGIA_AND_THE_SOUTH_SANDWICH_ISLANDS       ('GS')
#define LOBBYAPI_COUNTRY_SPAIN                                              ('ES')
#define LOBBYAPI_COUNTRY_SRI_LANKA                                          ('LK')
#define LOBBYAPI_COUNTRY_ST_HELENA                                          ('SH') // TODO: deprecate v9
#define LOBBYAPI_COUNTRY_ST_PIERRE_AND_MIQUELON                             ('PM')
#define LOBBYAPI_COUNTRY_SUDAN                                              ('SD')
#define LOBBYAPI_COUNTRY_SURINAME                                           ('SR')
#define LOBBYAPI_COUNTRY_SVALBARD_AND_JAN_MAYEN_ISLANDS                     ('SJ')
#define LOBBYAPI_COUNTRY_SWAZILAND                                          ('SZ')
#define LOBBYAPI_COUNTRY_SWEDEN                                             ('SE')
#define LOBBYAPI_COUNTRY_SWITZERLAND                                        ('CH')
#define LOBBYAPI_COUNTRY_SYRIAN_ARAB_REPUBLIC                               ('SY')
#define LOBBYAPI_COUNTRY_TAIWAN                                             ('TW')
#define LOBBYAPI_COUNTRY_TAJIKISTAN                                         ('TJ')
#define LOBBYAPI_COUNTRY_TANZANIA_UNITED_REPUBLIC_OF                        ('TZ')
#define LOBBYAPI_COUNTRY_THAILAND                                           ('TH')
#define LOBBYAPI_COUNTRY_TOGO                                               ('TG')
#define LOBBYAPI_COUNTRY_TOKELAU                                            ('TK')
#define LOBBYAPI_COUNTRY_TONGA                                              ('TO')
#define LOBBYAPI_COUNTRY_TRINIDAD_AND_TOBAGO                                ('TT')
#define LOBBYAPI_COUNTRY_TUNISIA                                            ('TN')
#define LOBBYAPI_COUNTRY_TURKEY                                             ('TR')
#define LOBBYAPI_COUNTRY_TURKMENISTAN                                       ('TM')
#define LOBBYAPI_COUNTRY_TURKS_AND_CAICOS_ISLANDS                           ('TC')
#define LOBBYAPI_COUNTRY_TUVALU                                             ('TV')
#define LOBBYAPI_COUNTRY_UGANDA                                             ('UG')
#define LOBBYAPI_COUNTRY_UKRAINE                                            ('UA')
#define LOBBYAPI_COUNTRY_UNITED_ARAB_EMIRATES                               ('AE')
#define LOBBYAPI_COUNTRY_UNITED_KINGDOM                                     ('GB')
#define LOBBYAPI_COUNTRY_UNITED_STATES                                      ('US')
#define LOBBYAPI_COUNTRY_UNITED_STATES_MINOR_OUTLYING_ISLANDS               ('UM')
#define LOBBYAPI_COUNTRY_URUGUAY                                            ('UY')
#define LOBBYAPI_COUNTRY_UZBEKISTAN                                         ('UZ')
#define LOBBYAPI_COUNTRY_VANUATU                                            ('VU')
#define LOBBYAPI_COUNTRY_VATICAN_CITY_STATE                                 ('VA')
#define LOBBYAPI_COUNTRY_VENEZUELA                                          ('VE')
#define LOBBYAPI_COUNTRY_VIETNAM                                            ('VN')
#define LOBBYAPI_COUNTRY_VIRGIN_ISLANDS_BRITISH                             ('VG')
#define LOBBYAPI_COUNTRY_VIRGIN_ISLANDS_US                                  ('VI')
#define LOBBYAPI_COUNTRY_WALLIS_AND_FUTUNA_ISLANDS                          ('WF')
#define LOBBYAPI_COUNTRY_WESTERN_SAHARA                                     ('EH')
#define LOBBYAPI_COUNTRY_YEMEN                                              ('YE')
#define LOBBYAPI_COUNTRY_YUGOSLAVIA                                         ('YU')
#define LOBBYAPI_COUNTRY_ZAIRE                                              ('ZR')
#define LOBBYAPI_COUNTRY_ZAMBIA                                             ('ZM')
#define LOBBYAPI_COUNTRY_ZIMBABWE                                           ('ZW')

// Countries: correction
#define LOBBYAPI_COUNTRY_SLOVAKIA                                           ('SK')

// Countries: added on Mar-25-2011
#define LOBBYAPI_COUNTRY_SERBIA_AND_MONTENEGRO                              ('CS')
#define LOBBYAPI_COUNTRY_MONTENEGRO                                         ('ME')
#define LOBBYAPI_COUNTRY_SERBIA                                             ('RS')
#define LOBBYAPI_COUNTRY_CONGO_DRC                                          ('CD')
#define LOBBYAPI_COUNTRY_PALESTINIAN_TERRITORY                              ('PS')
#define LOBBYAPI_COUNTRY_GUERNSEY                                           ('GG')
#define LOBBYAPI_COUNTRY_JERSEY                                             ('JE')
#define LOBBYAPI_COUNTRY_ISLE_OF_MAN                                        ('IM')
#define LOBBYAPI_COUNTRY_TIMOR_LESTE                                        ('TL')
#define LOBBYAPI_COUNTRY_ST_HELENA_ASCENSION_AND_TRISTAN_DA_CUNHA           ('SH')

// Default country
#define LOBBYAPI_COUNTRY_DEFAULT                    LOBBYAPI_COUNTRY_UNITED_STATES
#define LOBBYAPI_COUNTRY_DEFAULT_STR                "US"

/*** Macros ***********************************************************************/

//! toupper replacement
#define LOBBYAPI_LocalizerTokenToUpper(uCharToModify)                                       \
    ((((unsigned char)(uCharToModify) >= 'a') && ((unsigned char)(uCharToModify) <= 'z')) ? \
            (((unsigned char)(uCharToModify)) & (~32)) : \
            (uCharToModify))                   

//! tolower replacement
#define LOBBYAPI_LocalizerTokenToLower(uCharToModify)                                       \
    ((((unsigned char)(uCharToModify) >= 'A') && ((unsigned char)(uCharToModify) <= 'Z')) ? \
            (((unsigned char)(uCharToModify)) | (32)) :                                     \
            (uCharToModify))                   

//! Create a localizer token from shorts representing country and language
#define LOBBYAPI_LocalizerTokenCreate(uLanguage, uCountry)  \
    (((LOBBYAPI_LocalizerTokenShortToLower(uLanguage)) << 16) + (LOBBYAPI_LocalizerTokenShortToUpper(uCountry)))

//! Create a localizer token from strings containing country and language
#define LOBBYAPI_LocalizerTokenCreateFromStrings(strLanguage, strCountry) \
    (LOBBYAPI_LocalizerTokenCreate(LOBBYAPI_LocalizerTokenGetShortFromString(strLanguage),LOBBYAPI_LocalizerTokenGetShortFromString(strCountry)))

//! Create a localizer token from a single string (language + country - ex: "enUS")
#define LOBBYAPI_LocalizerTokenCreateFromString(strLocality) \
    (LOBBYAPI_LocalizerTokenCreateFromStrings(&(strLocality)[0], &(strLocality)[2]))

//! Get a int16_t integer from a string
#define LOBBYAPI_LocalizerTokenGetShortFromString(strInstring) (( (((unsigned char*)(strInstring) == NULL) || ((unsigned char*)strInstring)[0] == '\0')) ? \
                        ((uint16_t)(0)) : \
                        ((uint16_t)((((unsigned char*)strInstring)[0] << 8) | ((unsigned char*)strInstring)[1])))

//! Pull the country (int16_t) from a localizer token (int32_t)
#define LOBBYAPI_LocalizerTokenGetCountry(uToken)           ((uint16_t)((uToken) & 0xFFFF))

//! Pull the language (int16_t) from a localizer token (int32_t)
#define LOBBYAPI_LocalizerTokenGetLanguage(uToken)          ((uint16_t)(((uToken) >> 16) & 0xFFFF))

//! Replace the country in a locality value
#define LOBBYAPI_LocalizerTokenSetCountry(uToken, uCountry)    (uToken) = (((uToken) & 0xFFFF0000) | (uCountry));

//! Replace the language in a locality value
#define LOBBYAPI_LocalizerTokenSetLanguage(uToken, uLanguage)  (uToken) = (((uToken) & 0x0000FFFF) | ((uLanguage) << 16));

//! Write the country contained in a localizer token to a character string
#define LOBBYAPI_LocalizerTokenCreateCountryString(strOutstring, uToken)  \
    {   \
        (strOutstring)[0] = (char)(((uToken) >> 8) & 0xFF); \
        (strOutstring)[1] = (char)((uToken) & 0xFF); \
        (strOutstring)[2]='\0'; \
    }

//! Write the language contained in a localizer token to a character string
#define LOBBYAPI_LocalizerTokenCreateLanguageString(strOutstring, uToken)  \
    {   \
        (strOutstring)[0]=(char)(((uToken) >> 24) & 0xFF); \
        (strOutstring)[1]=(char)(((uToken) >> 16) & 0xFF); \
        (strOutstring)[2]='\0'; \
    }

//! Write the entire locality string to a character string
#define LOBBYAPI_LocalizerTokenCreateLocalityString(strOutstring, uToken)  \
    {   \
        (strOutstring)[0]=(char)(((uToken) >> 24) & 0xFF); \
        (strOutstring)[1]=(char)(((uToken) >> 16) & 0xFF); \
        (strOutstring)[2]=(char)(((uToken) >> 8) & 0xFF); \
        (strOutstring)[3]=(char)((uToken) & 0xFF); \
        (strOutstring)[4]='\0'; \
    }

//! Macro to provide and easy way to display the token in character format
#define LOBBYAPI_LocalizerTokenPrintCharArray(uToken)  \
    (char)(((uToken)>>24)&0xFF), (char)(((uToken)>>16)&0xFF), (char)(((uToken)>>8)&0xFF), (char)((uToken)&0xFF)

//! Provide a way to capitalize the elements in a int16_t
#define LOBBYAPI_LocalizerTokenShortToUpper(uShort)  \
    ((uint16_t)(((LOBBYAPI_LocalizerTokenToUpper(((uShort) >> 8) & 0xFF)) << 8) + \
                       (LOBBYAPI_LocalizerTokenToUpper((uShort) & 0xFF))))

//! Provide a way to lowercase the elements in a int16_t
#define LOBBYAPI_LocalizerTokenShortToLower(uShort)  \
    ((uint16_t)(((LOBBYAPI_LocalizerTokenToLower(((uShort) >> 8) & 0xFF)) << 8) + \
                       (LOBBYAPI_LocalizerTokenToLower((uShort) & 0xFF))))

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#endif // _lobbylang_h

