/*********************
 * 
 * Grab stocks using Wifi shield and display on Adafruit RGB LCD shield
 * 
 *  Buttons allow you to scroll through your stocks.  Look in header file
 *  for stock symbols to look up.
 * By Dennis Mabrey (dennisma@stormingrobots.com)
 * 
 *   Things to do:  
 *               1. Sometimes a stock download 'fails' but the others work.
 *                  I should change the RGB color to something that shows
 *                  it failed.  
 *               2. I'd like to make the RGB blink when the percent change
 *                  is above/below a threshold (say +- 5%) to warn me that 
 *                  something is going on.
 *               3. Add news feeds for each stock...  yeah right...well...
 **********************/
 
// Ok... Uncomment the STOCK_DEBUG if you want to debug with serial monitor
// NOTE: if you are using a Leonardo or Micro NOTHING will happen until you
// open the Serial Monitor from the IDE. Just saying. See RGBLCD_Stock.h

//#define STOCK_DEBUG
// Don't uncommnet this silly

// include the library code:
#include <WiFi.h>                     // for Wifi shield
#include <HttpClient.h>               // Because I need all the help I can get
#include <Wire.h>                     // Required for LCD Shield
#include <Adafruit_MCP23017.h>        // The chip on the LCD Shield
#include <Adafruit_RGBLCDShield.h>    // Library for accessing LCD Shield

#include "configuration.h"        // Edit this header for YOUR network...
#include "RGBLCD_Stock.h"         // Edit this file to set stocks.. should prob be in separate header but ahhh well

// four helper functions with the LCD.  Could extend the class I suppose but...
// Anyways 3 lcdprint functions to use.
void lcdprint(const __FlashStringHelper* str, int lineno=0, int colno=0){
  lcd.setCursor(0,lineno);
  lcd.setCursor(colno,lineno);
  lcd.print(str);    
}

void lcdprint(const char* str, int lineno=0, int colno=0){
  lcd.setCursor(0,lineno);
  lcd.setCursor(colno,lineno);
  lcd.print(str);    
}

void lcdprint(float val, int lineno=0, int colno=0){
  lcd.setCursor(0,lineno);
  lcd.setCursor(colno,lineno);
  lcd.print(val);    
}

// Clears one line on the LCD
void lcdclearline(int lineno=0){
  lcd.setCursor(0,lineno);
  lcd.print(F("                "));
}

// Initialize the network.  Display on LCD failure or success
void initNetwork(){
  int status = WL_IDLE_STATUS;
  if (WiFi.status() == WL_NO_SHIELD) {  //what the...
    lcdclearline();
    lcdprint(F("WiFi shield not present")); 
    // don't continue:
    while(true);
  } 
  while ( status != WL_CONNECTED) { 
    lcdclearline();
    lcdprint(F("Connecting"));
    lcdprint(F("SSID:"),1);
    lcdprint(ssid,1,6); 
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:    
    status = WiFi.begin(ssid, pass);
    DEBUG_PRINTLN(status);
    // wait 10 seconds for connection:
    delay(10000);
  }
  lcdprint(F("Connected    "));  // not sure why I print this it doesn't last long
}

// Inits the LCD and creates the up and down arrow defined in RGBLCD_Stock.h
void initLCD(){
  lcd.begin(16, 2);
  lcd.createChar(UPARROW, uparrow);
  lcd.createChar(DOWNARROW, downarrow);
  lcd.setBacklight(BACKLIGHT_NETWORK_SETUP);
}

// enumerates through the stocks.  
void checkstocks(){
  DEBUG_PRINTLN(numofstocks);
  lcd.clear();
  lcdprint(F("Wait getting..."));
  lcd.setBacklight(BACKLIGHT_UPDATING); 

  for (int i = 0;i< numofstocks;i++){
    lcdclearline(1);
    lcdprint(quotes[i].name,1);
    GetStockPrice(quotes[i]);
  }
  glastcheckstocks = millis();
  displayStock(quotes[gcurrentstock]);  //display last stock user was looking at
}

// Next two functions cycle through the stocks. Called when up down buttons pushed
void displayNextStock(){
  gcurrentstock++;
  gcurrentstock %= numofstocks; 
  DEBUG_PRINTLN(gcurrentstock);
  displayStock(quotes[gcurrentstock]);
}

void displayPrevStock(){
  gcurrentstock--;
  if (gcurrentstock <0) gcurrentstock = numofstocks-1; 
  DEBUG_PRINTLN(gcurrentstock);
  displayStock(quotes[gcurrentstock]);
}

// display stockquote info - assumes values has been filled in by checkstocks()
void displayStock(StockInfo& quote){  
  int truncat = 8;
  lcd.clear();
  char to_display[10];
  strncpy(to_display,quote.name,truncat);  //truncate the name some are large (like gold)
  to_display[truncat] = '\0';
  lcdprint(to_display);
  if (quote.change < 0){                 // Display appropriate arrow
    lcd.write(DOWNARROW);
  }
  else{
    lcd.write(UPARROW);
  }
  setbacklight(quote);                   // display RED or GREEN backlight
  lcdprint(quote.time,0,9);   
  lcdprint(quote.price,1,0); 

  if (gDisplayPercent){                  // user might be in 'display percent' mode instead
    char str[5];                         // of display the change in price
    float percentchanged = PercentChanged(quote);
    lcdprint(percentchanged,1,9); 
    lcd.write('%');
  } 
  else {
    lcdprint(quote.change,1,9); 
  }
}
// returns the percent changed for a particular stock
float PercentChanged(const StockInfo& quote){
  return (quote.change/(quote.price - quote.change))*100.00;
}

