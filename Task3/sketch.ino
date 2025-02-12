#include <Arduino.h>
#include <DHT.h>
#include <avr/wdt.h>

/*
--------------------------------------------------------------------------------
ПРОЕКТ "ЧАСЫ-ТЕРМОГИГРОМЕТР-БУДИЛЬНИК"

Инструкция по использованию устройства.

Управление осуществляется тремя кнопками:

КНОПКА 1 (A0) -  Режим/Сохранить/Переключение режимов
КНОПКА 2 (A1) -  Увеличение значения (часы или часы будильника)
КНОПКА 3 (A2) -  Увеличение значения (минуты или минуты будильника)

Режимы работы устройства (переключаются последовательно кратким нажатием КНОПКИ 1):

1. РЕЖИМ ВРЕМЕНИ (RGB светодиод - КРАСНЫЙ)
   - Отображает текущее время в формате ЧЧ.ММ.
   - Точка между минутами мигает каждую секунду.
   - ДОЛГОЕ нажатие КНОПКИ 1 (более 1 секунды): Вход в РЕЖИМ РЕДАКТИРОВАНИЯ ВРЕМЕНИ.
     - Все точки на индикаторе включаются.
     - КНОПКА 2: Увеличение часов.
     - КНОПКА 3: Увеличение минут.
     - КРАТКОВРЕМЕННОЕ нажатие КНОПКИ 1: Сохранение установленного времени и выход из режима редактирования.

2. РЕЖИМ ВЛАЖНОСТИ (RGB светодиод - СИНИЙ)
   - Отображает измеренную влажность в процентах (целая часть).

3. РЕЖИМ ТЕМПЕРАТУРЫ (RGB светодиод - ЖЕЛТЫЙ)
   - Отображает измеренную температуру в градусах Цельсия (целая часть).

4. РЕЖИМ БУДИЛЬНИКА (RGB светодиод - ЗЕЛЕНЫЙ)
   - Отображает установленное время будильника в формате ЧЧ.ММ.
   - КРАТКОВРЕМЕННОЕ нажатие КНОПКИ 1: Переключение в следующий режим (Время).
   - ДОЛГОЕ нажатие КНОПКИ 1 (более 1 секунды): Вход в РЕЖИМ РЕДАКТИРОВАНИЯ ВРЕМЕНИ БУДИЛЬНИКА.
     - Зеленый RGB светодиод начинает МИГАТЬ.
     - КНОПКА 2: Увеличение часов будильника.
     - КНОПКА 3: Увеличение минут будильника.
     - КРАТКОВРЕМЕННОЕ нажатие КНОПКИ 1: Сохранение установленного времени будильника и выход из режима редактирования. Зеленый RGB светодиод перестает мигать и горит постоянно.

5. РЕЖИМ ОШИБКИ (Отображается автоматически при ошибке чтения датчика DHT11)
   - На индикаторе отображается "Err ".
   - При успешном чтении данных с DHT11 устройство автоматически возвращается в предыдущий режим (обычно режим ВРЕМЕНИ).

СРАБАТЫВАНИЕ БУДИЛЬНИКА (ТОЛЬКО в режиме ВРЕМЕНИ):
- Когда текущее время совпадает с установленным временем будильника (часы и минуты), происходит следующее:
  - Семисегментный индикатор начинает МИГАТЬ текущим временем с частотой 2 Гц.
  - RGB светодиод начинает МИГАТЬ БЕЛЫМ цветом с частотой 2 Гц.
- Для отключения сработавшего будильника: КРАТКОВРЕМЕННО нажмите КНОПКУ 1 (устройство переключится в режим ВЛАЖНОСТИ и мигание прекратится).

ЦВЕТ RGB СВЕТОДИОДА В РАЗНЫХ РЕЖИМАХ:
- Режим ВРЕМЕНИ: КРАСНЫЙ
- Режим ВЛАЖНОСТИ: СИНИЙ
- Режим ТЕМПЕРАТУРЫ: ЖЕЛТЫЙ
- Режим БУДИЛЬНИКА: ЗЕЛЕНЫЙ
- Режим РЕДАКТИРОВАНИЯ ВРЕМЕНИ БУДИЛЬНИКА: МИГАЮЩИЙ ЗЕЛЕНЫЙ
- СРАБАТЫВАНИЕ БУДИЛЬНИКА: МИГАЮЩИЙ БЕЛЫЙ
- В "обычном режиме" (ВРЕМЯ, ВЛАЖНОСТЬ, ТЕМПЕРАТУРА) RGB светодиод ВЫКЛЮЧЕН (согласно требованию "и)") -  В коде текущей версии реализовано постоянное свечение цветом в режимах Время, Влажность, Температура и Будильник, а выключение RGB происходит только в режиме ошибки и при отсутствии активного будильника. При необходимости, логику можно изменить.

--------------------------------------------------------------------------------
ВАЖНО! ПЕРЕД ЗАГРУЗКОЙ КОДА:
1. Убедитесь, что правильно подключили все компоненты согласно схеме,
   особенно RGB светодиод с токоограничивающими резисторами (220-330 Ом)
   на каждый цвет (Красный, Зеленый, Синий). Без резисторов RGB светодиод
   может работать некорректно или выйти из строя!
2. Убедитесь, что установлены необходимые библиотеки:
   - DHT sensor library от Adafruit
   - Adafruit Unified Sensor (может установиться как зависимость DHT sensor library)
   Установка библиотек: в Arduino IDE, меню "Инструменты" -> "Менеджер библиотек..."
3. Проверьте соответствие пинов в коде и вашей схеме подключения.
   Если вы используете другие пины Arduino, измените константы пинов
   в начале кода на свои значения.
--------------------------------------------------------------------------------
*/

