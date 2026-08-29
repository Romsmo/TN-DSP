#include "doctest/doctest.h"

#include <cstring>
#include <set>
#include <string>

#include "ui/i18n/i18n.h"

using ui::i18n::Str;
using ui::i18n::t;

TEST_CASE("every string id resolves to non-empty German text") {
    for (int i = 0; i < static_cast<int>(Str::_Count); ++i) {
        const char* s = t(static_cast<Str>(i));
        REQUIRE(s != nullptr);
        CHECK(std::strlen(s) > 0);
    }
}

TEST_CASE("known strings are what the sketch shows") {
    CHECK(std::string(t(Str::ReportTitle)) == "Melden");
    CHECK(std::string(t(Str::Yes)) == "Ja");
    CHECK(std::string(t(Str::No)) == "Nein");
    CHECK(std::string(t(Str::ConfirmQuestion)) == "Gesehen?");
    CHECK(std::string(t(Str::CatTraffic)) == "Stau");
    // umlauts survive as UTF-8
    CHECK(std::string(t(Str::CatTrailerCam)) == "Anh\xC3\xA4nger");
}

TEST_CASE("out-of-range id is safe") {
    CHECK(std::string(t(static_cast<Str>(9999))).empty());
    CHECK(std::string(t(static_cast<Str>(-1))).empty());
}
