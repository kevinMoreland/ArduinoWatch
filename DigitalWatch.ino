#include <Adafruit_GFX.h>
#include <RTClib.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13

Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

//the pins used for the left and right button
#define LEFT_BUTTON 8
#define RIGHT_BUTTON 7

//give identification numbers to the mode options
#define MENU_SETTINGS 0
#define MENU_STOPWATCH 1
#define MENU_MONEYWATCH 2
#define MENU_TIME 3
int amountOfSubMenus[] = {2, 0, 0, 0};

//give identification numbers to the types of clicks
#define SHORT_LEFT_CLICK  0 
#define LONG_LEFT_CLICK  1
#define SHORT_RIGHT_CLICK  2 
#define LONG_RIGHT_CLICK  3
#define SHORT_SELECTION_CLICK  4
#define LONG_SELECTION_CLICK  5

//this is for accessing the Real Time Clock
RTC_PCF8523 rtc;

//define what each day of the week will be displayed as on the watch
char daysOfTheWeek[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

unsigned long stopWatchTotalMiliSeconds, stopWatchStartMiliSeconds, stopWatchPauseTime, stopWatchStartPause;
unsigned long moneyWatchTotalMiliSeconds, moneyWatchStartMiliSeconds, moneyWatchIntervalCountStart, moneyWatchPauseTime, moneyWatchStartPause;
unsigned long timeKeepingStart;

float originalPayPerHour;
float originalPercentKeepAfterTax;

int minutes, seconds;
int pausePrinted;
int toggleMenu, toggleSubMenu;
bool stopWatchStarted, moneyWatchStarted;
float hourlyWage, minuteWage;
float totalMoneyEarned;
int buttonStateLeft, buttonStateRight;
bool selectionClick;
int subMenuClicks;
bool refreshScreenBecauseFirstTime;

int intervalUpdateTimeKeep = 30000;
int intervalUpdateMoneyKeep = 5000;

float payPerHour;
float percentKeepAfterTax;

bool pauseStopWatch, pauseMoneyWatch;

void setup()   {
  //set input pins for the buttons
  pinMode(RIGHT_BUTTON, INPUT);
  pinMode(LEFT_BUTTON, INPUT);
  Serial.begin(9600);
  //initialize the OLED display
  display.begin(SSD1306_SWITCHCAPVCC);
  rtc.begin();
  
  //clear out the default splashscreen immediately so it doesn't appear
  display.clearDisplay();
  
  //create circle animation splashscreen
  circleSplashScreen();
  delay(2000);

  //clear out the circle splash screen
  display.clearDisplay();

  display.setTextColor(WHITE, BLACK);
  display.setTextSize(4);

  //setup watch features
  setupStopWatch();
  setupMoneyWatch();
  setupTimeKeeping();
    
  selectionClick = false;

  //this allows screen to refresh immediately when change to a new mode.
  refreshScreenBecauseFirstTime = true;
  
  //start menu. Default will be the time keeping mode
  toggleMenu = MENU_TIME;
  toggleSubMenu = 0;

  //used to handle all the clicks when inside a subMenu
  subMenuClicks = 0;
  
  /*sets time to current time when uploaded. Only need to use this once, then can comment it out (if Arduino turns off or restarts and this code is still here,
  it will display the time of the previous upload which would be incorrect*/
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
}

void loop() { 
  //read any clicks user inputs
  handleClicks();

  //display proper information to user
  switch(toggleMenu){
    case MENU_STOPWATCH:
      //stopwatch menu
      stopWatch();
    break;
    case MENU_MONEYWATCH:
      //moneywatch menu
      moneyWatch();
    break;
    case MENU_TIME:
      //time menu
      timeKeeping();      
    break;
    case MENU_SETTINGS:
      settings();
    break;
  }
}

void setupMoneyWatch(){
  pauseMoneyWatch = true;
  moneyWatchTotalMiliSeconds = 0;
  moneyWatchStarted = false;
  moneyWatchPauseTime = 0;
  moneyWatchStartPause = 0;
}
void setWage(float pay){
  payPerHour = pay;
  hourlyWage = payPerHour * percentKeepAfterTax;
  minuteWage = hourlyWage/60;
}
void setKeptAfterTaxes(float percentAftTax){
    percentKeepAfterTax = percentAftTax;
    hourlyWage = payPerHour * percentKeepAfterTax;
    minuteWage = hourlyWage/60;
}
void setupStopWatch(){
  pauseStopWatch = true;
  totalMoneyEarned  = 0;
  stopWatchStarted = false;
  stopWatchPauseTime = 0;
  stopWatchStartPause = 0;
}

void stopWatch(){

  if(!stopWatchStarted){
    stopWatchStartMiliSeconds = millis();
    stopWatchStarted = true;
    stopWatchTotalMiliSeconds = 0;
  }
  if(pauseStopWatch){
     display.setTextColor(BLACK,WHITE);
  }
  else{
    display.setTextColor(WHITE,BLACK);
    stopWatchTotalMiliSeconds = millis() - stopWatchStartMiliSeconds;
  }
  
  //use stopWatchTotalMiliSeconds = 0; to reset
  minutes = stopWatchTotalMiliSeconds/60000;
  seconds = (stopWatchTotalMiliSeconds/1000) - (minutes*60);
  String outputMin = String(minutes);
  String outputSec = String(seconds);
  display.clearDisplay();

  if(minutes>99){
    display.setTextSize(3);
  }
  else{
    display.setTextSize(4);
  }
   
  display.setCursor(0,0);
  display.print(outputMin);
  display.print(":");
  
  if(seconds<10){
    display.print("0");
  }
  
  display.print(outputSec);
  display.display();  
}

void moneyWatch(){  
  //normal behavior
          
  if(!moneyWatchStarted){
    moneyWatchStartMiliSeconds = millis();
    moneyWatchIntervalCountStart = millis();
    moneyWatchStarted = true;
    moneyWatchTotalMiliSeconds = 0;
  }
  if(pauseMoneyWatch){
     display.setTextColor(BLACK,WHITE);
  }
  else{
     display.setTextColor(WHITE,BLACK);
     moneyWatchTotalMiliSeconds = millis() - moneyWatchStartMiliSeconds;
  }
  
  //every 5 seconds or when this feature first starts, display amount earned.
  if(moneyWatchIntervalCountStart>=intervalUpdateMoneyKeep || refreshScreenBecauseFirstTime){
    moneyWatchIntervalCountStart = millis();
    refreshScreenBecauseFirstTime = false;
    
    String outputMoney = String(totalMoneyEarned);
    
    //update screen every 5 sec
    display.clearDisplay();
    
    if(totalMoneyEarned<=9){
      display.setTextSize(4);
    }
    else if(totalMoneyEarned>9){
      display.setTextSize(3);
    }
    else if(totalMoneyEarned>99){
      display.setTextSize(2);
    }
    else if(totalMoneyEarned>999){
      display.setTextSize(1);
    }
    display.setCursor(0,0);
    display.print("$");
    display.print(outputMoney);
    display.display();
        
    totalMoneyEarned = minuteWage * (moneyWatchTotalMiliSeconds / 60000);
  }
  else{
    moneyWatchTotalMiliSeconds = millis() - moneyWatchStartMiliSeconds;
    
    //every 5 seconds or when this feature first starts, display amount earned.
    if(moneyWatchTotalMiliSeconds>=5000){
      moneyWatchStartMiliSeconds = millis();
    }
  }  
}
void settings(){
  switch(toggleSubMenu){
    case 0:
      if(refreshScreenBecauseFirstTime){
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(0,0);
        display.print("Settings");
        display.display();
      }
    break;
    case 1:
    
      if(refreshScreenBecauseFirstTime){
        originalPayPerHour = payPerHour;
        refreshScreenBecauseFirstTime = false;        
      }

      //edit wage per hour
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0,0);
      display.print("hourly wage");
      display.print("\n$");
      setWage(originalPayPerHour + .25 * subMenuClicks);
      display.print(payPerHour);
      display.display();
    break;
    case 2:
      if(refreshScreenBecauseFirstTime){
        originalPercentKeepAfterTax = percentKeepAfterTax;
        refreshScreenBecauseFirstTime = false;
      }
      
      //edit amount kept after tax
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0,0);
      display.print("% kept after tax");
      display.print("\n");
      setKeptAfterTaxes(originalPercentKeepAfterTax + subMenuClicks * .01);
      display.print(percentKeepAfterTax * 100);
      display.print("%");
      display.display();
    break;
  }
}
void clearMoneyWatch(){
  totalMoneyEarned = 0;
}
void setupTimeKeeping(){
  timeKeepingStart = millis();
}
void timeKeeping(){
  if(millis() - timeKeepingStart > intervalUpdateTimeKeep || refreshScreenBecauseFirstTime){
    timeKeepingStart = millis();
    refreshScreenBecauseFirstTime = false;
    
    display.clearDisplay();
    DateTime now = rtc.now();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print(daysOfTheWeek[now.dayOfTheWeek()]);
    display.print(" ");
    display.print(now.month());
    display.print("/");
    display.print(now.day());
    display.print("/");
    display.print(now.year());
    
    display.print("\n   ");
    display.setTextSize(3);
    
    int curHour = now.hour();
    if(curHour>12){
      curHour -=12;
    }

    display.print(curHour);
    display.print(":");
    
    if(now.minute()<10){
      display.print("0");
    }
    
    display.print(now.minute());
    display.display();
  }
  
}

