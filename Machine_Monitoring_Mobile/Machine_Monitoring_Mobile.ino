#include <WiFi.h>
#include <FirebaseESP32.h>
#include "time.h"

#define FIREBASE_HOST ""
#define FIREBASE_AUTH ""
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// Firebase path
String path = "/Machines/Line 1/PD1/";
String machineStatusPath = path + "machineStatus";
String machineBreakdownTime = path + "machineBreakdownTime";


// Time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 21600; // GMT +7
const int   daylightOffset_sec = 3600;
char currentTime[20];


// LED
const int ledRunning = 13;
const int ledBreakdown = 14;
const int ledRepair = 27;
const int ledConfirm = 33;

// Button
const int btnBreakdown = 26;
const int btnConfirm = 25;

// Button State
int btnBreakdownState = 0;
int btnConfirmState = 0;

//Define FirebaseESP32 data object
FirebaseData firebaseData;

FirebaseJson json;

void printResult(FirebaseData &data);

void setup()
{

  Serial.begin(115200);

  pinMode(ledRunning, OUTPUT);
  pinMode(ledBreakdown, OUTPUT);
  pinMode(ledRepair, OUTPUT);
  pinMode(ledConfirm, OUTPUT);
  
  pinMode(btnBreakdown, INPUT);
  pinMode(btnConfirm, INPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  //Set database read timeout to 1 minute (max 15 minutes)
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.setwriteSizeLimit(firebaseData, "tiny");

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

} //End of void setup()


void loop()
{

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  strftime(currentTime, sizeof(currentTime), "%d-%m-%Y %H:%M:%S", &timeinfo);
  Serial.println(currentTime);
  Serial.println();


  btnBreakdownState = digitalRead(btnBreakdown);
  btnConfirmState = digitalRead(btnConfirm);
  
  Serial.println(btnBreakdownState);
  Serial.println(btnConfirmState);

  if (Firebase.get(firebaseData, machineStatusPath))
    {
      String machineStatus = firebaseData.stringData();
      Serial.println("PASSED");
      Serial.println("PATH: " + firebaseData.dataPath());
      Serial.print("STATUS: ");
      Serial.print(machineStatus);
      Serial.println("------------------------------------");
      Serial.println();

      if (machineStatus == "1") {
        digitalWrite(ledRunning, HIGH);
        digitalWrite(ledBreakdown, LOW);
        digitalWrite(ledRepair, LOW);
        digitalWrite(ledConfirm, LOW);
      } else if (machineStatus == "2") {
        digitalWrite(ledRunning, LOW);
        digitalWrite(ledBreakdown, HIGH);
        digitalWrite(ledRepair, LOW);
        digitalWrite(ledConfirm, LOW);
      } else if (machineStatus == "3") {
        digitalWrite(ledRunning, LOW);
        digitalWrite(ledBreakdown, LOW);
        digitalWrite(ledRepair, HIGH);
        digitalWrite(ledConfirm, LOW);
      } else if (machineStatus == "4") {
        digitalWrite(ledRunning, LOW);
        digitalWrite(ledBreakdown, LOW);
        digitalWrite(ledRepair, LOW);
        digitalWrite(ledConfirm, HIGH);
      }
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + firebaseData.errorReason());
      Serial.println("------------------------------------");
      Serial.println();
    }
      
  if (btnBreakdownState == HIGH) {
    Firebase.set(firebaseData, machineStatusPath, "2");
    Firebase.setString(firebaseData, machineBreakdownTime, currentTime);
  } 

  if (btnConfirmState == HIGH) {
    Firebase.set(firebaseData, machineStatusPath, "1");
  }
  
  delay(200);
  
}  //End of void loop()



void printResult(FirebaseData &data)
{

  if (data.dataType() == "int")
    Serial.println(data.intData());
  else if (data.dataType() == "float")
    Serial.println(data.floatData(), 5);
  else if (data.dataType() == "double")
    printf("%.9lf\n", data.doubleData());
  else if (data.dataType() == "boolean")
    Serial.println(data.boolData() == 1 ? "true" : "false");
  else if (data.dataType() == "string")
    Serial.println(data.stringData());
  else if (data.dataType() == "json")
  {
    Serial.println();
    FirebaseJson &json = data.jsonObject();
    //Print all object data
    Serial.println("Pretty printed JSON data:");
    String jsonStr;
    json.toString(jsonStr, true);
    Serial.println(jsonStr);
    Serial.println();
    Serial.println("Iterate JSON data:");
    Serial.println();
    size_t len = json.iteratorBegin();
    String key, value = "";
    int type = 0;
    for (size_t i = 0; i < len; i++)
    {
      json.iteratorGet(i, type, key, value);
      Serial.print(i);
      Serial.print(", ");
      Serial.print("Type: ");
      Serial.print(type == FirebaseJson::JSON_OBJECT ? "object" : "array");
      if (type == FirebaseJson::JSON_OBJECT)
      {
        Serial.print(", Key: ");
        Serial.print(key);
      }
      Serial.print(", Value: ");
      Serial.println(value);
    }
    json.iteratorEnd();
  }
  else if (data.dataType() == "array")
  {
    Serial.println();
    //get array data from FirebaseData using FirebaseJsonArray object
    FirebaseJsonArray &arr = data.jsonArray();
    //Print all array values
    Serial.println("Pretty printed Array:");
    String arrStr;
    arr.toString(arrStr, true);
    Serial.println(arrStr);
    Serial.println();
    Serial.println("Iterate array values:");
    Serial.println();
    for (size_t i = 0; i < arr.size(); i++)
    {
      Serial.print(i);
      Serial.print(", Value: ");

      FirebaseJsonData &jsonData = data.jsonData();
      //Get the result data from FirebaseJsonArray object
      arr.get(jsonData, i);
      if (jsonData.typeNum == FirebaseJson::JSON_BOOL)
        Serial.println(jsonData.boolValue ? "true" : "false");
      else if (jsonData.typeNum == FirebaseJson::JSON_INT)
        Serial.println(jsonData.intValue);
      else if (jsonData.typeNum == FirebaseJson::JSON_DOUBLE)
        printf("%.9lf\n", jsonData.doubleValue);
      else if (jsonData.typeNum == FirebaseJson::JSON_STRING ||
               jsonData.typeNum == FirebaseJson::JSON_NULL ||
               jsonData.typeNum == FirebaseJson::JSON_OBJECT ||
               jsonData.typeNum == FirebaseJson::JSON_ARRAY)
        Serial.println(jsonData.stringValue);
    }
  }
}
