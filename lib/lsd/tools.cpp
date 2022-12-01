#include "tools.h"
#include "BitStream.h"
#include "LenTable.h"

#include <boost/locale.hpp>
#include <fmt/format.h>
#include <map>
#include <assert.h>

namespace dictlsd {

int majorVersion(unsigned dictVersion) {
    return dictVersion >> 16;
}

int minorVersion(unsigned dictVersion) {
    return (dictVersion >> 12) & 0x0F;
}

int revisionVersion(unsigned dictVersion) {
    return dictVersion & 0xFFF;
}

unsigned UpperPrimeNumber(unsigned count) {
    if (count < 0x35)
        return 0x35;
    assert(false);
    return 0;
}

unsigned BitLength(unsigned num) {
    unsigned res = 1;
    while ((num >>= 1) != 0)
        res++;
    return res;
}

std::u16string readUnicodeString(IBitStream* bstr, int len, bool bigEndian) {
    std::u16string str;
    for (int i = 0; i < len; ++i) {
        uint16_t ch;
        bstr->readSome(&ch, 2);
        str += bigEndian ? reverse16(ch) : ch;
    }
    return str;
}

uint16_t reverse16(uint16_t n) {
    char* arr = (char*)&n;
    std::swap(arr[0], arr[1]);
    return n;
}

uint32_t reverse32(uint32_t n) {
    char* arr = (char*)&n;
    std::swap(arr[0], arr[3]);
    std::swap(arr[1], arr[2]);
    return n;
}

std::vector<char32_t> readSymbols(IBitStream* bstr) {
    int len = bstr->read(32);
    int bitsPerSymbol = bstr->read(8);
    std::vector<char32_t> res;
    for (int i = 0; i < len; i++) {
        char32_t symbol = bstr->read(bitsPerSymbol);
        res.push_back(symbol);
    }
    return res;
}

bool readReference(IBitStream& bstr, unsigned& reference, unsigned huffmanNumber) {
    int code = bstr.read(2);
    if (code == 3) {
        reference = bstr.read(32);
        return true;
    }
    int bitlen = BitLength(huffmanNumber);
    assert(bitlen >= 2);
    reference = (code << (bitlen - 2)) | bstr.read(bitlen - 2);
    return true;
}

std::string toUtf8(std::u16string u16str) {
    return boost::locale::conv::utf_to_utf<char>(u16str);
}

std::u16string toUtf16(std::string u8str) {
    return boost::locale::conv::utf_to_utf<char16_t>(u8str);
}

std::map<int, std::u16string> langMap {
    { 1555, u"Abazin" },
    { 1556, u"Abkhaz" },
    { 1557, u"Adyghe" },
    { 1078, u"Afrikaans" },
    { 1559, u"Agul" },
    { 1052, u"Albanian" },
    { 1545, u"Altaic" },
    { 1025, u"Arabic" },
    { 5121, u"ArabicAlgeria" },
    { 15361, u"ArabicBahrain" },
    { 3073, u"ArabicEgypt" },
    { 2049, u"ArabicIraq" },
    { 11265, u"ArabicJordan" },
    { 13313, u"ArabicKuwait" },
    { 12289, u"ArabicLebanon" },
    { 4097, u"ArabicLibya" },
    { 6145, u"ArabicMorocco" },
    { 8193, u"ArabicOman" },
    { 16385, u"ArabicQatar" },
    { 1025, u"ArabicSaudiArabia" },
    { 10241, u"ArabicSyria" },
    { 7169, u"ArabicTunisia" },
    { 14337, u"ArabicUAE" },
    { 9217, u"ArabicYemen" },
    { 1067, u"Armenian" },
    { 1067, u"ArmenianEastern" },
    { 33835, u"ArmenianGrabar" },
    { 32811, u"ArmenianWestern" },
    { 1101, u"Assamese" },
    { 1558, u"Awar" },
    { 1560, u"Aymara" },
    { 2092, u"AzeriCyrillic" },
    { 1068, u"AzeriLatin" },
    { 1561, u"Bashkir" },
    { 1069, u"Basque" },
    { 1059, u"Belarusian" },
    { 1562, u"Bemba" },
    { 1093, u"Bengali" },
    { 1563, u"Blackfoot" },
    { 1536, u"Breton" },
    { 1564, u"Bugotu" },
    { 1026, u"Bulgarian" },
    { 1109, u"Burmese" },
    { 1565, u"Buryat" },
    { 1059, u"Byelorussian" },
    { 1027, u"Catalan" },
    { 1566, u"Chamorro" },
    { 1544, u"Chechen" },
    { 1028, u"Chinese" },
    { 3076, u"ChineseHongKong" },
    { 5124, u"ChineseMacau" },
    { 2052, u"ChinesePRC" },
    { 4100, u"ChineseSingapore" },
    { 1028, u"ChineseTaiwan" },
    { 1074, u"Chuana" },
    { 1567, u"Chukcha" },
    { 1568, u"Chuvash" },
    { 1569, u"Corsican" },
    { 1546, u"CrimeanTatar" },
    { 1050, u"Croatian" },
    { 1570, u"Crow" },
    { 1029, u"Czech" },
    { 1632, u"Dakota" },
    { 1030, u"Danish" },
    { 1571, u"Dargin" },
    { 1571, u"Dargwa" },
    { 1572, u"Dungan" },
    { 1043, u"Dutch" },
    { 2067, u"DutchBelgian" },
    { 1043, u"DutchStandard" },
    { 1033, u"English" },
    { 3081, u"EnglishAustralian" },
    { 10249, u"EnglishBelize" },
    { 4105, u"EnglishCanadian" },
    { 9225, u"EnglishCaribbean" },
    { 6153, u"EnglishIreland" },
    { 8201, u"EnglishJamaica" },
    { 35849, u"EnglishLaw" },
    { 33801, u"EnglishMedical" },
    { 5129, u"EnglishNewZealand" },
    { 13321, u"EnglishPhilippines" },
    { 34825, u"EnglishProperNames" },
    { 7177, u"EnglishSouthAfrica" },
    { 11273, u"EnglishTrinidad" },
    { 2057, u"EnglishUnitedKingdom" },
    { 1033, u"EnglishUnitedStates" },
    { 12297, u"EnglishZimbabwe" },
    { 1573, u"EskimoCyrillic" },
    { 1581, u"EskimoLatin" },
    { 1537, u"Esperanto" },
    { 1061, u"Estonian" },
    { 1574, u"Even" },
    { 1575, u"Evenki" },
    { 1080, u"Faeroese" },
    { 1080, u"Faroese" },
    { 1065, u"Farsi" },
    { 1538, u"Fijian" },
    { 1035, u"Finnish" },
    { 2067, u"Flemish" },
    { 1036, u"French" },
    { 2060, u"FrenchBelgian" },
    { 3084, u"FrenchCanadian" },
    { 5132, u"FrenchLuxembourg" },
    { 6156, u"FrenchMonaco" },
    { 33804, u"FrenchProperNames" },
    { 1036, u"FrenchStandard" },
    { 4108, u"FrenchSwiss" },
    { 1122, u"Frisian" },
    { 1576, u"Frisian_Legacy" },
    { 1577, u"Friulian" },
    { 2108, u"Gaelic" },
    { 1084, u"GaelicScottish" },
    { 1552, u"Gaelic_Legacy" },
    { 1578, u"Gagauz" },
    { 1110, u"Galician" },
    { 1579, u"Galician_Legacy" },
    { 1580, u"Ganda" },
    { 1079, u"Georgian" },
    { 1031, u"German" },
    { 3079, u"GermanAustrian" },
    { 34823, u"GermanLaw" },
    { 5127, u"GermanLiechtenstein" },
    { 4103, u"GermanLuxembourg" },
    { 36871, u"GermanMedical" },
    { 32775, u"GermanNewSpelling" },
    { 35847, u"GermanNewSpellingLaw" },
    { 37895, u"GermanNewSpellingMedical" },
    { 39943, u"GermanNewSpellingProperNames" },
    { 38919, u"GermanProperNames" },
    { 1031, u"GermanStandard" },
    { 2055, u"GermanSwiss" },
    { 1032, u"Greek" },
    { 32776, u"GreekKathareusa" },
    { 1581, u"Greenlandic" },
    { 1140, u"Guarani" },
    { 1582, u"Guarani_Legacy" },
    { 1095, u"Gujarati" },
    { 1583, u"Hani" },
    { 1128, u"Hausa" },
    { 1652, u"Hausa_Legacy" },
    { 1141, u"Hawaiian" },
    { 1539, u"Hawaiian_Legacy" },
    { 1037, u"Hebrew" },
    { 1081, u"Hindi" },
    { 1038, u"Hungarian" },
    { 1039, u"Icelandic" },
    { 1584, u"Ido" },
    { 1057, u"Indonesian" },
    { 1585, u"Ingush" },
    { 1586, u"Interlingua" },
    { 2108, u"Irish" },
    { 1552, u"Irish_Legacy" },
    { 1040, u"Italian" },
    { 33808, u"ItalianProperNames" },
    { 1040, u"ItalianStandard" },
    { 2064, u"ItalianSwiss" },
    { 1041, u"Japanese" },
    { 1548, u"Kabardian" },
    { 1640, u"Kachin" },
    { 1587, u"Kalmyk" },
    { 1099, u"Kannada" },
    { 1589, u"KarachayBalkar" },
    { 1588, u"Karakalpak" },
    { 1120, u"Kashmiri" },
    { 2144, u"KashmiriIndia" },
    { 1590, u"Kasub" },
    { 1591, u"Kawa" },
    { 1087, u"Kazakh" },
    { 1592, u"Khakas" },
    { 1593, u"Khanty" },
    { 1107, u"Khmer" },
    { 1594, u"Kikuyu" },
    { 1595, u"Kirgiz" },
    { 1597, u"KomiPermian" },
    { 1596, u"KomiZyryan" },
    { 1598, u"Kongo" },
    { 1111, u"Konkani" },
    { 1042, u"Korean" },
    { 2066, u"KoreanJohab" },
    { 1599, u"Koryak" },
    { 1600, u"Kpelle" },
    { 1601, u"Kumyk" },
    { 1602, u"Kurdish" },
    { 1603, u"KurdishCyrillic" },
    { 1604, u"Lak" },
    { 1108, u"Lao" },
    { 1083, u"Lappish" },
    { 1142, u"Latin" },
    { 1540, u"Latin_Legacy" },
    { 1062, u"Latvian" },
    { 1655, u"LatvianGothic" },
    { 1605, u"Lezgin" },
    { 1063, u"Lithuanian" },
    { 2087, u"LithuanianClassic" },
    { 1606, u"Luba" },
    { 1071, u"Macedonian" },
    { 1607, u"Malagasy" },
    { 1086, u"Malay" },
    { 2110, u"MalayBruneiDarussalam" },
    { 1086, u"MalayMalaysian" },
    { 1100, u"Malayalam" },
    { 1608, u"Malinke" },
    { 1082, u"Maltese" },
    { 1112, u"Manipuri" },
    { 1609, u"Mansi" },
    { 1153, u"Maori" },
    { 1102, u"Marathi" },
    { 1610, u"Mari" },
    { 1611, u"Maya" },
    { 1612, u"Miao" },
    { 1613, u"Minankabaw" },
    { 1614, u"Mohawk" },
    { 1104, u"Mongol" },
    { 1615, u"Mordvin" },
    { 1616, u"Nahuatl" },
    { 1617, u"Nanai" },
    { 1618, u"Nenets" },
    { 1121, u"Nepali" },
    { 2145, u"NepaliIndia" },
    { 1619, u"Nivkh" },
    { 1620, u"Nogay" },
    { 1044, u"Norwegian" },
    { 1044, u"NorwegianBokmal" },
    { 2068, u"NorwegianNynorsk" },
    { 1621, u"Nyanja" },
    { 1622, u"Occidental" },
    { 1623, u"Ojibway" },
    { 32777, u"OldEnglish" },
    { 32780, u"OldFrench" },
    { 33799, u"OldGerman" },
    { 32784, u"OldItalian" },
    { 1657, u"OldSlavonic" },
    { 32778, u"OldSpanish" },
    { 1096, u"Oriya" },
    { 1547, u"Ossetic" },
    { 1145, u"Papiamento" },
    { 1624, u"Papiamento_Legacy" },
    { 1625, u"PidginEnglish" },
    { 1654, u"Pinyin" },
    { 1045, u"Polish" },
    { 1046, u"Portuguese" },
    { 1046, u"PortugueseBrazilian" },
    { 2070, u"PortugueseStandard" },
    { 1541, u"Provencal" },
    { 1094, u"Punjabi" },
    { 1131, u"Quechua" },
    { 1131, u"QuechuaBolivia" },
    { 2155, u"QuechuaEcuador" },
    { 3179, u"QuechuaPeru" },
    { 1626, u"Quechua_Legacy" },
    { 1047, u"RhaetoRomanic" },
    { 1048, u"Romanian" },
    { 2072, u"RomanianMoldavia" },
    { 1627, u"Romany" },
    { 1628, u"Ruanda" },
    { 1629, u"Rundi" },
    { 1049, u"Russian" },
    { 2073, u"RussianMoldavia" },
    { 34841, u"RussianOldOrtho" },
    { 32793, u"RussianOldSpelling" },
    { 33817, u"RussianProperNames" },
    { 1083, u"Saami" },
    { 1542, u"Samoan" },
    { 1103, u"Sanskrit" },
    { 1630, u"Selkup" },
    { 3098, u"SerbianCyrillic" },
    { 2074, u"SerbianLatin" },
    { 1631, u"Shona" },
    { 1113, u"Sindhi" },
    { 1632, u"Sioux" },
    { 1051, u"Slovak" },
    { 1060, u"Slovenian" },
    { 1143, u"Somali" },
    { 1633, u"Somali_Legacy" },
    { 1070, u"Sorbian" },
    { 1634, u"Sotho" },
    { 1034, u"Spanish" },
    { 11274, u"SpanishArgentina" },
    { 16394, u"SpanishBolivia" },
    { 13322, u"SpanishChile" },
    { 9226, u"SpanishColombia" },
    { 5130, u"SpanishCostaRica" },
    { 7178, u"SpanishDominicanRepublic" },
    { 12298, u"SpanishEcuador" },
    { 17418, u"SpanishElSalvador" },
    { 4106, u"SpanishGuatemala" },
    { 18442, u"SpanishHonduras" },
    { 2058, u"SpanishMexican" },
    { 3082, u"SpanishModernSort" },
    { 19466, u"SpanishNicaragua" },
    { 6154, u"SpanishPanama" },
    { 15370, u"SpanishParaguay" },
    { 10250, u"SpanishPeru" },
    { 33802, u"SpanishProperNames" },
    { 20490, u"SpanishPuertoRico" },
    { 1034, u"SpanishTraditionalSort" },
    { 14346, u"SpanishUruguay" },
    { 8202, u"SpanishVenezuela" },
    { 1635, u"Sunda" },
    { 1072, u"Sutu" },
    { 1089, u"Swahili" },
    { 1636, u"Swazi" },
    { 1053, u"Swedish" },
    { 2077, u"SwedishFinland" },
    { 1637, u"Tabassaran" },
    { 1553, u"Tagalog" },
    { 1639, u"Tahitian" },
    { 1064, u"Tajik" },
    { 1638, u"Tajik_Legacy" },
    { 1097, u"Tamil" },
    { 1092, u"Tatar" },
    { 1098, u"Telugu" },
    { 1054, u"Thai" },
    { 1105, u"Tibet" },
    { 1640, u"Tinpo" },
    { 1641, u"Tongan" },
    { 1073, u"Tsonga" },
    { 1074, u"Tswana" },
    { 1642, u"Tun" },
    { 1055, u"Turkish" },
    { 1090, u"Turkmen" },
    { 1656, u"TurkmenLatin" },
    { 1643, u"Turkmen_Legacy" },
    { 1644, u"Tuvin" },
    { 1645, u"Udmurt" },
    { 1646, u"Uighur" },
    { 1646, u"UighurCyrillic" },
    { 1647, u"UighurLatin" },
    { 1058, u"Ukrainian" },
    { 1653, u"Universal" },
    { 2080, u"UrduIndia" },
    { 1056, u"UrduPakistan" },
    { 1554, u"User" },
    { 2115, u"UzbekCyrillic" },
    { 1091, u"UzbekLatin" },
    { 1075, u"Venda" },
    { 1066, u"Vietnamese" },
    { 1648, u"Visayan" },
    { 1106, u"Welsh" },
    { 1543, u"Welsh_Legacy" },
    { 1070, u"Wend" },
    { 1160, u"Wolof" },
    { 1649, u"Wolof_Legacy" },
    { 1076, u"Xhosa" },
    { 1157, u"Yakut" },
    { 1650, u"Yakut_Legacy" },
    { 1085, u"Yiddish" },
    { 1651, u"Zapotec" },
    { 1077, u"Zulu" },
};

std::u16string langFromCode(int code) {
    if (langMap.find(code) == langMap.end())
        return u"unknown";
    return langMap[code];
}

void printLanguages() {
    for (auto pair : langMap) {
        fmt::print("{} {}\n", pair.first, toUtf8(pair.second));
    }
}

std::ofstream openForWriting(std::filesystem::path path) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error(
            fmt::format("Can't open file for writing: {}", path.u8string()));
    return file;
}

std::ifstream openForReading(std::filesystem::path path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error(
            fmt::format("Can't open file for reading: {}", path.u8string()));
    return file;
}

}
