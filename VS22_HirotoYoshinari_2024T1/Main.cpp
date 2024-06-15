#include <Siv3D.hpp>
#include <string>

void DrawCheckerboardBackground(int32 cellSize, const ColorF& cellColor)
{
	for (int32 y = 0; y < (Scene::Height() / cellSize); ++y)
	{
		for (int32 x = 0; x < (Scene::Width() / cellSize); ++x)
		{
			if (IsEven(x + y))
			{
				Rect{ (Point{ x, y } *cellSize), cellSize }.draw(cellColor);
			}
		}
	}
}

namespace constants {
	namespace brick {
		constexpr Size SIZE{ 40, 20 };
		constexpr int Y_COUNT = 5;
		constexpr int X_COUNT = 20;
		constexpr int MAX = Y_COUNT * X_COUNT;
	}

	namespace ball {
		constexpr double SPEED = 480.0;
		constexpr double GROWTH_RATE = 2.0;
	}

	namespace paddle {
		constexpr Size SIZE{ 60, 10 };
	}

	namespace reflect {
		constexpr Vec2 VERTICAL{ 1, -1 };
		constexpr Vec2 HORIZONTAL{ -1,  1 };
		constexpr Vec2 RETRYWALL{ -1, -1 };
	}
}

class Ball final {
private:
	Vec2 velocity;
	Circle ball;
	double radius;
	bool* isGameOver;

public:
	Ball(bool* gameOverFlag) : velocity({ 0, -constants::ball::SPEED }), ball({ 300, 300, 8 }), radius(8), isGameOver(gameOverFlag) {}

	void Update() {
		if (*isGameOver) return;

		ball.moveBy(velocity * Scene::DeltaTime());
		radius += constants::ball::GROWTH_RATE * Scene::DeltaTime();
		ball.r = radius;

		if (ball.y > Scene::Height() + 10) {
			*isGameOver = true;
		}
	}

	void Draw() const {
		ball.draw();
	}

	Circle GetCircle() const {
		return ball;
	}

	Vec2 GetVelocity() const {
		return velocity;
	}

	void SetVelocity(Vec2 newVelocity) {
		velocity = newVelocity.setLength(constants::ball::SPEED);
	}

	void Reflect(const Vec2 reflectVec) {
		velocity *= reflectVec;
	}
};

class Bricks final {
private:
	Rect brickTable[constants::brick::MAX];

public:
	Bricks() {
		using namespace constants::brick;
		for (int y = 0; y < Y_COUNT; ++y) {
			for (int x = 0; x < X_COUNT; ++x) {
				int index = y * X_COUNT + x;
				brickTable[index] = Rect{ x * SIZE.x, 60 + y * SIZE.y, SIZE };
			}
		}
	}

	void Intersects(Ball* const target);

	void Draw() const {
		for (const auto& brick : brickTable) {
			brick.stretched(-1).draw(HSV{ brick.y - 40 });
		}
	}
};

class Paddle final {
private:
	Rect paddle;

public:
	Paddle() : paddle(Rect(Arg::center(Cursor::Pos().x, 500), constants::paddle::SIZE)) {}

	void Update() {
		paddle.x = Cursor::Pos().x - (constants::paddle::SIZE.x / 2);
	}

	void Intersects(Ball* const target) const;

	void Draw() const {
		paddle.rounded(3).draw();
	}
};

class Wall {
public:
	static void Intersects(Ball* target) {
		using namespace constants;

		if (!target) return;

		auto velocity = target->GetVelocity();
		auto ball = target->GetCircle();

		if ((ball.y < 0) && (velocity.y < 0)) {
			target->Reflect(reflect::VERTICAL);
		}

		if (((ball.x < 0) && (velocity.x < 0)) || ((Scene::Width() < ball.x) && (0 < velocity.x))) {
			target->Reflect(reflect::HORIZONTAL);
		}

		if ((ball.y < 0) && (velocity.y)) {
			target->Reflect(reflect::RETRYWALL);
		}
	}
};

void Bricks::Intersects(Ball* const target) {
	using namespace constants::brick;

	if (!target) return;

	auto ball = target->GetCircle();

	for (int i = 0; i < MAX; ++i) {
		Rect& refBrick = brickTable[i];

		if (refBrick.intersects(ball)) {
			if (refBrick.bottom().intersects(ball) || refBrick.top().intersects(ball)) {
				target->Reflect(constants::reflect::VERTICAL);
			}
			else {
				target->Reflect(constants::reflect::HORIZONTAL);
			}

			refBrick.y -= 600;
			break;
		}
	}
}

void Paddle::Intersects(Ball* const target) const {
	if (!target) return;

	auto velocity = target->GetVelocity();
	auto ball = target->GetCircle();

	if ((0 < velocity.y) && paddle.intersects(ball)) {
		target->SetVelocity(Vec2{ (ball.x - paddle.center().x) * 10, -velocity.y });
	}
}

void Main() {
	Scene::SetBackground(ColorF{ 0.4 });

	bool isGameOver = false;

	const Font font{ FontMethod::MSDF,48 };

	int32 count = 0;

	Bricks bricks;
	Ball ball(&isGameOver);
	Paddle paddle;

	while (System::Update()) {

		DrawCheckerboardBackground(40, ColorF{ 0.45 });
		if (!isGameOver) {
			paddle.Update();
			ball.Update();

			bricks.Intersects(&ball);
			Wall::Intersects(&ball);
			paddle.Intersects(&ball);
		}

		bricks.Draw();
		ball.Draw();
		paddle.Draw();

		if (isGameOver) {
			// If the game is over, display the game over message
			font(U"ゲームオーバー"_fmt(count)).draw(80, Vec2{ 200, 200 }, ColorF{ 0.2, 0.6, 0.9 });
			font(U"Rキーを押してリトライしてください"_fmt(count)).draw(30, Vec2{ 200, 300 }, ColorF{ 0.2, 0.6, 0.9 });

			if (KeyR.down()) {
				// If the R key is pressed, restart the game
				isGameOver = false;
				bricks = Bricks(); // Reset the bricks
				ball = Ball(&isGameOver); // Reset the ball
				paddle = Paddle(); // Reset the paddle
			}
		}
	}
}
