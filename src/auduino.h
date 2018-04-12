class Auduino {
	public:
		Auduino(unsigned int pinAudio);
		void play(char *tune);
		void stop();
		void update();
		bool isPlaying();

	private:
		void parseTune();
		void parseNote();
		float parseDuration();
		float parseNumber();

		unsigned int pinAudio;
		char *tune;
		float tempo;
		unsigned int octave;
		float noteDuration;
		unsigned long nextNoteTime;
		unsigned int tuneIndex;
};