void handleClicks(){
  int buttonReading = readButtonClicks();
  
  if(buttonReading == SHORT_RIGHT_CLICK){
    display.setTextColor(WHITE,BLACK);

    if(toggleSubMenu == 0){
      //doesn't work with submenus because I set place holding variables only once in those. Instead, I also put this in short selection click
      //basically this is only true if switching from one menu to another or one submenu to another
      refreshScreenBecauseFirstTime = true;
      
      if(toggleMenu >=3){
        toggleMenu = 0;
      }
      else{
        toggleMenu++;
      }
    }
    else{
      //in a submenu
      subMenuClicks--;
    }
    
  }
  else if(buttonReading == SHORT_LEFT_CLICK){
    display.setTextColor(WHITE,BLACK);

    if(toggleSubMenu == 0){
    refreshScreenBecauseFirstTime = true;

      if(toggleMenu <=0){
        toggleMenu = 3;
      }
      else{
        toggleMenu--;
      }
    }    
    else{
      //in a submenu
      subMenuClicks++;
    }
  }
  else if(buttonReading == SHORT_SELECTION_CLICK){    
    //reset subMenuClicks since going to new submenu
    subMenuClicks = 0;
    refreshScreenBecauseFirstTime = true;
    //increment through toggleSubMenus
    if(amountOfSubMenus[toggleMenu] == toggleSubMenu){
      toggleSubMenu = 0;
    }
    else{
      toggleSubMenu++;
    }
    //pause money or stop watch
    if(toggleMenu == MENU_MONEYWATCH){
      pauseMoneyWatch = !pauseMoneyWatch;
      if(!pauseMoneyWatch){
        moneyWatchPauseTime = millis() - moneyWatchStartPause;
        if(moneyWatchTotalMiliSeconds>0){
          moneyWatchStartMiliSeconds += moneyWatchPauseTime;
        }
        else{
          moneyWatchStarted = false;
        }
      }
      else{
        moneyWatchStartPause = millis();
      }
    }
    else if(toggleMenu == MENU_STOPWATCH){
      pauseStopWatch = !pauseStopWatch;
      if(!pauseStopWatch){
        stopWatchPauseTime = millis() - stopWatchStartPause;

        /*this avoids the issue where the watch displays 60646 minutes when you pause the watch, reset it, and try to start it again
        the issue is that stopWatchStartMiliSeconds is made greater than millis(), resulting in a negative number for 
        stopWatchTotalMiliSeconds. Since I am dealing with unsigned longs with range 0 to 4,294,967,295 and the values wrap,
        the negative number translates to a very high number (4,294,967,295/60000 for the minutes)*/
        if(stopWatchTotalMiliSeconds>0){
          stopWatchStartMiliSeconds += stopWatchPauseTime;
        }
        else{
          /*make sure to call this after pause. If the watch is cleared, totalmili will be 0. Otherwise, watch will start timing again right after it is cleared cause it
          would reset those values at the time of reset only*/
          stopWatchStarted = false;
        }
      }
      else{
        stopWatchStartPause = millis();
      }
    }
  }
  else if(buttonReading == LONG_SELECTION_CLICK){
    //clear money and stop watch
    if(toggleMenu == MENU_MONEYWATCH){
      moneyWatchStarted = false;
    }
    else if(toggleMenu == MENU_STOPWATCH){
      stopWatchStarted = false;
    }
  }

}

