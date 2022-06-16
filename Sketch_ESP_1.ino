#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//************//
const char* ssid = "AgroNetwork";
const char* password = "xxxxxxxxx";
const char* mqtt_server = "192.168.4.1";

//************//
int id[2] = {16, 17};
#define TOPICO1 "area/umidadesolo/08" //determina o topico de envio de mensagens
#define TOPICO2 "area/temperatura/08"
uint32_t timer = 0;
//Pino sensor umidade do solo - Pino padrão A0
#define PIN_ANALOGICO A0
int value_umidade;
//Obetos - sensor Temperatura
OneWire pin(4); //Pino DATA, pino 3 do arduino
DallasTemperature barrament(&pin);
DeviceAddress sensor;
//******************

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (128)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
//VARIAVEIS GLOBAIS
String data;
float umidade;
float temperature;

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  pinMode(PIN_ANALOGICO, INPUT); //porta analogica
  barrament.begin();
  barrament.getAddress(sensor, 0);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  unsigned long now = millis();
  if (now - lastMsg > 2000) {
        lastMsg = now;
        data = serializeJson(id[0], getUmid(), "umidade");
        data.toCharArray(msg, 200);
        client.publish(TOPICO1, msg);
        Serial.println(msg);
        data = serializeJson(id[1], getTemp(), "temperatura");
        data.toCharArray(msg, 200);
        client.publish(TOPICO2, msg);
        Serial.println(msg);
    }

}

String serializeJson(int id, float value, String type)
{
    String data;
    data = "{\"id_dispositivo\": ";
    data += id;
    data += ", ";
    data += "\"dado\": ";
    data += "{\"";
    data += type;
    data += "\": ";
    data += value;
    data += " }}";
    return data;
}

float getTemp()
{
    barrament.requestTemperatures();
    temperature = barrament.getTempC(sensor);

    return temperature;
}

const int pinoSensor = A0; //PINO UTILIZADO PELO SENSOR
int valorLido; //VARIÁVEL QUE ARMAZENA O PERCENTUAL DE UMIDADE DO SOLO
 
int analogSoloSeco = 573; //VALOR MEDIDO COM O SOLO SECO (VOCÊ PODE FAZER TESTES E AJUSTAR ESTE VALOR)
int analogSoloMolhado = 307; //VALOR MEDIDO COM O SOLO MOLHADO (VOCÊ PODE FAZER TESTES E AJUSTAR ESTE VALOR)
int percSoloSeco = 0; //MENOR PERCENTUAL DO SOLO SECO (0% - NÃO ALTERAR)
int percSoloMolhado = 100; //MAIOR PERCENTUAL DO SOLO MOLHADO (100% - NÃO ALTERAR)

float getUmid()
{
  
    valorLido = constrain(analogRead(pinoSensor),analogSoloMolhado,analogSoloSeco); //MANTÉM valorLido DENTRO DO INTERVALO (ENTRE analogSoloMolhado E analogSoloSeco)
    valorLido = map(valorLido,analogSoloMolhado,analogSoloSeco,percSoloMolhado,percSoloSeco); //EXECUTA A FUNÇÃO "map" DE ACORDO COM OS PARÂMETROS PASSADOS

    return valorLido;
}
