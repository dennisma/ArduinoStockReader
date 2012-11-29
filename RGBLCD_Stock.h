/*********************
 * 
 * Grab stocks using Wifi shield and display on Adafruit RGB LCD shield
 * 
 *  Buttons allow you to scroll through your stocks.  Look in header file
 *  for stock symbols to look up.
 * By Dennis Mabrey (dennisma@stormingrobots.com)
 * 
 **********************/
#ifdef STOCK_DEBUG
#define DEBUG_INIT()       Serial.begin(9600);\
                           while (!Serial) ; // For Leo's and Micro's (so far)
#define DEBUG_PRINT(x)     Serial.print (x)
#define DEBUG_PRINTLN(x)   Serial.println (x)
#else
#define DEBUG_INIT()       
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)  
#endif

// LCD colors from Adafruit
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

// Light the RGB when doing certain things
#define BACKLIGHT_NETWORK_SETUP  WHITE       //init network
#define BACKLIGHT_UPDATING       BLUE        //downloading stocks
#define BACKLIGHT_STOCK_DOWN     RED         //stock is down
#define BACKLIGHT_STOCK_UP       GREEN       //stock is up (yeah)

#define CHECKEVERY 300000  // check for stocks every 5 min
#define LIGHTS_OUT 30000   // unused for now
#define DONT_CHECK_IF_BUTTON_PUSHED_WITHIN  20000  // if button pushed hold off check for stocks for 20 more seconds if ready

#define LASTCHECKEDSTOCKS (millis() - glastcheckstocks)  // when was the last time we downloaded stocks?
#define LASTPUSHEDBUTTON  (millis() - glastpushedbutton) // when was the last time user pushed button?
#define KEEP_LIGHT_OFF()    ((millis() - glastpushedbutton) > LIGHTS_OUT)  // unused for now
#define TIMETOCHECK() ((LASTCHECKEDSTOCKS >= CHECKEVERY) && (LASTPUSHEDBUTTON >= DONT_CHECK_IF_BUTTON_PUSHED_WITHIN))  // if true, we can go download stocks
#define BLINKTIME 1000
#define TIMETOBLINK() ((millis() - lastblinked) > BLINKTIME)

#define STOCK_SERVER "download.finance.yahoo.com"  // host to check for stocks

// Custom characters for the UP arrow and DOWN arrow (see font map below)
#define UPARROW   0
#define DOWNARROW 1

// font maps for the UP and DOWN arrows (when stocks are up and down)
// Seems the downarrow gets more use *&^%#$%!!!
byte uparrow[8] = {
  B00000,
  B00100,
  B01110,
  B11111,
  B01110,
  B01110,
  B01110,
};
byte downarrow[8] = {
  B00000,
  B01110,
  B01110,
  B01110,
  B11111,
  B01110,
  B00100,
};
// User wants to see percent displayed instead of price change
boolean gDisplayPercent = false;

// Basic stockinfo struct - symbol must be filled but everything else should be "" or 0.
struct StockInfo{
  char name[8];      //'pretty' name of stock/mututalfund/etf/futures....
  char symbol[13];   // the lookup symbol
  char time[7];      // time of quote price
  float price;       // the price
  float change;      // the amount changed
};

// here we set up our stock symbols.  Should test these symbols against yahoo
// use URL http://download.finance.yahoo.com/d/quotes.csv?s=MSFT&f=sl1d1t1c1ohgv&e=.csv
// but replace s=MSFT with s=stock symbol such as s=AAPL
// Reference - GCJ13.CMX is the price of gold in case you are curious and 
// EURUSD=X is the price of the Euro (the change value is always N/A but the price is always correct)
// Remember the more stocks you add the more memory is taken up and the longer it takes to download. 
// ....  I'm just sayin...
StockInfo quotes[] = {
  {"MSFT","MSFT","",0.00,0.00        },  //Microsoft is too large a name to fit
  {"Apple","AAPL","",0.00,0.00       },
  {"Gold","GCJ13.CMX","",0.00,0.00   },
  {"Euro","EURUSD=X","",0.00,0.00    },
};

int gcurrentstock = 0;  // this is the current stock in the array the user is looking at on the LCD

// numofstocks is the number of stocks (duh).  It will determine the number of 
// stocks added so you never have to update it
const int numofstocks = sizeof(quotes)/sizeof(StockInfo);

unsigned long glastcheckstocks;  // last time stocks were checked in millis
unsigned long glastpushedbutton; // last time user pushed a button in millis

// RGB LCD object (with buttons)
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// I like to put function decl here.  I dunno why.
void  lcdprint(const __FlashStringHelper* str, int lineno, int colno);
void  lcdprint(const char* str, int lineno, int colno);
void  lcdprint(float val, int lineno, int colno);
void  lcdclearline(int lineno);
void  initNetwork();
void  initLCD();
void  checkstocks();
void  displayNextStock();
void  displayPrevStock();
void  displayStock(StockInfo& quote);  
float PercentChanged(const StockInfo& quote);
void  GetStockPrice(StockInfo& quote);
void  setbacklight(const StockInfo& quote);
boolean YahooFinanceRequest(Client& client,const char* symbol) ;





// end of the header file...

