//
// Created by Eric Kelley on 2019-03-05.
//
#include "SwishDB.h"

#include <FS.h>
#include <lwip/def.h>

void SwishDBClass::init(const char* dir,
                        uint32_t filePeriodMinutes) {
    _dir = dir;
    uint32_t filePeriodSeconds = filePeriodMinutes * 60;
    if ( filePeriodSeconds > UINT32_MAX ) {
        Log.error(F("Unable to set large filePeriodMillis, setting to UINT32_MAX" CR));
        _maxFilePeriodSeconds = UINT32_MAX;
    }
    else {
        _maxFilePeriodSeconds = filePeriodSeconds;
    }
    if (!SPIFFS.begin()) {
        Log.error(F("Failed to mount file system." CR));
    }
    _lastFlushTime = now();
    bzero(&_currentIndex, sizeof(SwishIndex));
}

void SwishDBClass::setCurrentTimeSecondsFunction(time_t (*secondsFuncP)() ) {
    _secondsFuncP = secondsFuncP;
}


void SwishDBClass::listFilesToDebug() {
    Dir dir = SPIFFS.openDir(_dir);
    while (dir.next()) {
        String fileName = dir.fileName();
        size_t filesize = dir.fileSize();
        Log.trace(F("FileName: %s size: %l " CR), fileName.c_str(), filesize);
    }
}

time_t SwishDBClass::now() {
    if ( _secondsFuncP != nullptr ){
        return (_secondsFuncP)();
    }
    else {
        Log.fatal(F("SwishDBClass::now() - Timesource function not set." CR));
        return 0;
    }
}


/**
 *
 * This method looks up the index files that fall within the specified time span
 * and writes them to the buffer with the following format:
 * uint32_t recordSize
 * byte[recordSize] indexFile
 *
 * Index files can vary in size since they contain a variable number of typeIds
 *
 * This routine will attempt to write as many indexes into the buffer as possible.
 * If there are still more index files for the specified range, then the span parameter
 * will be modified with the span corresponding to the remaining indexes
 *
 * totalRecords
 *
 * byte
 *
 * @param span
 * @param buffer  a pointer to the buffer
 * @param bufferSize  the size of the buffer in bytes
 * @param totalRecords  is set to the value of the number of indexes for the span
 * @return  number of bytes written into the buffer
 */

ICACHE_FLASH_ATTR uint32_t SWFileIterator::filenameToTimeStamp(String &fileName) {
    String secondsString = fileName.substring(_dirPrefixLen, fileName.length() - _suffixLen).c_str();
//    Log.trace(F("Attempting to parse out: \'%s\'" CR), secondsString.c_str());

    uint32_t secondsValue = strtoul(secondsString.c_str(), NULL, 0);
    if(secondsValue != 0) {
        char timeString[20];
        snprintf(timeString, 20, "%u", secondsValue);
 //       Log.trace(F("Parsed out: %s" CR), timeString);
    }
    else {
        Log.error(F("Unable to parse unsigned long from: %s"), secondsString.c_str());
    }
    return secondsValue;
}

bool timeInTimeSpan( time_t timeStamp, const TimeSpan* span) {
    return ( timeStamp >= span->base && timeStamp <= (span->base + span->duration) );
}

ICACHE_FLASH_ATTR uint32_t SWFileIterator::count() {
    uint32_t count = 0;
    Dir dir = SPIFFS.openDir(_dirName);
    while (dir.next()) {
        String fileName = dir.fileName();
        if ( fileName.endsWith(_suffix) ) {
            if( timeInTimeSpan(filenameToTimeStamp(fileName), &_span)) {
                count++;
            }
        }
    }
    return count;
}


ICACHE_FLASH_ATTR SWFileIterator::SWFileIterator(const TimeSpan span, const char* dir, const char* suffix) {
    init(span, dir, suffix);
}

ICACHE_FLASH_ATTR void SWFileIterator::init(const TimeSpan span, const char* dir, const char* suffix) {
    _span = span;
    _suffix = suffix;
    _suffixLen = strnlen(_suffix, MAX_PATH_LENGTH);

    _dir = SPIFFS.openDir(dir);
    _dirName = dir;
    _dirPrefixLen = strnlen(dir, MAX_PATH_LENGTH) + 1;
}


ICACHE_FLASH_ATTR bool SWFileIterator::next() {
    bool nextFileFound = false;
    while( !nextFileFound && _dir.next()) {
        //Does the next file match our requirements?
        String fileName = _dir.fileName();
        if( fileName.endsWith(_suffix) ) {
            nextFileFound = timeInTimeSpan(filenameToTimeStamp(fileName), &_span);
        }

    }
    return nextFileFound;
}

ICACHE_FLASH_ATTR File SWFileIterator::open() {
    return _dir.openFile("r");
}

