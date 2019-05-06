//
// Created by Eric Kelley on 2019-03-05.
//

#ifndef SWISHDB_SWISHDB_H
#define SWISHDB_SWISHDB_H


#include <Arduino.h>
#include <c_types.h>
#include <ArduinoLog.h>
#include <string.h>
#include <FS.h>

#define MAX_TYPES 50
#define BUFFER_SIZE 100

#define MAX_PATH_LENGTH 31 + 1

/**
 *
 * To save storage space, we are storing
 * timestamps in millis since epoch as 32 bit
 * unsigned ints, relative to basetime.
 * This means that we can hold data that spans
 * at most 2^32 - 1 == 4,294,967,295 == 49.71026962963 days
 * of data at MOST in a single SwishDB file.
 *
 */



/**
 *
 * We represent our data as a 32 bit integer
 * Floating point values can be stored by
 * premultiplying by 100 or 10000 to achieve a given
 * level of precision
 *
 */


/** Base storage type for Swish */
typedef struct {
    /** timestamp */
    time_t time;
    /** integer index for this type of data */
    uint8_t typeIndex;
    /** data value */
    int32_t value;
} SwishData;

typedef struct {
    uint8_t version = 0;
    time_t time_base = 0;
    uint32_t duration = 0;
    uint8_t typeCount = 0;
    uint32_t typeInfoList[MAX_TYPES];
} SwishIndex;

typedef struct {
    time_t base;
    uint32_t duration;
} TimeSpan;





class SWFileIterator {
public:
    ICACHE_FLASH_ATTR SWFileIterator() {};
    ICACHE_FLASH_ATTR SWFileIterator(const TimeSpan span, const char* dir, const char* suffix);
    ICACHE_FLASH_ATTR void init(const TimeSpan span, const char* dir, const char* suffix);
    ICACHE_FLASH_ATTR bool next();
    ICACHE_FLASH_ATTR File open();
    ICACHE_FLASH_ATTR uint32_t count(); //somewhat expensive as it opens a new directory handle and lists all files
    ICACHE_FLASH_ATTR String fileName() { return _dir.fileName(); }
    ICACHE_FLASH_ATTR size_t fileSize() { return _dir.fileSize(); }

private:
    uint32_t filenameToTimeStamp(String &fileName);
    TimeSpan _span;
    const char* _suffix;
    uint8_t _suffixLen;
    const char* _dirName;
    Dir _dir;
    uint8_t _dirPrefixLen;
};

class SwishDBClass {

public:
    ICACHE_FLASH_ATTR void init(const char *dir,
              uint32_t filePeriodMinutes);
//    size_t writeIndexesToBufferForSpan(TimeSpan* span, char* buffer, size_t bufferSize, uint32_t* recordsWritten, uint32_t* totalRecords);
    ICACHE_FLASH_ATTR bool store(const SwishData *data, size_t length);
    ICACHE_FLASH_ATTR void setCurrentTimeSecondsFunction(time_t (*secondsFuncP)() );

    ICACHE_FLASH_ATTR void listFilesToDebug();
    ICACHE_FLASH_ATTR size_t suggestStorageLimitForMetricRate(float metricRate);
    ICACHE_FLASH_ATTR uint8_t suggestStorageLimitTimePeriodForMetricRate(float metricRate);
    const char* indexFileSuffix = ".idx";
    const char* dataFileSuffix = ".dat";

    ICACHE_FLASH_ATTR SWFileIterator* getIndexFileIterator(const TimeSpan span, SWFileIterator* base);
    ICACHE_FLASH_ATTR SWFileIterator* getDataFileIterator(const TimeSpan span, SWFileIterator* base);

private:
    //Allow clients to specify the source of currentTime in seconds
    time_t (*_secondsFuncP)() = nullptr;
    time_t now();

    SwishData _buffer[BUFFER_SIZE];
    uint32_t _currentBufferSize = 0;

    size_t _storageLimit = 0;
    uint8_t _storageLimitTimePeriodDays = 0;
    const char *_dir;

    uint32_t _maxSecondsBeforeFlush = 60;
    time_t _lastFlushTime;

    bool _indexOpen = false;
    SwishIndex _currentIndex;

    uint32_t _maxFilePeriodSeconds = 60 * 60 * 24; //Maximum time span for a data file

    ICACHE_FLASH_ATTR void createNewFileForTimebase(time_t timeBase);
    ICACHE_FLASH_ATTR bool openLatestIndexFile();
    ICACHE_FLASH_ATTR void readIndexFile(time_t timebase);
    ICACHE_FLASH_ATTR void writeIndexFile();

    ICACHE_FLASH_ATTR bool timeToFlush();
    ICACHE_FLASH_ATTR bool roomInBuffer(uint32_t newDataLength);
    ICACHE_FLASH_ATTR void flushBuffer();

    ICACHE_FLASH_ATTR File openDataFile(time_t timebase, const char* mode);
    ICACHE_FLASH_ATTR void flushData(SwishData* item, File dataFile);

    friend class SwishDBTester;
    friend class DataCreateTest;
    };

#endif //SWISHDB_SWISHDB_H
