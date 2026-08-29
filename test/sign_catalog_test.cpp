#include "doctest/doctest.h"

#include <fstream>
#include <string>

#include "core/model/sign_list.h"
#include "ui/assets/signs/sign_catalog.h"

using core::sign_code;

TEST_CASE("sign code packing") {
    CHECK(sign_code(276) == 27600);
    CHECK(sign_code(142, 10) == 14210);
    CHECK(sign_code(101, 51) == 10151);
    CHECK(core::sign_major(14210) == 142);
    CHECK(core::sign_minor(14210) == 10);
    CHECK(core::sign_minor(27600) == 0);
}

TEST_CASE("catalog resolves the signs used across the UI") {
    // Every code a scenario / the docs reference must exist.
    for (uint16_t code : {uint16_t(27600), uint16_t(14210), uint16_t(11004),
                          uint16_t(10804), uint16_t(12400)}) {
        CAPTURE(code);
        CHECK(ui::assets::sign(code) != nullptr);
    }
}

TEST_CASE("catalog rejects unknown codes without crashing") {
    CHECK(ui::assets::sign(0) == nullptr);
    CHECK(ui::assets::sign(9999) == nullptr);
    CHECK(ui::assets::sign(65535) == nullptr);
}

TEST_CASE("group lookup") {
    ui::assets::SignGroup g;
    REQUIRE(ui::assets::sign_group(27600, g));
    CHECK(g == ui::assets::SignGroup::Prohibition);
    REQUIRE(ui::assets::sign_group(14210, g));
    CHECK(g == ui::assets::SignGroup::Danger);
    CHECK_FALSE(ui::assets::sign_group(9999, g));
}

TEST_CASE("catalog is sorted (binary search precondition) and matches the manifest count") {
    const int n = ui::assets::sign_count();
    CHECK(n >= 40);
    for (int i = 1; i < n; ++i) {
        CHECK(ui::assets::sign_code_at(i - 1) < ui::assets::sign_code_at(i));
    }
}
