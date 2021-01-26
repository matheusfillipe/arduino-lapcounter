enum SensorState {SLEFT, SENTERED, SHOLD};
volatile SensorState p1sState;
volatile SensorState p2sState;
volatile unsigned long p1Time;
volatile unsigned long p2Time;
volatile bool P1sensorHandled;
volatile bool P2sensorHandled;

void handleSensorInput(volatile int pin, volatile SensorState &psState, volatile unsigned long &pTime, volatile bool &sensorHandled){
    int value = digitalRead(pin);

    if(psState != SLEFT && (psState == SENTERED or psState == SHOLD) && value == HIGH){ // On sensor leave 
      psState = SLEFT;
      pTime = millis();
      sensorHandled = false;
    }
    else if(psState != SENTERED && psState == SLEFT && value == LOW){     // On sensor enter
      psState = SENTERED;
      pTime = millis();
      sensorHandled = false;
    }
}

void checkHold(int pin, volatile SensorState &psState, volatile unsigned long pTime, volatile bool &PsensorHandled){
   if(psState != SLEFT && digitalRead(pin) == HIGH){
      PsensorHandled = false;
      psState = SLEFT;  
   }
   if(psState == SENTERED && millis() - pTime >= SHOLD_DELAY){
      psState = SHOLD;
      PsensorHandled = false;
   }
}

void sensorHandleChange(void (&handler)(volatile SensorState), volatile bool &PsensorHandled, volatile SensorState &psState){
   debug("Cheking");
   handler(psState);
   PsensorHandled = true;
}


void p1PitStop();
void p2PitStop();

void onSensorChange(int last_state, volatile SensorState psState, PlayerState &P, bool &animating, reaction &matrix_print_reaction, bool  (*matrix_print)(int), LabelWidget &OS, LabelWidget &BLS, LabelWidget &FS){
   if(psState == last_state)
      return;
   last_state=psState;

   if(psState == SLEFT){
      debug("P"+String(P.num)+" LEFT");
      freePitStop(P);
      addLap(P, animating, matrix_print_reaction, matrix_print, OS, BLS, FS);
      P.justRefueled = false;

   }else if (psState == SENTERED){
      debug("P"+String(P.num)+" Entered");

   }else if (psState == SHOLD){
      debug("P"+String(P.num)+" Hold");
      if (options.autonomy > 0 && P.p_laps >= 0 && !P.isOnPit && !P.isOnPitDelay){ 
         if(P.num==1)
         p1PitStop();
         if(P.num==2)
         p2PitStop();
      }
   }
}

// PLayer specific functions

void p1PitStop(){
   debug("P1 pit stop Check");
   freePitStop(P1);
   P1.pitDelay = app.delay(PITSTOP_TIME, [](){
     P1.isOnPitDelay = true;
     if(digitalRead(LAPP1) == HIGH){
       freePitStop(P1);
       return;
     }
     debug("Pit stop delay");
     OS1.screen->clearBuffer();
     OS1.screen->clear();
     OS1.cursor.x = 5;
     iwrite(TEXT_RACE_PITSTOP, OS1);
     OS1.cursor.x = TEXT_X_OFFSET;
     OS1.screen->updateDisplayArea(0, 2, 16, 4);
     P1.pitstop = app.repeat(PITSTOP_TIME, [](){
         if(!P1.isOnPit){
           alertSound();
           ASemOff(SEMA[P1.num-ONE]);
           analogWrite(SEMA[P1.num-ONE][ONE], 255);
         }
         P1.isOnPit = true;
         if(digitalRead(LAPP1) == LOW){
           refuel(P1, FS1);
         } else{
           freePitStop(P1);
         }
     });
   });
}

void p2PitStop(){
   debug("P2 pit stop Check");
   freePitStop(P2);
   P2.pitDelay = app.delay(PITSTOP_TIME, [](){
     P2.isOnPitDelay = true;
     if(digitalRead(LAPP2) == HIGH){
       freePitStop(P2);
       return;
     }
     debug("Pit stop delay");
     OS2.screen->clear();
     OS2.screen->clearBuffer();
     OS2.cursor.x = 5;
     iwrite(TEXT_RACE_PITSTOP, OS2);
     OS2.cursor.x = TEXT_X_OFFSET;
     OS2.screen->updateDisplayArea(0, 2, 26, 4);
     P2.pitstop = app.repeat(PITSTOP_TIME, [](){
         if(!P2.isOnPit){
           alertSound();
           ASemOff(SEMA[P2.num-ONE]);
           analogWrite(SEMA[P2.num-ONE][ONE], 255);
         }
         P2.isOnPit = true;
         if(digitalRead(LAPP2) == LOW){
           refuel(P2, FS2);
         } else{
           freePitStop(P2);
         }
     });
   });
}


void onSensor1Change(volatile SensorState psState){
   static int last_state = -1;
   static bool animating = false;
   onSensorChange(last_state, psState, P1, animating, matrix1_print_reaction, matrix1_print, OS1, BLS1, FS1);
}


void onSensor2Change(volatile SensorState psState){
   static int last_state = -1;
   static bool animating = false;
   onSensorChange(last_state, psState, P2, animating, matrix2_print_reaction, matrix2_print, OS2, BLS2, FS2);
}

