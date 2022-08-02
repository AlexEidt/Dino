// Alex Eidt
// Dino Game

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OLC_PGEX_SOUND
#include "olcPGEX_Sound.h"

// Audio.
#define GAMEOVER_AUDIO 0
#define JUMP_AUDIO 1
#define SCORE_AUDIO 2

// Sprites.
#define NUMBER 0
#define CHAR_H 10
#define CHAR_I 11
#define ARROW 12
#define CACTUS 12
#define CLOUD 22
#define DINO 23
#define GAMEOVER 32
#define GROUND 33
#define PTERANODON 33
#define SPRITES 36

class DinoGame : public olc::PixelGameEngine
{
public:
	DinoGame()
	{
		sAppName = "Dino Game";
	}

private:
	float time = 0.0f;
	float groundSpeed = 500.0f;
	float maxGroundSpeed = 1500.0f;
	float cloudSpeed = 150.0f;
	float fps = 30.0f;
	float period = 1 / fps;
	float elapsed = 0.0f;
	int dinoIndex = 1;

	float jumpDuration = 0.75f; // Jump duration in seconds.

	bool started = false;
	bool isJumping = false;
	bool isDucking = false;
	bool gameOver = false;

	float jumpTimer = 0.0f;
	float pteranodonTimer = 0.0f;

	bool scoreBlinking = false;
	float scoreTimer = 0.0f;

	int maxScore = 0;
	int score = 0;

	struct enemy
	{
		int width;
		int height;
		int index;
		int type;
		olc::vf2d pos;
	};

	olc::vf2d position; // Player position.
	std::vector<olc::Decal*> sprites;
	std::vector<olc::vf2d> clouds;
	std::vector<enemy> enemies;

