#include "LoRaWan_APP.h"
//#include <Wire.h>
//#include <Adafruit_Sensor.h>
//#include <Adafruit_BME680.h>

/* OTAA para */
uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x06, 0xB3, 0x3C };
uint8_t appEui[] = { 0x34, 0x34, 0x56, 0x67, 0x36, 0x78, 0x53, 0xF4 };
uint8_t appKey[] = { 0xD4, 0x3D, 0xFD, 0xC0, 0xCC, 0xFA, 0xB8, 0xC7, 0xF0, 0x37, 0xF8, 0xAB, 0x27, 0xC0, 0x00, 0x00 };

/* ABP para */
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t devAddr = (uint32_t)0x007e6ae1;

/* LoraWan channelsmask */
uint16_t userChannelsMask[6] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };

/* LoraWan region, select in arduino IDE tools */
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/* LoraWan Class, Class A and Class C are supported */
DeviceClass_t loraWanClass = CLASS_A;

/* The application data transmission duty cycle. value in [ms]. */
uint32_t appTxDutyCycle = 15000;

/* OTAA or ABP */
bool overTheAirActivation = true;

/* ADR enable */
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;

/* Number of trials to transmit the frame, if the LoRaMAC layer did not receive an acknowledgment */
uint8_t confirmedNbTrials = 4;

/* Prepares the payload of the frame */
static void prepareTxFrame(uint8_t port)
{
    // unsigned long endTime = bme.beginReading();
    // delay(150); // Wait for the measurement to complete
    // if (!bme.endReading()) {
    //     Serial.println(F("Failed to perform reading :("));
    //     return;
    // }
    // appDataSize = 12;
    // appData[0] = (uint8_t)(bme.temperature * 100) >> 8;
    // appData[1] = (uint8_t)(bme.temperature * 100);
    // appData[2] = (uint8_t)(bme.humidity * 100) >> 8;
    // appData[3] = (uint8_t)(bme.humidity * 100);
    // appData[4] = (uint8_t)(bme.pressure / 10) >> 8;
    // appData[5] = (uint8_t)(bme.pressure / 10);
    // appData[6] = (uint8_t)(bme.gas_resistance / 10) >> 8;
    // appData[7] = (uint8_t)(bme.gas_resistance / 10);
    // appData[8] = 0;
    // appData[9] = 0;
    // appData[10] = 0;
    // appData[11] = 0;

    // Dummy data for transmission
    float temperature = 22.55;  //example 22.55 *C
  float humidity = 72.5;    //example 72.5 %

  int int_temp = temperature * 100; //remove comma
  int int_hum = humidity * 10; //remove comma

  appDataSize = 4;
  appData[0] = int_temp >> 8;
  appData[1] = int_temp;
  appData[2] = int_hum >> 8;
  appData[3] = int_hum;
}

RTC_DATA_ATTR bool firstrun = true;

/* Global variable to track join status */
bool joinAttempted = false;

/* Flag to indicate sleep mode message has been printed */
bool sleepMessagePrinted = false;

void setup() {
    Serial.begin(115200);
    Mcu.begin();
    if (firstrun)
    {
        LoRaWAN.displayMcuInit();
        firstrun = false;
        Serial.println("LORAWAN STARTING...");
    }
    // if (!bme.begin()) {
    //     Serial.println("Could not find a valid BME680 sensor, check wiring!");
    //     while (1);
    // }
    // Set up oversampling and filter initialization
    // bme.setTemperatureOversampling(BME680_OS_8X);
    // bme.setHumidityOversampling(BME680_OS_2X);
    // bme.setPressureOversampling(BME680_OS_4X);
    // bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    // bme.setGasHeater(320, 150); // 320*C for 150 ms
    deviceState = DEVICE_STATE_INIT;
}

void loop()
{
    switch (deviceState)
    {
    case DEVICE_STATE_INIT:
    {
#if(LORAWAN_DEVEUI_AUTO)
        LoRaWAN.generateDeveuiByChipID();
#endif
        Serial.println("Initializing LoRaWAN...");
        LoRaWAN.init(loraWanClass, loraWanRegion);
        deviceState = DEVICE_STATE_JOIN;
        sleepMessagePrinted = false;
        break;
    }
    case DEVICE_STATE_JOIN:
    {
        if (!joinAttempted)  // Check if join has not been attempted yet
        {
            Serial.println("Attempting to join the network...");
            LoRaWAN.displayJoining();
            LoRaWAN.join();
            joinAttempted = true;
            Serial.println("Join attempt finished.");
        }
        deviceState = DEVICE_STATE_SEND; // Proceed to send state
        sleepMessagePrinted = false;
        break;
    }
    case DEVICE_STATE_SEND:
    {
        Serial.println("Preparing to send data...");
        LoRaWAN.displaySending();
        prepareTxFrame(appPort);
        LoRaWAN.send();
        Serial.println("Data sent.");
        deviceState = DEVICE_STATE_CYCLE;
        sleepMessagePrinted = false;
        break;
    }
    case DEVICE_STATE_CYCLE:
    {
        Serial.println("Cycling...");
        // Schedule next packet transmission
        txDutyCycleTime = appTxDutyCycle + randr(0, APP_TX_DUTYCYCLE_RND);
        LoRaWAN.cycle(txDutyCycleTime);
        deviceState = DEVICE_STATE_SLEEP;
        sleepMessagePrinted = false;
        break;
    }
    case DEVICE_STATE_SLEEP:
    {
        if (!sleepMessagePrinted)
        {
            Serial.println("Entering sleep mode...");
            sleepMessagePrinted = true;
        }
        LoRaWAN.displayAck();
        LoRaWAN.sleep(loraWanClass);
        break;
    }
    default:
    {
        deviceState = DEVICE_STATE_INIT;
        sleepMessagePrinted = false;
        break;
    }
    }
}
