#include <SPI.h>
#include <SdFat.h>
#include <digitalWriteFast.h>
#include <font.h>
#include <LM15SGFNZ07.h>

#include "auduino.h"
#include "tileset.h"

#define ROW_SIZE        20
#define ROW_BELOW       20
#define ROW_BELOW_LEFT  19
#define ROW_BELOW_RIGHT 21
#define ROW_ABOVE_LEFT  -21
#define ROW_ABOVE       -20
#define ROW_ABOVE_RIGHT -19
#define ROW_LEFT        -1
#define ROW_RIGHT       1

#define TILE_EMPTY      0
#define TILE_GRASS      1
#define TILE_ROCK       2
#define TILE_HEART      3
#define TILE_WALL       4
#define TILE_METAL      5
#define TILE_GRENADE    6
#define TILE_PLASMA     7
#define TILE_HEART_GLOW 8
#define TILE_BALLOON    12
#define TILE_MOVE_L     16
#define TILE_MOVE_R     24
#define TILE_DOOR       32
#define TILE_PLASMA     36
#define TILE_DWARF      40
#define TILE_EXPLOSION  48
#define TILE_NUMBER     55

#define MARKER_UPDATED    0x8000
#define MARKER_REPAINT    0x4000
#define MARKER_FALLING    0x2000
#define MARKER_EXPLOSION  0x1000
#define MARKER_NOANIM     0x0800
#define FILTER_MARKERS    0xFF00
#define FILTER_TILE       0x00FF

char music[] = "T200L16O4P32EP32EP32>E<P32EP32EP32>E<P32EP32EP32>F<P32EP32EP32>D<P32EP32EP32>EP64";
char sndExplosion[] = "T250L64O4BGEC<FEC<GDC<BAEC";
char sndRockLand[]  = "T250L64O3BAGF";
char sndHeartLand[] = "T180L64O4G>G";
char sndHeart[]     = "T180O5G32L64B>CEF>G";
char sndGrassLand[] = "T180L64O3A";
char sndPipe[]      = "T250L32O6GEC<AFD<BG16";
char sndDoor[]      = "T250L64O2GAGABABA>GAGABABA>GAGABABA";
char sndGoal[]      = "T160O5L16CD#FG#P16F16L32G#A#G#A#G#A#G#A#G#A#G#A#G#A#G#A#";
unsigned short tileMap[240];
unsigned short scroll = 0;
unsigned short frame = 0;
unsigned short countDown = 0;
unsigned short hearts = 0;
unsigned short playerPosition = 0;
uint8_t levelNumber = 1;
bool playerDead = false;
bool playerGoal = false;
bool playerLeft = false;
bool scrolling = false;
bool showTitle = true;


struct JoystickState {
	bool up;
	bool down;
	bool left;
	bool right;
	bool buttonA;
	bool buttonB;
	bool buttonC;
	bool buttonD;
};


LM15SGFNZ07 lcd(6, 5, 4);
Auduino auduino(7);
SdFat SD;
SdFile file;
JoystickState Joystick;


void setup() {
	lcd.init();
	lcd.setContrast(32);
	pinMode(A0, INPUT_PULLUP);
	pinMode(A1, INPUT_PULLUP);
	pinMode(A2, INPUT_PULLUP);

	title();
}


void loop() {
	const long t = millis();
	frame ++;

	if (showTitle) {
		if (analogRead(2) < 512) {
			showTitle = false;
			lcd.fillRect(0, 0, 101, 80, 0x000);
			lcd.fillRect(0, 72, 101, 8, 0x404);
			lcd.fillRect(0, 73, 101, 6, 0x606);
			lcd.fillRect(0, 74, 101, 4, 0x909);
			lcd.fillRect(0, 75, 101, 2, 0xD0D);
			lcd.drawBitmap(10, 72, 8, 8, tileset[TILE_HEART]);
			startLevel(levelNumber);
		}

		while((millis() - t) < 100) {
			auduino.update();
			if (!auduino.isPlaying()) auduino.play(music);
			delay(1);
		}
	} else {
		drawLevel();
		updateMap();

		if (!playerDead && !playerGoal) {
			updatePlayer();
		} else {
			countDown --;
			if (countDown == 0) {
				if (playerGoal) {
					levelNumber ++;
				}
				startLevel(levelNumber);
			}
		}

		while((millis() - t) < 100) {
			auduino.update();
			delay(1);
		}
	}
}