ICACHE_FLASH_ATTR SWFileIterator* SwishDBClass::getIndexFileIterator(const TimeSpan span, SWFileIterator* base) {
    if ( base == nullptr ) {
        return new SWFileIterator(span, _dir, indexFileSuffix);
    }
    else {
        base->init(span, _dir, indexFileSuffix);
        return base;
    }
}
ICACHE_FLASH_ATTR SWFileIterator* SwishDBClass::getDataFileIterator(const TimeSpan span, SWFileIterator* base) {
    if ( base == nullptr ) {
        return new SWFileIterator(span, _dir, dataFileSuffix);
    }
    else {
        base->init(span, _dir, dataFileSuffix);
        return base;
    }
}


ICACHE_FLASH_ATTR void printArray(uint32_t* array, uint32_t size) {
    for(int i = 0; i < size; i++) {
        Log.trace(F("%l,"), array[i]);
    }
}

// http://192.168.1.31/api/async/getMetricsDistribution?startTime=1553443684&duration=100000&wsClientId=1

// Returns false if an index file couldn't be found
bool SwishDBClass::openLatestIndexFile() {
    Dir dir = SPIFFS.openDir(_dir);
    uint32_t latestSeconds = 0;
    while (dir.next()) {
        String fileName = dir.fileName();
        if ( fileName.endsWith(indexFileSuffix) ) {
            Log.trace(F("Found: %s" CR), fileName.c_str());
            String secondsString = fileName.substring(strnlen(_dir, MAX_PATH_LENGTH) + 1, fileName.length() - 4).c_str();
            uint32_t secondsValue = strtoul(secondsString.c_str(), NULL, 0);
            if(secondsValue != 0) {
                char timeString[20];
                snprintf(timeString, 20, "%u", secondsValue);
                Log.trace(F("Parsed out: %s" CR), timeString);
            }
            else {
                Log.error(F("Unable to parse unsigned long from: %s"), secondsString.c_str());
            }
            if( secondsValue > latestSeconds) {
                latestSeconds = secondsValue;
            }
        }
    }

    if ( latestSeconds == 0 ) {
        return false;
    }
    else {
        readIndexFile(latestSeconds);
        return true;
    }
}

//All multibyte ints written in network order
ICACHE_FLASH_ATTR void SwishDBClass::writeIndexFile() {
    char indexPath[MAX_PATH_LENGTH];
    snprintf(indexPath, MAX_PATH_LENGTH, "%s/%lu%s", _dir, _currentIndex.time_base, indexFileSuffix);
    File indexFile = SPIFFS.open(indexPath, "w");
    if ( indexFile ) {
        indexFile.write(&_currentIndex.version, sizeof(uint8_t));
        int32_t time_base = lwip_htonl(_currentIndex.time_base);
        indexFile.write((uint8_t*)&time_base, sizeof(int32_t));

        uint32_t netEndianDuration = lwip_htonl(_currentIndex.duration);
        indexFile.write((uint8_t*)&netEndianDuration, sizeof(uint32_t));

        indexFile.write(&_currentIndex.typeCount, sizeof(uint8_t));
       //In memory we are storing the typeIndex in a possibly sparse array
        //for easy lookup, however we serialize to the minimal array to
        //reduce space
        for(uint8_t i=0; i < MAX_TYPES; i++) {
            if(_currentIndex.typeInfoList[i] > 0){
                Log.trace(F("Writing out typeIndex %d with count: %l" CR), i, _currentIndex.typeInfoList[i]);
                indexFile.write(&i, sizeof(uint8_t));
                uint32_t typeCount = lwip_htonl(_currentIndex.typeInfoList[i]);
                indexFile.write((uint8_t*)&typeCount, sizeof(uint32_t));
            }
        }
    }
    else {
        Log.error(F("Unable to open file: %s for write." CR), indexPath);
    }
    indexFile.close();
}


ICACHE_FLASH_ATTR void SwishDBClass::readIndexFile(time_t timebase) {
    char indexPath[MAX_PATH_LENGTH];
    snprintf(indexPath, MAX_PATH_LENGTH, "%s/%lu%s", _dir, timebase, indexFileSuffix);
    File indexFile = SPIFFS.open(indexPath, "r");
    if ( indexFile ) {
        indexFile.read(&_currentIndex.version, sizeof(uint8_t));
        indexFile.read((uint8_t*)&_currentIndex.time_base, sizeof(int32_t));
        _currentIndex.time_base = lwip_ntohl(_currentIndex.time_base);
        indexFile.read((uint8_t*)&_currentIndex.duration, sizeof(uint32_t));
        _currentIndex.duration = lwip_ntohl(_currentIndex.duration);

        indexFile.read(&_currentIndex.typeCount, sizeof(uint8_t));
        for(uint8_t i=0; i < _currentIndex.typeCount; i++) {
            uint8_t typeIndex = 0;
            indexFile.read(&typeIndex, sizeof(uint8_t));
            indexFile.read((uint8_t*)&_currentIndex.typeInfoList[typeIndex], sizeof(uint32_t));
            _currentIndex.typeInfoList[typeIndex] = lwip_ntohl(_currentIndex.typeInfoList[typeIndex]);
        }
    }
    else {
        Log.error(F("Unable to open file: %s for read" CR), indexPath);
    }
    indexFile.close();
}


