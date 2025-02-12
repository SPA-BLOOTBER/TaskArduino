//Нажатие - температура / влажность
//Зажатие - изменение врменени


#include <Arduino.h>
#include <DHT.h>        // Библиотека для работы с DHT11
#include <avr/wdt.h>   // Библиотека для Watchdog Timer

// ----- Пины и константы (как в Задании 1) -----
const int segmentPins[] = {2, 3, 4, 5, 6, 7, 8, 9};
const int digitPins[] = {10, 11, 12, 13};
const int button1Pin = A0;
const int button2Pin = A1;
const int button3Pin = A2;
const unsigned long longPressThreshold = 1000;

// ----- Пины и константы для DHT11 -----
#define DHTPIN A3     // Пин, к которому подключен DHT11 (DATA пин)
#define DHTTYPE DHT11   // Тип датчика DHT11
DHT dht(DHTPIN, DHTTYPE);

// ----- Переменные (дополнения для Задания 2) -----
int hours = 13;
int minutes = 25;
int seconds = 0;
bool editMode = false;
unsigned long button1PressTime = 0;

enum DisplayMode { TIME_MODE, TEMP_MODE, HUM_MODE, ERROR_MODE }; // Режимы отображения
DisplayMode currentDisplayMode = TIME_MODE;

float temperature = 0.0;
float humidity = 0.0;
bool dhtError = false;


// ----- Функция отображения цифры (как в Задании 1) -----
void displayDigit(int digit, int number, bool dot) {
    // ... (код функции displayDigit из Задания 1 - без изменений) ...
  for (int i = 0; i < 8; i++) {
    digitalWrite(segmentPins[i], LOW); // Выключаем все сегменты
  }
  digitalWrite(digitPins[digit], LOW); // Включаем нужный разряд

  byte segments = 0b00000000; // Маска для сегментов
  if (number >= 0 && number <= 9) {
    switch (number) {
      case 0: segments = 0b00111111; break; // abcdef
      case 1: segments = 0b00000110; break; // bc
      case 2: segments = 0b01011011; break; // abged
      case 3: segments = 0b01001111; break; // abcgd
      case 4: segments = 0b01100110; break; // fgbc
      case 5: segments = 0b01101101; break; // afgcd
      case 6: segments = 0b01111101; break; // afgcde
      case 7: segments = 0b00000111; break; // abc
      case 8: segments = 0b01111111; break; // abcdefg
      case 9: segments = 0b01101111; break; // abfgcd
    }
  }

  for (int i = 0; i < 7; i++) { // Сегменты a-g (0-6)
    if ((segments >> i) & 0x01) {
      digitalWrite(segmentPins[i], HIGH);
    }
  }
  if (dot) {
    digitalWrite(segmentPins[7], HIGH); // DP
  }
}


// ----- Функция отображения текста (для "Err ") -----
void displayText(const char* text) {
    for (int digit = 0; digit < 4; digit++) {
        digitalWrite(digitPins[digit], LOW); // Включаем разряд
        byte segments = 0b00000000;
        char character = text[digit];

        switch (character) {
            case 'E': segments = 0b01111001; break; // afged
            case 'r': segments = 0b01010000; break; // ge
            case ' ': segments = 0b00000000; break; // Пробел (все сегменты выключены)
            default: segments = 0b00000000; // По умолчанию пробел
        }

        for (int i = 0; i < 7; i++) {
            if ((segments >> i) & 0x01) {
                digitalWrite(segmentPins[i], HIGH);
            }
        }
        delay(1);
        digitalWrite(digitPins[digit], HIGH); // Выключаем разряд
    }
}


// ----- Функция обновления индикации (обновленная для Задания 2) -----
void updateDisplay() {
    switch (currentDisplayMode) {
        case TIME_MODE: {
            int displayHours = hours;
            int displayMinutes = minutes;
            if (displayHours > 23) displayHours = 0;
            if (displayMinutes > 59) displayMinutes = 0;

            displayDigit(0, displayHours / 10, editMode);
            delay(1);
            digitalWrite(digitPins[0], HIGH);

            displayDigit(1, displayHours % 10, editMode);
            delay(1);
            digitalWrite(digitPins[1], HIGH);

            displayDigit(2, displayMinutes / 10, seconds % 2 == 0 && !editMode);
            delay(1);
            digitalWrite(digitPins[2], HIGH);

            displayDigit(3, displayMinutes % 10, editMode);
            delay(1);
            digitalWrite(digitPins[3], HIGH);
            break;
        }
        case TEMP_MODE: {
            int temp_int = static_cast<int>(temperature); // Целая часть температуры
            int temp_dec = static_cast<int>((temperature - temp_int) * 10); // Десятые доли (не используется, но можно добавить)

            displayDigit(0, temp_int / 10, false); // Старший разряд температуры
            delay(1);
            digitalWrite(digitPins[0], HIGH);

            displayDigit(1, temp_int % 10, false); // Младший разряд температуры
            delay(1);
            digitalWrite(digitPins[1], HIGH);

            // Отображение 'C' для Цельсия на 3 и 4 разрядах
            byte segmentsC = 0b00111001; // Сегменты для 'C' (afed)
            for (int i = 0; i < 7; i++) {
                digitalWrite(segmentPins[i], LOW);
            }
             for (int i = 0; i < 7; i++) {
                if ((segmentsC >> i) & 0x01) {
                    digitalWrite(segmentPins[i], HIGH);
                }
            }
            digitalWrite(digitPins[2], LOW);
            delay(1);
            digitalWrite(digitPins[2], HIGH);


            displayDigit(3, 0, false); // Пусто, или можно попробовать отобразить символ градуса, если получится. Но проще 0 как в примере '25 0'
            delay(1);
            digitalWrite(digitPins[3], HIGH);

            break;
        }
        case HUM_MODE: {
            int humidity_int = static_cast<int>(humidity); // Целая часть влажности

             displayDigit(0, humidity_int / 100, false); // Сотни влажности (если нужно, или 0)
            delay(1);
            digitalWrite(digitPins[0], HIGH);

            displayDigit(1, (humidity_int % 100) / 10, false); // Десятки влажности
            delay(1);
            digitalWrite(digitPins[1], HIGH);

            displayDigit(2, humidity_int % 10, false); // Единицы влажности
            delay(1);
            digitalWrite(digitPins[2], HIGH);

            displayDigit(3, 0, false); // Пусто, или младший разряд, если нужна дробная часть, пока 0 для примера '5386' из задания.
            delay(1);
            digitalWrite(digitPins[3], HIGH);
            break;
        }
        case ERROR_MODE: {
            displayText("Err "); // Отображение "Err "
            break;
        }
    }
}


