//
// Created by Eric Kelley on 2019-03-05.
//

#ifndef SWISHDB_SWISHDB_H
#define SWISHDB_SWISHDB_H

#include <inttypes.h>
#include <stdlib.h>

#include <stdio.h>

#include <sys/time.h>
#include <string.h>
#include <fstream>

#include <dirent.h>

#define MAX_TYPES 50
#define BUFFER_SIZE 100

#define MAX_PATH_LENGTH 31 + 1

uint64_t current_timestamp();

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
    uint64_t time;
    /** integer index for this type of data */
    uint8_t typeIndex;
    /** data value */
    int32_t value;
} SwishData;

typedef struct {
    uint8_t version = 0;
    uint64_t time_base = 0;
    uint8_t typeCount = 0;
    uint32_t typeInfoList[MAX_TYPES];
} SwishIndex;


class SwishDBClass {

public:
    void init(const char *dir,
              uint32_t filePeriodMinutes);
    size_t suggestStorageLimitForMetricRate(float metricRate);

    uint8_t suggestStorageLimitTimePeriodForMetricRate(float metricRate);

    bool store(const SwishData *data, size_t length);

private:
    SwishData _buffer[BUFFER_SIZE];
    uint32_t _currentBufferSize = 0;

    size_t _storageLimit = 0;
    uint8_t _storageLimitTimePeriodDays = 0;
    const char *_dir;

    uint32_t _maxMillisBeforeFlush = 60000;
    uint64_t _lastFlushTime;

    bool _indexOpen = false;
    SwishIndex _currentIndex;

//    bool _dataOpen = false;
//    FILE* dataFilep = NULL;

    uint32_t _maxFilePeriodMillis = 1000 * 60 * 60 * 24; //Maximum time span for a data file

    void readIndexFile(uint64_t timebase);

    void writeIndexFile();

    bool timeToFlush();

    bool shouldCreateNewFile();

    bool openLatestIndexFile();

    bool roomInBuffer(uint32_t newDataLength);

    void flushBuffer();
    void createNewFileForTimebase(uint64_t timeBase);
    FILE* openDataFile(uint64_t timebase, const char* mode);
    void flushData(SwishData* item, FILE* dataFilep);

};

extern SwishDBClass SwishDB;

#endif //SWISHDB_SWISHDB_H
