#include "Vector3.h"
#include <math.h>

static const int kColumnWidth = 60;
static const int kRowHeight = 20;


Vector3 Add(const Vector3& v1, const Vector3& v2);

Vector3 Subtract(const Vector3& v1, const Vector3& v2);

Vector3 Multiply(float scalar, const Vector3& v);

float Dot(const Vector3& v1, const Vector3& v2);

float Length(const Vector3& v);

Vector3 Normalize(const Vector3& v1);

Vector3 Cross(const Vector3& v1, const Vector3& v2);

bool IsFront(const Vector3& v1, const Vector3 obj[3]);
