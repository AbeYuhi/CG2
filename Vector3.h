#pragma once

struct Vector3 {
	float x;
	float y;
	float z;
};

//算術演算子のオーバーロード
Vector3 operator+(Vector3 num1, Vector3 num2);
Vector3 operator-(Vector3 num1, Vector3 num2);