void title() {
	if (!SD.begin(3)) {
		lcd.clear(0xF00);
		while(true);
	}

	loadBmp("/heart/credits.bmp", 0, 0);
	delay(2000);
	loadBmp("/heart/hl_title.bmp", 0, 0);
}


void startLevel(uint8_t levelNumber) {
	const char lookup[56] = " .@$#%&     0   <       >       !   =   *              ";
	file.open("/heart/levels.hl");

	uint8_t data;
	uint8_t skipCount = 0;
	while (skipCount < levelNumber) {
		data = file.read();
		if (data == '{') {
			skipCount ++;
		}
	}

	hearts = 0;
	uint8_t i = 0;
	while (i < 240) {
		data = file.read();
		if (data == 0x0A) {
			while (i % 20 != 0) {
				tileMap[i] = TILE_EMPTY | MARKER_REPAINT;
				i ++;
			}
		} else {
			for (int j = 0; j < 48; j ++) {
				if (lookup[j] == data) {
					if (j == TILE_HEART) hearts ++;
					if (j == TILE_DWARF) playerPosition = i;
					tileMap[i] = j | MARKER_REPAINT;
					i ++;
					break;
				}
			}
		}
	}

	file.close();

	unsigned char col = 0;
	if (playerPosition % 20 > 6) {
		col = min((playerPosition % 20) - 6, 8);
	}
	unsigned char row = 0;
	if (playerPosition / 20 > 5) {
		row = min((playerPosition / 20) - 5, 3);
	}
	scroll = row * 20 + col;
	playerGoal = false;
	playerDead = false;
	drawStatus();
}


void drawLevel() {
	unsigned short mapPos = scroll;
	for (byte i = 0; i < 9; i ++) {
		for (byte j = 0; j < 12; j ++) {
			if (scrolling || tileMap[mapPos + j] & MARKER_REPAINT) {
				tileMap[mapPos + j] &= (~MARKER_REPAINT);
				lcd.drawBitmap(2 + j * 8, i * 8, 8, 8, tileset[getTileAt(mapPos + j)]);
			}
		}
		mapPos += ROW_BELOW;
	}

	scrolling = false;
}


void updatePlayer() {
	char tile = getTileAt(playerPosition);

	if (tile >= TILE_MOVE_R + 4 && tile <= TILE_MOVE_R + 7) {
		setTile(playerPosition, tile - 4);
		playerPosition ++;
		tile = getTileAt(playerPosition);
		if (tile == TILE_EMPTY || tile == TILE_GRASS) {
			setTile(playerPosition, TILE_DWARF);
		} else if (tile == TILE_HEART || (tile >= TILE_HEART_GLOW && tile < TILE_HEART_GLOW + 4)) {
			setTile(playerPosition, TILE_DWARF);
			grabHeart();
		} else {
			setTile(playerPosition, tile + 4);
		}
	} else if (tile >= TILE_MOVE_L + 4 && tile <= TILE_MOVE_L + 7) {
		setTile(playerPosition, tile - 4);
		playerPosition --;
		tile = getTileAt(playerPosition);
		if (tile == TILE_EMPTY || tile == TILE_GRASS) {
			setTile(playerPosition, TILE_DWARF + 3);
		} else if (tile == TILE_HEART || (tile >= TILE_HEART_GLOW && tile < TILE_HEART_GLOW + 4)) {
			setTile(playerPosition, TILE_DWARF + 3);
			grabHeart();
		} else {
			setTile(playerPosition, tile + 4);
		}
	} else {
		getKeyStates();
		if (Joystick.up) {
			movePlayer(ROW_ABOVE);
		} else if (Joystick.down) {
			movePlayer(ROW_BELOW);
		} else if (Joystick.left && playerPosition % 20 > 0) {
			playerLeft = true;
			movePlayer(ROW_LEFT);
		} else if (Joystick.right && playerPosition % 20 < 19) {
			playerLeft = false;
			movePlayer(ROW_RIGHT);
		} else if (Joystick.buttonB) {
			startLevel(++ levelNumber);
		} else if (Joystick.buttonC) {
			startLevel(max(1, -- levelNumber));
		} else if (Joystick.buttonD) {
			killDwarf();
		}
	}

	unsigned char col = 0;
	if (playerPosition % 20 > 6) {
		col = min((playerPosition % 20) - 6, 8);
	}
	unsigned char row = 0;
	if (playerPosition / 20 > 5) {
		row = min((playerPosition / 20) - 5, 3);
	}
	if (scroll != row * 20 + col) {
		scroll = row * 20 + col;
		scrolling = true;
	}
}


