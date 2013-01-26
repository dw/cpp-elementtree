#include <string>

#include <ctype.h>      // isspace
#include <locale.h>     // setlocale
#include <stdlib.h>     // strtol
#include <string.h>     // strncmp
#include <time.h>       // strptime, time_t

namespace etree {
namespace feed {


// liferea/src/date.c.
static struct tzinfo {
    const char *name;
    int offset;
} tzOffsets_[] = {
    {"A", -100},    {"ACDT", 1030}, {"ACST", 930},   {"ADT", -300},
    {"AEDT", 1100}, {"AEST", 1000}, {"AKDT", -800},  {"AKST", -900},
    {"AST", -400},  {"BT", 300},    {"CDT", -500},   {"CEDT", 200},
    {"CEST", 200},  {"CET", 100},   {"CNST", 800},   {"CST", -600},
    {"EDT", -400},  {"EEDT", 300},  {"EEST", 300},   {"EET", 200},
    {"EST", -500},  {"GMT", 0},     {"HAST", -1000}, {"HDT", -900},
    {"ICT", 700},   {"IDLE", 1200}, {"IDLW", -1200}, {"IRST", 430},
    {"IRT", 330},   {"IST", 100},   {"IST", 530},    {"JST", 900},
    {"M", -1200},   {"MDT", -600},  {"MEST", 200},   {"MESZ", 200},
    {"MEZ", 100},   {"MSD", 400},   {"MSK", 300},    {"MST", -700},
    {"N", 100},     {"NDT", -230},  {"NFT", 1130},   {"NST", -330},
    {"NZDT", 1300}, {"NZST", 1200}, {"PDT", -700},   {"PST", -800},
    {"PT", -800},   {"UT", 0},      {"VST", -430},   {"WEDT", 100},
    {"WEST", 100},  {"WESZ", 100},  {"WET", 0},      {"WEZ", 0},
    {"Y", 1200},    {"YDT", -800},  {"YST", -900},   {"Z", 0}
};

static const int tzOffsetsCount_ = sizeof tzOffsets_ / sizeof tzOffsets_[0];


static void stripWs_(std::string &s)
{
    int start, end;
    for(start = 0; isspace(s[start]) && start < s.size(); start++);
    for(end = s.size() - 1; isspace(s[end]) && end >= 0; end--);
    s = s.substr(start, 1 + end - start);
}


static time_t parseRfc822Tz_(const char *token)
{
    int offset = 0;

    if(*token == '+' || *token == '-') {
        offset = ::strtol(token, NULL, 10);
    } else {
        if(*token == '(') {
            token++;
        }

        for(int i = 0; i < tzOffsetsCount_; i++) {
            struct tzinfo &tz = tzOffsets_[i];
            if(0 == strncmp(token, tz.name, strlen(tz.name))) {
                offset = tz.offset;
                break;
            }
        }
    }
    return 60 * ((offset / 100) * 60 + (offset % 100));
}


bool parseRfc822Date_(std::string s, time_t &out)
{
    static const char *formats[] = {
        "%d %b %Y %H:%M:%S",
        "%d %b %Y %H:%M",
        "%d %b %y %T",
        "%d %b %y %T"
    };
    static const int formatsCount = sizeof formats / sizeof formats[0];

    std::string oldLocale(::setlocale(LC_TIME, NULL));
    setlocale(LC_TIME, "C");

    stripWs_(s);
    s = s.substr(s.find(' ') + 1);

    tm tm;
    ::memset(reinterpret_cast<void *>(&tm), 0, sizeof tm);

    char *npos = NULL;
    for(int i = 0; i < formatsCount && !npos; i++) {
        npos = strptime(s.c_str(), formats[i], &tm);
    }

    ::setlocale(LC_TIME, oldLocale.empty() ? NULL : oldLocale.c_str());
    if(! npos) {
        return false;
    }

    out = mktime(&tm);
    if(out == -1) {
        return false;
    }

    /* GMT time, with no daylight savings time correction. (Usually,
     * there is no daylight savings time since the input is GMT.) */
    out -= parseRfc822Tz_(npos);
    time_t t2 = mktime(gmtime(&out));
    out -= (t2 - out);
    return true;
}


} // namespace
} // namespace
