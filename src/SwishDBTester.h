//
// Created by Eric Kelley on 2019-03-22.
//

#ifndef SWISHDB_SWISHDBTESTER_H
#define SWISHDB_SWISHDBTESTER_H

#include <Arduino.h>
#include <c_types.h>
#include <string.h>
#include <FS.h>
#include <ArduinoUnit.h>
#include <TimeLib.h>

class SwishDBTester : public TestOnce  {
public:
    void setup();
    SwishDBTester(const char *name)
            : TestOnce(name) {
        verbosity = TEST_VERBOSITY_ASSERTIONS_FAILED;

    }
    void once();

private:
};


class DataCreateTest : public TestOnce  {
public:
    void setup();
    DataCreateTest(const char *name)
            : TestOnce(name) {
        verbosity = TEST_VERBOSITY_ASSERTIONS_FAILED;

    }
    void once();

private:
};


class IndexFileListTest : public TestOnce {
public:
    void setup();
    IndexFileListTest(const char *name)
        : TestOnce(name) {
        verbosity = TEST_VERBOSITY_ASSERTIONS_FAILED;
    }
    void once();
};

#endif //SWISHDB_SWISHDBTESTER_H