	// Audio.
	std::vector<int> sounds;

public:
	bool OnUserCreate() override
	{
		std::filesystem::path dir (std::filesystem::current_path().string());
    	std::filesystem::path spriteDir ("sprites");
		std::filesystem::path audioDir ("audio");

		olc::SOUND::InitialiseAudio(44100, 1, 8, 512);

		std::vector<std::string> files;
		for (const auto& file : std::filesystem::directory_iterator(dir / audioDir))
		{
			files.push_back(file.path().string());
		}

		std::sort(files.begin(), files.end());

		for (const auto& file : files)
		{
			int id = olc::SOUND::LoadAudioSample(file);
			if (id == -1) throw std::runtime_error("Failed to load audio sample: " + file);
			sounds.push_back(id);
		}

		files.clear();
		for (const auto& file : std::filesystem::directory_iterator(dir / spriteDir))
		{
			files.push_back(file.path().string());
		}
		
		std::sort(files.begin(), files.end());

		for (const auto& file : files)
		{
			sprites.push_back(new olc::Decal(new olc::Sprite(file)));
		}

		// Delete unused sprites. These are present for completeness but are not used in this game.
		delete sprites[DINO + 0]->sprite;
		delete sprites[DINO + 0];
		delete sprites[SPRITES]->sprite;
		delete sprites[SPRITES];

		srand(std::time(0));

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		Clear(olc::Pixel(230, 230, 230));

		float elapsedTime = gameOver ? 0.0f : fElapsedTime;
		DrawScore(elapsedTime);
		DrawGround(elapsedTime);
		DrawClouds(elapsedTime);
		DrawEnemies(elapsedTime);

		int screenWidth = ScreenWidth();
		int screenHeight = ScreenHeight();

		// Draw Dinosaur Character.
		elapsed += fElapsedTime;
		if (elapsed > period * 3.0f) // Update every 3 frames.
		{
			elapsed = 0.0f;
			if (gameOver) // Game over.
				dinoIndex = 6;
			else if (isJumping)
				dinoIndex = 1;
			else if (isDucking) // Dinosaur ducking animation.
				dinoIndex = dinoIndex == 7 ? 8 : 7;
			else if (started) // Dinosaur running animation.
				dinoIndex = dinoIndex == 3 ? 4 : 3;
			else // While the dinosaur is standing, it will randomly blink.
				dinoIndex = rand() % 15 ? 1 : 2;

			// Update score.
			if (started && !gameOver)
			{
				score++;
				if (score > 0 && score % 100 == 0) olc::SOUND::PlaySample(sounds[SCORE_AUDIO]);
			}
		}

		olc::Decal* dino = sprites[DINO + dinoIndex];
		int dinoWidth = dino->sprite->width;
		int dinoHeight = dino->sprite->height;

		int maxJumpHeight = dinoHeight * 15;
		float jumpParabola = -maxJumpHeight * (jumpTimer * jumpTimer - jumpDuration * jumpTimer);
		position.x = 60.0f;
		position.y = std::max(screenHeight - dinoHeight - 8 - jumpParabola, 0.0f);
		DrawDecal(position, dino);

		// If space is pressed, start the game.
		if (GetKey(olc::Key::SPACE).bPressed)
		{
			if (!started)
			{
				gameOver = false;
				started = true;
				enemies.clear();
			}
			if (started)
			{
				isJumping = true;
				if (jumpTimer == 0.0f) olc::SOUND::PlaySample(sounds[JUMP_AUDIO]);
			};
		}

		// If down arrow is pressed, duck the dinosaur.
		if (GetKey(olc::Key::DOWN).bPressed) if (started) isDucking = true;
		if (GetKey(olc::Key::DOWN).bReleased) isDucking = false;

		if (isJumping && !gameOver)
		{
			jumpTimer += fElapsedTime;
			if (jumpTimer > jumpDuration)
			{
				isJumping = false;
				jumpTimer = 0.0f;
			}
		}

		if (gameOver)
		{
			// Draw "Game Over" sprite in the middle of the screen.
			olc::Decal* gameOverSprite = sprites[GAMEOVER];
			olc::vf2d gameOverPos = {
				(float) (screenWidth / 2 - gameOverSprite->sprite->width / 2),
				(float) (screenHeight / 2 - gameOverSprite->sprite->height / 2 - screenHeight / 6)
			};
			DrawDecal(gameOverPos, gameOverSprite);
			olc::Decal* arrowSprite = sprites[ARROW];
			olc::vf2d arrowPos = {
				(float) (screenWidth / 2 - arrowSprite->sprite->width / 2),
				(float) (screenHeight / 2 - arrowSprite->sprite->height / 2 + screenHeight / 10)
			};
			DrawDecal(arrowPos, arrowSprite);
		}
		else
		{
			// Collision Detection.
			for (auto& enemy : enemies)
			{
				float ex0 = enemy.pos.x;
				float ey0 = enemy.pos.y;
				float ex1 = enemy.pos.x + enemy.width;
				float ey1 = enemy.pos.y + enemy.height;

				float px0 = position.x;
				float py0 = position.y;
				float px1 = position.x + dinoWidth;
				float py1 = position.y + dinoHeight;

				if (px0 > ex1) break;

				// Rectangular Bounding Box Method.
				bool noOverlap = ex0 > px1 || px0 > ex1 || ey0 > py1 || py0 > ey1;
				if (!noOverlap)
				{
					gameOver = true;
					started = false;
					maxScore = std::max(maxScore, score);
					score = 0;
					scoreTimer = 0;
					scoreBlinking = false;
					groundSpeed = 500.0f;

					olc::SOUND::PlaySample(sounds[GAMEOVER_AUDIO]);
					break;
				}
			}
		}

		return true;
	}

	bool OnUserDestroy()
	{
		olc::SOUND::DestroyAudio();

		return true;
	}

	void DrawScore(float fElapsedTime)
	{
		if ((score > 0 && score % 100 == 0) || scoreBlinking)
		{
			if (!scoreBlinking) scoreBlinking = true;
			scoreTimer += fElapsedTime;
			if (scoreTimer > 2.0f) // 2 second timer.
			{
				scoreBlinking = false;
				scoreTimer = 0.0f;
			}
		}

		if (started || gameOver)
		{
			int screenWidth = ScreenWidth();
			int tempScore = score;

			// All number/letter sprites have the same width.
			int digitWidth = sprites[NUMBER + 0]->sprite->width;
			int digitHeight = sprites[NUMBER + 0]->sprite->height;

			int offset = 2 * digitWidth;
			if (scoreBlinking)
			{
				if (((int) (scoreTimer * 3)) % 2 == 0)
				{
					drawNumber(score / 100 * 100, offset, 5);
				}
			}
			else
			{
				drawNumber(score, offset, 5);
			}

			if (maxScore > 0)
			{
				offset += 6 * digitWidth;
				drawNumber(maxScore, offset, 5);
				offset += 6 * digitWidth;
				DrawDecal({ (float) (screenWidth - offset), (float) (digitHeight / 2) }, sprites[CHAR_I]);
				offset += digitWidth;
				DrawDecal({ (float) (screenWidth - offset), (float) (digitHeight / 2) }, sprites[CHAR_H]);
			}
		}
	}

