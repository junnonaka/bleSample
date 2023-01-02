
#include <M5Core2.h>
#include <NimBLEDevice.h>

#define LOCAL_NAME                  "TEST BLE"
#define COMPLETE_LOCAL_NAME         "TEST BLE"

#define SERVICE_UUID                                    "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE"
#define CHARACTERISTIC_UUID_WRITE                       "AAAAAAAA-AAAA-BBBB-BBBB-BBBBBBBBBBBB"
#define CHARACTERISTIC_UUID_READ                        "AAAAAAAA-CCCC-BBBB-BBBB-BBBBBBBBBBBB"
#define CHARACTERISTIC_UUID_NOTIFY                      "AAAAAAAA-DDDD-BBBB-BBBB-BBBBBBBBBBBB"
#define CHARACTERISTIC_UUID_INDICATE                    "AAAAAAAA-EEEE-BBBB-BBBB-BBBBBBBBBBBB"

//画像
#include "image.cpp"
//Characteristic
NimBLECharacteristic * pNotifyCharacteristic;
NimBLECharacteristic * pIndicateCharacteristic;
NimBLECharacteristic * pWriteCharacteristic;
NimBLECharacteristic * pReadCharacteristic;

//NimBLEServer
NimBLEServer *pServer = NULL;

//接続状態管理
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool isConnected = false;

//Buttonの押下管理
bool isButtonAPressed = false;
bool isButtonBPressed = false;
bool isButtonCPressed = false;

//Serviceのコールバック
class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        Serial.println("Client connected");
        deviceConnected = true;
    };
    void onDisconnect(NimBLEServer* pServer) {
        Serial.println("Client disconnected - start advertising");
        deviceConnected = false;
    };
    void onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) {
        Serial.printf("MTU updated: %u for connection ID: %u\n", MTU, desc->conn_handle);
    };
};

//BLECharacteristicコールバック
class MyCallbacks: public BLECharacteristicCallbacks {
    //Writeコールバック
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      M5.Lcd.drawString("write from central", 60, 150);
      M5.Lcd.drawString(rxValue.c_str(), 60, 170);
      delay(300);
      M5.Lcd.drawString("                                     ", 0,150);
      M5.Lcd.drawString("                                     ", 0,170);
      Serial.println(rxValue.c_str());
    }
    //Readコールバック    
    void onRead(BLECharacteristic *pCharacteristic) {
      M5.Lcd.drawString("read from central", 60, 150);
      delay(300);
      M5.Lcd.drawString("                                     ", 0,150);
      pCharacteristic->setValue("READ From M5Stack");
    } 

};




void setup() {
    // Initialize the M5Stack object
  M5.begin();
  Serial.begin(115200);
  M5.Lcd.print("Setup....");

  M5.Lcd.println("Done");
  M5.Lcd.clear(TFT_BLACK);

  M5.Lcd.setTextSize(2);
  M5.Lcd.drawString("DisConnected    ", 90, 115);
  
  Serial.println("Starting NimBLE Server");
  //CompleteLocalNameのセット
  NimBLEDevice::init(COMPLETE_LOCAL_NAME);
  //TxPowerのセット
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  //セキュリティセッティング
  //bonding,MITM,sc
  //セキュリティ無し
  NimBLEDevice::setSecurityAuth(false, false, false);
  //サーバー作成
  pServer = NimBLEDevice::createServer();
  //サーバーにコールバック設定
  pServer->setCallbacks(new ServerCallbacks());
  //サーバーにUUIDをセット
  NimBLEService *pService = pServer->createService(SERVICE_UUID);

  //pWriteCharacteristicをセット
  pWriteCharacteristic= pService->createCharacteristic(CHARACTERISTIC_UUID_WRITE, NIMBLE_PROPERTY::WRITE);
  //pWriteCharacteristicにコールバックをセット
  pWriteCharacteristic->setCallbacks(new MyCallbacks());
  //pNotifyCharacteristicをセット
  pNotifyCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_NOTIFY, NIMBLE_PROPERTY::NOTIFY);
  //pIndicateCharacteristicをセット
  pIndicateCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_INDICATE,NIMBLE_PROPERTY::INDICATE);
  //pReadCharacteristicをセット
  pReadCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_READ,NIMBLE_PROPERTY::READ);
  //pReadCharacteristicにコールバックをセット
  pReadCharacteristic->setCallbacks(new MyCallbacks());
  //Serivice開始
  pService->start();
  //アドバタイズの設定
  NimBLEAdvertising *pNimBleAdvertising = NimBLEDevice::getAdvertising();
  //アドバタイズするUUIDのセット
  pNimBleAdvertising->addServiceUUID(SERVICE_UUID);
  //アドバタイズにTxPowerセット
  pNimBleAdvertising->addTxPower();

  //アドバタイズデータ作成
  NimBLEAdvertisementData advertisementData;
  //アドバタイズにCompleteLoacaNameセット
  advertisementData.setName(COMPLETE_LOCAL_NAME);  
  //アドバタイズのManufactureSpecificにデータセット
  advertisementData.setManufacturerData("NORA");  
  //ScanResponseを行う
  pNimBleAdvertising->setScanResponse(true);
  //ScanResponseにアドバタイズデータセット
  pNimBleAdvertising->setScanResponseData(advertisementData);  
  //アドバタイズ開始
  pNimBleAdvertising->start();
  
}