void movePlayer(unsigned short direction) {
	char tile = getTileAt(playerPosition + direction);
	switch (tile) {
		case TILE_HEART:
		case TILE_HEART_GLOW ... TILE_HEART_GLOW + 3:
			grabHeart();

		case TILE_EMPTY:
		case TILE_GRASS:
			setTile(playerPosition, TILE_EMPTY);
			playerPosition += direction;
			setTile(playerPosition, playerLeft ? TILE_DWARF + 2 : TILE_DWARF + 1);
			break;

		case TILE_ROCK:
		case TILE_GRENADE:
		case TILE_BALLOON ... TILE_BALLOON + 3:
			if (frame % 2 == 0 && (direction == ROW_LEFT || direction == ROW_RIGHT) && getTileAt(playerPosition + (direction * 2)) == TILE_EMPTY) {
				setTile(playerPosition + (direction * 2), tile);
				setTile(playerPosition, TILE_EMPTY);
				playerPosition += direction;
				setTile(playerPosition, playerLeft ? TILE_DWARF + 2 : TILE_DWARF + 1);
			}
			break;

		case TILE_MOVE_R ... TILE_MOVE_R + 3:
			if (direction == ROW_RIGHT) enterPipe(tile, direction);
			break;

		case TILE_MOVE_L ... TILE_MOVE_L + 3:
			if (direction == ROW_LEFT) enterPipe(tile, direction);
			break;

		case TILE_DOOR + 3:
			setTile(playerPosition, TILE_EMPTY);
			playerPosition += direction;
			setTile(playerPosition, TILE_DWARF + 4);
			auduino.play(sndGoal);
			playerGoal = true;
			countDown = 20;
			break;
	}
}


void enterPipe(char tile, unsigned short direction) {
	unsigned short i = playerPosition + direction;
	char endTile;
	while((endTile = getTileAt(i)) == tile) {
		i += direction;
	}

	if (endTile == TILE_EMPTY || endTile == TILE_GRASS || endTile == TILE_HEART || (endTile >= TILE_HEART_GLOW && endTile < TILE_HEART_GLOW + 4)); {
		auduino.play(sndPipe);
		setTile(playerPosition, TILE_EMPTY);
		playerPosition += direction;
		setTile(playerPosition, getTileAt(playerPosition) + 4);
	}
}


void updateMap() {
	unsigned short i;

	for (i = 0; i < 240; i ++) {
		if (!(tileMap[i] & MARKER_UPDATED)) {
			const char tile = getTileAt(i);
			
			if (tileMap[i] & MARKER_EXPLOSION || tile == TILE_GRENADE + 1) {
				updateExplodingTile(i);
				tileMap[i] |= MARKER_NOANIM;
			} else if (tile >= TILE_BALLOON && tile <= TILE_BALLOON + 3) {
				updateBalloon(tileMap[i], i);
			} else if (isDropableTile(i)) {
				updateFallingTile(tileMap[i], i);
			} else if (tile == TILE_DOOR && hearts == 0) {
				auduino.play(sndDoor);
				setTile(i, (TILE_DOOR + 1) | MARKER_NOANIM);
			}
		}
	}

	for (i = 0; i < 240; i ++) {
		if (!(tileMap[i] & MARKER_NOANIM)) {
			updateAnimations(tileMap[i], i);
		}
		tileMap[i] &= ~(MARKER_NOANIM | MARKER_UPDATED);
	}
}


