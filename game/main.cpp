#include "raylib.h"
#include "raymath.h"
#include "cmath"
#include "ctime"
#include "string"
#include "iostream"
#include "array"
#include "vector"
#include "algorithm"
#include "sstream"
#include "assets/explosion.h"
#include "assets/coin.h"
#include "assets/power.h"
#include "assets/level.h"
#include "base64.hpp"
#include "cstdio"

Sound explosion;
Sound coin;
Sound power;
Sound levelup;

float dt;
using namespace std;


vector<string> splitString(string input, char delimiter)
{
	vector<string> tokens;
	stringstream ss(input);
	string token;
	while (getline(ss, token, delimiter)) {
		tokens.push_back(token);
	}
	return tokens;
}


Color set_alpha(Color original, float alpha) {
	original.a = alpha;
	return original;
}

struct Spike {
	int i; // type
	float f = -25.0f; // frame
};

static array<Spike,10 > reSpikeInternal(bool isfirst = false) {
	// spike generation function
	array<Spike, 10> s;
	bool g = false;
	bool t = false;
	for (int i = 0; i < 10; i++) {
		int c = GetRandomValue(1, 4);
		if (c == 2) {
			if (!g) {
				g = true;
				s[i].i = c;
			}
			else {
				s[i].i = 3;
			}
		}
		else if (c == 4) {
			if (GetRandomValue(1, 2) == 2) {
				if (!t) {
					t = true;
					s[i].i = c;
				}
				else {
					s[i].i = 3;
				}
			}
		}
		else {
			s[i].i = c;
		}
	}
	if (isfirst) {
		s[0].i = 3;
	}
	return s;
}

struct SpikeArray {
	array<Spike, 10> left = reSpikeInternal(true);
	array<Spike, 10> right = reSpikeInternal();
};

class LocalPlayer {
public:
	float yvel = 0;
	Vector2 pos = { 10, 10 };
	Color color = {(unsigned char)GetRandomValue(0,255),(unsigned char)GetRandomValue(0,255),(unsigned char)GetRandomValue(0,255),255};
	int dir = 1;
	int level = 1;
	int highscore = 0;
	int jumps = 0;
	int frame = 0;
	int coins = 0;
	int menu = 3;
	int submenu = 0;
	int menutime = time(nullptr);
	float starttimer = 0.0f;
	bool colls = false;
	bool debug = false;
	int deaths = 0;
	int gravityMul = 1;
	float round = 0;
	Rectangle collider = { pos.x,pos.y,20,20 };
	SpikeArray spikes;

	LocalPlayer() {
		if (FileExists("ats2.savedata")) {
			vector<string> vec = splitString(base64::from_base64(LoadFileText("ats2.savedata")), '\n');
			if (vec.size() == 7) {
				highscore = atoi(vec.at(0).c_str());
				coins = atoi(vec.at(1).c_str());
				deaths = atoi(vec.at(2).c_str());
				color.r = atoi(vec.at(3).c_str());
				color.g = atoi(vec.at(4).c_str());
				color.b = atoi(vec.at(5).c_str());
				round = atof(vec.at(6).c_str());
			}
		}
	}

	void Move() {
		frame++;
		starttimer += dt*0.01;
		if (starttimer > 1) starttimer = 1;
		// move function
		yvel += 0.15f * dt;
		pos = Vector2Add(pos, { (dir * 5.0f) * dt,(yvel * gravityMul) * dt });
		if (pos.x > 480) {
			dir = -1;
			pos.x = 479;
			level++;
			PlaySound(levelup);
			spikes.left = reSpikeInternal();
		}
		if (pos.x < 0) {
			dir = 1;
			pos.x = 1;
			level++;
			PlaySound(levelup);
			spikes.right = reSpikeInternal();
		}
		if ((pos.y > 480 && gravityMul == 1) || (pos.y < 0 && gravityMul == -1)) {
			Die();
		}
		if ((pos.y < 0 && gravityMul==1) || (pos.y > 480 && gravityMul==-1)) {
			if (gravityMul == 1) pos.y = 0;
			if (gravityMul == -1) pos.y = 480;
			yvel = yvel * -2;
		}
		collider = { pos.x,pos.y,20,20 };
	}

	void Draw() {
		// draw function
		DrawRectangleRounded({ pos.x,pos.y,20,20 }, round,5, set_alpha(color, 255 * starttimer));
	}

	void Die() {
		PlaySound(explosion);
		setmenu(2);
		if (level > highscore) {
			highscore = level;
		}
		deaths++;
		jumps = 0;
	}

