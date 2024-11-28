#include <ETH.h>
#include <WiFi.h>
/* 
   * ETH_CLOCK_GPIO0_IN   - default: external clock from crystal oscillator
   * ETH_CLOCK_GPIO0_OUT  - 50MHz clock from internal APLL on GPIO0
   * ETH_CLOCK_GPIO16_OUT - 50MHz clock from internal APLL on GPIO16
   * ETH_CLOCK_GPIO17_OUT - 50MHz clock from internal APLL inverted on GPIO17
*/
#ifdef ETH_CLK_MODE
  #undef ETH_CLK_MODE
#endif
#define ETH_CLK_MODE    ETH_CLOCK_GPIO0_IN
// Pin# of the enable signal for the external crystal oscillator (-1 to disable for internal APLL source)
#define ETH_POWER_PIN   -1
// Type of the Ethernet PHY (LAN8720 or TLK110)
#define ETH_TYPE        ETH_PHY_LAN8720
// I²C-address of Ethernet PHY (0 or 1 for LAN8720, 31 for TLK110)
#define ETH_ADDR        0
// Pin# of the I²C clock signal for the Ethernet PHY
#define ETH_MDC_PIN     23
// Pin# of the I²C IO signal for the Ethernet PHY
#define ETH_MDIO_PIN    18

#define ETH_RESET     16

#define LED_PIN       2

static bool eth_connected = false;

WiFiServer server(80);

// Select the IP address according to your local network
IPAddress myIP(10, 1, 0, 182);
IPAddress myGW(10, 1, 0, 252);
IPAddress mySN(255, 255, 0, 0);
IPAddress myDNS(8, 8, 8, 8);

void myEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      ETH.setHostname("esp32-ethernet");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      //Serial.println("ETH Got IP");
      //Serial.println(ETH);
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("ETH Lost IP");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

void setup() {
  //ETH Reset assert
  pinMode(ETH_RESET, OUTPUT);
  digitalWrite(ETH_RESET, LOW);

  pinMode(LED_PIN, OUTPUT);
  for(int i=0; i<5; i++){
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }

  //ETH Reset deassert
  digitalWrite(ETH_RESET, HIGH);
  delay(200);

  //Serial - start
  Serial.begin(115200);

  WiFi.onEvent(myEvent);
  ETH.begin(ETH_TYPE, ETH_ADDR,
            ETH_MDC_PIN, ETH_MDIO_PIN,
            ETH_POWER_PIN, ETH_CLK_MODE);
  //ETH.config(myIP, myGW, mySN, myDNS);

  while (!ETH.connected()){}
  server.begin();
}

void loop() {
  // listen for incoming clients
  WiFiClient client = server.available();

  if (client) {
    Serial.println("********New Client********");

    String currentLine = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        //Serial.write(c);
        if (c == '\n') {

          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print("<H2>Click <a href=\"/H\">here</a> to turn ON the LED.<br>");
            client.print("Click <a href=\"/L\">here</a> to turn OFF the LED.<br>");
            client.print("Click <a href=\"/SEND\">here</a> to write to Serial (USB).<br></H2>");
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }

        //if (currentLine.endsWith("GET /H")) {
        if (currentLine.indexOf("GET /H") >= 0) {
          digitalWrite(LED_PIN, HIGH);
        }
        //if (currentLine.endsWith("GET /L")) {
        if (currentLine.indexOf("GET /L") >= 0) {
          digitalWrite(LED_PIN, LOW);
        }
        if (currentLine.endsWith("GET /SEND")) {
          Serial.println("\r\nHELLO - you send message via Serial(USB)");
        }
      }
    }

    delay(10);
    client.stop();
    Serial.println("********Client Disconnected********");
  }
}