int readButtonClicks(){
  buttonStateLeft = digitalRead(LEFT_BUTTON);
  buttonStateRight = digitalRead(RIGHT_BUTTON);

  //if click greater than .5s, it is a long click
  long compare = 500;
  long pressTimeLength;
  
  if(buttonStateLeft == 0){
    pressTimeLength = timeClick(LEFT_BUTTON, RIGHT_BUTTON);

    if(pressTimeLength < compare && !selectionClick){
      //single, short press from leftButton
      return SHORT_LEFT_CLICK;
    }
    else if(pressTimeLength >= compare && !selectionClick){
      //single, long press from leftButton
      return LONG_LEFT_CLICK;
    }
    else if(pressTimeLength <  compare && selectionClick){
      //selection click, short press
      return SHORT_SELECTION_CLICK;
    }
    else if(pressTimeLength >=compare && selectionClick){
      //selection click, long press
      return LONG_SELECTION_CLICK;
    }
  }
  else if(buttonStateRight == 0){
    pressTimeLength = timeClick(RIGHT_BUTTON, LEFT_BUTTON);
        if(pressTimeLength < compare && !selectionClick){
      //single, short press from rightButton
      return SHORT_RIGHT_CLICK;
    }
    else if(pressTimeLength >= compare && !selectionClick){
      //single, long press from rightButton
      return LONG_RIGHT_CLICK;
    }
    else if(pressTimeLength < compare && selectionClick){
      //selection click, short press
      return SHORT_SELECTION_CLICK;
    }
    else if(pressTimeLength >= compare && selectionClick){
      //selection click, long press
      return LONG_SELECTION_CLICK;
    }
  }
  
  return -1;
}
long timeClick(int button, int otherButton){
  long startPress, elapsedPress;
  selectionClick = false;
  startPress = millis();
  
  while(digitalRead(button) == 0){
    //wait for user to release press...
    //if the other button is pressed while this one is pressed, it is a selection click
    if(digitalRead(otherButton) == 0){
      selectionClick = true;
    }
  }
  elapsedPress = millis() - startPress;
  return elapsedPress;
}


void circleSplashScreen(void) {
  //this is the expanding ripple that appears when watch first starts up
  for (int16_t i=0; i<display.height()*2.5; i++) {
    display.drawCircle(display.width()/2, display.height()/2, i, WHITE);
    display.display();

    //adds the extra effect of changing thickness in the ripple
    if(i%5==0){
      display.clearDisplay();
    }
  }
}