	void drawNumber(int num, int offset, int pad)
	{
		int screenWidth = ScreenWidth();
		int digitWidth = sprites[NUMBER + 0]->sprite->width;
		int digitHeight = sprites[NUMBER + 0]->sprite->height;
		int addedOffset = 0;
		int count = 0;
		while (num > 0)
		{
			int digit = num % 10;
			num /= 10;
			olc::Decal* digitDecal = sprites[NUMBER + digit];
			DrawDecal({ (float) (screenWidth - offset - addedOffset), (float) (digitHeight / 2) }, digitDecal);
			addedOffset += digitWidth;
			count++;
		}

		for (int i = 0; i < pad - count; i++)
		{
			DrawDecal({ (float) (screenWidth - offset - addedOffset), (float) (digitHeight / 2) }, sprites[NUMBER + 0]);
			addedOffset += digitWidth;
		}
	}

	void DrawGround(float fElapsedTime)
	{
		olc::Decal* ground = sprites[GROUND];
		int groundWidth = ground->sprite->width;
		int groundHeight = ground->sprite->height;
		int screenWidth = ScreenWidth();
		int screenHeight = ScreenHeight();

		// Draw Ground moving right to left.
		if (started || gameOver)
		{
			int x = - time * groundSpeed;
			if (x <= -groundWidth) time = 0.0f;
			if (x + groundWidth <= screenWidth)
			{
				DrawDecal({ (float) (x + groundWidth), (float) (screenHeight - groundHeight) }, ground);
			}
			DrawDecal({ (float) x, (float) (screenHeight - groundHeight) }, ground);
		}
		else
		{
			DrawDecal({ 0.0f, (float) (screenHeight - groundHeight) }, ground);
		}

		time += fElapsedTime;
		groundSpeed = std::min(groundSpeed + fElapsedTime, maxGroundSpeed);
	}

	void DrawEnemies(float fElapsedTime)
	{
		pteranodonTimer += fElapsedTime;
		if (pteranodonTimer > period * 5.0f) // Update every 5 frames.
		{
			pteranodonTimer = 0.0f;
			// Flap the wings of the pteranodons.
			for (auto& enemy : enemies)
			{
				if (enemy.type == PTERANODON) enemy.index = enemy.index % 2 + 1;
			}
		}
		// Draw Enemies.
		for (auto& enemy : enemies)
		{
			olc::Decal* sprite = sprites[enemy.type + enemy.index];
			if (sprite == nullptr) continue;
			enemy.pos.x -= fElapsedTime * groundSpeed;
			DrawDecal(enemy.pos, sprite);
		}

		// Remove enemies that are off the screen.
		if (enemies.size() > 0)
		{
			auto i = std::remove_if(
				enemies.begin(),
				enemies.end(),
				[](const enemy& enemy) { return enemy.pos.x < -enemy.width; }
			);
			if (i != enemies.end()) enemies.erase(i);
		}

		// Add enemies. Wait until the user has a score of 25 to add enemies.
		if (score >= 25 && rand() % 200 == 0)
		{
			if (rand() % 200 > 50)
			{
				addCactus();
			}
			else
			{
				addPteranodon();
			}			
		}
	}