// retrieves stock info based quote.symbol stock symbol
// from yahoo. 
// Parses the HTML and CSV file
void GetStockPrice(StockInfo& quote){
  char line[90] = "";
  char *ptr = line;
  WiFiClient client;
  float stockprice=0;
  //  currentLine.reserve(90);
  boolean linefound = false;

  // makes call to Yahoo finance website for CSV of data
  if (!YahooFinanceRequest(client,quote.symbol)){
    DEBUG_PRINT(F("failled YahooFinanceRequest"));
    return ;
  }

  char c = '\n';
  char prev_char;
  //call was successful now while we can read data parse out the ONE line
  //that has stock information (which starts with "MSFT" or "AAPL" but I just check for ")
  //it is not the best but it is faster and takes up much less memory.
  while(client.connected()){
    while (client.available()) {
      prev_char = c;
      c = client.read();
      //      DEBUG_PRINT(c);
      if (c == '\"' && prev_char == '\n')linefound = true;
      if (linefound){
        if (c == '\n') {
          linefound = false;     
        }
        else if (c!= '\"'){
          //          DEBUG_PRINT(c);
          *ptr++ = c;
        }
      }
    }
  }
  client.stop();  // close up client we are done with this call

  //now parse the the one line string- it has the the stock information
  *ptr++ = '\0';
  DEBUG_PRINTLN(F("Line Received"));  
  DEBUG_PRINTLN(line);
  const char token[] = ",";
  if (strlen(line)> 10){ //if it is bigger than a breadbox it "probably" worked 
    strtok(line,token);                    // parse symbol - we don't need that
    char val[14];
    strcpy(val,strtok(NULL,token));        // parse price - convert to float
    quote.price = atof(val);
    val[0] = '\0';
    strtok(NULL,token);                    //date -  unused for now 
    strcpy(quote.time,strtok(NULL,token)); // parse out time
    strcpy(val,strtok(NULL,token));        // parse out change (pos or neg float)
    quote.change = atof(val);              // convert to float
    //print to serial the info for debugging purposes.
    DEBUG_PRINT(F("symbol  "));
    DEBUG_PRINTLN(quote.symbol);
    DEBUG_PRINT(F("price  "));  
    DEBUG_PRINTLN(quote.price);
    DEBUG_PRINT(F("time  "));
    DEBUG_PRINTLN(quote.time);
    DEBUG_PRINT(F("change  "));
    DEBUG_PRINTLN(quote.change);
  }
}

// this method makes a HTTP connection to the server:
boolean YahooFinanceRequest(Client& client,const char* symbol) {
  // if there's a successful connection:
  if (client.connect(STOCK_SERVER, 80)) {
    DEBUG_PRINTLN(F("connecting..."));
    // send the HTTP PUT request:
    //http://download.finance.yahoo.com/d/quotes.csv?s=MSFT&f=sl1d1t1c1ohgv&e=.csv
    //    client.println(F("GET /d/quotes.csv?s=MSFT&f=sl1d1t1c1ohgv&e=.csv HTTP/1.1"));
    client.print(F("GET /d/quotes.csv?s="));
    client.print(symbol);
    client.println(F("&f=sl1d1t1c1&e=.csv HTTP/1.1"));     // this tells what fields to return.
    client.println(F("Host: download.finance.yahoo.com"));
    client.println(F("User-Agent: arduino-ethernet"));
    client.println(F("Connection: close"));
    client.println();
    return true;
  } 
  else {
    // if you couldn't make a connection:
    DEBUG_PRINTLN(F("connection failed"));
    DEBUG_PRINTLN(F("disconnecting."));
    client.stop();
    return false;  // I don't particularly care about failures.  I'll know it based on the time
  }
}

// simple, price is up go Green, price down go RED
void setbacklight(const StockInfo& quote){
  if (quote.change < 0.00){
    lcd.setBacklight(BACKLIGHT_STOCK_DOWN);
  }
  else{
    lcd.setBacklight(BACKLIGHT_STOCK_UP);
  }  
}

// Inits everything like a good setup function
void setup() {
  DEBUG_INIT();
  initLCD();
  
  DEBUG_PRINTLN(F(""));
  DEBUG_PRINTLN(F("--------------------------------"));  
  DEBUG_PRINTLN(F("Running"));
  DEBUG_PRINTLN(F("--------------------------------"));

  initNetwork();
  checkstocks();   //first call
}

void loop() {
  static boolean turnoff = true;

  if (TIMETOCHECK()){  // check the stocks every so often
    checkstocks();
  }
  uint8_t buttons = lcd.readButtons();
  if (buttons) {
    glastpushedbutton = millis();  // no update for 20 sec if button pushed
    if (buttons & BUTTON_UP) {
      displayPrevStock();      
    }
    if (buttons & BUTTON_DOWN) {
      displayNextStock();
    }
    if (buttons & BUTTON_SELECT) {  //select button updates individual stock. fancy huh
      displayStock(quotes[gcurrentstock]);
      lcdclearline(1);
      lcdprint("Updating",1);
      lcd.setBacklight(BACKLIGHT_UPDATING); // Light to tell user downloading stuff.
      GetStockPrice(quotes[gcurrentstock]);
      displayStock(quotes[gcurrentstock]);
    } 
    if (buttons & BUTTON_RIGHT){
      gDisplayPercent = !gDisplayPercent; 
      displayStock(quotes[gcurrentstock]);
    }
    delay(300);   // prevents accidental skipping over stocks
  }  

}  //the end of loop()

// this is the end... my only friend... the end