// ----- Пины и константы -----
const int segmentPins[] = {2, 3, 4, 5, 6, 7, 8, 9}; // Пины для сегментов a-g, dp
const int digitPins[] = {10, 11, 12, 13};         // Пины для разрядов D1-D4
const int button1Pin = A0;                         // Кнопка 1 (Режим/Сохранить/Переключение режимов)
const int button2Pin = A1;                         // Кнопка 2 (Увеличить часы/часы будильника)
const int button3Pin = A2;                         // Кнопка 3 (Увеличить минуты/минуты будильника)
const unsigned long longPressThreshold = 1000;     // Порог долгого нажатия для кнопки 1 (1 секунда)

// ----- Пины и константы для DHT11 -----
#define DHTPIN A3             // Пин, к которому подключен DHT11 (DATA пин)
#define DHTTYPE DHT11           // <-----  ТИП ДАТЧИКА УСТАНОВЛЕН КАК DHT11  <-----
DHT dht(DHTPIN, DHTTYPE);

// ----- Пины для RGB светодиода (общий катод) -----
const int rgbRedPin = A4;       // Красный пин RGB светодиода (через резистор 220-330 Ом)
const int rgbGreenPin = A5;      // Зеленый пин RGB светодиода (через резистор 220-330 Ом)
const int rgbBluePin = 1;       // Синий пин RGB светодиода (через резистор 220-330 Ом)

// ----- Переменные -----
int hours = 13;                 // Начальное значение часов
int minutes = 25;               // Начальное значение минут
int seconds = 0;                // Секунды
bool editMode = false;          // Флаг режима редактирования времени
unsigned long button1PressTime = 0; // Переменная для отслеживания времени нажатия кнопки 1

enum DisplayMode { TIME_MODE, TEMP_MODE, HUM_MODE, ALARM_MODE, ERROR_MODE }; // Перечисление режимов работы
DisplayMode currentDisplayMode = TIME_MODE; // Изначально устанавливаем режим ВРЕМЕНИ

float temperature = 0.0;        // Переменная для хранения температуры
float humidity = 0.0;           // Переменная для хранения влажности
bool dhtError = false;          // Флаг ошибки чтения DHT11

int alarmHours = 7;             // Часы будильника по умолчанию
int alarmMinutes = 0;           // Минуты будильника по умолчанию
bool alarmSet = false;          // Флаг установки будильника (вкл/выкл - не реализовано в коде в полной мере, только для срабатывания)
bool alarmActive = false;       // Флаг срабатывания будильника (для мигания)
unsigned long alarmBlinkTimer = 0; // Таймер для мигания индикатора и RGB при срабатывании будильника
const unsigned long alarmBlinkInterval = 500; // Интервал мигания (500 мс вкл/выкл = 2Гц)
bool alarmEditMode = false;     // Флаг режима редактирования времени будильника
unsigned long ledBlinkTimer = 0;   // Таймер для мигания зеленого светодиода в режиме редактирования будильника