// ----- Функция проверки кнопок (обновленная для Задания 2) -----
void checkButtons() {
    // Кнопка 1 (Режим/Сохранить/Переключение режимов)
    int button1State = digitalRead(button1Pin);
    if (button1State == LOW) {
        if (button1PressTime == 0) {
            button1PressTime = millis();
        }
        if (millis() - button1PressTime >= longPressThreshold && !editMode) {
            editMode = true; // Долгий клик - вход в режим редактирования (только в режиме времени)
             if (currentDisplayMode == TIME_MODE) {
                 for (int i = 0; i < 8; i++) {
                    if (i == 7) digitalWrite(segmentPins[i], HIGH);
                 }
             }
        }
    } else {
        if (button1PressTime != 0) {
            if (millis() - button1PressTime < longPressThreshold) {
                if (editMode && currentDisplayMode == TIME_MODE) {
                    editMode = false; // Короткий клик в режиме редактирования времени - сохранить
                    for (int i = 0; i < 8; i++) {
                         if (i == 7) digitalWrite(segmentPins[i], LOW);
                    }
                } else if (!editMode) { // Короткий клик вне режима редактирования - переключение режимов
                    currentDisplayMode = static_cast<DisplayMode>((currentDisplayMode + 1) % 4); // Переключение режимов по кругу
                    if (currentDisplayMode == ERROR_MODE && !dhtError) { // Если режим ошибки, но ошибки нет - пропускаем его
                        currentDisplayMode = static_cast<DisplayMode>((currentDisplayMode + 1) % 4);
                    }
                }
            }
            button1PressTime = 0;
        }
    }

    // Кнопки 2 и 3 (редактирование времени - как в Задании 1)
    int button2State = digitalRead(button2Pin);
    if (button2State == LOW && editMode && currentDisplayMode == TIME_MODE) { // Редактирование только в режиме времени и режиме редактирования
        hours++;
        if (hours > 23) hours = 0;
        delay(200);
    }

    int button3State = digitalRead(button3Pin);
    if (button3State == LOW && editMode && currentDisplayMode == TIME_MODE) { // Редактирование только в режиме времени и режиме редактирования
        minutes++;
        if (minutes > 59) minutes = 0;
        delay(200);
    }
}


// ----- Функция чтения данных с DHT11 -----
void readDHTData() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        dhtError = true;
        if (currentDisplayMode != ERROR_MODE) {
            currentDisplayMode = ERROR_MODE; // Переключаемся в режим ошибки, если есть ошибка DHT и не в режиме ошибки
        }
        return;
    }

    dhtError = false; // Сброс флага ошибки, если чтение прошло успешно
    if (currentDisplayMode == ERROR_MODE) { // Если были в режиме ошибки, переключаемся обратно на время
         currentDisplayMode = TIME_MODE;
    }
    humidity = h;
    temperature = t;
}



// ----- Функция setup (дополнения для Задания 2) -----
void setup() {
    // Инициализация пинов сегментов, разрядов, кнопок (как в Задании 1)
    for (int i = 0; i < 8; i++) {
        pinMode(segmentPins[i], OUTPUT);
        digitalWrite(segmentPins[i], LOW);
    }
    for (int i = 0; i < 4; i++) {
        pinMode(digitPins[i], OUTPUT);
        digitalWrite(digitPins[i], HIGH);
    }
    pinMode(button1Pin, INPUT_PULLUP);
    pinMode(button2Pin, INPUT_PULLUP);
    pinMode(button3Pin, INPUT_PULLUP);

    // Инициализация DHT11
    dht.begin();

    // Инициализация Serial Monitor для отладки (необязательно для работы, но полезно)
    Serial.begin(9600);
    Serial.println(F("DHT11 test!"));

    // Настройка Watchdog Timer (как в Задании 1)
    wdt_enable(WDTO_8S);
    wdt_reset();
}


// ----- Функция loop (обновленная для Задания 2) -----
void loop() {
    wdt_reset();

    checkButtons(); // Проверка кнопок (как в Задании 1, но обновленная логика)
    readDHTData(); // Чтение данных с DHT11 в начале цикла
    updateDisplay(); // Обновление индикации (обновленная логика режимов)


    static unsigned long lastSecondTick = 0;

    if (currentDisplayMode == TIME_MODE) { // Отсчет секунд только в режиме времени
        if (millis() - lastSecondTick >= 1000) {
            lastSecondTick += 1000;
            seconds++;
            if (seconds > 59) {
                seconds = 0;
                minutes++;
                if (minutes > 59) {
                    minutes = 0;
                    hours++;
                    if (hours > 23) {
                        hours = 0;
                    }
                }
            }
        }
    } else {
         lastSecondTick = millis(); // Сброс таймера секунд в режимах температуры/влажности, чтобы секунды не шли в фоне.
    }


}