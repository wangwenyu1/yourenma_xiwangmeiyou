#define INTERVAL_SENSOR   10000             //定义传感器采样时间间隔 
#define INTERVAL_NET      10000             //定义发送时间
//#define USER_SEL_VERSION VERSION_18        //定义与wifi模块连接的串口，在.h文件中该波特率为9600

#include <I2Cdev.h> 
#include <Wire.h>  
#include <Microduino_SHT2x.h>                                
#include <ESP8266.h>              //调用库
#define  sensorPin_1  D6      //此处需要改接口
#define  sensorPin_2  A0
//传感器部分（人体红外）================================   

#define SSID        "LAPTOP-37O86VOL 9220" //改为你的Wi-Fi名称
#define PASSWORD    "#8l5549J"//Wi-Fi密码
//WIFI部分(ESP8266)==================================

#define HOST_NAME   "api.heclouds.com"
#define DEVICEID    "505096127" //OneNet上的设备ID
#define PROJECTID   "183685" //OneNet上的产品ID
#define HOST_PORT   (80)
String apiKey="eoRo3Eb=3mIMTGpLcLnlTFc4jsw=";//与你的产品绑定的APIKey
//OneNet部分=======================================  

#define IDLE_TIMEOUT_MS  3000                     //定义未收到数据的等待时间  
char buf[10]; 

#define INTERVAL_sensor 2000
unsigned long sensorlastTime = millis();

bool  hongwai;                                     //用于是否有人判断
//float lightOLED;                                   //用于灯亮判断
float sensor_lux;
char sensor_lux_c[7];

String mCottenData;
String jsonToSend;

#include <SoftwareSerial.h>
SoftwareSerial mySerial(2, 3); /* RX:D3, TX:D2 */        //使用软串口
#define EspSerial mySerial
ESP8266 wifi(&EspSerial);                           //定义一个ESP8266（wifi）的对象

unsigned long net_time1 = millis();                          //数据上传服务器时间
unsigned long sensor_time = millis();                        //传感器采样时间计时器

String postString;                                //用于存储发送数据的字符串

void setup(void)     //初始化函数  

{       
  //初始化串口波特率  
    Wire.begin();
    Serial.begin(115200);
    while (!Serial); // wait for Leonardo enumeration, others continue immediately
    Serial.print(F("setup begin\r\n"));
    delay(100);
    pinMode(sensorPin_1, INPUT);
    pinMode(sensorPin_2, INPUT);
    WifiInit(EspSerial, 9600);
    
    //ESP8266初始化
    Serial.print(F("FW Version:"));
    Serial.println(wifi.getVersion().c_str());
   if (wifi.setOprToStationSoftAP()) {
    Serial.print(F("to station + softap ok\r\n"));
  } else {
    Serial.print(F("to station + softap err\r\n"));
  }

     //加入无线网
  if (wifi.joinAP(SSID, PASSWORD)) {
    Serial.print(F("Join AP success\r\n"));
    Serial.print(F("IP:"));
    Serial.println( wifi.getLocalIP().c_str());
  } else {
    Serial.print(F("Join AP failure\r\n"));
  }
  if (wifi.disableMUX()) {
    Serial.print(F("single ok\r\n"));
  } else {
    Serial.print(F("single err\r\n"));
  }
  Serial.print(F("setup end\r\n"));
}

void loop(void)     
{   
  if (sensor_time > millis())  sensor_time = millis();  
    
  if(millis() - sensor_time > INTERVAL_SENSOR)              //传感器采样时间间隔  
  {  
    getSensorData();                                        //读串口中的传感器数据
    sensor_time = millis();
  }  
     
  if (net_time1 > millis())  net_time1 = millis();
  
  if (millis() - net_time1 > INTERVAL_NET)                  //发送数据时间间隔
  {                
    updateSensorData();                                     //将数据上传到服务器的函数
    net_time1 = millis();
  }
}

//通过与设定的阈值相比较判断是否有人函数==========================================
void getSensorData()
{  
    delay(1000);
    hongwai = digitalRead(sensorPin_1)==HIGH?true:false;
    sensor_lux = analogRead(A0); 
    dtostrf(sensor_lux, 3, 1, sensor_lux_c);
}

//建立TCP连接及数据传递函数============================================
void updateSensorData() {
  if (wifi.createTCP(HOST_NAME, HOST_PORT)) { 
    Serial.print("\ncreate tcp ok\r\n");

    jsonToSend="{\"hongwai\":";
    jsonToSend+=hongwai?"1":"0";                                //ture为1，false为0
    jsonToSend+=",\"Light\":";
    dtostrf(sensor_lux,1,2,buf);
    jsonToSend+="\""+String(buf)+"\"";
    jsonToSend+="}";

 Serial.print("注意：有人为1，没人为0");
 Serial.println("\nSend:"+jsonToSend);

    postString="POST /devices/";
    postString+=DEVICEID;
    postString+="/datapoints?type=3 HTTP/1.1";
    postString+="\r\n";
    postString+="api-key:";
    postString+=apiKey;
    postString+="\r\n";
    postString+="Host:api.heclouds.com\r\n";
    postString+="Connection:close\r\n";
    postString+="Content-Length:";
    postString+=jsonToSend.length();
    postString+="\r\n";
    postString+="\r\n";
    postString+=jsonToSend;
    postString+="\r\n";
    postString+="\r\n";
    postString+="\r\n";

  const char *postArray = postString.c_str();                 //将str转化为char数组
  wifi.send((const uint8_t*)postArray, strlen(postArray));    //send发送命令，参数必须是这两种格式，尤其是(const uint8_t*)
  Serial.println("send success");   
     
     //释放TCP连接
     if (wifi.releaseTCP()) {                                 
        Serial.print("release tcp ok\r\n");
        } 
     else {
        Serial.print("release tcp err\r\n");
        }
      postArray = NULL;                                       //清空数组，等待下次传输数据
 
  } else {
    Serial.print("\ncreate tcp err\r\n");
  }
  
}
