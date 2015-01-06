
/*******************************************************************************

 Bare Conductive Touch MP3 player
 ------------------------------
 
 Touch_MP3.ino - touch triggered MP3 playback
 
 Based on code by Jim Lindblom and plenty of inspiration from the Freescale 
 Semiconductor datasheets and application notes.
 
 Bare Conductive code written by Stefan Dzisiewski-Smith and Peter Krige.
 
 This work is licensed under a Creative Commons Attribution-ShareAlike 3.0 
 Unported License (CC BY-SA 3.0) http://creativecommons.org/licenses/by-sa/3.0/
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

*******************************************************************************/

// touch includes
#include <MPR121.h>
#include <Wire.h>
#define MPR121_ADDR 0x5C
#define MPR121_INT 4

// mp3 includes
#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h> 
#include <SFEMP3Shield.h>

// mp3 variables
SFEMP3Shield MP3player;
byte result;
int lastPlayed = 0;

// timing
#define TARGET_LOOP_TIME 41640   // 694 (1/60 seconds) / 24 samples = 694 microseconds per sample
//#define DEBUG_TIMING
int loopTime = 0;
int prevTime = 0;
int loopCounter = 0;


// touch behaviour definitions
#define firstPin 0
#define lastPin 11

// sd card instantiation
SdFat sd;

// define LED_BUILTIN for older versions of Arduino
#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

/////////////////////////
// aac
#define NUM_LETTERS         5
#define LETTERS_OFFSET      7
#define DEBUG_LETTERS

boolean pins[12];

// STRUCT LETRAS ////////
typedef struct {
  boolean pressed;
  boolean passedToKeyboard;
} 
Letters;

Letters letters[NUM_LETTERS];

int letterPositions[5] = {
  'a',        // 0
  'e',        // 1
  'i',        // 2
  'o',        // 3
  'u',        // 4
};

int shiftLetterPositions[5] = {
  'A',        // 0
  'E',        // 1
  'I',        // 2
  'O',        // 3
  'U',        // 4
};

int WELCOME_TRACK = 6;
int SHIFT_TRACK = 7;
int letterTracks[5] = {
  1,
  2,
  3,
  4,
  5,
};

boolean shiftPressed = false;


///////////////////////////
// FUNCTIONS //////////////
///////////////////////////
void addDelay();


