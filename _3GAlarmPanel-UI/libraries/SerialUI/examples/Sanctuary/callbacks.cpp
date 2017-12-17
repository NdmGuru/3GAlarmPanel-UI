

/*
   callbacks.cpp -- part of the Sanctuary project.
   Implementation of callbacks -- YOUR code goes here!

   This example was actually generated (by Druid Builder) and
   created as part of the builder tutorial series.

   It includes a few commands, a number of input requests and
   submenus... you can see the whole process of creating the
   system in the learning section of the devicedruid.com site
    https://devicedruid.com/learn-more/learn-druid-builder/


   Generated by DruidBuilder [https://devicedruid.com/],
   as part of project "08a0574074914bed8e8d0c039c85c353N4AD2BFZEU",
   aka Sanctuary.

   Druid4Arduino, Device Druid, Druid Builder, the builder
   code brewery and its wizards, SerialUI and supporting
   libraries, as well as the generated parts of this program
   are
              Copyright (C) 2013-2017 Pat Deegan
   [http://psychogenic.com/ | http://flyingcarsandstuff.com/]
   and distributed under the terms of their respective licenses.
   See http://devicedruid.com for details.


   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE
   PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE,
   YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR
   CORRECTION.

   Keep in mind that there is no warranty and you are solely
   responsible for the use of all these cool tools.

   Play safe, have fun.

*/


/* we need the SerialUI lib */
#include <SerialUI.h>
#include "SanctuarySettings.h"

/* our project specific types and functions are here */
#include "Sanctuary.h"



/*
   In addition to any custom globals you declared,
   here you have access to:

 * *** MySUI -- the SerialUI instance.
   Use it as you would the Serial device, e.g.
  	MySUI.println(F("Helloooo..."));


 * *** MyInputs -- a container for
   values submitted by users. Contents:

      MyInputs.WebCam (bool)
      MyInputs.SaltTanks (long int)
      MyInputs.FreshTanks (long int)
      MyInputs.Lighting (bool)
      MyInputs.Fountains (long int)
      MyInputs.Lamp (long int)



 * *** MyTracked -- a container for values tracked
   by druid and displayed to users in "state" pane.  Changes to:

      MyTracked.Counter (unsigned long)
      MyTracked.Office (float)
      MyTracked.Outter (unsigned long)
      MyTracked.LowLev (bool)
      MyTracked.TigerFood (unsigned long)

   will automatically be reported to the UI, on the next refresh/ping.


*/

// custom declarations

int SuperDuperCounter = 0;

FoodLevels Stock;

void notifyIfStockLow() {
  // we'll report on "low stock levels" using
  // this tracking variable
  if (Stock.runningLow()) {
    MyTracked.LowLev = true;
  } else {
    MyTracked.LowLev = false;
  }
}


void doMaintenanceStuff()
{
  // a little function to simulate using a sensor
  // and to give us some interesting tracked variable
  // output

  MyTracked.Counter++;
  // keep it reasonable
  if (MyTracked.Counter > 300)
  {
    MyTracked.Counter = 0;
  }

  MyTracked.Office = MyInputs.SaltTanks + 7 * sin(MyTracked.Counter / 10.0);

  MyTracked.Outter = 25 + random(3, 18);

  // dealing with our imaginary sensor
  SPI.transfer(0xde);
  SPI.transfer(0xad);




}



// "heartbeat" function, called periodically while connected
void CustomHeartbeatCode () {

  // will be called when a user is present.
  doMaintenanceStuff();

}

/* ********* callbacks and validation functions ********* */



/* *** Animals *** */
namespace Animals {


void doStockLevels() {

  /* Food types:
  	TIGER_FOOD = 0,
  	MONKEY_FOOD = 1,
  	REPTILE_FOOD = 2,
  	FISH_FOOD = 3
  */
  // let's prep some strings for output, with
  // same indices as the food types
  const char * anTypes[] = {
    "Tigers",
    "Monkeys",
    "Reptiles",
    "Fish",
    NULL
  };

  MySUI.println(F("Current Stock levels:"));

  uint8_t i = 0;
  while (anTypes[i] != NULL) {
    MySUI.print('\t');
    MySUI.print(anTypes[i]); // switch to next type, as we print.
    MySUI.print(": ");
    MySUI.println(Stock.level( (FoodType)i ) );
    i++;
  }

}

void doFeedAll() {
  Animals::Monkeys::doFeed();
  Animals::Fishes::doFeed();
  Animals::Tigers::doFeed();
  Animals::Reptiles::doFeed();

  MySUI.println(F("Everybody happy."));

}






/* *** Animals -> Tigers *** */
namespace Tigers {


void doEmptyLitter() {

  /* Empty Litter triggered */
  MySUI.println(F("Empty Litter triggered!"));

}

void doFeed() {
  Stock.decrement(TIGER_FOOD);
  MySUI.println(F("Tigers Fed"));

  MyTracked.TigerFood = Stock.level(TIGER_FOOD);
  notifyIfStockLow();


}




} /* namespace Tigers */





/* *** Animals -> Monkeys *** */
namespace Monkeys {


void doFeed() {
  Stock.decrement(MONKEY_FOOD);
  MySUI.println(F("Monkeys Fed"));
  notifyIfStockLow();

}

void WebCamChanged() {

  /* Web Cam value was modified.
    It is a bool accessible in MyInputs.WebCam
  */
  MySUI.print(F("Web Cam is now:"));
  MySUI.println(MyInputs.WebCam ? F("ON") : F("OFF"));

}




} /* namespace Monkeys */





/* *** Animals -> Fishes *** */
namespace Fishes {


void doFeed() {
  Stock.decrement(FISH_FOOD);
  MySUI.println(F("Fishies  Fed"));
  notifyIfStockLow();


}

void SaltTanksChanged() {

  /* Salt Tanks value was modified.
    It is a long int accessible in MyInputs.SaltTanks
  */
  MySUI.print(F("Salt Tanks is now:"));
  MySUI.println(MyInputs.SaltTanks);

}

bool FreshTanksIsOk(long int& newVal) {
  if (newVal < 22 || newVal > 28) {
    return false;
  }

  return true;

}

void FreshTanksChanged() {

  /* Fresh Tanks value was modified.
    It is a long int accessible in MyInputs.FreshTanks
  */
  MySUI.print(F("Fresh Tanks is now:"));
  MySUI.println(MyInputs.FreshTanks);

}





} /* namespace Fishes */





/* *** Animals -> Reptiles *** */
namespace Reptiles {


void doFeed() {
  Stock.decrement(REPTILE_FOOD);
  MySUI.println(F("Reptiles Fed"));

  notifyIfStockLow();

}

void LightingChanged() {

  /* Lighting value was modified.
    It is a bool accessible in MyInputs.Lighting
  */
  MySUI.print(F("Lighting is now:"));
  MySUI.println(MyInputs.Lighting ? F("ON") : F("OFF"));

}

void FountainsChanged() {

  MySUI.print(F("Fountain is: "));

  MySUI.println(
    ((SUI::MenuItem::Request::OptionsList*)
     MySUI.currentMenuItem())->optionBySelection(MyInputs.Fountains));
}

void LampChanged() {
  MySUI.print(F("Heat Lamp now: "));

  switch (MyInputs.Lamp) {
    case 1:
      MySUI.println(F("so low"));
      break;
    case 2:
      MySUI.println(F("nice"));
      break;
    case 3:
      MySUI.println(F("toasty"));
      break;
    default:
      // should never happen
      MySUI.println(F("what's this??? "));
      break;
  }

}


} /* namespace Reptiles */
} /* namespace Animals */