void updateFallingTile(unsigned short tile, unsigned short i) {
	// Tile below is empty so fall down.
	if (getTileAt(i + ROW_BELOW) == TILE_EMPTY) {
		setTile(i, TILE_EMPTY);
		setTile(i + ROW_BELOW, tile | MARKER_FALLING);

	// Tile is sitting on top of an unstable tile so slide left or right.
	} else if (!(tile & MARKER_FALLING) && isUnstableTile(i + ROW_BELOW)) {
		if (getTileAt(i + ROW_LEFT) == TILE_EMPTY && getTileAt(i + ROW_BELOW_LEFT) == TILE_EMPTY) {
			if (!isDropableTile(i + ROW_ABOVE_LEFT)) {
				setTile(i, TILE_EMPTY);
				setTile(i + ROW_LEFT, tile | MARKER_FALLING);
			}

		} else if (getTileAt(i + ROW_RIGHT) == TILE_EMPTY && getTileAt(i + ROW_BELOW_RIGHT) == TILE_EMPTY) {
			if (!isDropableTile(i + ROW_ABOVE_RIGHT)) {
				setTile(i, TILE_EMPTY);
				setTile(i + ROW_RIGHT, tile | MARKER_FALLING);
			}
		}

	// Tile is landing on top of another tile.
	} else if (tile & MARKER_FALLING) {
		tileMap[i] ^= MARKER_FALLING;
		unsigned short tileBelow = getTileAt(i + ROW_BELOW);

		switch(tile & FILTER_TILE) {
			case TILE_GRENADE:
				if (tileBelow != TILE_GRASS) {
					// Grenade lands something hard. Make it explode!
					if (tileBelow >= TILE_DWARF && tileBelow <= TILE_DWARF + 3) killDwarf();
					setTile(i, TILE_GRENADE + 1 | MARKER_EXPLOSION);
				} else {
					auduino.play(sndGrassLand);
				}
				break;

			case TILE_ROCK:
			case TILE_HEART:
			case TILE_HEART_GLOW ... TILE_HEART_GLOW + 3:
				if (tileBelow >= TILE_DWARF && tileBelow <= TILE_DWARF + 3) {
					killDwarf();
				} else if (tileBelow == TILE_GRASS) {
					auduino.play(sndGrassLand);
				} else {
					auduino.play(tile == TILE_ROCK ? sndRockLand : sndHeartLand);
				}
				break;
		}

		// If non grenade tile lands on a grenade then it explodes!
		if (getTileAt(i + ROW_BELOW) == TILE_GRENADE && (tile & FILTER_TILE) != TILE_GRENADE) {
			setTile(i + ROW_BELOW, TILE_GRENADE + 1);
		}
	}
}


void updateExplodingTile(unsigned short i) {
	uint8_t tile = getTileAt(i);

	if (tile == TILE_GRENADE + 1) {
		auduino.play(sndExplosion);
		setTile(i, TILE_EXPLOSION + 1);

		const int8_t deltas[4] = { ROW_ABOVE, ROW_LEFT, ROW_RIGHT, ROW_BELOW };
		for (uint8_t j = 0; j < 4; j ++) {
			if (isExplodable(i + deltas[j])) {
				tile = getTileAt(i + deltas[j]);
				if (tile == TILE_GRENADE) {
					setTile(i + deltas[j], TILE_GRENADE + 1); //| MARKER_EXPLOSION);
				} else if (tile >= TILE_DWARF && tile <= TILE_DWARF + 3) {
					killDwarf();
				} else {
					setTile(i + deltas[j], (TILE_EXPLOSION + (j >> 1)) | MARKER_NOANIM);
				}
			}
		}
	} else {
		setTile(i, TILE_GRENADE + 1);
	}
}