/*
 static bool animating = false;
 static bool wasEntered = false;
 static bool pitStopLaunched = false;
 int value = digitalRead(LAPP1);
 debug(String(value));

 if(wasEntered && value == HIGH){ // On sensor leave
   freePitStop(P1);
   addLap(P1, animating, matrix1_print_reaction, matrix1_print, OS1, BLS1, FS1);
   wasEntered = false;
   P1.justRefueled = false;
   pitStopLaunched = false;
 }else if (options.autonomy > 0 && !pitStopLaunched && P1.p_laps >= 0 && !P1.isOnPit && !P1.isOnPitDelay){ // On sensor enter
   pitStopLaunched = true;
   debug("Pit stop Check");
   freePitStop(P1);
   P1.pitDelay = app.delay(2*PITSTOP_TIME, [](){
     P1.isOnPitDelay = true;
     if(digitalRead(LAPP1) == HIGH){
       freePitStop(P1);
       return;
     }
     debug("Pit stop delay");
     OS1.screen->clear();
     OS1.cursor.x = 5;
     iwrite(TEXT_RACE_PITSTOP, OS1);
     OS1.cursor.x = TEXT_X_OFFSET;
     OS1.screen->updateDisplayArea(0, 2, 16, 4);
     P1.pitstop = app.repeat(PITSTOP_TIME, [](){
         if(!P1.isOnPit){
           alertSound();
           ASemOff(SEMA[P1.num-ONE]);
           analogWrite(SEMA[P1.num-ONE][ONE], 255);
         }
         P1.isOnPit = true;
         if(digitalRead(LAPP1) == LOW){
           refuel(P1, FS1);
         } else{
           freePitStop(P1);
         }
     });
   });
 }
 if (value == LOW)
   wasEntered = true;

*/
/*

  rman.add(app.onPinChange(LAPP1, [](){
    static bool animating = false;
    static bool wasEntered = false;
    static bool pitStopLaunched = false;
    int value = digitalRead(LAPP1);
    debug(String(value));

    if(wasEntered && value == HIGH){ // On sensor leave
      freePitStop(P1);
      addLap(P1, animating, matrix1_print_reaction, matrix1_print, OS1, BLS1, FS1);
      wasEntered = false;
      P1.justRefueled = false;
      pitStopLaunched = false;
    }else if (options.autonomy > 0 && !pitStopLaunched && P1.p_laps >= 0 && !P1.isOnPit && !P1.isOnPitDelay){ // On sensor enter
      pitStopLaunched = true;
      debug("Pit stop Check");
      freePitStop(P1);
      P1.pitDelay = app.delay(2*PITSTOP_TIME, [](){
        P1.isOnPitDelay = true;
        if(digitalRead(LAPP1) == HIGH){
          freePitStop(P1);
          return;
        }
        debug("Pit stop delay");
        OS1.screen->clear();
        OS1.cursor.x = 5;
        iwrite(TEXT_RACE_PITSTOP, OS1);
        OS1.cursor.x = TEXT_X_OFFSET;
        OS1.screen->updateDisplayArea(0, 2, 16, 4);
        P1.pitstop = app.repeat(PITSTOP_TIME, [](){
            if(!P1.isOnPit){
              alertSound();
              ASemOff(SEMA[P1.num-ONE]);
              analogWrite(SEMA[P1.num-ONE][ONE], 255);
            }
            P1.isOnPit = true;
            if(digitalRead(LAPP1) == LOW){
              refuel(P1, FS1);
            } else{
              freePitStop(P1);
            }
        });
      });
    }
    if (value == LOW)
      wasEntered = true;
  }));
  */

  //-----------------------------------------------------------------------------
/*
  rman.add(app.onPinChange(LAPP2, [](){
    static bool animating = false;
    static bool wasEntered = false;
    static bool pitStopLaunched = false;
    int value = digitalRead(LAPP2);
    debug(String(value));

    if(wasEntered && value == HIGH){ // On sensor leave
      freePitStop(P2);
      addLap(P2, animating, matrix2_print_reaction, matrix2_print, OS2, BLS2, FS2);
      wasEntered = false;
      P2.justRefueled = false;
      pitStopLaunched = false;
    }
    else if (options.autonomy > 0 && !pitStopLaunched && P2.p_laps >= 0 && !P2.isOnPit && !P2.isOnPitDelay){ // On sensor enter
      pitStopLaunched = true;
      debug("Pit stop Check");
      freePitStop(P2);
      P2.pitDelay = app.delay(2*PITSTOP_TIME, [](){
        P2.isOnPitDelay = true;
        if(digitalRead(LAPP2) == HIGH){
          freePitStop(P2);
          return;
        }
        debug("Pit stop delay");
        OS2.screen->clear();
        OS2.cursor.x = 5;
        iwrite(TEXT_RACE_PITSTOP, OS2);
        OS2.cursor.x = TEXT_X_OFFSET;
        OS2.screen->updateDisplayArea(0, 2, 26, 4);
        P2.pitstop = app.repeat(PITSTOP_TIME, [](){
            if(!P2.isOnPit){
              alertSound();
              ASemOff(SEMA[P2.num-ONE]);
              analogWrite(SEMA[P2.num-ONE][ONE], 255);
            }
            P2.isOnPit = true;
            if(digitalRead(LAPP2) == LOW){
              refuel(P2, FS2);
            } else{
              freePitStop(P2);
            }
        });
      });
    }
    if (value == LOW)
      wasEntered = true;
  }));
*/
