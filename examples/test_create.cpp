//
// Created by Eric Kelley on 2019-03-06.
//

#include <iostream>
#include "../SwishDB.h"

#include <unistd.h>

#include <math.h>


int main(int argc, char **argv) {
    std::cout << "Hello, World!" << std::endl;

    SwishDB.init("/tmp/swd", 2);

    SwishData someData[2];

    for( float i = 0; i<=3.14159 ; i+=0.01 ) {
    }
    float zeroToPi = 0.0;
    for(int i = 0; i<=100000; i++) {
        if(zeroToPi > 3.14159) {
            zeroToPi = 0;
        }
        printf("Creating data for %f\n", zeroToPi);
        uint64_t base = current_timestamp();
        someData[0].value = sin(zeroToPi) * 100000;
        someData[0].time = base;
        someData[0].typeIndex = 0;

        someData[1].value = cos(zeroToPi) * 100000;
        someData[1].time = base;
        someData[1].typeIndex = 1;
        SwishDB.store(someData, 2);

        zeroToPi+=0.0001;
        usleep(50000);
    }

}