void updateBalloon(unsigned short tile, unsigned short i) {
	// Tile above is empty so move up.
	if (getTileAt(i + ROW_ABOVE) == TILE_EMPTY) {
		setTile(i + ROW_ABOVE, TILE_BALLOON + 1);
		setTile(i, TILE_EMPTY);

	// If tile above is dropable and tile above that is empty...
	} else if ((isDropableTile(i + ROW_ABOVE) || playerPosition == i + ROW_ABOVE) && getTileAt(i + ROW_ABOVE + ROW_ABOVE) == TILE_EMPTY) {
		// If falling marker not set move dropable and balloon up and set falling flag.
		if (!(tile & MARKER_FALLING)) {
			setTile(i + ROW_ABOVE + ROW_ABOVE, getTileAt(i + ROW_ABOVE));
			setTile(i + ROW_ABOVE, (TILE_BALLOON + 1) | MARKER_FALLING);
			setTile(i, TILE_EMPTY);
			if (playerPosition == i + ROW_ABOVE) playerPosition += ROW_ABOVE;
		// If falling marker set do not move, just clear the flag.
		} else {
			tileMap[i] ^= MARKER_FALLING;
		}

	// If two tiles above are droppable and tile below is empty then drop balloon.
	} else if (isDropableTile(i + ROW_ABOVE) && isDropableTile(i + ROW_ABOVE + ROW_ABOVE) && getTileAt(i + ROW_BELOW) == TILE_EMPTY) {
		setTile(i, TILE_EMPTY);
		setTile(i + ROW_BELOW, (TILE_BALLOON + 1) | MARKER_FALLING);

	// If falling marker set, but unable to push up just clear the falg.
	} else if (tile & MARKER_FALLING) {
		tileMap[i] ^= MARKER_FALLING;
	}
}


void updateAnimations(unsigned short tile, unsigned short i) {
	switch (tile & FILTER_TILE) {
		// Start random twinkling hearts.
		case TILE_HEART:
			if (!random(8)) {
				setTile(i, random(TILE_HEART_GLOW, TILE_HEART_GLOW + 4) | (tile & FILTER_MARKERS));
			}
			break;

		// Reset heart twinkle.
		case TILE_HEART_GLOW ... TILE_HEART_GLOW + 3:
			setTile(i, TILE_HEART | (tile & FILTER_MARKERS));
			break;

		// Tiles that have forward animation.
		case TILE_BALLOON   ... TILE_BALLOON + 2:
		case TILE_MOVE_L    ... TILE_MOVE_L + 2:
		case TILE_MOVE_R    ... TILE_MOVE_R + 2:
		case TILE_DOOR + 1  ... TILE_DOOR + 2:
		case TILE_EXPLOSION ... TILE_EXPLOSION + 5:
		case TILE_PLASMA    ... TILE_PLASMA + 2:
			setTile(i, tile + 1);
			break;

		// Loop tile animations.
		case TILE_BALLOON + 3:
		case TILE_MOVE_L + 3:
		case TILE_MOVE_R + 3:
		case TILE_PLASMA + 3:
			setTile(i, tile - 3);
			break;

		// Clear explosion.
		case TILE_EXPLOSION + 6:
			setTile(i, TILE_EMPTY);
			break;

		// Dwarf walk right.
		case TILE_DWARF + 1:
			setTile(i, TILE_DWARF);
			break;

		// Dwarf walk left.
		case TILE_DWARF + 2:
			setTile(i, TILE_DWARF + 3);
			break;

		// Dwarf jumping.
		case TILE_DWARF + 4:
			setTile(i, TILE_DWARF + 5);
			break;
		case TILE_DWARF + 5:
			setTile(i, TILE_DWARF + 4);
			break;
	}
}


void grabHeart() {
	hearts --;
	auduino.play(sndHeart);
	drawStatus();
}