// ----- Функция отображения цифры на одном разряде индикатора -----
void displayDigit(int digit, int number, bool dot) {
    // segments bit map for digits 0-9, and off
    byte segments = 0b00000000;
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

    // Выключаем все сегменты перед отображением новой цифры
    for (int i = 0; i < 8; i++) {
        digitalWrite(segmentPins[i], LOW); // LOW для общего катода - выключаем сегмент
    }
    digitalWrite(digitPins[digit], LOW); // Включаем нужный разряд (активный низ для общего катода)

    // Зажигаем нужные сегменты для отображения цифры
    for (int i = 0; i < 7; i++) { // Сегменты a-g (индексы 0-6 в массиве segmentPins)
        if ((segments >> i) & 0x01) { // Проверяем бит в маске segments
            digitalWrite(segmentPins[i], HIGH); // HIGH для общего катода - включаем сегмент
        }
    }
    if (dot) { // Отображение точки (DP)
        digitalWrite(segmentPins[7], HIGH); // Включаем точку, если dot == true
    }
    delay(1); // Небольшая задержка для динамической индикации
    digitalWrite(digitPins[digit], HIGH); // Выключаем разряд после отображения (гасим разряд)
}


// ----- Функция отображения текста "Err " на индикаторе -----
void displayText(const char* text) {
    for (int digit = 0; digit < 4; digit++) {
        digitalWrite(digitPins[digit], LOW); // Включаем разряд
        byte segments = 0b00000000; // Маска для сегментов
        char character = text[digit]; // Берем символ для отображения на текущем разряде

        // Определяем сегменты для каждой буквы/символа (только E, r, пробел реализованы для "Err ")
        switch (character) {
            case 'E': segments = 0b01111001; break; // afged
            case 'r': segments = 0b01010000; break; // ge
            case ' ': segments = 0b00000000; break; // Пробел (все сегменты выключены)
            default: segments = 0b00000000;      // По умолчанию пробел для неизвестных символов
        }

        // Зажигаем нужные сегменты для отображения буквы/символа
        for (int i = 0; i < 7; i++) {
            if ((segments >> i) & 0x01) {
                digitalWrite(segmentPins[i], HIGH);
            }
        }
        delay(1); // Небольшая задержка для динамической индикации
        digitalWrite(digitPins[digit], HIGH); // Выключаем разряд
    }
}