	void addPteranodon()
	{
		int screenWidth = ScreenWidth();
		int screenHeight = ScreenHeight();

		int dinoHeight = sprites[DINO + 1]->sprite->height;
		int dinoDuckHeight = sprites[DINO + 7]->sprite->height;
		int lowPteranodonHeight = (screenHeight - dinoHeight - 8) - dinoDuckHeight;
		int highPteranodonHeight = screenHeight - 2 * dinoHeight - 8 - 8;
		int y = rand() % 2 ? lowPteranodonHeight : highPteranodonHeight;

		int index = rand() % 2 + 1;
		olc::Decal* sprite = sprites[PTERANODON + index];

		if (enemies.size() == 0)
		{
			enemies.push_back({
				sprite->sprite->width, sprite->sprite->height, index, PTERANODON,
				{ (float) screenWidth, (float) y }
			});
		}
		else
		{
			enemy last = enemies.back();
			int jumpLength = jumpDuration * groundSpeed;
			if (last.pos.x < screenWidth - last.width - jumpLength)
			{
				enemies.push_back({
					sprite->sprite->width, sprite->sprite->height, index, PTERANODON,
					{ (float) screenWidth, (float) y }
				});
			}
		}
	}

	void addCactus()
	{
		int screenWidth = ScreenWidth();
		int screenHeight = ScreenHeight();

		if (enemies.size() == 0)
		{
			int index = rand() % 8 + 1; // Randomly choose any cactus other than "cactus_9".
			olc::Decal* sprite = sprites[CACTUS + index];
			enemies.push_back({
				sprite->sprite->width, sprite->sprite->height, index, CACTUS,
				{ (float) screenWidth, (float) (screenHeight - sprite->sprite->height - 4) }
			});
		}
		else
		{
			enemy last = enemies.back();
			int jumpLength = jumpDuration * groundSpeed;
			if (last.pos.x < screenWidth - last.width - jumpLength)
			{
				int index = rand() % 9 + 1;
				// "cactus_9" is the largest cactus sprite.
				// If we choose this cactus, then we only draw one sprite.
				if (index == 9 && sprites[CACTUS + 9]->sprite->width <= jumpLength)
				{
					olc::Decal* sprite = sprites[CACTUS + 9];
					enemies.push_back({
						sprite->sprite->width, sprite->sprite->height, index, CACTUS,
						{ (float) screenWidth, (float) (screenHeight - sprite->sprite->height - 4) }
					});
				}
				else
				{
					int offset = 0;
					// Add 1-4 cactuses in a row.
					for (int i = 0; i < rand() % 4 + 1; i++)
					{
						if (offset > jumpLength) break;
						int index = rand() % 8 + 1;
						olc::Decal* sprite = sprites[CACTUS + index];
						enemies.push_back({
							sprite->sprite->width, sprite->sprite->height, index, CACTUS,
							{ (float) screenWidth + offset, (float) (screenHeight - sprite->sprite->height - 4) }
						});
						offset += enemies.back().width;
					}
				}
			}
		}
	}

	void DrawClouds(float fElapsedTime)
	{
		int screenWidth = ScreenWidth();
		int screenHeight = ScreenHeight();

		// Draw Clouds.
		olc::Decal* cloud = sprites[CLOUD];
		int cloudWidth = cloud->sprite->width;
		int cloudHeight = cloud->sprite->height;
		for (auto& pos : clouds)
		{
			pos.x -= fElapsedTime * cloudSpeed;
			DrawDecal(pos, cloud);
		}

		// Remove clouds that are off the screen.
		if (clouds.size() > 0)
		{
			auto i = std::remove_if(
				clouds.begin(),
				clouds.end(),
				[cloudWidth](const olc::vf2d& pos) { return pos.x < -cloudWidth; }
			);
			if (i != clouds.end()) clouds.erase(i);
		}

		// Randomly add clouds. Limit to 5 clouds.
		if (started && rand() % 1000 == 0 && clouds.size() < 5)
		{
			// All dino sprites have the same dimensions.
			int dinoHeight = sprites[DINO + 1]->sprite->height;
			int begin = cloudHeight + 10;
			int end = screenHeight - cloudHeight - dinoHeight - 8;
			int y = rand() % (begin - end) + begin;

			if (clouds.size() == 0)
			{
				clouds.push_back({ (float) screenWidth, (float) y });
			}
			else
			{
				olc::vf2d last = clouds.back();
				if (last.x < screenWidth - 3 * cloudWidth / 2)
				{
					clouds.push_back({ (float) screenWidth, (float) y });
				}
			}
		}
	}
};

int main()
{
	DinoGame demo;
	if (demo.Construct(1100, 320, 1, 1))
		demo.Start();

	return 0;
}