#pragma once

struct Vector2 {
	float x;
	float y;
};
struct Vector2Int {
	int x;
	int y;
};

Vector2Int operator+(Vector2Int num1, Vector2Int num2);

Vector2Int operator-(Vector2Int num1, Vector2Int num2);