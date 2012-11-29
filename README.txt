Ok real simple...

Edit the configuration.h file and enter in your network SSID and passphrase/key.
Remember... DON'T share this with anyone OK?

Next edit the RGBLCD_Stock.h file and edit the StockInfo structure array called 'quotes'
It will look like this:

StockInfo quotes[] = {
  {"MSFT","MSFT","",0.00,0.00        },  //Microsoft is too large a name to fit
  {"Apple","AAPL","",0.00,0.00       },
  {"Gold","GCJ13.CMX","",0.00,0.00   },
  {"Euro","EURUSD=X","",0.00,0.00    },
};

The first field is the 'display name' and the second field is the stock ticker.  
Leave the other fields blank or set to 0.00.  Note that I truncate the name after 
8 chars so make it fit somehow.

You can increase or decrease the number of stocks.  You won't have to change anything else 
because there is a constant defined as: 

       const int numofstocks = sizeof(quotes)/sizeof(StockInfo);

that will calculate the number of stocks in the list at compile time.

That's it.