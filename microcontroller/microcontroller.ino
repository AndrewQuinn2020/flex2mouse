/*
    microcontroller

    Okay, now I want to get Tornado working so that I can send up
    simple commands to a web server.
*/

//----------------  #define Initializations   ----------------------

/*
    Question 1: Why am I using WebSockets here.

    I'm doing this project at the tail end of an entire class on Communication Networks, so I have
    enough knowledge of the underlying layers of the Internet now that I feel confident I could code
    this up using bare TCP or even UDP if I wanted to. Why don't I want to?

    First off, WebSockets already exists. No need to reinvent the wheel until we find it's insufficient
    for our purposes. Also, my advisor recommended it.

    Second, I like that WebSockets takes care of the authentication and handshake stuff for me. I don't
    have to have a panic attack over accidentally sending unencrypted TCP packets and having someone else
    see them.
*/

#include <WiFi.h>
#include <WebSocketsServer.h>

// Constants
// Something I'll probably want to use Python to rip up and change on-the-fly later on. Hm.
const char* ssid     = "___________________";
const char* password = "___________________";

#define BAUD_RATE 115200        // (lol) How quickly to resolve the 1s and 0s sent over Serial into ASCII.
#define LOOP_RATE   1000        // (ms)  How many milliseconds to pause before running the loop again.
#define DOT_RATE     125        // (ms)  How quickly the dots will appear over Serial as we go to the next loop.
#define ADC_PIN       36        // (---) Number pin we're going to use to read in from our MyoWare sensor.

#define WEBSOCKET_PORT 80       // WebSockets are designed to work over HTTP ports 80 and 443.
#define BROADCAST_LENGTH 128    // Maximum number of characters we can broadcast at once. You probably won't need to change this.


//----------------  #Global objects   ----------------------


WebSocketsServer webSocket = WebSocketsServer(WEBSOCKET_PORT);


int c;
int emg_in;
bool sending;
char broadcast_message[BROADCAST_LENGTH];

//----------------  #Global objects   ----------------------



// Called when receiving any WebSocket message
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  // Figure out the type of WebSocket event
  switch (type) {
    case WStype_DISCONNECTED:                         // Client has disconnected
      {
        Serial.printf("[%u] Disconnected!\n", num);
        break;
      }
    case WStype_CONNECTED:                            // New client has connected
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connection from ", num);
        Serial.println(ip.toString());
      }
      break;
    case WStype_TEXT:                                 // Echo text message back to client
      Serial.printf("[%u] Text: %s\n", num, payload);
      webSocket.sendTXT(num, payload);
      break;
    case WStype_BIN:                                  // For everything else: do nothing
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }
}


//----------------  #Global objects   ----------------------


void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial) { ; } // wait for serial port to connect.
  Serial.println("\n===\nSerial connection established\n===\n");

    // Connect to access point
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n===\nWiFi connection established\n===\n");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());

    // Start WebSocket server and assign callback
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  Serial.println("\n===\nWebSocket server established\n===\n");


  c = 0;

  Serial.println("\n===\nEntering loop()\n===\n");
  delay(1000);

}

/*
 * Here's how our loop works.
 *
 * First we read in from the ADC_PIN; this is the pin connected to the wire
 * that links the ESP-32 with your EMG sensor. Your EMG sensor is constantly
 * pushing out a voltage along this wire that the ESP-32 detects and assigns a
 * number to. The higher the number, the more voltage is being pushed along the
 * wire, the stronger a flex your EMG sensor is picking up.
 *
 * The range of numbers is between 0 and 4,000-something, I think 4096 but I'm
 * not sure of the exact amount; and the voltages those correspond to are actually
 * between 0 and 3.3 volts. So if your EMG sensor is constantly giving you maxed-out
 * readings when you flex just a little bit, it means it's sending out the maximum 3.3
 * volts. How do you fix that?
 *
 * You've got a few options.
 * - If there's a "gain knob" on your EMG sensor, try playing around with that.
 *   That knob actually dials up and down the resistance inside the sensor; higher resistances
 *   will lower your voltage out, and lower resistances will make it higher.
 *   (If you want greater sensitivity to more precise flexes, you can try lowering the gain knob!)
 * - You can also try reducing the voltage that goes into your EMG sensor in the first place.
 *   I wouldn't recommend this, because if you set the voltage too low, you might get some
 *   weird things happening with the integrated chips and whatnot on it. But it is an option.
 * - If you *really* want to get experimental, you can put a resistor in-between the
 *   EMG sensor and the ESP-32's ADC pin in. (Make sure you put it in series!) See, that
 *   3.3 volts or whatever that's coming in at the ADC pin -- that's got to be going through
 *   some later circuitry and going down to 0 volts anyway, meaning that you can think about
 *   the entire ESP-32 from that point on as basically a big resistor with some fancy side
 *   effects for running current through it. I have no idea what kind of resistors you would
 *   need for this, because I didn't try it myself -- but if I were going to, I would go down in
 *   powers of 10 until I found one that started to work, say 100 kiliohms to 10 kiliohms to 1 kiliohms,
 *   and then I would mess around with resistors in that range until I got something I could
 *   work with.
 */

void loop() {
  webSocket.loop();

  // First, we want to create the message we're going to be sending out.
  // First we're going to get the data from our ADC_PIN, then we're going
  // to make a String to send out out of that.

  emg_in = analogRead(ADC_PIN);
  sprintf(broadcast_message, "%u", emg_in);

  Serial.println("-------------------- STATUS --------------------");
  Serial.print("Looping rate (milliseconds)                 : ");
  Serial.println(LOOP_RATE);
  Serial.print("WiFi address (make sure this is 2.4 GHz)    : ");
  Serial.println(ssid);
  Serial.print("Local IP (copy and paste into Python code)  : ");
  Serial.println(WiFi.localIP());
  Serial.print("Connected clients: ");
  Serial.println(webSocket.connectedClients());

  Serial.print("Value coming in from our pin     : ");
  Serial.println(emg_in);

  Serial.print("Message being broadcasted        : ");
  if (webSocket.broadcastTXT(broadcast_message))
    Serial.println(broadcast_message);
  else {
    Serial.println("(Warning: Can't broadcast!");
    Serial.print("                                   "); // Proper spacing.
    Serial.println("Might be an error,");
    Serial.print("                                   "); // Proper spacing.
    Serial.println("might just be connecting to a new client.)");
  }

  for (int dot_time = LOOP_RATE; dot_time > 0; dot_time = dot_time - DOT_RATE) {
    Serial.print(".");
    delay(DOT_RATE);
  }
  Serial.println();
}
