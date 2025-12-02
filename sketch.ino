#include <Adafruit_SSD1306.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>


#define BTN_UP       6
#define BTN_DOWN     7
#define SERVO_PIN    2
#define BUZZER_PIN   3
#define LED_RED      4
#define LED_GREEN    5
#define OLED_SDA     8
#define OLED_SCL     9


int currentDigit = 0;
int pin[4] = {0};
int pinIndex = 0;
bool doorLocked = true;


unsigned long lastStableTime = 0;
int lastStableDigit = -1;


SemaphoreHandle_t xMutex;
SemaphoreHandle_t xPinCompleteSemaphore;


Adafruit_SSD1306 display(128, 64, &Wire, -1);


void taskInput(void *pvParameters);
void taskDisplay(void *pvParameters);
void taskValidator(void *pvParameters);


void setServoAngle(int angle) {
  int pulseWidth = map(angle, 0, 180, 500, 2500);
  
  
  for(int i = 0; i < 10; i++) { 
    digitalWrite(SERVO_PIN, HIGH);
    delayMicroseconds(pulseWidth);
    digitalWrite(SERVO_PIN, LOW);
    delay(20); 
  }
}

void setup() {
  Serial.begin(115200);
  
  
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(SERVO_PIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  
  setServoAngle(0);
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);

  
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display();
  delay(1000);

  
  xMutex = xSemaphoreCreateMutex();
  xPinCompleteSemaphore = xSemaphoreCreateBinary();

  
  xTaskCreate(taskInput, "Input Task", 2048, NULL, 2, NULL);
  xTaskCreate(taskDisplay, "Display Task", 2048, NULL, 1, NULL);
  xTaskCreate(taskValidator, "Validator Task", 2048, NULL, 1, NULL);

  Serial.println(" System Started - Ready for PIN Input");
  Serial.println(" PIN: 1-2-3-4 to unlock");
}

void loop() {
  vTaskDelete(NULL);
}

void taskInput(void *pvParameters) {
  int lastButtonUp = HIGH, lastButtonDown = HIGH;

  while(1) {
    int btnUp = digitalRead(BTN_UP);
    int btnDown = digitalRead(BTN_DOWN);

    
    if (btnUp == LOW && lastButtonUp == HIGH) {
      xSemaphoreTake(xMutex, portMAX_DELAY);
      currentDigit = (currentDigit + 1) % 10;
      Serial.print("UP: ");
      Serial.println(currentDigit);
      xSemaphoreGive(xMutex);
      tone(BUZZER_PIN, 800, 100);
      delay(200); 
    }

    
    if (btnDown == LOW && lastButtonDown == HIGH) {
      xSemaphoreTake(xMutex, portMAX_DELAY);
      currentDigit = (currentDigit - 1 + 10) % 10;
      Serial.print("DOWN: ");
      Serial.println(currentDigit);
      xSemaphoreGive(xMutex);
      tone(BUZZER_PIN, 800, 100);
      delay(200); 
    }

    lastButtonUp = btnUp;
    lastButtonDown = btnDown;

    
    xSemaphoreTake(xMutex, portMAX_DELAY);
    if (currentDigit == lastStableDigit) {
      if ((millis() - lastStableTime) > 3000) {
       
        pin[pinIndex] = currentDigit;
        Serial.print(" Digit ");
        Serial.print(pinIndex + 1);
        Serial.print(" saved: ");
        Serial.println(currentDigit);
        
        pinIndex = (pinIndex + 1) % 4;
        tone(BUZZER_PIN, 1200, 300);
        
        if (pinIndex == 0) {
          xSemaphoreGive(xPinCompleteSemaphore);
          Serial.println(" PIN Complete - Validating..");
        }
        
        currentDigit = 0;
        lastStableDigit = -1;
      }
    } else {
      lastStableDigit = currentDigit;
      lastStableTime = millis();
    }
    xSemaphoreGive(xMutex);

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void taskDisplay(void *pvParameters) {
  while(1) {
    xSemaphoreTake(xMutex, portMAX_DELAY);
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);

    display.println(" 2-Button PIN System");
    display.println("UP/DOWN: Change Digit");
    display.println("Wait 3s: Auto-Confirm");
    display.println("-------------------");
    
    display.print("Current: [");
    display.print(currentDigit);
    display.println("]");
    
    display.print("PIN: ");
    for(int i=0; i<4; i++) {
      if(i < pinIndex) {
        display.print(pin[i]);
      } else if(i == pinIndex) {
        display.print("[");
        display.print(currentDigit);
        display.print("]");
      } else {
        display.print("_");
      }
      display.print(" ");
    }
    
    
    
    
    display.display();
    xSemaphoreGive(xMutex);

    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void taskValidator(void *pvParameters) {
  while(1) {
    if (xSemaphoreTake(xPinCompleteSemaphore, portMAX_DELAY)) {
      xSemaphoreTake(xMutex, portMAX_DELAY);
      
      
      bool correct = (pin[0] == 1 && pin[1] == 2 && pin[2] == 3 && pin[3] == 4);
      
      if (correct) {
        
        Serial.println(" ACCESS GRANTED! Door UNLOCKED");
        doorLocked = false;
        setServoAngle(90); 
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_RED, LOW);
        tone(BUZZER_PIN, 2000, 1000);
        
        
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0,0);
        display.println(" ACCESS GRANTED!");
        display.println("PIN: CORRECT");
        display.println("Door: UNLOCKED");
        display.println("Welcome!");
        display.display();
      } else {
       
        Serial.println(" ACCESS DENIED! Wrong PIN");
        doorLocked = true;
        setServoAngle(0); 
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GREEN, LOW);
        tone(BUZZER_PIN, 500, 1000);
        
        
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0,0);
        display.println(" ACCESS DENIED!");
        display.println("PIN: INCORRECT");
        display.println("Door: LOCKED");
        display.print("Entered: ");
        for(int i=0; i<4; i++) {
          display.print(pin[i]);
        }
        display.display();
      }
      
      xSemaphoreGive(xMutex);

      
      vTaskDelay(4000 / portTICK_PERIOD_MS);

      
      xSemaphoreTake(xMutex, portMAX_DELAY);
      for(int i=0; i<4; i++) pin[i] = 0;
      pinIndex = 0;
      currentDigit = 0;
      doorLocked = true;
      setServoAngle(0);
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      xSemaphoreGive(xMutex);
      
      Serial.println(" System Reset - Ready for New PIN");
    }
  }
}