	void Reset(bool isReset = false) {
		// die function
		if (isReset) {
			if (level > highscore) {
				highscore = level;
			}
			deaths++;
			jumps = 0;
		}
		starttimer = 0;
		yvel = 0;
		pos = { 10, 10 };
		dir = 1;
		gravityMul = 1;
		level = 1;
		frame = 0;
		spikes.left = reSpikeInternal();
		spikes.right = reSpikeInternal();
		spikes.left[0].i = 3;
	}

	void setmenu(int index) {
		menu = index;
		submenu = 0;
		menutime = time(nullptr);
	}
};

int main() {
	Color rbowc[14] = { RED,BLUE,GREEN,YELLOW,PURPLE,GRAY,GOLD,ORANGE,PINK,MAROON,VIOLET,BROWN,MAGENTA,WHITE };
	InitWindow(500, 500, "Avoid That Spike 2");
	SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
	SetExitKey(0);
	InitAudioDevice();
	HideCursor();
	explosion = LoadSoundFromWave({EXPLOSION_FRAME_COUNT,EXPLOSION_SAMPLE_RATE,EXPLOSION_SAMPLE_SIZE,EXPLOSION_CHANNELS,EXPLOSION_DATA});
	coin = LoadSoundFromWave({COIN_FRAME_COUNT,COIN_SAMPLE_RATE,COIN_SAMPLE_SIZE,COIN_CHANNELS,COIN_DATA});
	power = LoadSoundFromWave({ POWER_FRAME_COUNT,POWER_SAMPLE_RATE,POWER_SAMPLE_SIZE,POWER_CHANNELS,POWER_DATA });
	levelup = LoadSoundFromWave({ LEVEL_FRAME_COUNT,LEVEL_SAMPLE_RATE,LEVEL_SAMPLE_SIZE,LEVEL_CHANNELS,LEVEL_DATA });

	LocalPlayer player;
	bool save = true;

	while (!WindowShouldClose()) {
		dt = GetFrameTime() * (60 + player.level + (player.coins / 10));
		// Do Shit
		if (player.menu == 0) {
			if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
				player.yvel = -5;
				player.jumps++;
			}
		}
		if (IsKeyPressed(KEY_TAB)) {
			player.debug = !player.debug;
		}
		if (IsKeyPressed(KEY_F3)) {
			player.colls = !player.colls;
		}
		if (IsKeyPressed(KEY_R) && player.menu == 0) {
			player.Reset(true);
		}
		if (IsKeyPressed(KEY_ESCAPE)) {
			if (player.menu == 0) player.setmenu(1);
			else if (player.menu == 1) player.setmenu(0);
		}

		if (player.menu == 0) player.Move();


		// Draw
		BeginDrawing();
		ClearBackground(BLACK);
		int tWidth;
		if (player.menu != 3) {
			string lvText = to_string(player.level);
			tWidth = MeasureText(lvText.c_str(), 70);
			DrawText(lvText.c_str(), 250 - (tWidth * 0.5), 180, 70, GRAY);
			string cText = "C:" + to_string(player.coins) + " | HS:" + to_string(player.highscore) + " | J:" + to_string(player.jumps) + " | D:" + to_string(player.deaths);
			tWidth = MeasureText(cText.c_str(), 20);
			DrawText(cText.c_str(), 250 - (tWidth * 0.5), 250, 20, YELLOW);

			player.Draw();

			for (int i = 0; i < 10; i++) {
				if (player.menu == 0) player.spikes.right[i].f += player.dir * dt;
				if (player.spikes.right[i].f > 0) { player.spikes.right[i].f = 0; }
				if (player.spikes.right[i].i == 1) {
					Vector2 vx = { 495 - player.spikes.right[i].f, i * 50 };
					Vector2 vy = { 470 - player.spikes.right[i].f, i * 50 + 25 };
					Vector2 vz = { 495 - player.spikes.right[i].f, i * 50 + 50 };
					DrawTriangle(vx, vy, vz, set_alpha(RED, 255 * player.starttimer));
					Rectangle spike = { 485, (i * 50) + 10, 10, 30 };
					if (player.colls) DrawRectangleLinesEx(spike, 1, WHITE);
					if (player.menu == 0) {
						if (CheckCollisionRecs(player.collider, spike)) {
							player.Die();
						}
					}
				}
				if (player.spikes.right[i].i == 2) {
					if (player.spikes.right[i].f < -25) { player.spikes.right[i].f = -25; }
					Color coincolor = { 253, 249, 0, 255 - abs((int)player.spikes.right[i].f * 10) };
					DrawCircle(475, i * 50 + 25, 5, set_alpha(coincolor, coincolor.a * player.starttimer));
					Rectangle coinCollider = { 465,i * 50 + 15,20,20 };
					if (player.colls) DrawRectangleLinesEx(coinCollider, 1, WHITE);
					if (CheckCollisionRecs(player.collider, coinCollider)) {
						player.spikes.right[i].i = 3;
						player.coins++;
						PlaySound(coin);
					}
				}
				if (player.spikes.right[i].i == 4) {
					if (player.spikes.right[i].f < -20) { player.spikes.right[i].f = -20; }
					Color coincolor = { 255,0,255,255 };
					Rectangle coinCollider = { 465,i * 50 + 15,20,20 };
					DrawRectanglePro({ 475,i * 50 + 25.0f,player.spikes.right[i].f + 20,player.spikes.right[i].f + 20 }, { 10,10 }, 45, set_alpha(coincolor, 255 * player.starttimer));
					if (player.colls) DrawRectangleLinesEx(coinCollider, 1, WHITE);
					if (CheckCollisionRecs(player.collider, coinCollider)) {
						player.spikes.right[i].i = 3;
						player.gravityMul = -1 * player.gravityMul;
						player.coins+=5;
						player.yvel *= -1;
						PlaySound(power);
					}
				}
			}
			for (int i = 0; i < 10; i++) {
				if (player.spikes.left[i].f > 0) { player.spikes.left[i].f = 0; }
				if (player.menu == 0) player.spikes.left[i].f -= player.dir * dt;
				if (player.spikes.left[i].i == 1) {
					Vector2 vx = { 5 + player.spikes.left[i].f, i * 50 };
					Vector2 vy = { 30 + player.spikes.left[i].f, i * 50 + 25 };
					Vector2 vz = { 5 + player.spikes.left[i].f, i * 50 + 50 };
					DrawTriangle(vx, vz, vy, set_alpha(RED, 255 * player.starttimer));
					Rectangle spike = { 5, (i * 50) + 10, 10, 30 };
					if (player.colls) DrawRectangleLinesEx(spike, 1, WHITE);
					if (player.menu == 0) {
					if (CheckCollisionRecs(player.collider, spike)) {
						player.Die();
					}}
				}
				if (player.spikes.left[i].i == 2) {
					if (player.spikes.right[i].f < -25) { player.spikes.right[i].f = -25; }
					Color coincolor = { 253, 249, 0, abs((int)player.spikes.right[i].f * 10) };
					DrawCircle(25, i * 50 + 25, 5, set_alpha(coincolor, coincolor.a * player.starttimer));
					Rectangle coinCollider = { 15,i * 50 + 15,20,20 };
					if (player.colls) DrawRectangleLinesEx(coinCollider, 1, WHITE);
					if (CheckCollisionRecs(player.collider, coinCollider)) {
						player.spikes.left[i].i = 3;
						player.coins++;
						PlaySound(coin);
					}
				}
				if (player.spikes.left[i].i == 4) {
					if (player.spikes.left[i].f < -20) { player.spikes.left[i].f = -20; }
					Color coincolor = { 255,0,255,255 };
					Rectangle coinCollider = { 15,i * 50 + 15,20,20 };
					DrawRectanglePro({ 25,i * 50 + 25.0f,player.spikes.left[i].f + 20,player.spikes.left[i].f + 20 }, { 10,10 }, 45, set_alpha(coincolor, 255 * player.starttimer));
					if (player.colls) DrawRectangleLinesEx(coinCollider, 1, WHITE);
					if (CheckCollisionRecs(player.collider, coinCollider)) {
						player.spikes.left[i].i = 3;
						player.gravityMul = -1 * player.gravityMul;
						player.coins += 5;
						player.yvel *= -1;
						PlaySound(power);
					}
				}
			}
		if (player.debug) {
			string text = "FPS: " + to_string(GetFPS());
			text += "\nX: " + to_string((int)player.pos.x) + "\nY: " + to_string((int)player.pos.y) + "\n";
			text += "VelX: " + to_string(player.dir*5.0f*dt) + "\nVelY: " + to_string(player.yvel);
			text += "\nDT: " + to_string(dt);
			text += "\nFrame: " + to_string(player.frame);
			DrawText(text.c_str(), 10, 10, 20, {255,255,255,64});
		}
		}
		if (player.menu != 0 ) {
			DrawRectangle(0, 0, 500, 500, { 0,0,0,128 });
		}


		// other menus
		
		if (player.menu == 1) {
				string cText = "PAUSED";
				tWidth = MeasureText(cText.c_str(), 20);
				DrawText(cText.c_str(), 250 - (tWidth * 0.5), 50, 20, RED);
		}
		if (player.menu == 2) {
			int ctime = time(nullptr);
			string cText = "YOU DIED";
			tWidth = MeasureText(cText.c_str(), 20);
			DrawText(cText.c_str(), 250 - (tWidth * 0.5), 50, 20, RED);
			cText = "R: Restart";
			tWidth = MeasureText(cText.c_str(), 20);
			DrawText(cText.c_str(), 250 - (tWidth * 0.5), 75, 20, RED);
			cText = "Returning To Menu in " + to_string(player.menutime - (ctime - 3)) + " Seconds";
			tWidth = MeasureText(cText.c_str(), 20);
			DrawText(cText.c_str(), 250 - (tWidth * 0.5), 100, 20, RED);
			if (IsKeyPressed(KEY_R)) {
				player.setmenu(0);
				player.Reset();
			}
			if (player.menutime - (ctime - 3) < 1) player.setmenu(3);
		}
		if (player.menu == 3) {
			if (player.submenu == 0) {
				int ctime = time(nullptr);
				player.frame++;
				player.pos = { 460,460 };
				player.starttimer = 10000;
				player.Draw();
				string cText = "Avoid That Spike 2";
				DrawText(cText.c_str(), 10, 10, 40, rbowc[(int)GetTime()%14]);
				cText = "1: Start Playing\n2^: Player Roundness " + to_string(player.round) + "\n3^: Player RED " + to_string(player.color.r) + "\n4^: Player GREEN " + to_string(player.color.g) + "\n5^: Player BLUE " + to_string(player.color.b);
				cText += "\n9: Clear Save File";
				cText += "\n0: Quit Game :(\n\nHighscore: " + to_string(player.highscore);
				cText += "\nCoins: " + to_string(player.coins);
				cText += "\nDeaths: " + to_string(player.deaths);
				DrawText(cText.c_str(), 10, 50, 20, WHITE);
				cText = "Menu Items with ^ are sliders, press up or down arrows to change the value.\nMade by Spelis :)";
				DrawText(cText.c_str(), 5, 475,10,GRAY);

				if (IsKeyPressed(KEY_ZERO)) {
					break;
				}
				if (IsKeyPressed(KEY_ONE)) {
					player.Reset();
					player.setmenu(0);
				}
				if (IsKeyDown(KEY_TWO)) {
					player.round += (IsKeyDown(KEY_UP) - IsKeyDown(KEY_DOWN)) * 0.001;
				}
				if (IsKeyDown(KEY_THREE)) {
					if (player.frame % 7 == 2) {
						player.color.r += IsKeyDown(KEY_UP) - IsKeyDown(KEY_DOWN);
					}
				}
				if (IsKeyDown(KEY_FOUR)) {
					if (player.frame % 7 == 2) {
					player.color.g += IsKeyDown(KEY_UP) - IsKeyDown(KEY_DOWN);
				}}
				if (IsKeyDown(KEY_FIVE)) {
					if (player.frame % 7 == 2) {
					player.color.b += IsKeyDown(KEY_UP) - IsKeyDown(KEY_DOWN);
				}}
				if (IsKeyPressed(KEY_NINE)) {
					player.yvel = 0;
					player.pos = { 10, 10 };
					player.color = { (unsigned char)GetRandomValue(0,255),(unsigned char)GetRandomValue(0,255),(unsigned char)GetRandomValue(0,255),255 };
					player.dir = 1;
					player.level = 1;
					player.highscore = 0;
					player.jumps = 0;
					player.frame = 0;
					player.coins = 0;
					player.menu = 3;
					player.submenu = 0;
					player.menutime = time(nullptr);
					player.starttimer = 0.0f;
					player.colls = false;
					player.debug = false;
					player.deaths = 0;
					player.gravityMul = 1;
					player.round = 0;
					remove("ats2.savedata");
					
				}
			}
		}
		DrawRectangle(GetMouseX() - 5, GetMouseY() - 5, 10, 10, rbowc[(int)GetTime() % 14]);


		EndDrawing();
	}

	// save player state idk

	string saveData = to_string(player.highscore) + "\n" + to_string(player.coins) + "\n" + to_string(player.deaths) + "\n" + to_string(player.color.r) + "\n" + to_string(player.color.g) + "\n" + to_string(player.color.b) + "\n" + to_string(player.round);
	saveData = base64::to_base64(saveData);
	const char* fileText = saveData.c_str();
	SaveFileText("ats2.savedata",(char*)fileText);

	CloseWindow();
	return 0;
}