// ----- Функция обновления индикации в зависимости от текущего режима -----
void updateDisplay() {
    switch (currentDisplayMode) {
        case TIME_MODE: { // Режим ВРЕМЕНИ
            int displayHours = hours;
            int displayMinutes = minutes;
            if (displayHours > 23) displayHours = 0; // Коррекция часов (0-23)
            if (displayMinutes > 59) displayMinutes = 0; // Коррекция минут (0-59)

            displayDigit(0, displayHours / 10, editMode);                  // Старший разряд часов, точка = режим редактирования
            delay(1);
            digitalWrite(digitPins[0], HIGH);

            displayDigit(1, displayHours % 10, editMode);                  // Младший разряд часов, точка = режим редактирования
            delay(1);
            digitalWrite(digitPins[1], HIGH);

            displayDigit(2, displayMinutes / 10, seconds % 2 == 0 && !editMode); // Старший разряд минут, точка мигает только в режиме часов и не в режиме редактирования
            delay(1);
            digitalWrite(digitPins[2], HIGH);

            displayDigit(3, displayMinutes % 10, editMode);                  // Младший разряд минут, точка = режим редактирования
            delay(1);
            digitalWrite(digitPins[3], HIGH);
            break;
        }
        case TEMP_MODE: { // Режим ТЕМПЕРАТУРЫ
            int temp_int = static_cast<int>(temperature); // Целая часть температуры (округление к меньшему)

            displayDigit(0, temp_int / 10, false);      // Старший разряд температуры
            delay(1);
            digitalWrite(digitPins[0], HIGH);

            displayDigit(1, temp_int % 10, false);      // Младший разряд температуры
            delay(1);
            digitalWrite(digitPins[1], HIGH);

            // Отображение буквы 'C' (градусы Цельсия) на 3-м разряде
            byte segmentsC = 0b00111001; // Сегменты для 'C' (a,f,e,d)
            for (int i = 0; i < 7; i++) digitalWrite(segmentPins[i], LOW); // Выключаем все сегменты перед отображением 'C'
            for (int i = 0; i < 7; i++) {
                if ((segmentsC >> i) & 0x01) digitalWrite(segmentPins[i], HIGH); // Включаем сегменты для 'C'
            }
            digitalWrite(digitPins[2], LOW); // Включаем 3-й разряд
            delay(1);
            digitalWrite(digitPins[2], HIGH);

            displayDigit(3, 0, false);      // Пусто на 4-м разряде (или можно символ градуса, если есть)
            delay(1);
            digitalWrite(digitPins[3], HIGH);

            break;
        }
        case HUM_MODE: { // Режим ВЛАЖНОСТИ
            int humidity_int = static_cast<int>(humidity); // Целая часть влажности (округление к меньшему)

             displayDigit(0, humidity_int / 100, false);     // Сотни влажности (всегда 0 для влажности < 100)
            delay(1);
            digitalWrite(digitPins[0], HIGH);

            displayDigit(1, (humidity_int % 100) / 10, false); // Десятки влажности
            delay(1);
            digitalWrite(digitPins[1], HIGH);

            displayDigit(2, humidity_int % 10, false);      // Единицы влажности
            delay(1);
            digitalWrite(digitPins[2], HIGH);

            displayDigit(3, 0, false);      // Пусто на 4-м разряде (или можно попробовать отобразить '%')
            delay(1);
            digitalWrite(digitPins[3], HIGH);
            break;
        }
        case ALARM_MODE: { // Режим БУДИЛЬНИКА
            int displayAlarmHours = alarmHours;
            int displayAlarmMinutes = alarmMinutes;
            if (displayAlarmHours > 23) displayAlarmHours = 0;
            if (displayAlarmMinutes > 59) displayAlarmMinutes = 0;

            displayDigit(0, displayAlarmHours / 10, alarmEditMode); // Старший разряд часов будильника, точка = режим редактирования будильника
            delay(1);
            digitalWrite(digitPins[0], HIGH);

            displayDigit(1, displayAlarmHours % 10, alarmEditMode); // Младший разряд часов будильника, точка = режим редактирования будильника
            delay(1);
            digitalWrite(digitPins[1], HIGH);

            displayDigit(2, displayAlarmMinutes / 10, false);         // Старший разряд минут будильника, точка не мигает
            delay(1);
            digitalWrite(digitPins[2], HIGH);

            displayDigit(3, displayAlarmMinutes % 10, false);         // Младший разряд минут будильника, точка не мигает
            delay(1);
            digitalWrite(digitPins[3], HIGH);
            break;
        }

        case ERROR_MODE: { // Режим ОШИБКИ DHT11
            displayText("Err "); // Отображаем "Err "
            break;
        }
    }
}


// ----- Функция установки цвета RGB светодиода (общий катод) -----
void setRGBColor(int red, int green, int blue) {
    Serial.print("RGB: R="); Serial.print(red); Serial.print(green); Serial.print(", G="); Serial.print(green); Serial.print(", B="); Serial.println(blue); // Добавили отладочный вывод в Serial Monitor
    digitalWrite(rgbRedPin, red);   // HIGH - включить красный, LOW - выключить
    digitalWrite(rgbGreenPin, green); // HIGH - включить зеленый, LOW - выключить
    digitalWrite(rgbBluePin, blue);  // HIGH - включить синий, LOW - выключить
}

// ----- Функция выключения RGB светодиода -----
void turnOffRGB() {
    setRGBColor(LOW, LOW, LOW); // Выключаем все цвета RGB
}

