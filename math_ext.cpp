
#include "math_ext.h"

#include "vector3i.h"

Vector2i MathExt::vector2i_max(const Vector2i &a, const Vector2i &b) {
	return Vector2i(MAX(a.x, b.x), MAX(a.y, b.y));
}
Vector2i MathExt::vector2i_min(const Vector2i &a, const Vector2i &b) {
	return Vector2i(MIN(a.x, b.x), MIN(a.y, b.y));
}
Vector2i MathExt::vector2i_abs(const Vector2i &a) {
	return Vector2i(Math::abs(a.x), Math::abs(a.y));
}

Vector2i MathExt::vector2i_sign(const Vector2i &a) {
	return Vector2i(SIGN(a.x), SIGN(a.y));
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

Rect2i MathExt::rect2i_intersection(const Rect2i &a, const Rect2i &b) {
		if (!rect2i_intersects(a, b)) {
			return Rect2i();
		}

		Rect2i new_rect = b;

		new_rect.position.x = MAX(b.position.x, a.position.x);
		new_rect.position.y = MAX(b.position.y, a.position.y);

		Point2i a_rect_end = a.position + a.size;
		Point2i b_rect_end = b.position + b.size;
		Point2i end = a.position + a.size;

		new_rect.size.x = MIN(b_rect_end.x, a_rect_end.x) - new_rect.position.x;
		new_rect.size.y = MIN(b_rect_end.y, a_rect_end.y) - new_rect.position.y;

		return new_rect;
}

bool MathExt::rect2i_intersects(const Rect2i &a, const Rect2i &b) {
		if (a.position.x > (b.position.x + b.size.width)) {
			return false;
		}
		if ((a.position.x + a.size.width) < b.position.x) {
			return false;
		}
		if (a.position.y > (b.position.y + b.size.height)) {
			return false;
		}
		if ((a.position.y + a.size.height) < b.position.y) {
			return false;
		}

		return true;
}
