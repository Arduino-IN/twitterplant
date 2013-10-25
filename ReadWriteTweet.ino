/*
  The Arduino Plant
 
 Retrieves tweets to the plant, tweets a thanks to the user for the watering, and waters the plant
 We control the water pump via a relay 
 Uses Temboo APIs with the Arduino Yun
 
 This example assumes basic familiarity with Arduino sketches, and that your Yun 
 is connected to the Internet.
 
 Code by :
 
 Avik Dhupar @avikd
 Ankit Daftery @ankitdaf
 
 Edited on 25 October 2013 
 */

#include <Bridge.h>
#include <Temboo.h>
#include "TembooAccount.h" // contains Temboo account information

const int relay = 13;

String oldAuthor = "NULL";

int numRuns = 1;   // execution count, so this doesn't run forever
int maxRuns = 3;   // the max number of times the Twitter HomeTimeline Choreo should run, remove this limit for long runs

void setup() { 
  Serial.begin(9600);
  // For debugging, wait until a serial console is connected.
  delay(4000);
  while(!Serial);
  Bridge.begin();
  pinMode(relay,OUTPUT);  
}

void loop()
{
  // while we haven't reached the max number of runs...
  if (numRuns <= maxRuns) {
    bool toReply = 0;  // To reply or not to reply
    Serial.print(F("Running ReadATweet - Run #"));
    Serial.println(String(numRuns++));

    toReply = retrieveTweets();  // To reply or not to reply
    if(toReply){
      returnTweet(oldAuthor);
    }
  }

  Serial.println(F("Waiting..."));
  delay(61000); // wait 61 seconds between HomeTimeline calls to avoid hitting the Twitter API limits
}



void performTask(){
  digitalWrite(relay,HIGH);
  delay(2000);
  digitalWrite(relay,LOW);
  Serial.println(F("*** Task performed *** "));
}

bool retrieveTweets() {

  bool toReply = 0;  // To reply or not to reply

  TembooChoreo HomeTimelineChoreo;

  // invoke the Temboo client.
  HomeTimelineChoreo.begin();

  // set Temboo account credentials
  HomeTimelineChoreo.setAccountName(TEMBOO_ACCOUNT);
  HomeTimelineChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
  HomeTimelineChoreo.setAppKey(TEMBOO_APP_KEY);

  // tell the Temboo client which Choreo to run (Twitter > Timelines > HomeTimeline)
  HomeTimelineChoreo.setChoreo(F("/Library/Twitter/Timelines/Mentions"));
  HomeTimelineChoreo.addInput("AccessToken", TWITTER_ACCESS_TOKEN);
  HomeTimelineChoreo.addInput("AccessTokenSecret", TWITTER_ACCESS_TOKEN_SECRET);
  HomeTimelineChoreo.addInput("ConsumerKey", TWITTER_CONSUMER_KEY);
  HomeTimelineChoreo.addInput("ConsumerSecret", TWITTER_CONSUMER_SECRET);

  // next, we'll define two output filters that let us specify the 
  // elements of the response from Twitter that we want to receive.

  // we want the text of the tweet
  HomeTimelineChoreo.addOutputFilter("tweet", F("/[1]/text"), "Response");

  // and the name of the author
  HomeTimelineChoreo.addOutputFilter("author", F("/[1]/user/screen_name"), "Response");

  // tell the Process to run and wait for the results. The 
  unsigned int returnCode = HomeTimelineChoreo.run();

  //a response code of 0 means success; print the API response
  if(returnCode == 0) {

    String author; // a String to hold the tweet author's name
    String tweet; // a String to hold the text of the tweet

      // choreo outputs are returned as key/value pairs, delimited with 
    // newlines and record/field terminator characters, for example:
    // Name1\n\x1F
    // Value1\n\x1E
    // Name2\n\x1F
    // Value2\n\x1E      

    while(HomeTimelineChoreo.available()) {
      // read the name of the output item
      String name = HomeTimelineChoreo.readStringUntil('\x1F');
      name.trim();

      // read the value of the output item
      String data = HomeTimelineChoreo.readStringUntil('\x1E');
      data.trim();

      // assign the value to the appropriate String
      if (name == "tweet") {
        tweet = data;
      } 
      else if (name == "author") {
        author = data;
      }
    }

    Serial.println("@" + author + " - " + tweet);
    if ((oldAuthor != author ) && StringContains(tweet,"water")) {
      performTask();
      toReply = 1;
      oldAuthor = author;
    }

  } 
  else {
    // there was an error
    // print the raw output from the choreo
    while(HomeTimelineChoreo.available()) {
      char c = HomeTimelineChoreo.read();
      Serial.print(c);
    }
  }
  HomeTimelineChoreo.close();
  return toReply;
}


void returnTweet(String myAuthor){

  Serial.print(F("Running SendATweet to user : "));
  Serial.println(myAuthor);

  // define the text of the tweet we want to send
  String tweetText = "Thank you @";
  TembooChoreo StatusesUpdateChoreo;

  // invoke the Temboo client
  // NOTE that the client must be reinvoked, and repopulated with
  // appropriate arguments, each time its run() method is called.
  StatusesUpdateChoreo.begin();

  // set Temboo account credentials
  StatusesUpdateChoreo.setAccountName(TEMBOO_ACCOUNT);
  StatusesUpdateChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
  StatusesUpdateChoreo.setAppKey(TEMBOO_APP_KEY);

  // identify the Temboo Library choreo to run (Twitter > Tweets > StatusesUpdate)
  StatusesUpdateChoreo.setChoreo(F("/Library/Twitter/Tweets/StatusesUpdate"));

  // set the required choreo inputs
  // see https://www.temboo.com/library/Library/Twitter/Tweets/StatusesUpdate/ 
  // for complete details about the inputs for this Choreo

  // add the Twitter account information
  StatusesUpdateChoreo.addInput("AccessToken", TWITTER_ACCESS_TOKEN);
  StatusesUpdateChoreo.addInput("AccessTokenSecret", TWITTER_ACCESS_TOKEN_SECRET);
  StatusesUpdateChoreo.addInput("ConsumerKey", TWITTER_CONSUMER_KEY);    
  StatusesUpdateChoreo.addInput("ConsumerSecret", TWITTER_CONSUMER_SECRET);

  // and the tweet we want to send
  StatusesUpdateChoreo.addInput("StatusUpdate", tweetText + myAuthor);

  // tell the Process to run and wait for the results. The 
  // return code (returnCode) will tell us whether the Temboo client 
  // was able to send our request to the Temboo servers
  unsigned int returnCode = StatusesUpdateChoreo.run();

  // a return code of zero (0) means everything worked
  if (returnCode == 0) {
    Serial.println(F("Success! Tweet sent!"));
  } 
  else {
    // a non-zero return code means there was an error
    // read and print the error message
    while (StatusesUpdateChoreo.available()) {
      char c = StatusesUpdateChoreo.read();
      Serial.print(c);
    }
  } 
  StatusesUpdateChoreo.close();
}


int StringContains(String s, String search) {
  int max = s.length() - search.length();
  int lgsearch = search.length();

  for (int i = 0; i <= max; i++) {
    if (s.substring(i, i + lgsearch) == search) return i;
  }
  return -1;
}