// ----- Функция мигания белым цветом RGB светодиода (для будильника) -----
void blinkRGBWhite() {
    unsigned long currentMillis = millis();
    if (currentMillis - alarmBlinkTimer >= alarmBlinkInterval) {
        alarmBlinkTimer = currentMillis;
        if (alarmActive) {
            setRGBColor(HIGH, HIGH, HIGH); // Включаем белый цвет (все цвета RGB включены)
        } else {
            turnOffRGB();           // Выключаем RGB
        }
        alarmActive = !alarmActive; // Инвертируем состояние для следующего мигания (вкл/выкл)
    }
}

// ----- Функция мигания времени на индикаторе (для будильника) -----
void blinkTimeDisplay() {
    unsigned long currentMillis = millis();
    if (currentMillis - alarmBlinkTimer >= alarmBlinkInterval) {
        alarmBlinkTimer = currentMillis;
        if (alarmActive) {
            updateDisplay(); // Отображаем текущее время на индикаторе
        } else {
             for (int i = 0; i < 4; i++) { // Гасим все разряды индикатора
                digitalWrite(digitPins[i], LOW);
            }
        }
        alarmActive = !alarmActive; // Инвертируем состояние для следующего мигания (вкл/выкл)
    }
}



// ----- Функция проверки состояния кнопок -----
void checkButtons() {
    // Кнопка 1 (Режим/Сохранить/Переключение режимов/Редактирование будильника)
    int button1State = digitalRead(button1Pin); // Читаем состояние кнопки 1
    if (button1State == LOW) { // Кнопка нажата (LOW из-за INPUT_PULLUP)
        if (button1PressTime == 0) { // Если кнопка только что нажата (не была нажата в предыдущем цикле)
            button1PressTime = millis(); // Запоминаем время нажатия
        }
        if (millis() - button1PressTime >= longPressThreshold && !editMode) { // Проверяем долгое нажатие (более 1 секунды) и что не в режиме редактирования времени
            editMode = true; // Входим в режим редактирования времени (только в режиме времени)
            if (currentDisplayMode == TIME_MODE) { // Включаем точки, только если мы в режиме времени
                for (int i = 0; i < 8; i++) { // Включаем все точки на индикаторе в режиме редактирования времени
                    if (i == 7) digitalWrite(segmentPins[i], HIGH); // Включаем DP (десятичную точку)
                }
            } else if (currentDisplayMode == ALARM_MODE) { // Долгий клик в режиме будильника - вход в редактирование времени будильника
                alarmEditMode = true; // Входим в режим редактирования времени будильника
                ledBlinkTimer = millis(); // Запускаем таймер мигания зеленого светодиода
            }
        }
    } else { // Кнопка отпущена
        if (button1PressTime != 0) { // Если кнопка была нажата ранее
            if (millis() - button1PressTime < longPressThreshold) { // Проверяем на кратковременное нажатие (менее 1 секунды)
                if (editMode && currentDisplayMode == TIME_MODE) { // Короткий клик в режиме редактирования времени - сохранить время
                    editMode = false; // Выходим из режима редактирования времени
                    for (int i = 0; i < 8; i++) { // Выключаем точки индикатора
                        if (i == 7) digitalWrite(segmentPins[i], LOW);
                    }
                } else if (!editMode && !alarmEditMode) { // Короткий клик вне режимов редактирования - переключение режимов (включая будильник)
                    currentDisplayMode = static_cast<DisplayMode>((currentDisplayMode + 1) % 5); // Переключаемся на следующий режим (циклически)
                    if (currentDisplayMode == ERROR_MODE && !dhtError) { // Если переключились в режим ошибки, но ошибки DHT нет, пропускаем режим ошибки (чтобы не зацикливаться на ошибке, если ее уже нет)
                        currentDisplayMode = static_cast<DisplayMode>((currentDisplayMode + 1) % 5);
                    }
                } else if (alarmEditMode) { // Короткий клик в режиме редактирования будильника - выход из редактирования будильника
                    alarmEditMode = false; // Выходим из режима редактирования будильника
                    turnOffRGB(); // Выключаем мигание зеленого светодиода при выходе из режима редактирования будильника
                }
            }
            button1PressTime = 0; // Сбрасываем время нажатия кнопки 1
        }
    }

    // Кнопка 2 (Увеличение часов/часов будильника)
    int button2State = digitalRead(button2Pin); // Читаем состояние кнопки 2
    if (button2State == LOW) { // Кнопка 2 нажата
        if (editMode && currentDisplayMode == TIME_MODE) { // Редактирование часов в режиме времени
            hours++; // Увеличиваем часы
            if (hours > 23) hours = 0; // Переход на 0 после 23
            delay(200); // Небольшая задержка для устранения дребезга кнопки
        } else if (alarmEditMode && currentDisplayMode == ALARM_MODE) { // Редактирование часов будильника в режиме будильника и режиме редактирования будильника
            alarmHours++; // Увеличиваем часы будильника
            if (alarmHours > 23) alarmHours = 0; // Переход на 0 после 23
            delay(200); // Задержка для устранения дребезга
        }
    }

    // Кнопка 3 (Увеличение минут/минут будильника)
    int button3State = digitalRead(button3Pin); // Читаем состояние кнопки 3
    if (button3State == LOW) { // Кнопка 3 нажата
        if (editMode && currentDisplayMode == TIME_MODE) { // Редактирование минут в режиме времени
            minutes++; // Увеличиваем минуты
            if (minutes > 59) minutes = 0; // Переход на 0 после 59
            delay(200); // Задержка для устранения дребезга
        } else if (alarmEditMode && currentDisplayMode == ALARM_MODE) { // Редактирование минут будильника в режиме будильника и режиме редактирования будильника
            alarmMinutes++; // Увеличиваем минуты будильника
            if (alarmMinutes > 59) alarmMinutes = 0; // Переход на 0 после 59
            delay(200); // Задержка для устранения дребезга
        }
    }
}


