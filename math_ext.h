
#ifndef MATH_EXT_H
#define MATH_EXT_H

#include "core/math/math_defs.h"
#include "core/math/vector2.h"
#include "core/math/rect2.h"
#include "core/math/math_funcs.h"

class MathExt {
public:
    static Vector2i vector2i_max(const Vector2i &a, const Vector2i &b);
    static Vector2i vector2i_min(const Vector2i &a, const Vector2i &b);
    static Vector2i vector2i_abs(const Vector2i &a);
    static Vector2i vector2i_sign(const Vector2i &a);

    static Point2i rect2i_get_end(const Rect2i &a);
    static void rect2i_set_end(Rect2i *a, const Point2i &p);
    static Rect2i rect2i_abs(const Rect2i &a);

    static Rect2i rect2i_intersection(const Rect2i &a, const Rect2i &b);
	static bool rect2i_intersects(const Rect2i &a, const Rect2i &b);
};

#endif