void killDwarf() {
	if (!playerGoal) {
		setTile(playerPosition, TILE_EXPLOSION);
		auduino.play(sndExplosion);
		playerDead = true;
		countDown = 30;
	}
}


bool isUnstableTile(unsigned short i) {
	const char tile = getTileAt(i);
	return tile == TILE_ROCK ||
		tile == TILE_HEART ||
		tile == TILE_WALL ||
		tile == TILE_GRENADE ||
		(tile >= TILE_HEART_GLOW && tile < TILE_HEART_GLOW + 4) ||
		(tile >= TILE_PLASMA && tile < TILE_PLASMA + 4) ||
		(tile >= TILE_EXPLOSION && tile < TILE_EXPLOSION + 7);
}


bool isDropableTile(unsigned short i) {
	const char tile = getTileAt(i);
	return tile == TILE_ROCK ||
		tile == TILE_HEART ||
		tile == TILE_GRENADE ||
		(tile >= TILE_HEART_GLOW && tile < TILE_HEART_GLOW + 4);
}


bool isExplodable(unsigned short i) {
	const char tile = getTileAt(i);
	return tile != TILE_METAL && !(tile >= TILE_MOVE_L && tile <= TILE_MOVE_R + 7) && tile != TILE_GRENADE + 1;
}


char getTileAt(unsigned short i) {
	return i < 240 ? (tileMap[i] & FILTER_TILE) : TILE_METAL;
}


void setTile(unsigned short i, unsigned short tile) {
	if (i < 240) {
		tileMap[i] = tile | MARKER_UPDATED | MARKER_REPAINT;
	}
}


void drawStatus() {
	lcd.drawBitmap(18, 72, 8, 8, tileset[TILE_NUMBER + hearts / 10]);
	lcd.drawBitmap(26, 72, 8, 8, tileset[TILE_NUMBER + hearts % 10]);
	lcd.drawBitmap(74, 72, 8, 8, tileset[TILE_NUMBER + levelNumber / 10]);
	lcd.drawBitmap(82, 72, 8, 8, tileset[TILE_NUMBER + levelNumber % 10]);
}


void loadBmp(char *fileName, uint8_t x, uint8_t y) {
	file.open(fileName);

	file.seekSet(0x0A);
	uint16_t bmpOffset = file.read() + (file.read() << 8);
	file.seekSet(0x12);
	uint8_t width = file.read();
	uint8_t padding = width % 4;
	file.seekSet(0x16);
	uint8_t height = file.read();
	file.seekSet(bmpOffset);

	for (int8_t i = y + height - 1; i >= y; i --) {
		digitalWriteFast(6, LOW);
		lcd.setWindow(x, i, width, 1);
		digitalWriteFast(6, HIGH);

		for (uint8_t j = 0; j < width; j ++) {
			uint8_t gb = (file.read() & 0xF0) >> 4;
			gb += file.read() & 0xF0;
			uint8_t r = (file.read() & 0xF0) >> 4;

			digitalWriteFast(6, LOW);
			SPI.transfer(r);
			SPI.transfer(gb);
			digitalWriteFast(6, HIGH);
		}

		for (uint8_t j = 0; j < padding; j ++) {
			file.read();
		}
	}

	file.close();
}


void getKeyStates() {
	unsigned short joystickX = analogRead(0);
	unsigned short joystickY = analogRead(1);
	unsigned short buttons   = analogRead(2);

	Joystick.up    = joystickY > 11  && joystickY < 19;
	Joystick.down  = joystickY > 143 && joystickY < 151;
	Joystick.left  = joystickX > 143 && joystickX < 151;
	Joystick.right = joystickX > 11  && joystickX < 19;

	Joystick.buttonA = buttons > 320 && buttons < 328;
	Joystick.buttonB = buttons > 242 && buttons < 250;
	Joystick.buttonC = buttons > 11  && buttons < 19;
	Joystick.buttonD = buttons > 143 && buttons < 151;
}

