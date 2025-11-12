#define MQTT_MAX_PACKET_SIZE 512
#include <PubSubClient.h>
#include <Ethernet.h>
#include <LiquidCrystal.h>
#include <ArduinoJson.h>
#include <NTPClient.h>

#define LIGHT 4
#define MQTT_AUTH true
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
#define JSON_BUFFER_SIZE 512

// ======================== VARIAVEIS DE DEFINIÇÃO DO AMBIENTE ====================== //

// Dipfinição do sensor de distância
const int trigPin = 7;
const int echoPin = 6;

// Definição dos LEDs
const int ledVerde = 8;
const int ledVermelho = 13;

// Variaveis de controle
long duracao;
int distancia;
bool statusServico;
String statusAtualProduto = "Desconhecido";

// Eventos de atualização
const String PECA_APROVADA  = "peca_aprovada";
const String PECA_REPROVADA = "peca_reprovada";

// constantes de envio ao MQTT
const String HOSTNAME   = "test.mosquitto.org";
const char* MQTT_SERVER = "test.mosquitto.org";

// Constantes de topicos do MQTT
const String MQTT_TOPIC_BASE      = "iot/riodosul/si/BSN22025T26F8/cell/09/device/c09-ayrton-djonatan/";
const String MQTT_ACTION_TOPIC    = MQTT_TOPIC_BASE + "cmd";
const String MQTT_TOPIC_TELEMETRY = MQTT_TOPIC_BASE + "telemetry";
const String MQTT_TOPIC_STATE     = MQTT_TOPIC_BASE + "state";
const String MQTT_TOPIC_EVENT     = MQTT_TOPIC_BASE + "event";

byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};

// Libs
EthernetClient client;
EthernetUDP ntpUDP;
PubSubClient clientMqtt(MQTT_SERVER, 1883, client);
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3*3600);
LiquidCrystal lcd_1(12, 11, 5, 4, 3, 2);


// ===================== INICIO LÓGICA DO SISTEMA =================== //

// Valida a conexão com a internet
bool checkEthernetConnection(){
  if(Ethernet.linkStatus() == LinkOFF){
    Serial.println("tentando se conectar a internet...");
    Ethernet.begin(mac);

    Serial.println("Conectado a internet!");
    Serial.println(Ethernet.localIP());
    Serial.println(Ethernet.subnetMask());
    Serial.println(Ethernet.gatewayIP());
    Serial.println(Ethernet.dnsServerIP());    
    return false;
  }

  return true;
}

// Valida a conexão com o MQTT
bool checkMqttConnection(){
  if (!clientMqtt.connected()) {
    Serial.print("Tentando conectar ao MQTT... ");
    if (MQTT_AUTH ?
        clientMqtt.connect(HOSTNAME.c_str(), MQTT_USERNAME, MQTT_PASSWORD) :
        clientMqtt.connect(HOSTNAME.c_str())) {
      Serial.println("CONECTADO!");
      clientMqtt.subscribe(MQTT_ACTION_TOPIC.c_str());
    } else {
      Serial.print("Falha, ao Conectar...");
      Serial.println(clientMqtt.state());

      delay(5000);
    }
  }

  return clientMqtt.connected();
}

// Função de callback para recebimento de dados
void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<256> jsonDoc;

  // Convertendo payload para string
  String payloadStr;
  payloadStr.reserve(length);
  for (int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }

  Serial.print("Mensagem recebida: ");
  Serial.println(payloadStr);

  // Valida se é o tópico esperado
  if (String(topic) != MQTT_ACTION_TOPIC) return;

  DeserializationError error = deserializeJson(jsonDoc, payloadStr);

  if (!error) {
    const char* action = jsonDoc["action"];
    if (action != nullptr) {

      if (strcasecmp(action, "get_status") == 0) {
        statusServico = true;
        turnOn();
        return;
      }
    }
  }

  if (payloadStr.equalsIgnoreCase("get_status")) {
    statusServico = true;
    turnOn();
    return;
  }

  Serial.println("Comando desconhecido!");
}

// função que inicializa o sistema
void turnOn(){
  disparaStateAplicacao("On");

  delay(5000);

  do{
    trataDadosEnvioMqtt();

    if(!clientMqtt.connected()){
      checkEthernetConnection();
      checkMqttConnection();
    }

  } while(statusServico);
}

// Realiza o tratamento inicial de dados para envio ao MQTT
void trataDadosEnvioMqtt(){
  int distanciaCalculada = realizaCalculoDistanciaSensor();
  
  mostraDadosSerial(distanciaCalculada);
  mostraDadosLcd(distanciaCalculada);
  trataLeds(distanciaCalculada);

  String status = validaStatusAtual(distanciaCalculada);

  disparaJsonTelemetry(distanciaCalculada, status);

  if(!statusAtualProduto.equals(status)){
    statusAtualProduto = status;

    if(status.equals("Aprovado")){
      disparaJsonEventoAlteracaoSituacao(status, PECA_APROVADA);
    } else {
      disparaJsonEventoAlteracaoSituacao(status, PECA_REPROVADA);
    }
  }

// Atraso de 3 segundos entre as requisições
  delay(3000);
}


