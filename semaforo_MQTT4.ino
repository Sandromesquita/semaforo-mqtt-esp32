#include <WiFi.h>           // Importação da Biblioteca WiFi
#include <PubSubClient.h>   // Importação da Biblioteca PubSubClient

/* 
 *  IMPORTANTE: recomendamos fortemente alterar os nomes
               desses tópicos. Caso contrário, há grandes
               chances de você enviar e receber mensagens de um ESP32
               de outro usuário.
*/

// Tópico MQTT para recepção no ESP32 das informações do broker MQTT
#define TOPICO_SUBSCRIBE "SEMAFORO_recebe_informacao"   

// Tópico MQTT para envio de informações do ESP32 para broker MQTT
#define TOPICO_PUBLISH   "SEMAFORO_envia_informacao"  

// O ID MQTT serve para identificação da sessão
/* IMPORTANTE: este deve ser único no broker (ou seja, 
               se um client MQTT tentar entrar com o mesmo 
               id de outro já conectado ao broker, o broker 
               irá fechar a conexão de um deles).
*/
#define ID_MQTT  "Semaforo_Cliente_MQTT"     

const char* SSID = "Rede do WiFi"; 
const char* PASSWORD = "Senha do WiFi"; 
  
// URL do broker MQTT que deseja utilizar
const char* BROKER_MQTT = "broker.hivemq.com"; 
// Porta do Broker MQTT
int BROKER_PORT = 1883;
 
unsigned long tempo_atual = 0;
int tempo = 0;
String estado = "vermelho", msg = "normal";
bool status_yellow_led = 0;

#define red_led 14
#define yellow_led 12
#define green_led 13

WiFiClient espClient;
PubSubClient MQTT(espClient);
  
void inicializa_serial(void);
void inicializa_wifi(void);
void inicializa_mqtt(void);
void reconecta_wifi(void); 
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void verifica_conexoes_wifi_mqtt(void);
 
void setup() 
{
    inicializa_serial();
    inicializa_wifi();
    inicializa_mqtt();
    tempo_atual = millis();
    pinMode(red_led, OUTPUT);
    pinMode(yellow_led, OUTPUT);
    pinMode(green_led, OUTPUT);
}
  

void inicializa_serial(){
  Serial.begin(115200);
}


void inicializa_wifi(void) 
{
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    reconecta_wifi();
}
  
void inicializa_mqtt(void) 
{
    // informa a qual broker e porta deve ser conectado
    MQTT.setServer(BROKER_MQTT, BROKER_PORT); 
    /* atribui função de callback (função chamada quando qualquer informação do 
    tópico subescrito chega) */
    MQTT.setCallback(mqtt_callback);            
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    msg = "";
 
    //pega a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }
    Serial.print("[MQTT] Mensagem recebida: ");
    Serial.println(msg);
    if (msg == "alerta"){
      Serial.println("Semaforo em alerta, piscando amarelo.");
      digitalWrite(red_led, HIGH);
      digitalWrite(yellow_led, HIGH);
      digitalWrite(green_led, HIGH);
    }
}
  
void reconecta_mqtt(void) 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) 
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE); 
        } 
        else
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}
  
void reconecta_wifi() 
{
    /* se já está conectado a rede WI-FI, nada é feito. 
       Caso contrário, são efetuadas tentativas de conexão */
    if (WiFi.status() == WL_CONNECTED)
        return;
         
    WiFi.begin(SSID, PASSWORD);
     
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(100);
        Serial.print(".");
    }
   
    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}

void verifica_conexoes_wifi_mqtt(void)
{
    /* se não houver a conexão com o WiFI, será tentado a reconexão */
    reconecta_wifi(); 
    /* se não houver a conexão com o Broker, será tentado a reconexão */
    if (!MQTT.connected()) 
        reconecta_mqtt(); 
} 
 
void loop() 
{   
    verifica_conexoes_wifi_mqtt();

    tempo = millis() - tempo_atual;
    if ((tempo > 11000)and(estado == "amarelo")and(msg == "normal")){
      estado = "vermelho";
      MQTT.publish(TOPICO_PUBLISH, "Semaforo Vermelho.");
      digitalWrite(red_led, LOW);
      digitalWrite(yellow_led, HIGH);
      digitalWrite(green_led, HIGH);
      tempo_atual = millis();
    }
    else if ((tempo > 8000)and(estado == "verde")and(msg == "normal")){
      estado = "amarelo";
      MQTT.publish(TOPICO_PUBLISH, "Semaforo Amarelo.");
      digitalWrite(red_led, HIGH);
      digitalWrite(yellow_led, LOW);
      digitalWrite(green_led, HIGH);
    }
    else if ((tempo > 4000)and(estado == "vermelho")and(msg == "normal")){
      estado = "verde";
      MQTT.publish(TOPICO_PUBLISH, "Semaforo Verde.");
      digitalWrite(red_led, HIGH);
      digitalWrite(yellow_led, HIGH);
      digitalWrite(green_led, LOW);
    }
    else if (msg == "alerta"){
      MQTT.publish(TOPICO_PUBLISH, "Semaforo em ALERTA.");
      status_yellow_led = !status_yellow_led;
      digitalWrite(yellow_led, status_yellow_led);
      tempo_atual = millis();
    }
     
    MQTT.loop();
    delay(1000);   
}
