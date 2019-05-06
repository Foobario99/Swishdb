//
// Created by Eric Kelley on 2019-03-22.
//

#include "SwishDBTester.h"
#include "SwishDB.h"

void SwishDBTester::setup() {

}

void SwishDBTester::once() {
    Log.notice(F("Testing index file create." CR));
    SwishDBClass SwishDB;
    SwishDB.setCurrentTimeSecondsFunction(&now);
    SwishDB.init("/swd", 2);

    SwishDB.listFilesToDebug();
    SwishDB.createNewFileForTimebase(99999999);

    SwishDB._currentIndex.duration = 100000;
    SwishDB._currentIndex.time_base = 99999999;
    SwishDB._currentIndex.typeCount = 1;
    SwishDB._currentIndex.typeInfoList[0] = 999;
    SwishDB._currentIndex.version = 1;

    SwishDB.writeIndexFile();

    SwishDB.readIndexFile(99999999);

    assertEqual( SwishDB._currentIndex.duration, 100000 );
    assertEqual( SwishDB._currentIndex.time_base, 99999999 );
    assertEqual( SwishDB._currentIndex.typeCount, 1 );
    assertEqual( SwishDB._currentIndex.typeInfoList[0],  999 );
    assertEqual( SwishDB._currentIndex.version, 1 );

}

void DataCreateTest::setup() {

}
void DataCreateTest::once() {

    Log.notice(F("Testing data file create." CR));
    SwishDBClass SwishDB;
    SwishDB.setCurrentTimeSecondsFunction(&now);
    SwishDB.init("/swd", 2);

    SwishDB.listFilesToDebug();
    time_t startEventTime = now();
    time_t eventTime = startEventTime;

    //Create two hundred data points
    for(int i = 0; i < 100; i++) {
        SwishData someData[2];
        someData[0].value = 11111;
        someData[0].time = eventTime;
        someData[0].typeIndex = 0;

        someData[1].value = 22222;
        someData[1].time = eventTime;
        someData[1].typeIndex = 1;
        SwishDB.store(someData, 2);
        delay(10);
        eventTime = now();
    }

    File datafile = SwishDB.openDataFile(startEventTime, "r");
    assertTrue(datafile);
    assertMore(datafile.size(), 0);

}

void IndexFileListTest::setup() {

}

void IndexFileListTest::once() {
    Log.notice(F("Testing index file list ..." CR));
    SwishDBClass SwishDB;
    SwishDB.setCurrentTimeSecondsFunction(&now);
    SwishDB.init("/swd", 2);

    SwishDB.listFilesToDebug();
    TimeSpan testSpan;
    testSpan.base = 1553442477;
    testSpan.duration = 10000;

    SWFileIterator indexIterator;
    SwishDB.getIndexFileIterator(testSpan, &indexIterator);
    while(indexIterator.next()) {
        File indexFile = indexIterator.open();
        if (indexFile) {
            Log.notice(F("Found index file: %s" CR), indexFile.name());
        }
    }



}


//
//float zeroToPi = 0.0;
//
//time_t getTimestamp() {
//    return 1553287790;
//}
//
//void testDataCreate() {
//    Log.notice(F("Testing data file create." CR));
//    SwishDBClass SwishDB;
//    SwishDB.setCurrentTimeSecondsFunction(&getTimestamp);
//    SwishDB.init("/swd", 2);
//
//    SwishDB.listFilesToDebug();
//
//    SwishData someData[2];
//    char timeString[20];
//    snprintf(timeString, 20, "%f", zeroToPi);
//    Log.trace(F("Creating data for %s\n"), timeString);
//    uint32_t base = now();
//    someData[0].value = sin(zeroToPi) * 100000;
//    someData[0].time = base;
//    someData[0].typeIndex = 0;
//
//    someData[1].value = cos(zeroToPi) * 100000;
//    someData[1].time = base;
//    someData[1].typeIndex = 1;
//    SwishDB.store(someData, 2);
//
//    zeroToPi+=0.0001;
//
//}