void setup(){  
  Serial.begin(57600);
  
  pinMode(LED_BUILTIN, OUTPUT);
   
  //while (!Serial) ; {} //uncomment when using the serial monitor 
  Serial.println("Bare Conductive Touch MP3 player");

  if(!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();

  if(!MPR121.begin(MPR121_ADDR)) Serial.println("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);

  if(!MPR121.begin(MPR121_ADDR)) {
    Serial.println("error setting up MPR121");
  } else {
    Serial.println("MPR121 ok");
  }
  MPR121.setInterruptPin(MPR121_INT);

  // this is the touch threshold - setting it low makes it more like a proximity trigger
  // default value is 40 for touch
  MPR121.setTouchThreshold(11, 4); //12
  MPR121.setTouchThreshold(10, 4); //12
  MPR121.setTouchThreshold(9, 4); //12
  MPR121.setTouchThreshold(8, 4); //20
  MPR121.setTouchThreshold(7, 4); //
  MPR121.setTouchThreshold(6, 4); //
  MPR121.setTouchThreshold(5, 12); //
  MPR121.setTouchThreshold(4, 12); //
  MPR121.setTouchThreshold(3, 12); //
  MPR121.setTouchThreshold(2, 12); //12
  MPR121.setTouchThreshold(1, 12); //12
  MPR121.setTouchThreshold(0, 12); //12
  
  // this is the release threshold - must ALWAYS be smaller than the touch threshold
  // default value is 20 for touch
  MPR121.setReleaseThreshold(11, 3);  //10
  MPR121.setReleaseThreshold(10, 3);  //10
  MPR121.setReleaseThreshold(9, 3);  //10
  MPR121.setReleaseThreshold(8, 3);  //18
  MPR121.setReleaseThreshold(7, 3);  //
  MPR121.setReleaseThreshold(6, 3);  //
  MPR121.setReleaseThreshold(5, 10);  //
  MPR121.setReleaseThreshold(4, 10);  //
  MPR121.setReleaseThreshold(3, 10);  //
  MPR121.setReleaseThreshold(2, 10);  //10
  MPR121.setReleaseThreshold(1, 10);  //10
  MPR121.setReleaseThreshold(0, 10);  //10


  result = MP3player.begin();
  MP3player.setVolume(10,3); // MP3player.setVolume(10,10);
  
  MP3player.playTrack(WELCOME_TRACK);
 
  if(result != 0) {
    Serial.print("Error code: ");
    Serial.print(result);
    Serial.println(" when trying to start MP3 player");
   }
   
}


void releaseAfterDelayAndStopTrack(int i, int letter, boolean shiftPressed) {
  
  Serial.println("playTrackAndReleaseAfterDelay- begin ");
  delay(100);  // una decima de segundo
  releaseLetter(i, letter, shiftPressed);
  Serial.println("playTrackAndReleaseAfterDelay- end");

}

void releaseLetter(int i, int letter, boolean shiftPressed) {
#ifdef DEBUG_LETTERS
          Serial.print("release letter:");
          Serial.println(i);
#endif
          Keyboard.release(letter);
          letters[i].passedToKeyboard = false;

          // acciones al soltar una tecla
          digitalWrite(LED_BUILTIN, LOW);
          delay(700);  // 7 decimas segundo
          MP3player.stopTrack();
          if (shiftPressed) {
            MP3player.playTrack(SHIFT_TRACK);
          }

}

int playOrStopTrack(int track, int lastPlayed) {
  Serial.print("playOrStopTrack -track");
  Serial.print(track);
  Serial.print(" + lastplayed + ");
  Serial.println(track);

  int lastPlayedOutput;
  if(MP3player.isPlaying()){
     if(lastPlayed==track){
       // if we're already playing the requested track, stop it
       MP3player.stopTrack();
       Serial.print("stopping track ");
       Serial.println(lastPlayed);
     } else {
       // if we're already playing a different track, stop that 
       // one and play the newly requested one
       MP3player.stopTrack();
       MP3player.playTrack(track);
       Serial.print("playing track ");
       Serial.println(track);
                  
       // don't forget to update lastPlayed - without it we don't
       // have a history
       lastPlayedOutput = track;
     }
   } else {
     // if we're playing nothing, play the requested track 
     // and update lastplayed
     MP3player.playTrack(track);
     Serial.print("playing track ");
     Serial.println(track);
     lastPlayedOutput = track;
   }
   return lastPlayedOutput;
}

void loop(){
  readTouchInputs();
}


void readTouchInputs(){
  if(MPR121.touchStatusChanged()){
    
    MPR121.updateTouchData();

    // only make an action if we have one or fewer pins touched
    // ignore multiple touches
    
    if(MPR121.getNumTouches()<=1){
      int track;
      boolean playNow = false;
      
      for (int i=0; i < 12; i++){  // Check which electrodes were pressed
        if(MPR121.isNewTouch(i)){
        
            //pin i was just touched
            Serial.print("pin ");
            Serial.print(i);
            Serial.println(" was just touched");
            //digitalWrite(LED_BUILTIN, HIGH);
            if (i >= LETTERS_OFFSET) {
              // una de las vocales
              letters[i-LETTERS_OFFSET].pressed = true;
              letters[i-LETTERS_OFFSET].passedToKeyboard = false;
              track = letterTracks[i-LETTERS_OFFSET];
              playNow = true;
            } else {
              // en cualquier otro caso intrepreto shift
              shiftPressed = true;
              digitalWrite(LED_BUILTIN, HIGH);
            }
            
            
        }else{
          if(MPR121.isNewRelease(i)){
            Serial.print("pin ");
            Serial.print(i);
            Serial.println(" is no longer being touched");
            //digitalWrite(LED_BUILTIN, LOW);
         } 
        }
      }
      
      // presionar/liberar teclas
      for (int i=0; i<NUM_LETTERS; i++) {
        boolean pressLetter = false;
        boolean releaseLetter = false;
        if (letters[i].pressed && (! letters[i].passedToKeyboard)) {
          pressLetter = true;
        } else if (letters[i].passedToKeyboard && (! letters[i].pressed)){
          releaseLetter = true;
        }

        int letter = letterPositions[i];
        if (shiftPressed) {
          letter = shiftLetterPositions[i];
        }

        if (pressLetter) {
#ifdef DEBUG_LETTERS
          Serial.print("press letter:");
          Serial.println(i);
          //Serial.println(letter);
#endif
          Keyboard.press(letter);
          letters[i].passedToKeyboard = true;
          letters[i].pressed = false;
          if(playNow){
             lastPlayed = playOrStopTrack(track, lastPlayed);
             releaseAfterDelayAndStopTrack(i, letter, shiftPressed);
             shiftPressed = false;
          }     

        } else if (releaseLetter) {
          // do nothing
        }
      }

      
    }
  }
}
