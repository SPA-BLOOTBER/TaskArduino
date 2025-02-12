#include <Arduino.h>
#include <avr/wdt.h> // Библиотека для работы с Watchdog Timer для точного времени

// Пины для сегментов     (A, B, C, D, E, F, G, DP)
const int segmentPins[] = {2, 3, 4, 5, 6, 7, 8, 9};
// Пины для разрядов (D1, D2, D3, D4)
const int digitPins[] = {10, 11, 12, 13};
// Пины для кнопок
const int button1Pin = A0; // Кнопка 1 (Режим/Сохранить)
const int button2Pin = A1; // Кнопка 2 (Увеличить часы)
const int button3Pin = A2; // Кнопка 3 (Увеличить минуты)

// Переменные для хранения времени
int hours = 13; // Начальное значение часов (пример)
int minutes = 25; // Начальное значение минут (пример)
int seconds = 0;

// Переменные для режима редактирования
bool editMode = false;
unsigned long button1PressTime = 0;
const unsigned long longPressThreshold = 1000; // Порог долгого нажатия (1 секунда)

// Функция для отображения цифры на одном разряде
void displayDigit(int digit, int number, bool dot) {
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

void updateDisplay() {
  int displayHours = hours;
  int displayMinutes = minutes;

  if (displayHours > 23) displayHours = 0;
  if (displayMinutes > 59) displayMinutes = 0;


  displayDigit(0, displayHours / 10, editMode); // Старший разряд часов
  delay(1);
  digitalWrite(digitPins[0], HIGH); // Выключаем разряд

  displayDigit(1, displayHours % 10, editMode); // Младший разряд часов
  delay(1);
  digitalWrite(digitPins[1], HIGH); // Выключаем разряд

  displayDigit(2, displayMinutes / 10, seconds % 2 == 0 && !editMode); // Старший разряд минут, точка мигает
  delay(1);
  digitalWrite(digitPins[2], HIGH); // Выключаем разряд

  displayDigit(3, displayMinutes % 10, editMode); // Младший разряд минут
  delay(1);
  digitalWrite(digitPins[3], HIGH); // Выключаем разряд
}

void checkButtons() {
  // Кнопка 1 (Режим/Сохранить)
  int button1State = digitalRead(button1Pin);
  if (button1State == LOW) { // Кнопка нажата (подтянуто к VCC внутренним резистором)
    if (button1PressTime == 0) {
      button1PressTime = millis(); // Запоминаем время нажатия
    }
    if (millis() - button1PressTime >= longPressThreshold && !editMode) {
      editMode = true; // Входим в режим редактирования при долгом нажатии
      for (int i = 0; i < 8; i++) { // Включаем все точки в режиме редактирования
        if (i == 7) digitalWrite(segmentPins[i], HIGH); // Включаем DP
      }
    }
  } else { // Кнопка отпущена
    if (button1PressTime != 0) { // Кнопка была нажата
      if (millis() - button1PressTime < longPressThreshold && editMode) {
        editMode = false; // Выходим из режима редактирования при коротком нажатии (сохранение)
        for (int i = 0; i < 8; i++) { // Выключаем все точки при выходе из режима редактирования
          if (i == 7) digitalWrite(segmentPins[i], LOW); // Выключаем DP
        }
      }
      button1PressTime = 0; // Сбрасываем время нажатия
    }
  }

  // Кнопка 2 (Увеличить часы)
  int button2State = digitalRead(button2Pin);
  if (button2State == LOW && editMode) {
    hours++;
    if (hours > 23) hours = 0;
    delay(200); // Задержка для предотвращения дребезга
  }

  // Кнопка 3 (Увеличить минуты)
  int button3State = digitalRead(button3Pin);
  if (button3State == LOW && editMode) {
    minutes++;
    if (minutes > 59) minutes = 0;
    delay(200); // Задержка для предотвращения дребезга
  }
}


void setup() {
  // Инициализация пинов сегментов как выходы
  for (int i = 0; i < 8; i++) {
    pinMode(segmentPins[i], OUTPUT);
    digitalWrite(segmentPins[i], LOW); // Выключаем сегменты изначально
  }
  // Инициализация пинов разрядов как выходы
  for (int i = 0; i < 4; i++) {
    pinMode(digitPins[i], OUTPUT);
    digitalWrite(digitPins[i], HIGH); // Выключаем разряды изначально (активный низ для общего катода)
  }

  // Инициализация пинов кнопок как входы с внутренним подтягивающим резистором
  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);
  pinMode(button3Pin, INPUT_PULLUP);


  // Настройка Watchdog Timer для точного времени
  wdt_enable(WDTO_8S); // Включаем Watchdog с периодом 8 секунд (максимальный)
  wdt_reset();         // Сбрасываем Watchdog для начала отсчета времени

}

void loop() {
  wdt_reset(); // Сбрасываем Watchdog в начале каждого цикла loop, чтобы предотвратить перезагрузку

  checkButtons();
  updateDisplay();

  static unsigned long lastSecondTick = 0; // Переменная для отслеживания времени последней секунды

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

}