// ----- Функция чтения данных с датчика DHT11 -----
void readDHTData() {
    float h = dht.readHumidity();     // Читаем влажность
    float t = dht.readTemperature();  // Читаем температуру в градусах Цельсия

    if (isnan(h) || isnan(t)) { // Проверяем, удалось ли прочитать данные
        Serial.println(F("Failed to read from DHT sensor!")); // Выводим сообщение об ошибке в Serial Monitor (для отладки)
        dhtError = true; // Устанавливаем флаг ошибки DHT
        if (currentDisplayMode != ERROR_MODE) { // Если текущий режим не режим ошибки
            currentDisplayMode = ERROR_MODE; // Переключаемся в режим ошибки
        }
        return; // Выходим из функции, дальше не обрабатываем данные DHT
    }

    dhtError = false; // Сбрасываем флаг ошибки, если данные прочитаны успешно
    if (currentDisplayMode == ERROR_MODE) { // Если были в режиме ошибки ранее
         currentDisplayMode = TIME_MODE; // Возвращаемся в режим ВРЕМЕНИ (или можно в предыдущий режим)
    }
    humidity = h;       // Сохраняем значение влажности
    temperature = t;    // Сохраняем значение температуры
}



// ----- Функция SETUP (выполняется один раз при запуске Arduino) -----
void setup() {
    // Инициализация пинов сегментов как выходы
    for (int i = 0; i < 8; i++) {
        pinMode(segmentPins[i], OUTPUT);
        digitalWrite(segmentPins[i], LOW); // Изначально выключаем все сегменты
    }
    // Инициализация пинов разрядов как выходы
    for (int i = 0; i < 4; i++) {
        pinMode(digitPins[i], OUTPUT);
        digitalWrite(digitPins[i], HIGH); // Изначально выключаем все разряды (активный низ для общего катода)
    }

    // Инициализация пинов кнопок как входы с внутренним подтягивающим резистором
    pinMode(button1Pin, INPUT_PULLUP);  // Кнопка 1 с внутренним подтягивающим резистором
    pinMode(button2Pin, INPUT_PULLUP);  // Кнопка 2 с внутренним подтягивающим резистором
    pinMode(button3Pin, INPUT_PULLUP);  // Кнопка 3 с внутренним подтягивающим резистором

    // Инициализация DHT11 датчика
    dht.begin();

    // Инициализация пинов RGB светодиода как выходы
    pinMode(rgbRedPin, OUTPUT);
    pinMode(rgbGreenPin, OUTPUT);
    pinMode(rgbBluePin, OUTPUT);
    turnOffRGB(); // Изначально выключаем RGB светодиод

    // Инициализация Serial Monitor для отладки и вывода сообщений (скорость 9600 бод)
    Serial.begin(9600);
    Serial.println(F("DHT11 test!")); // Выводим сообщение в Serial Monitor при старте

    // Настройка Watchdog Timer для более точного отсчета времени (период 8 секунд - максимальный)
    wdt_enable(WDTO_8S);
    wdt_reset(); // Сброс Watchdog Timer для начала отсчета

}


