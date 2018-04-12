#include "auduino.h"
#include <Arduino.h>

const unsigned short baseFrequencies[] PROGMEM = {
  440, 415, 466,      // A
  494, 466, 523,      // B
  262, 247, 277,      // C
  294, 277, 311,      // D
  330, 311, 349,      // E
  349, 330, 370,      // F
  392, 370, 415       // G
};


Auduino::Auduino(unsigned int pinAudio) {
  this->pinAudio = pinAudio;
  pinMode(this->pinAudio, OUTPUT);
  this->stop();
}


void Auduino::play(char *tune) {
  this->tune = tune;
  this->tempo = 120.0f;
  this->octave = 4;
  this->noteDuration = 4.0f;
  this->nextNoteTime = millis();
  this->tuneIndex = 0;
}


void Auduino::stop() {
  this->tune = "";
  this->tuneIndex = 0;
}


void Auduino::update() {
  if (millis() >= this->nextNoteTime && this->tune[this->tuneIndex] != 0) {
    this->parseTune();
  }
}


bool Auduino::isPlaying() {
  return this->tune[this->tuneIndex] != 0;
}


void Auduino::parseTune() {
  while (this->tune[this->tuneIndex] != 0) {
    
    // Decrease octave if greater than 1.
    if (this->tune[this->tuneIndex] == '<' && this->octave > 1) {
      this->octave --;
      
    // Increase octave if less than 7.
    } else if (this->tune[this->tuneIndex] == '>' && this->octave < 7) {
      this->octave ++;
      
    // Set octave.
    } else if (this->tune[this->tuneIndex] == 'O' && this->tune[this->tuneIndex + 1] >= '1' && this->tune[this->tuneIndex + 1] <= '7') {
      this->octave = this->tune[++ this->tuneIndex] - 48;
      
    // Set default note duration.
    } else if (this->tune[this->tuneIndex] == 'L') {
      this->tuneIndex ++;
      float duration = this->parseNumber();
      if (duration != 0) this->noteDuration = duration;
      
    // Set song tempo.
    } else if (this->tune[this->tuneIndex] == 'T') {
      this->tuneIndex ++;
      this->tempo = this->parseNumber();
      
    // Pause.
    } else if (this->tune[this->tuneIndex] == 'P') {
      this->tuneIndex ++;
      this->nextNoteTime = millis() + this->parseDuration();
      break;
      
    // Next character is a note A..G so play it.
    } else if (this->tune[this->tuneIndex] >= 'A' && this->tune[this->tuneIndex] <= 'G') {
      this->parseNote();
      break;
    }
    
    this->tuneIndex ++;
  }
}


void Auduino::parseNote() {
  // Get index of note in base frequenct table.
  char note = (this->tune[this->tuneIndex ++] - 65) * 3;
  if (this->tune[this->tuneIndex] == '-') {
    note ++;
    this->tuneIndex ++;
  } else if (this->tune[this->tuneIndex] == '+') {
    note += 2;
    this->tuneIndex ++;
  }
  
  // Calculate note frequency.
  unsigned int noteFrequency;
  if (this->octave < 4) {
    noteFrequency = pgm_read_word_near(baseFrequencies + note) >> (4 - this->octave);
  } else if (this->octave > 4) {
    noteFrequency = pgm_read_word_near(baseFrequencies + note) << (this->octave - 4);
  } else {
    noteFrequency = pgm_read_word_near(baseFrequencies + note);
  }
  
  // Get duration, set delay and play note.
  float duration = this->parseDuration();
  this->nextNoteTime = millis() + duration;
  tone(this->pinAudio, noteFrequency, duration * 0.85);
}


float Auduino::parseDuration() {
  // Get custom note duration or use default note duration.
  float duration = this->parseNumber();
  if (duration == 0) {
    duration = 4.0f / this->noteDuration;
  } else {
    duration = 4.0f / duration;
  }
  
  // See whether we need to double the duration
  if (this->tune[this->tuneIndex] == '.') {
    duration *= 1.5f;
    this->tuneIndex ++;
  }
  
  // Calculate note duration in ms.
  duration = (60.0f / this->tempo) * duration * 1000;
  return duration;
}


float Auduino::parseNumber() {
  float number = 0.0f;
  if (this->tune[this->tuneIndex] != 0 && this->tune[this->tuneIndex] >= '0' && this->tune[this->tuneIndex] <= '9') {
    while (this->tune[this->tuneIndex] != 0 && this->tune[this->tuneIndex] >= '0' && this->tune[this->tuneIndex] <= '9') {
      number = number * 10 + (this->tune[this->tuneIndex ++] - 48);
    }
    this->tuneIndex --;
  }
  return number;
}
