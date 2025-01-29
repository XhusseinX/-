#define BLYNK_TEMPLATE_ID "TMPL4rmwiLFqh"  // معرف القالب المستخدم في Blynk
#define BLYNK_TEMPLATE_NAME "IoT Energy Guard"  // اسم القالب في Blynk
#define BLYNK_AUTH_TOKEN "gBtkOsEL-vg-gi3pJNl168LDgj0GM068"  // رمز المصادقة لمنصة Blynk
#define BLYNK_PRINT Serial  // تمكين طباعة البيانات في شاشة السيريال لمراقبة الكود

#include <WiFi.h>  // مكتبة الاتصال بشبكة الواي فاي باستخدام ESP32
#include <WiFiClient.h>  // مكتبة الاتصال بالعملاء على الشبكة
#include <BlynkSimpleEsp32.h>  // مكتبة Blynk الخاصة بـ ESP32
#include "ACS712.h"  // مكتبة حساس التيار ACS712 لقياس التيار المتردد (AC)

// بيانات الاتصال بشبكة الواي فاي
char auth[] = BLYNK_AUTH_TOKEN;  // رمز المصادقة الخاص بمنصة Blynk
char ssid[] = "NCD";  // اسم شبكة الواي فاي
char pass[] = "N123456n";  // كلمة مرور شبكة الواي فاي

// تحديد الأرجل المتصلة بالمفاتيح للتحكم بالأجهزة
const int switchPins[] = {13, 2, 5, 12};  // الأرجل المتصلة بالمفاتيح
const int switchPin4 = 4;  // مفتاح التحكم في بدء الطباعة
const int irPin = 25;  // حساس الأشعة تحت الحمراء للكشف عن وجود الأشخاص

// تعريف الحساسات لقياس التيار المتردد (AC) على المنافذ التناظرية
ACS712 currentSensors[] = {
    ACS712(34, 5.0, 1023, 30),  
    ACS712(35, 5.0, 1023, 30),
    ACS712(32, 5.0, 1023, 30),
    ACS712(39, 5.0, 1023, 30)
    };

// المنافذ الافتراضية لعرض البيانات على Blynk
const int gaugePins[] = {V0, V2, V5, V12};  // كل حساس لديه منفذ خاص على Blynk لعرض قيم التيار
const int statusPin = V1;  // منفذ حالة وجود الأشخاص في الغرفة (حساس IR)
const int totalConsumptionPin = V20;  // منفذ عرض الاستهلاك الكلي للطاقة على Blynk
const int predictionPin = V21;  // منفذ استقبال التنبؤات من Raspberry Pi

// متغيرات التحكم في جمع البيانات
bool startPrintingData = false;  // لتحديد ما إذا كان يجب جمع البيانات
bool sentA = false;  // للتبديل بين الحالة "A" عند الضغط على المفتاح
bool lastSwitchState = HIGH;  // لتتبع حالة المفتاح في الدورة السابقة

void setup() {
    Serial.begin(115200);  // بدء الاتصال التسلسلي بسرعة 115200 لعرض البيانات
    Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);  // بدء الاتصال مع منصة Blynk
    delay(1000);  // تأخير بسيط لضمان الاتصال

    // إعداد المفاتيح كمدخلات مع مقاومة سحب عالية (INPUT_PULLUP)
    for (int i = 0; i < 4; i++) {
        pinMode(switchPins[i], INPUT_PULLUP);
    }
    pinMode(switchPin4, INPUT_PULLUP);  // مفتاح التحكم في الطباعة
    pinMode(irPin, INPUT);  // حساس الأشعة تحت الحمراء

    // تهيئة حساسات التيار
    for (int i = 0; i < 4; i++) {
        currentSensors[i].autoMidPoint();  // حساب نقطة المنتصف تلقائيًا لكل حساس
    }
}

void loop() {
    Blynk.run();  // تنفيذ التفاعل مع منصة Blynk

    // التحقق من حالة المفتاح الرابع (الذي يتحكم في بدء جمع البيانات)
    int switchState = digitalRead(switchPin4);  // قراءة حالة المفتاح
    if (switchState == LOW && lastSwitchState == HIGH) {  // التبديل بين حالتين عند الضغط
        sentA = !sentA;  // تبديل حالة "A"
        startPrintingData = !startPrintingData;  // بدء أو إيقاف جمع البيانات
        Serial.println("A");  // طباعة "A" في شاشة السيريال
        delay(300);  // تأخير لمنع التبديل السريع
    }
    lastSwitchState = switchState;  // حفظ الحالة الحالية للمفتاح

    // إذا كان يجب جمع البيانات، نبدأ في قراءة الحساسات
    if (startPrintingData) {
        // قراءة حالة حساس الأشعة تحت الحمراء (IR) للكشف عن وجود شخص في الغرفة
        int irState = digitalRead(irPin);  // إذا كان هناك شخص في الغرفة
        Blynk.virtualWrite(statusPin, irState == HIGH ? "There is someone in the room" : "No one in the room");

        // المتغير الخاص بتخزين قيم التيار من الحساسات للطباعة على شاشة السيريال
        String valuesToPrint = "";

        // المتغير الخاص بحساب الاستهلاك الكلي للطاقة
        double totalConsumption = 0.0;

        // قراءة البيانات من جميع الحساسات
        for (int i = 0; i < 4; i++) {
            int switchState = digitalRead(switchPins[i]);  // قراءة حالة المفتاح المتصل بالحساس
            double current = (switchState == HIGH) ? currentSensors[i].getCurrentAC() : 0.0;  // قراءة قيمة التيار إذا كان المفتاح في حالة تشغيل

            Blynk.virtualWrite(gaugePins[i], current);  // إرسال قيمة التيار إلى Blynk
            totalConsumption += current;  // إضافة القيمة إلى الاستهلاك الكلي للطاقة

            valuesToPrint += String(current, 3) + ",";  // إضافة القيم إلى السلسلة لتتم طباعتها
        }

        // إرسال الاستهلاك الكلي إلى Blynk فقط (دون طباعته على السيريال)
        Blynk.virtualWrite(totalConsumptionPin, totalConsumption);

        // إزالة الفاصلة الزائدة في النهاية للطباعة بشكل صحيح
        valuesToPrint.remove(valuesToPrint.length() - 1);
        Serial.println(valuesToPrint);  // طباعة قيم التيار فقط على شاشة السيريال

        delay(1000);  // تأخير بسيط بين القراءات
    }

    // استقبال بيانات التنبؤات من Raspberry Pi عبر السيريال
    if (Serial.available()) {
          String receivedData = Serial.readStringUntil('\n');
          receivedData.trim();
          int firstComma = receivedData.indexOf(',');
          int secondComma = receivedData.indexOf(',', firstComma + 1);
          
          if (firstComma != -1 && secondComma != -1) {
            double nextDay = receivedData.substring(0, firstComma).toFloat();
            double nextWeek = receivedData.substring(firstComma + 1, secondComma).toFloat();
            double nextMonth = receivedData.substring(secondComma + 1).toFloat();
            
            Blynk.virtualWrite(nextDayPin, nextDay);
            Blynk.virtualWrite(nextWeekPin, nextWeek);
            Blynk.virtualWrite(nextMonthPin, nextMonth);
          }
}
