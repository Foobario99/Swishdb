# SwishDB
Incredibly micro time series database for ESP8226 / Arduino. Inspired by WhisperDB


## Introduction


## Compatibility

## Download

[Download ESP8266TrueRandom library](https://github.com/marvinroger/ESP8266TrueRandom/archive/master.zip). Extract the zip file, and copy the directory to your Arduino libraries folder.


## Example

```c++
#include <ESP8266TrueRandom.h>

void setup() {
  Serial.begin(115200);

  Serial.print("I threw a random die and got ");
  Serial.print(random(1,7));

  Serial.print(". Then I threw a TrueRandom die and got ");
  Serial.println(ESP8266TrueRandom.random(1,7));

}

void loop() {
  ; // Do nothing
}
```

