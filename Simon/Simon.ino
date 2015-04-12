// Wiring:
// Buzzer + : D9
// Buzzer - : GND
// Vibrator + : TX n/a
// Vibrator - : GND n/a
// LED strip + : D6
// LED strip - : GND
// LED stip in : D10
// TOUCH OUT : RX
// YELLOW : SCL
// GREEN : VBATT
// RED : D12
// BLUE : SDA

#include <CapacitiveSensor.h>
#include <Adafruit_NeoPixel.h>
#include "pitches.h"

//Power is D6 and Input is D10 on Flora
Adafruit_NeoPixel strip = Adafruit_NeoPixel(6,10,NEO_GRB + NEO_KHZ800);

#define MAXROUNDS 255

#define TOUCH_OUT 0 //RX on Flora

CapacitiveSensor cs_Green = CapacitiveSensor(TOUCH_OUT,4); //VBATT on Flora
CapacitiveSensor cs_Yellow = CapacitiveSensor(TOUCH_OUT,3); //SCL on Flora (orange)
CapacitiveSensor cs_Red = CapacitiveSensor(TOUCH_OUT,12); //D12 on Flora (purple)
CapacitiveSensor cs_Blue = CapacitiveSensor(TOUCH_OUT,2); //SDA on Flora

int motor = 1; //TX on Flora n/a
int buzzer = 9; //D9 on Flora

#define ERROR_COLOR strip.Color(255,0,0)
#define ERROR_SOUND NOTE_A2
#define PRETTY_COLOR strip.Color(255,0,255)

CapacitiveSensor sensors[] = {cs_Green, cs_Yellow, cs_Red, cs_Blue};
String buttons[] = {"GREEN", "YELLOW", "RED", "BLUE"};
uint32_t colors[] = {strip.Color(0,255,0), strip.Color(255,255,0), strip.Color(255,0,0), strip.Color(0,0,255)};
int sounds[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5};

//boolean currentPressed[] = {false, false, false, false};

#define THRESH 1

// notes in the melody:
int melody_start[] =
{
	NOTE_C5, NOTE_D5, NOTE_G5, NOTE_D5, NOTE_C5, NOTE_D5, NOTE_G5, NOTE_D5
};

int melody_end[] =
{
	NOTE_A3, NOTE_A3, NOTE_A3, NOTE_A3, NOTE_C4, NOTE_B3, NOTE_B3, NOTE_A3, NOTE_A3, NOTE_GS3, NOTE_A3
};


// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations_start[] =
{
	5, 5, 4, 5, 5, 9, 3, 6
};

int noteDurations_end[] =
{
	4, 5, 9, 4, 5, 9, 5, 9, 5, 9, 2
};


void setup()
{
	Serial.begin(9600);
	Serial.println("start");
	pinMode(motor, HIGH);
	strip.begin();
	strip.show();
	
}

void loop()
{
	startingSequence();
	delay(250);
	
	playGame();
	
	endingSequence();
	delay(1000);
	
}

void startingSequence(){
	// Create a vector of ids of lights that will light up with the music
	int light_sequence[8];
	uint32_t color_sequence[8];
	for (int i = 0; i < 8 ; i++) {
		int id = int(random(4));
		if (i != 0){
			while (id == light_sequence[i-1]) {
				id = int(random(4));
			}
		}
		light_sequence[i] = id;
		color_sequence[i] = strip.Color(int(random(255)),int(random(255)),int(random(255)));
	}
	playMelody(melody_start, noteDurations_start, 8, 2, light_sequence, color_sequence);
}

void endingSequence(){
	// Create a vector of ids of lights that will light up with the music
	int light_sequence[11];
	uint32_t color_sequence[11];
	for (int i = 0; i < 11 ; i++) {
		light_sequence[i] = i % 4;
		color_sequence[i] = ERROR_COLOR;
	}
	playMelody(melody_end, noteDurations_end, 11, 1, light_sequence, color_sequence);
}

int checkForUserInput()
{
	bool new_pressed = false;
	int result = -1;
	
	for (int i=0;i<4;i++) {
		
		long total = sensors[i].capacitiveSensor(50);
		Serial.print(buttons[i]);Serial.print(": ");Serial.println(total);
		
		// check if we are sensing that a finger is touching the pad
		// and that it wasnt already pressed
		if ((total > THRESH)){// && (! currentPressed[i])) {
			Serial.print("Key pressed #"); Serial.print(i);
			Serial.print(" ("); Serial.print(buttons[i]); Serial.println(")");
		//	currentPressed[i] = true;
		//	activateColor(i);
			if (!new_pressed) {
				result = i;
			} else {
				result = -1;
			}
		}
	/*	else if ((total <= THRESH) && (currentPressed[i])) {
		// key was released (no touch, and it was pressed before)
			Serial.print("Key released #"); Serial.print(i);
			Serial.print(" ("); Serial.print(buttons[i]); Serial.println(")");
			currentPressed[i] = false;
			deactivateColor(i);
			
		}*/
		
	}
	
	return result;
}