// Bluetooth LE loop
void loopBLE() {
    // 切断中
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // BLEスタックの準備時間
        pServer->startAdvertising(); // アドバタイズの再開
        Serial.println("startAdvertising");
        oldDeviceConnected = deviceConnected;
        //画面をDisConnectedに変更
        M5.Lcd.setTextSize(2);
        M5.Lcd.drawString("DisConnected    ", 90, 115);
        M5.Lcd.drawString("            ", 100, 95);
        M5.Lcd.fillRect(2, 10, 43, 54, BLACK);
        isConnected = false;
         M5.Lcd.drawString("                           ",0, 200);

    }
    // 接続中
    if (deviceConnected && !oldDeviceConnected){
        oldDeviceConnected = deviceConnected;
      //画面をConnectedに変更
       M5.Lcd.setTextSize(2);
       M5.Lcd.drawString("Connected     ", 90, 115);
       M5.Lcd.drawBitmap(3,10,41,52,bleimage);


       M5.Lcd.setTextSize(2);
       M5.Lcd.drawString("disConne",0, 200);
       M5.Lcd.drawString("notify",120, 200);
       M5.Lcd.drawString("indicate",220, 200);


       isConnected = true;
    }
    //ボタンAが押された時の処理：Notifyの送信
    if (M5.BtnA.pressedFor(100)) {
      if(isButtonAPressed == false){
        if (isConnected){
          pServer->disconnect(0);
        }
      }
    //ボタンBが押された時の処理：Notifyの送信
    }else if(M5.BtnB.pressedFor(100)){
      if(isButtonBPressed == false){
        if (isConnected){
          M5.Lcd.drawString("Notify from m5Stack", 50, 150);
          delay(300);
          M5.Lcd.drawString("                                     ", 0,150);
          Serial.println("BtnB pressed");
          isButtonBPressed = true;
          pNotifyCharacteristic->setValue("Notify:From m5Stack");
          pNotifyCharacteristic->notify();
        }
      }
    //ボタンCが押された時の処理：Indicateの送信
    }else if(M5.BtnC.pressedFor(100)){
      if(isButtonCPressed == false){
        if (isConnected){

          M5.Lcd.drawString("Indicate from m5Stack", 50, 150);
          delay(300);
          M5.Lcd.drawString("                                     ", 0,150);
          Serial.println("BtnC pressed");
          isButtonCPressed = true;
          pIndicateCharacteristic->setValue("Indicate:From m5Stack");
          pIndicateCharacteristic->indicate();
        }
      }
    }else{
    }
    
    if (M5.BtnA.isReleased()) {
      if(isButtonAPressed == true){
        isButtonAPressed = false;
        Serial.println("BtnA rereiced");
        
      }
    }
    
    if(M5.BtnB.isReleased()){
      if(isButtonBPressed == true){
        isButtonBPressed = false;
        Serial.println("BtnB rereiced");
      }
    }

    if(M5.BtnC.isReleased()){
        if(isButtonCPressed == true){
        isButtonCPressed = false;
        Serial.println("BtnC rereiced");
      }
    }

    
}

void loop() {
  M5.update();
  loopBLE();
}
