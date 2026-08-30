#include "doctest/doctest.h"

#include "core/bus/spsc_queue.h"
#include "core/cache/cache_store.h"
#include "core/cache/offline_cache.h"
#include "core/model/display_state.h"
#include "core/model/hazard_type.h"
#include "core/settings/settings_store.h"
#include "core/state/warning_filter_apply.h"
#include "test/fakes.h"

using namespace core;

TEST_CASE("SpscQueue: bounded FIFO, overwrite mode keeps latest") {
    SpscQueue<int, 4> q;
    CHECK(q.empty());
    CHECK(q.push(1));
    CHECK(q.push(2));
    CHECK(q.push(3));
    CHECK_FALSE(q.push(4));           // capacity-1 usable slots
    CHECK(q.size() == 3);
    CHECK(q.pop().value() == 1);
    CHECK(q.pop().value() == 2);
    CHECK(q.push(4));
    CHECK(q.pop().value() == 3);
    CHECK(q.pop().value() == 4);
    CHECK_FALSE(q.pop().has_value());

    SpscQueue<int, 2> latest;
    latest.push_overwrite(10);
    latest.push_overwrite(20);       // drops 10
    latest.push_overwrite(30);       // drops 20
    CHECK(latest.pop().value() == 30);
    CHECK(latest.empty());
}

TEST_CASE("OfflineCache: keeps newest by seq, ages by epoch") {
    OfflineCache c;
    CHECK_FALSE(c.has());

    DisplayState a;
    a.stateSeq = 5;
    a.epochS = 1000;
    a.speedLimit = 50;
    c.put(a);
    CHECK(c.has());

    DisplayState old;
    old.stateSeq = 3;
    old.epochS = 900;
    c.put(old);                       // ignored (older seq)
    CHECK(c.get().speedLimit.value() == 50);

    CHECK(c.age_s(1000) == 0);
    CHECK(c.age_s(1300) == 300);
    CHECK_FALSE(c.is_stale(1300, 10));   // 5 min < 10 min
    CHECK(c.is_stale(1000 + 11 * 60, 10));
}

TEST_CASE("CacheStore round-trips a DisplayState through a fake NVS") {
    fakes::Nvs nvs;
    CacheStore store(nvs);

    DisplayState empty;
    CHECK_FALSE(store.load(empty));              // nothing stored

    DisplayState s;
    s.stateSeq = 42;
    s.speedLimit = 70;
    s.epochS = 1'700'000'123;
    s.hazardActive = true;
    s.hazardType = HazardType::MobileSpeedCamera;
    s.hazardDistanceM = 300;
    CHECK(store.save(s));

    DisplayState loaded;
    REQUIRE(store.load(loaded));
    CHECK(loaded.stateSeq == 42);
    CHECK(loaded.speedLimit.value() == 70);
    CHECK(loaded.hazardActive);
    CHECK(loaded.hazardType.value() == HazardType::MobileSpeedCamera);

    store.clear();
    CHECK_FALSE(store.load(loaded));
}

TEST_CASE("SettingsStore round-trips through a fake NVS; bad blob -> defaults") {
    fakes::Nvs nvs;
    SettingsStore store(nvs);

    CHECK(store.load() == Settings{});          // nothing stored yet

    Settings s;
    s.soundEnabled = true;
    s.soundMode = SoundMode::Voice;
    s.useMph = true;
    s.filter.set(HazardType::Traffic, false);
    s.nightHours = {22, 5};
    s.cacheStaleMinutes = 20;
    CHECK(store.save(s));
    CHECK(nvs.commits == 1);

    Settings loaded = store.load();
    CHECK(loaded == s);
    CHECK_FALSE(loaded.filter.enabled(HazardType::Traffic));

    // Corrupt the blob -> defaults, no crash.
    nvs.store[fakes::Nvs::k("settings", "v")] = {0xFF, 0x00};
    CHECK(store.load() == Settings{});
}

TEST_CASE("apply_filter suppresses only the disabled hazard category") {
    WarningFilter f;
    f.set(HazardType::Traffic, false);

    DisplayState s;
    s.hazardActive = true;
    s.hazardType = HazardType::Traffic;
    s.hazardDistanceM = 300;
    CHECK(apply_filter(s, f));
    CHECK_FALSE(s.hazardActive);
    CHECK_FALSE(s.hazardType.has_value());
    CHECK_FALSE(s.hazardDistanceM.has_value());

    DisplayState s2;
    s2.hazardActive = true;
    s2.hazardType = HazardType::Ice;
    s2.hazardDistanceM = 300;
    CHECK_FALSE(apply_filter(s2, f));   // Ice still enabled
    CHECK(s2.hazardActive);
}
