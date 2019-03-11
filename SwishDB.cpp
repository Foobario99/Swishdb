//
// Created by Eric Kelley on 2019-03-05.
//


#include "SwishDB.h"

uint64_t current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    uint64_t milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    //printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

void SwishDBClass::init(const char* dir,
          uint32_t filePeriodMinutes) {
    _dir = dir;
    uint64_t filePeriodMillis = filePeriodMinutes * 60 * 1000;
    if ( filePeriodMillis > UINT32_MAX ) {
        printf("Unable to set large filePeriodMillis, setting to UINT32_MAX");
        _maxFilePeriodMillis = UINT32_MAX;
    }
    else {
        _maxFilePeriodMillis = filePeriodMillis;
    }


    _lastFlushTime = current_timestamp();
}

/**
 *
 * Does the datafile for the current index file need
 * to be cycled, based on either storageLimitKbytes or
 * _storageLimitTimePeriodDays
 *
 * @return
 */

bool SwishDBClass::shouldCreateNewFile() {

}

bool endsWith(const char *str, const char *suffix) {
    if (!str || !suffix)
        return false;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return false;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

// Returns false if an index file couldn't be found
bool SwishDBClass::openLatestIndexFile() {
    DIR* dirp = opendir(_dir);
    if (dirp == NULL) {
        printf("Unable to open directory: %s for read.\n", _dir);
        return false;
    }
    struct dirent* dp;
    uint64_t latest = 0;
    while ((dp = readdir(dirp)) != NULL) {
        if (dp->d_namlen > 4 && endsWith(dp->d_name, ".idx")) {
            printf("Found: %s\n", dp->d_name);
            std::string filename = std::string(dp->d_name);
            uint64_t value = std::stoull(filename.substr(0, filename.length() - 4));
            printf("Parsed out: %llu\n", value);
            if( value > latest) {
                latest = value;
            }
        }
    }
    closedir(dirp);

    if ( latest == 0 ) {
        return false;
    }
    else {
        readIndexFile(latest);
        return true;
    }
 }

void SwishDBClass::writeIndexFile() {
    char indexPath[MAX_PATH_LENGTH];
    snprintf(indexPath, MAX_PATH_LENGTH, "%s/%llu.idx", _dir, _currentIndex.time_base);
    FILE* indexFilep = fopen(indexPath, "w");
    if ( indexFilep != NULL ) {
        fwrite(&_currentIndex.version, sizeof(uint8_t), 1, indexFilep);
        fwrite(&_currentIndex.time_base, sizeof(uint64_t), 1, indexFilep);
        fwrite(&_currentIndex.typeCount, sizeof(uint8_t), 1, indexFilep);
        //In memory we are storing the typeIndex in a possibly sparse array
        //for easy lookup, however we serialize to the minimal array to
        //reduce space
        for(uint8_t i=0; i < MAX_TYPES; i++) {
            if(_currentIndex.typeInfoList[i] > 0){
                fwrite(&i, sizeof(uint8_t), 1, indexFilep);
                fwrite(&_currentIndex.typeInfoList[i], sizeof(uint32_t), 1, indexFilep);
            }
        }
    }
    else {
        printf("Unable to open file: %s for write.\n", indexPath);
    }
    fclose(indexFilep);
}


void SwishDBClass::readIndexFile(uint64_t timebase) {
    char indexPath[MAX_PATH_LENGTH];
    snprintf(indexPath, MAX_PATH_LENGTH, "%s/%llu.idx", _dir, timebase);
    FILE* indexFilep = fopen(indexPath, "r");
    if ( indexFilep != NULL ) {
        fread(&_currentIndex.version, sizeof(uint8_t), 1, indexFilep);
        fread(&_currentIndex.time_base, sizeof(uint64_t), 1, indexFilep);
        fread(&_currentIndex.typeCount, sizeof(uint8_t), 1, indexFilep);
        for(uint8_t i=0; i < _currentIndex.typeCount; i++) {
            uint8_t typeIndex = 0;
            fread(&typeIndex, sizeof(uint8_t), 1, indexFilep);
            fread(&_currentIndex.typeInfoList[typeIndex], sizeof(uint32_t), 1, indexFilep);
        }
    }
    else {
        printf("Unable to open file: %s for read (%d)\n", indexPath, errno);
    }
    fclose(indexFilep);
}


void SwishDBClass::createNewFileForTimebase(uint64_t timeBase) {
    bzero(&_currentIndex, sizeof(SwishIndex));
    _currentIndex.time_base = timeBase;
    _currentIndex.version = 0;
    writeIndexFile();
    _indexOpen = true;
}


FILE* SwishDBClass::openDataFile(uint64_t timebase, const char* mode) {
    if(timebase == 0 || mode == nullptr)
        return NULL;
    char dataPath[MAX_PATH_LENGTH];
    snprintf(dataPath, MAX_PATH_LENGTH, "%s/%llu.dat", _dir, timebase);
    FILE* dataFilep = fopen(dataPath, mode);
    if(dataFilep == NULL ) {
        printf("Unable to open file: %s for mode: %s (%d)\n", dataPath, mode, errno);
    }
    return dataFilep;
}

void SwishDBClass::flushData(SwishData* item, FILE* dataFilep) {
    //Update our counters
    if( _currentIndex.typeInfoList[item->typeIndex] == 0 ) {
        _currentIndex.typeCount++;
    }
    _currentIndex.typeInfoList[item->typeIndex]++;
    //We store our timestamps as deltas from timebase
    uint32_t timeDelta = item->time - _currentIndex.time_base;
    fwrite(&item->time, sizeof(uint32_t), 1, dataFilep);
    fwrite(&item->typeIndex, sizeof(uint8_t), 1, dataFilep);
    fwrite(&item->value, sizeof(uint32_t), 1, dataFilep);
}

void SwishDBClass::flushBuffer() {
    printf("Flushing buffer...\n");
    if(_currentBufferSize == 0) return;

    //Do we have an open Index file?
    if ( !_indexOpen ) {
        _indexOpen = openLatestIndexFile();
    }

    //If we don't have an index file, or if we need a new file
    //create the new file.
    if ( !_indexOpen || _buffer[0].time - _currentIndex.time_base  > _maxFilePeriodMillis ) {
        createNewFileForTimebase(_buffer[0].time);
    }

    FILE* dataFilep = openDataFile(_currentIndex.time_base, "a");
    for(uint32_t i = 0; i < _currentBufferSize; i++) {
        flushData(&_buffer[i], dataFilep);
        bzero(&_buffer[i], sizeof(SwishData));
    }
    _currentBufferSize = 0;
    _lastFlushTime = current_timestamp();
    fclose(dataFilep);
    writeIndexFile();

}

bool SwishDBClass::roomInBuffer(uint32_t newDataLength) {
    return _currentBufferSize + newDataLength <= BUFFER_SIZE;
}

bool SwishDBClass::timeToFlush() {
    return _lastFlushTime + _maxMillisBeforeFlush < current_timestamp();
}

bool SwishDBClass::store(const SwishData *data, size_t length) {
    if(length > BUFFER_SIZE ) {
        printf("Invalid input length: %ld\n", length);
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


size_t SwishDBClass::suggestStorageLimitForMetricRate(float metricRate) {
    return 0;
}

uint8_t SwishDBClass::suggestStorageLimitTimePeriodForMetricRate(float metricRate) {
    return 0;
}

SwishDBClass SwishDB;