void playGame(){
	//Variable keeps track of rounds
  int round = 0; //sequence
  int turn = 0; // press
  
  //Declarelist of color IDs
  int colorIds[MAXROUNDS];
  
  boolean playing = true;
  
  while(playing && round < MAXROUNDS){
    //Add a random color to the colorID list
    colorIds[round] = generateColor();
    
    //Simon's turn. Show sequence of colors.
      displayColors(colorIds, round);
      
     //Reset turn;
      turn = 0;
    
    //Checks if each user input matches sequence
    while (turn <= round){
      Serial.println("It's your turn now...");
      //User's Turn. Add input
      int userInput = -1;
      while (userInput == -1) { // need to add a timeout here if the user does not input anything
      	userInput = checkForUserInput();	// the sound/feedback for touching is dealt with the touching instead of being a feedback from the game loop
      }
      Serial.print("You said ");Serial.println(buttons[userInput]);
      
      //Simon checks if you have what it takes.
      if (simonCheck(turn, userInput, colorIds) == true) {
      	activateColor(userInput);
        turn++;
        Serial.println("Success!");
        delay(200);
        deactivateAll();
        delay(200);
        continue;
      }
      else {
      	badFeedback();
        round = 0;
        playing = false;
        Serial.println("LOSER");
        break;
      }
    }
    deactivateAll();
    if (playing) {
    	round++;
    	delay(500);
    }
  }
}

//Method that prints array of colors
void displayColors(int colorIds[], int round){
  Serial.println("Simon says...");
  for (int i = 0; i <= round; i++) {
  	  Serial.println(buttons[colorIds[i]]);
	  activateColor(colorIds[i]);
	  delay(400);
	  deactivateColor(colorIds[i]);
	  delay(400);
  }
}

boolean simonCheck(int turn, int userInput, int colorIds[]) {
  if (userInput == colorIds[turn]) {
    return true;
  }
  else{
    return false;
  }
}

//Method that adds random color to list
int generateColor() {
  int colorId = int(random(4));
  return colorId;
}

void activateColor(int index) 
{
	strip.setPixelColor(index,colors[index]);
	strip.show();
	analogWrite(motor, 2000);  
	tone(buzzer, sounds[index],500);
}

void deactivateColor(int index)
{
	strip.setPixelColor(index,strip.Color(0,0,0));
	strip.show();
	analogWrite(motor, 0); 
}

void deactivateAll()
{
	for (int i=0;i<4;i++) {
		deactivateColor(i);
	}
}

void cheerfulFeedback(){
	
}

void badFeedback(){
	
	tone(buzzer, ERROR_SOUND,1000);
	for (int repeat=0;repeat<4;repeat++)
	{
		analogWrite(motor, 2000);  
		for (int i=0;i<4;i++) {
			strip.setPixelColor(i,ERROR_COLOR);
			strip.show();
		}
		delay(250);
		for (int i=0;i<4;i++) {
			deactivateColor(i);
		}
		delay(250);
	}
}

void playMelody(int melody[], int durations[], int nb, int repeat, int light_sequence[], uint32_t color_sequence[])
{
	for (int j = 0; j < repeat; j++)
	{
		for (int thisNote = 0; thisNote < nb; thisNote++)
		{

			// to calculate the note duration, take one second
			// divided by the note type.
			//e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
			int noteDuration = 1000 / durations[thisNote];
			tone(buzzer, melody[thisNote], noteDuration);

			// light up the corresponding light in the light sequence
			strip.setPixelColor(light_sequence[thisNote],color_sequence[thisNote]);
			strip.show();

			// to distinguish the notes, set a minimum time between them.
			// the note's duration + 30% seems to work well:
			int pauseBetweenNotes = noteDuration * 1.30;
			delay(pauseBetweenNotes);
			// stop the tone playing:
			noTone(buzzer);
			
			// turn off the light
			strip.setPixelColor(light_sequence[thisNote],strip.Color(0,0,0));
			strip.show();
		}
	}
}