ICACHE_FLASH_ATTR void SwishDBClass::createNewFileForTimebase(time_t timeBase) {
    bzero(&_currentIndex, sizeof(SwishIndex));
    _currentIndex.time_base = timeBase;
    _currentIndex.version = 0;
    _currentIndex.duration = _maxFilePeriodSeconds;
    writeIndexFile();
    _indexOpen = true;
}


ICACHE_FLASH_ATTR File SwishDBClass::openDataFile(time_t timebase, const char* mode) {
    char dataPath[MAX_PATH_LENGTH];
    snprintf(dataPath, MAX_PATH_LENGTH, "%s/%lu%s", _dir, timebase, dataFileSuffix);
    File dataFile = SPIFFS.open(dataPath, mode);
    if(!dataFile) {
        Log.error(F("Unable to open file: %s for mode: %s" CR), dataPath, mode);
    }
    return dataFile;
}

//Writes all longs in network order
ICACHE_FLASH_ATTR void SwishDBClass::flushData(SwishData* item, File dataFile) {
    //Update our counters
    if( _currentIndex.typeInfoList[item->typeIndex] == 0 ) {
        _currentIndex.typeCount++;
    }
    _currentIndex.typeInfoList[item->typeIndex]++;
    //We store our timestamps as deltas from timebase
    uint32_t timeDelta = lwip_htonl(item->time - _currentIndex.time_base);
    if(item->time < _currentIndex.time_base ) {
        Log.warning(F("SwishData has time < time_base. Setting to time_base..." CR));
        timeDelta = 0;
    }
    uint32_t itemValue = lwip_htonl(item->value);
    dataFile.write((uint8_t*)&timeDelta, sizeof(uint32_t));
    dataFile.write(&item->typeIndex, sizeof(uint8_t));
    dataFile.write((uint8_t*)&itemValue, sizeof(uint32_t));
}

ICACHE_FLASH_ATTR void SwishDBClass::flushBuffer() {
    Log.trace(F("Flushing buffer..." CR));
    if(_currentBufferSize == 0) return;

    //Do we have an open Index file?
    if ( !_indexOpen ) {
        _indexOpen = openLatestIndexFile();
    }

    //Guard against zero time data
    time_t eventTime = _buffer[0].time > 0 ? _buffer[0].time : now();
    //If we don't have an index file, or if we need a new file
    //create the new file.
    if ( !_indexOpen || eventTime - _currentIndex.time_base  > _maxFilePeriodSeconds ) {
        createNewFileForTimebase(eventTime);
    }

    File dataFile = openDataFile(_currentIndex.time_base, "a");
    for(uint32_t i = 0; i < _currentBufferSize; i++) {
        if(_buffer[i].time == 0){
            Log.warning(F("SwishDB:flushBuffer() - data with zero timestamp." CR));
        }
        flushData(&_buffer[i], dataFile);
        bzero(&_buffer[i], sizeof(SwishData));
    }
    _currentBufferSize = 0;
    _lastFlushTime = now();
    dataFile.close();
    writeIndexFile();

}

ICACHE_FLASH_ATTR bool SwishDBClass::roomInBuffer(uint32_t newDataLength) {
    return _currentBufferSize + newDataLength <= BUFFER_SIZE;
}

ICACHE_FLASH_ATTR bool SwishDBClass::timeToFlush() {
    return _lastFlushTime + _maxSecondsBeforeFlush < now();
}

ICACHE_FLASH_ATTR bool SwishDBClass::store(const SwishData *data, size_t length) {
    if(length > BUFFER_SIZE ) {
        Log.error(F("Invalid input length: %lu" CR), length);
        return false;
    }

    if ( !roomInBuffer(length) || (_currentBufferSize != 0 && timeToFlush() ) ) {
        flushBuffer();
    }
    //For each item in our incoming array, copy the item into the buffer
    memcpy(&_buffer[_currentBufferSize], data, length * sizeof(SwishData));
    _currentBufferSize = _currentBufferSize + length;

    return true;
}


ICACHE_FLASH_ATTR size_t SwishDBClass::suggestStorageLimitForMetricRate(float metricRate) {
    return 0;
}

ICACHE_FLASH_ATTR uint8_t SwishDBClass::suggestStorageLimitTimePeriodForMetricRate(float metricRate) {
    return 0;
}

