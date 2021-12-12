
#include "math_ext.h"

Vector2i MathExt::vector2i_max(const Vector2i &a, const Vector2i &b) {
	return Vector2i(MAX(a.x, b.x), MAX(a.y, b.y));
}
Vector2i MathExt::vector2i_min(const Vector2i &a, const Vector2i &b) {
	return Vector2i(MIN(a.x, b.x), MIN(a.y, b.y));
}
Vector2i MathExt::vector2i_abs(const Vector2i &a) {
	return Vector2i(Math::abs(a.x), Math::abs(a.y));
}

Point2i MathExt::rect2i_get_end(const Rect2i &a) {
	return a.get_position() + a.get_size();
}
void MathExt::rect2i_set_end(Rect2i *a, const Point2i &p) {
	a->set_size(p - a->get_position());
}

Rect2i MathExt::rect2i_abs(const Rect2i &a) {
	return Rect2i(Point2i(a.get_position().x + MIN(a.get_size().x, 0), a.get_position().y + MIN(a.get_size().y, 0)), vector2i_abs(a.get_size()));
}