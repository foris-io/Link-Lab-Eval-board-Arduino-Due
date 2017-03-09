#include <SymphonyLink.h>

#include <dht.h>
#include <Streaming.h>
dht DHT;

#define DHT11_PIN 7

//Instantiate a SymphonyLink class using the SymphonyLink Arduino Library
SymphonyLink symlink; 

uint8_t txData[1];    //TX data buffer. Could be up to 256 bytes.
uint8_t rxData[128];   //RX data buffer. Could be up to 128 bytes.
uint8_t rxDataLength;
uint8_t radioPath = 1; // set 1 for U.FL, 2 for integrated trace antenna

sym_module_state_t currentSymphonyState;
sym_module_state_t lastSymphonyState;

const int analogInPin = A1;  // Analog input pin that the sensor output is attached to
const int analogOutPin = 9; // Analog output pin that the LED is attached to

int salinityValue = 0;        // value read from the sensor
int ph_pin = A7;

void setup() 
{
  int ii;
  
  //Arduino Due allows debug and status signals out of the Serial port. UART communications with the SymphonyLink module are done using Serial1.
  Serial.begin(115200);

  //Configure the following to match your network and application tokens
  //Set desired network token 
  //uint32_t netToken = 0x4f50454e;
  uint32_t netToken = 0x0abc019c;
  //Insert your network token. For example, the OPEN network token is 0x4f50454e.
                                   //A module can only talk to gateways using the same network token.
  //Set desired application token
  uint8_t appToken[APP_TOKEN_LEN] = {0xf2,0xc2,0x66,0x4c,0x54,0x85,0x08,0xa0,0x45,0x24}; //Generate an application token in your Conductor account use it here.
                                                                                          //The application token identifies this dataflow in Conductor.
  
  //Initialize the SymphonyLink object and open UART communications with the module on Serial1.
  symlink.begin(netToken, appToken, LL_DL_MAILBOX, 15);

  //Initialize the txData. This is the array of hex bytes to be sent over the air, as an example.
  for (ii = 0; ii < sizeof(txData); ii++)
  {
    txData[ii] = 0;
  }

  rxDataLength = sizeof(rxData);

  //Set RF path
  symlink.setAntenna(radioPath);
  
  //Update the state of the SymphonyLink module (aka Modem)
  lastSymphonyState = symlink.updateModemState();
}

void loop()
{

  int chk = DHT.read11(DHT11_PIN);
  int moistureValue= analogRead(A0);
  salinityValue = analogRead(analogInPin);
  int measure = analogRead(ph_pin);
  double voltage = 5 / 1024.0 * measure;
  float Po = 7 + ((2.5 - voltage) / 0.18);
  int PhValue=(int) Po;
  PhValue=PhValue*(-1);
  Serial.println(DHT.temperature);
  Serial.println(moistureValue);
  Serial.println(salinityValue);
  Serial.println(PhValue);
  //Serial << "45 in hex is " << _HEX(moistureValue) << endl;
  //uint8_t myhex=_HEX(moistureValue);
  //String payloadValue=String(moistureValue);
  delay(1000);
  //Update the state of the SymphonyLink module (aka Modem)
  currentSymphonyState = symlink.updateModemState();
  switch (currentSymphonyState)
  {
    case SYMPHONY_READY:                            
      if (SYMPHONY_TRANSMITTING != lastSymphonyState) //When SymphonyLink module is ready, send txData
      {
        //txData[0]++;                                 //Increment payload
        //uint8_t data[2]={DHT.temperature}; 
        
        uint8_t data[6]={}; 
        data[0]=DHT.temperature;
        data[1] = (uint8_t)(moistureValue>>8);
        data[2] = (uint8_t)(moistureValue & 0x00FF);
        data[3] = (uint8_t)(salinityValue>>8);
        data[4] = (uint8_t)(salinityValue & 0x00FF);
        data[5] = PhValue;
        //data[6] = (uint8_t)(PhValue & 0x00FF);
        //symlink.write(txData, sizeof(txData), true); //Uplink the payload to Conductor
        symlink.write(data, sizeof(data), true);
       
      
        //Print the payload to the Serial monitor for debug.
        Serial.print("\t... Outbound payload is ");
        //symlink.printPayload(txData, sizeof(txData));
        //symlink.printPayload(DHT.temperature, sizeof(txData));
      }
      else 
      {
         // If last uplink failed, do not increment payload.
         if (LL_TX_STATE_SUCCESS != symlink.getTransmitState())
         {
          txData[0]--;
         }

         //Check for downlink data
         symlink.read(rxData, rxDataLength);
      }
      break;
    default:
      break;
  }
  lastSymphonyState = currentSymphonyState;
}