// ==================== TRATAMENTO DE DADOS E COMPONENTES =============== //

// Realiza a captação e calculo dos dados do sensor de distância
int realizaCalculoDistanciaSensor(){
  // Gera pulso no TRIG
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Mede o tempo no ECHO
  duracao = pulseIn(echoPin, HIGH);

  // Calcula a distância em cm
  distancia = duracao * 0.034 / 2;

  return distancia;
}

// Retorna as informações do produto no Serial monitor
void mostraDadosSerial(int distancia){
   // Retorna os valores calculados na serial
  Serial.print("Distancia: ");
  Serial.print(distancia);
  Serial.print(" cm");
  Serial.print("\n");
}

// Retorna a informação no lcd
void mostraDadosLcd(int distancia){
  //retorna os valores no lcdjsonDoc
  lcd_1.setCursor(0, 0);
  lcd_1.print("Distancia ");
  lcd_1.setCursor(0, 1);
  lcd_1.print(distancia);
  lcd_1.print(" cm");
}

// retorna o status atual do produto pelo qual foi realizada a medição
// Regras: Menor que 5cm e maior que 7cm = reprovado
//         Entre 5cm e 7cm = aprovado
String validaStatusAtual(int distancia){
  String statusProduto;

    if (distancia < 5 || distancia > 7) {
      statusProduto = "Reprovado";
    } else {
      statusProduto = "Aprovado";
    }

  return statusProduto;
}

// realiza o tratamento dos leds
void trataLeds(float distancia){
  if (distancia < 5 || distancia > 7) {
      digitalWrite(ledVermelho, LOW);
      digitalWrite(ledVerde, HIGH);
    } else {
      digitalWrite(ledVerde, LOW);
      digitalWrite(ledVermelho, HIGH);
    }
}


// ===================== ENVIO DE DADOS MQTT ================== //

// Dispara os dados de Telemetry
void disparaJsonTelemetry(int distancia, String status){
  StaticJsonDocument<512> Json;
  StaticJsonDocument<512> JsonMetricas;
  StaticJsonDocument<512> JsonThresholds;

  JsonMetricas["dist_cm"]   = distancia;
  JsonMetricas["qualidade"] = status;

  JsonThresholds["min_cm"] = 5.0;
  JsonThresholds["max_cm"] = 7.0;

  Json["ts"]         = agoraEpochStr();
  Json["cellId"]     = 9;
  Json["devId"]      = "c09-ayrton-djonatan";
  Json["metrics"]    = JsonMetricas;
  Json["status"]     = status;
  Json["units"]      = "cm";
  Json["thresholds"] = JsonThresholds;

  char buf[360];
  size_t n = serializeJson(Json, buf, sizeof(buf));

  disparaRequestMqtt(buf, n, MQTT_TOPIC_TELEMETRY);
}

// Dispara os dados de event (quando possui alteração)
void disparaJsonEventoAlteracaoSituacao(String statusProduto, String evento){
  StaticJsonDocument<512> Json;

  Json["ts"]   = agoraEpochStr();
  Json["type"] = evento;
  Json["info"] = statusProduto;

  char buf[360];
  size_t n = serializeJson(Json, buf, sizeof(buf));

  disparaRequestMqtt(buf, n, MQTT_TOPIC_EVENT);
}

// Dispara a requisição ao MQTT (função para envio padrão de topico JSON)
void disparaRequestMqtt(char* buf, size_t n, String Topic){
  if(clientMqtt.connected()) {
    clientMqtt.publish(Topic.c_str(), (const uint8_t*)buf, n, /*retained=*/true);
    Serial.print("Mensagem enviada: ");
    Serial.println(buf);
  } else {
    Serial.println("Sem conexão com o MQTT, tentando reconectar");
    Serial.println(buf);
  }
}

// Dispara informação com o status da Aplicação
void disparaStateAplicacao(String status){
  clientMqtt.publish(MQTT_TOPIC_STATE.c_str(), status.c_str());
}

// =================== METODOS PADRÃO OBRIGATORIOS ================= //
void setup() {
  // Inicia a Serial
  Serial.begin(9600);
  pinMode(LIGHT, OUTPUT);

  // Realiza a conexão com a internet
  Ethernet.begin(mac);
  Serial.println("Conectado a internet!");
  Serial.println(Ethernet.localIP());
  Serial.println(Ethernet.subnetMask());
  Serial.println(Ethernet.gatewayIP());
  Serial.println(Ethernet.dnsServerIP()); 
  clientMqtt.setCallback(callback);
  
  // Inicia o LCD
  lcd_1.begin(16, 2);
  lcd_1.print("Iniciando!");
  delay(1000);
  lcd_1.clear();
  
  // Inicia os leds
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledVermelho, OUTPUT);
}

void loop() {
  // Checa a conexão com a internet
  if(checkEthernetConnection()){
    // Checa a conexão com o MQTT
    if(checkMqttConnection()){
      clientMqtt.loop();
    }
  }
}


// ======================== METODO AUXILIAR ======================= //

// Gera um dado timestamp para uso nos envios de JSON
String agoraEpochStr() {
  unsigned long s = millis() / 1000UL;
  return String(s);
}