// ----- Функция LOOP (выполняется постоянно в цикле) -----
void loop() {
    wdt_reset(); // Сброс Watchdog Timer в начале каждого цикла loop, чтобы предотвратить перезагрузку

    checkButtons();  // Проверяем состояние кнопок и обрабатываем нажатия
    readDHTData();   // Читаем данные с датчика DHT11

    // Установка цвета RGB светодиода в зависимости от текущего режима работы устройства
    switch (currentDisplayMode) {
        case TIME_MODE: setRGBColor(HIGH, LOW, LOW); break;    // Режим ВРЕМЕНИ: Красный цвет
        case HUM_MODE: setRGBColor(LOW, LOW, HIGH); break;     // Режим ВЛАЖНОСТИ: Синий цвет
        case TEMP_MODE: setRGBColor(HIGH, HIGH, LOW); break;    // Режим ТЕМПЕРАТУРЫ: Желтый цвет (Красный + Зеленый)
        case ALARM_MODE: setRGBColor(LOW, HIGH, LOW); break;   // Режим БУДИЛЬНИКА: Зеленый цвет
        default: turnOffRGB(); break; // Для режима ERROR_MODE и по умолчанию - выключаем RGB светодиод
    }

    if (alarmEditMode && currentDisplayMode == ALARM_MODE) { // Если находимся в режиме редактирования будильника в режиме будильника
        blinkRGBWhite(); // Мигаем зеленым светодиодом в режиме редактирования будильника
    }  else if (currentDisplayMode != ALARM_MODE) { // В режимах, кроме ALARM_MODE и редактирования будильника
        turnOffRGB(); // Выключаем RGB светодиод (согласно требованию "и)")
    }


    // Проверка, сработал ли будильник (только если находимся в режиме ВРЕМЕНИ)
    if (currentDisplayMode == TIME_MODE && hours == alarmHours && minutes == alarmMinutes && seconds == 0 && alarmSet) {
        blinkRGBWhite();      // Мигаем белым RGB светодиодом (сигнал будильника)
        blinkTimeDisplay();   // Мигаем временем на индикаторе (сигнал будильника)
    } else { // Если будильник не должен срабатывать
        updateDisplay();      // Обычное обновление индикации (отображение времени, температуры, влажности или времени будильника)
        alarmActive = false;  // Выключаем флаг alarmActive, чтобы мигание будильника прекратилось, если будильник не срабатывает
        turnOffRGB();       // Выключаем RGB светодиод, если будильник не активен и режим не будильник и не редактирование будильника (согласно требованию "и)")
    }


    static unsigned long lastSecondTick = 0; // Переменная для отслеживания времени последней секунды

    if (currentDisplayMode == TIME_MODE) { // Отсчет секунд и обновление времени только в режиме ВРЕМЕНИ
        if (millis() - lastSecondTick >= 1000) { // Проверяем, прошла ли секунда (1000 миллисекунд)
            lastSecondTick += 1000; // Обновляем время последнего тика секунды
            seconds++;              // Увеличиваем секунды
            if (seconds > 59) {      // Если секунды достигли 60
                seconds = 0;        // Сбрасываем секунды
                minutes++;          // Увеличиваем минуты
                if (minutes > 59) {    // Если минуты достигли 60
                    minutes = 0;      // Сбрасываем минуты
                    hours++;          // Увеличиваем часы
                    if (hours > 23) {    // Если часы достигли 24
                        hours = 0;      // Сбрасываем часы
                    }
                }
            }
        }
    } else { // В режимах ТЕМПЕРАТУРЫ, ВЛАЖНОСТИ, БУДИЛЬНИКА и ОШИБКИ
         lastSecondTick = millis(); // Сбрасываем таймер секунд, чтобы секунды не шли "в фоне" в этих режимах
    }
}