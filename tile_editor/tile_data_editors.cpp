/*************************************************************************/
/*  tile_data_editors.cpp                                                */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "tile_data_editors.h"

#include "tile_set_editor.h"

#include "../geometry_2d.h"
#include "core/os/keyboard.h"

#include "editor/editor_properties.h"
#include "editor/editor_properties_array_dict.h"
#include "editor/editor_scale.h"

#include "core/os/input_event.h"
#include "core/os/keyboard.h"

void RTileDataEditor::_tile_set_changed_plan_update() {
	_tile_set_changed_update_needed = true;
	call_deferred("_tile_set_changed_deferred_update");
}

void RTileDataEditor::_tile_set_changed_deferred_update() {
	if (_tile_set_changed_update_needed) {
		_tile_set_changed();
		_tile_set_changed_update_needed = false;
	}
}

RTileData *RTileDataEditor::_get_tile_data(RTileMapCell p_cell) {
	ERR_FAIL_COND_V(!tile_set.is_valid(), nullptr);
	ERR_FAIL_COND_V(!tile_set->has_source(p_cell.source_id), nullptr);

	RTileData *td = nullptr;
	RTileSetSource *source = *tile_set->get_source(p_cell.source_id);
	RTileSetAtlasSource *atlas_source = Object::cast_to<RTileSetAtlasSource>(source);
	if (atlas_source) {
		ERR_FAIL_COND_V(!atlas_source->has_tile(p_cell.get_atlas_coords()), nullptr);
		ERR_FAIL_COND_V(!atlas_source->has_alternative_tile(p_cell.get_atlas_coords(), p_cell.alternative_tile), nullptr);
		td = Object::cast_to<RTileData>(atlas_source->get_tile_data(p_cell.get_atlas_coords(), p_cell.alternative_tile));
	}

	return td;
}

void RTileDataEditor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_tile_set_changed_deferred_update"), &RTileDataEditor::_tile_set_changed_deferred_update);
	ClassDB::bind_method(D_METHOD("_tile_set_changed_plan_update"), &RTileDataEditor::_tile_set_changed_plan_update);
	ClassDB::bind_method(D_METHOD("_property_value_changed", "p_property", "p_value", "p_field"), &RTileDataEditor::_property_value_changed);

	ADD_SIGNAL(MethodInfo("needs_redraw"));
}

void RTileDataEditor::set_tile_set(Ref<RTileSet> p_tile_set) {
	if (tile_set.is_valid()) {
		tile_set->disconnect("changed", this, "_tile_set_changed_plan_update");
	}
	tile_set = p_tile_set;
	if (tile_set.is_valid()) {
		tile_set->connect("changed", this, "_tile_set_changed_plan_update");
	}
	_tile_set_changed_plan_update();
}

bool RDummyObject::_set(const StringName &p_name, const Variant &p_value) {
	if (properties.has(p_name)) {
		properties[p_name] = p_value;
		return true;
	}
	return false;
}

bool RDummyObject::_get(const StringName &p_name, Variant &r_ret) const {
	if (properties.has(p_name)) {
		r_ret = properties[p_name];
		return true;
	}
	return false;
}

bool RDummyObject::has_dummy_property(StringName p_name) {
	return properties.has(p_name);
}

void RDummyObject::add_dummy_property(StringName p_name) {
	ERR_FAIL_COND(properties.has(p_name));
	properties[p_name] = Variant();
}

void RDummyObject::remove_dummy_property(StringName p_name) {
	ERR_FAIL_COND(!properties.has(p_name));
	properties.erase(p_name);
}

void RDummyObject::clear_dummy_properties() {
	properties.clear();
}

void RGenericTilePolygonEditor::_base_control_draw() {
	ERR_FAIL_COND(!tile_set.is_valid());

	real_t grab_threshold = EDITOR_GET("editors/polygon_editor/point_grab_radius");

	Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
	const Ref<Texture> handle = get_icon(("EditorPathSharpHandle"), ("EditorIcons"));
	const Ref<Texture> add_handle = get_icon(("EditorHandleAdd"), ("EditorIcons"));
	const Ref<StyleBox> focus_stylebox = get_stylebox(("Focus"), ("EditorStyles"));

	// Draw the focus rectangle.
	if (base_control->has_focus()) {
		base_control->draw_style_box(focus_stylebox, Rect2(Vector2(), base_control->get_size()));
	}

	// Draw tile-related things.
	Size2 tile_size = tile_set->get_tile_size();

	Transform2D xform;
	xform.set_origin(base_control->get_size() / 2 + panning);
	xform.set_scale(Vector2(editor_zoom_widget->get_zoom(), editor_zoom_widget->get_zoom()));
	base_control->draw_set_transform_matrix(xform);

	// Draw the tile shape filled.
	Transform2D tile_xform;
	tile_xform.set_scale(tile_size);
	tile_set->draw_tile_shape(base_control, tile_xform, Color(1.0, 1.0, 1.0, 0.3), true);

	// Draw the background.
	if (background_texture.is_valid()) {
		base_control->draw_texture_rect_region(background_texture, Rect2(-background_region.size / 2 - background_offset, background_region.size), background_region, background_modulate, background_transpose);
	}

	// Draw the polygons.
	for (unsigned int i = 0; i < polygons.size(); i++) {
		const Vector<Vector2> &polygon = polygons[i];
		Color color = polygon_color;
		if (!in_creation_polygon.empty()) {
			color = color.darkened(0.3);
		}
		color.a = 0.5;
		Vector<Color> v_color;
		v_color.push_back(color);
		base_control->draw_polygon(polygon, v_color);

		color.a = 0.7;
		for (int j = 0; j < polygon.size(); j++) {
			base_control->draw_line(polygon[j], polygon[(j + 1) % polygon.size()], color);
		}
	}

	// Draw the polygon in creation.
	if (!in_creation_polygon.empty()) {
		for (int i = 0; i < in_creation_polygon.size() - 1; i++) {
			base_control->draw_line(in_creation_polygon[i], in_creation_polygon[i + 1], Color(1.0, 1.0, 1.0));
		}
	}

	Point2 in_creation_point = xform.affine_inverse().xform(base_control->get_local_mouse_position());
	float in_creation_distance = grab_threshold * 2.0;
	_snap_to_tile_shape(in_creation_point, in_creation_distance, grab_threshold / editor_zoom_widget->get_zoom());
	if (button_pixel_snap->is_pressed()) {
		_snap_to_half_pixel(in_creation_point);
	}

	if (drag_type == DRAG_TYPE_CREATE_POINT && !in_creation_polygon.empty()) {
		base_control->draw_line(in_creation_polygon[in_creation_polygon.size() - 1], in_creation_point, Color(1.0, 1.0, 1.0));
	}

	// Draw the handles.
	int tinted_polygon_index = -1;
	int tinted_point_index = -1;
	if (drag_type == DRAG_TYPE_DRAG_POINT) {
		tinted_polygon_index = drag_polygon_index;
		tinted_point_index = drag_point_index;
	} else if (hovered_point_index >= 0) {
		tinted_polygon_index = hovered_polygon_index;
		tinted_point_index = hovered_point_index;
	}

	base_control->draw_set_transform_matrix(Transform2D());
	if (!in_creation_polygon.empty()) {
		for (int i = 0; i < in_creation_polygon.size(); i++) {
			base_control->draw_texture(handle, xform.xform(in_creation_polygon[i]) - handle->get_size() / 2);
		}
	} else {
		for (int i = 0; i < (int)polygons.size(); i++) {
			const Vector<Vector2> &polygon = polygons[i];
			for (int j = 0; j < polygon.size(); j++) {
				const Color modulate = (tinted_polygon_index == i && tinted_point_index == j) ? Color(0.5, 1, 2) : Color(1, 1, 1);
				base_control->draw_texture(handle, xform.xform(polygon[j]) - handle->get_size() / 2, modulate);
			}
		}
	}

	// Draw the text on top of the selected point.
	if (tinted_polygon_index >= 0) {
		Ref<Font> font = get_font(("font"), ("Label"));
		String text = multiple_polygon_mode ? vformat("%d:%d", tinted_polygon_index, tinted_point_index) : vformat("%d", tinted_point_index);
		Size2 text_size = font->get_string_size(text);
		base_control->draw_string(font, xform.xform(polygons[tinted_polygon_index][tinted_point_index]) - text_size * 0.5, text, Color(1.0, 1.0, 1.0, 0.5));
	}

	if (drag_type == DRAG_TYPE_CREATE_POINT) {
		base_control->draw_texture(handle, xform.xform(in_creation_point) - handle->get_size() / 2, Color(0.5, 1, 2));
	}

	// Draw the point creation preview in edit mode.
	if (hovered_segment_index >= 0) {
		base_control->draw_texture(add_handle, xform.xform(hovered_segment_point) - add_handle->get_size() / 2);
	}

	// Draw the tile shape line.
	base_control->draw_set_transform_matrix(xform);
	tile_set->draw_tile_shape(base_control, tile_xform, grid_color, false);
	base_control->draw_set_transform_matrix(Transform2D());
}

void RGenericTilePolygonEditor::_center_view() {
	panning = Vector2();
	base_control->update();
	button_center_view->set_disabled(true);
}

void RGenericTilePolygonEditor::_zoom_changed() {
	base_control->update();
}

void RGenericTilePolygonEditor::_advanced_menu_item_pressed(int p_item_pressed) {
	UndoRedo *undo_redo = use_undo_redo ? editor_undo_redo : memnew(UndoRedo);
	switch (p_item_pressed) {
		case RESET_TO_DEFAULT_TILE: {
			undo_redo->create_action(TTR("Reset Polygons"));
			undo_redo->add_do_method(this, "clear_polygons");
			Vector<Vector2> polygon = tile_set->get_tile_shape_polygon();
			for (int i = 0; i < polygon.size(); i++) {
				polygon.write[i] = polygon[i] * tile_set->get_tile_size();
			}
			undo_redo->add_do_method(this, "add_polygon", polygon);
			undo_redo->add_do_method(base_control, "update");
			undo_redo->add_do_method(this, "emit_signal", "polygons_changed");
			undo_redo->add_undo_method(this, "clear_polygons");
			for (unsigned int i = 0; i < polygons.size(); i++) {
				undo_redo->add_undo_method(this, "add_polygon", polygons[i]);
			}
			undo_redo->add_undo_method(base_control, "update");
			undo_redo->add_undo_method(this, "emit_signal", "polygons_changed");
			undo_redo->commit_action();
		} break;
		case CLEAR_TILE: {
			undo_redo->create_action(TTR("Clear Polygons"));
			undo_redo->add_do_method(this, "clear_polygons");
			undo_redo->add_do_method(base_control, "update");
			undo_redo->add_do_method(this, "emit_signal", "polygons_changed");
			undo_redo->add_undo_method(this, "clear_polygons");
			for (unsigned int i = 0; i < polygons.size(); i++) {
				undo_redo->add_undo_method(this, "add_polygon", polygons[i]);
			}
			undo_redo->add_undo_method(base_control, "update");
			undo_redo->add_undo_method(this, "emit_signal", "polygons_changed");
			undo_redo->commit_action();
		} break;
		case ROTATE_RIGHT:
		case ROTATE_LEFT:
		case FLIP_HORIZONTALLY:
		case FLIP_VERTICALLY: {
			undo_redo->create_action(TTR("Rotate Polygons Left"));
			for (unsigned int i = 0; i < polygons.size(); i++) {
				Vector<Point2> new_polygon;
				for (int point_index = 0; point_index < polygons[i].size(); point_index++) {
					Vector2 point = polygons[i][point_index];
					switch (p_item_pressed) {
						case ROTATE_RIGHT: {
							point = Vector2(-point.y, point.x);
						} break;
						case ROTATE_LEFT: {
							point = Vector2(point.y, -point.x);
						} break;
						case FLIP_HORIZONTALLY: {
							point = Vector2(-point.x, point.y);
						} break;
						case FLIP_VERTICALLY: {
							point = Vector2(point.x, -point.y);
						} break;
						default:
							break;
					}
					new_polygon.push_back(point);
				}
				undo_redo->add_do_method(this, "set_polygon", i, new_polygon);
			}
			undo_redo->add_do_method(base_control, "update");
			undo_redo->add_do_method(this, "emit_signal", "polygons_changed");
			for (unsigned int i = 0; i < polygons.size(); i++) {
				undo_redo->add_undo_method(this, "set_polygon", polygons[i]);
			}
			undo_redo->add_undo_method(base_control, "update");
			undo_redo->add_undo_method(this, "emit_signal", "polygons_changed");
			undo_redo->commit_action();
		} break;
		default:
			break;
	}
	if (!use_undo_redo) {
		memdelete(undo_redo);
	}
}

void RGenericTilePolygonEditor::_grab_polygon_point(Vector2 p_pos, const Transform2D &p_polygon_xform, int &r_polygon_index, int &r_point_index) {
	const real_t grab_threshold = EDITOR_GET("editors/polygon_editor/point_grab_radius");
	r_polygon_index = -1;
	r_point_index = -1;
	float closest_distance = grab_threshold + 1.0;
	for (unsigned int i = 0; i < polygons.size(); i++) {
		const Vector<Vector2> &polygon = polygons[i];
		for (int j = 0; j < polygon.size(); j++) {
			float distance = p_pos.distance_to(p_polygon_xform.xform(polygon[j]));
			if (distance < grab_threshold && distance < closest_distance) {
				r_polygon_index = i;
				r_point_index = j;
				closest_distance = distance;
			}
		}
	}
}

void RGenericTilePolygonEditor::_grab_polygon_segment_point(Vector2 p_pos, const Transform2D &p_polygon_xform, int &r_polygon_index, int &r_segment_index, Vector2 &r_point) {
	const real_t grab_threshold = EDITOR_GET("editors/polygon_editor/point_grab_radius");

	Point2 point = p_polygon_xform.affine_inverse().xform(p_pos);
	r_polygon_index = -1;
	r_segment_index = -1;
	float closest_distance = grab_threshold * 2.0;
	for (unsigned int i = 0; i < polygons.size(); i++) {
		const Vector<Vector2> &polygon = polygons[i];
		for (int j = 0; j < polygon.size(); j++) {
			Vector2 segment[2] = { polygon[j], polygon[(j + 1) % polygon.size()] };
			Vector2 closest_point = Geometry2D::get_closest_point_to_segment(point, segment);
			float distance = closest_point.distance_to(point);
			if (distance < grab_threshold / editor_zoom_widget->get_zoom() && distance < closest_distance) {
				r_polygon_index = i;
				r_segment_index = j;
				r_point = closest_point;
				closest_distance = distance;
			}
		}
	}
}

void RGenericTilePolygonEditor::_snap_to_tile_shape(Point2 &r_point, float &r_current_snapped_dist, float p_snap_dist) {
	ERR_FAIL_COND(!tile_set.is_valid());

	Vector<Point2> polygon = tile_set->get_tile_shape_polygon();
	for (int i = 0; i < polygon.size(); i++) {
		polygon.write[i] = polygon[i] * tile_set->get_tile_size();
	}
	Point2 snapped_point = r_point;

	// Snap to polygon vertices.
	bool snapped = false;
	for (int i = 0; i < polygon.size(); i++) {
		float distance = r_point.distance_to(polygon[i]);
		if (distance < p_snap_dist && distance < r_current_snapped_dist) {
			snapped_point = polygon[i];
			r_current_snapped_dist = distance;
			snapped = true;
		}
	}

	// Snap to edges if we did not snap to vertices.
	if (!snapped) {
		for (int i = 0; i < polygon.size(); i++) {
			Point2 segment[2] = { polygon[i], polygon[(i + 1) % polygon.size()] };
			Point2 point = Geometry2D::get_closest_point_to_segment(r_point, segment);
			float distance = r_point.distance_to(point);
			if (distance < p_snap_dist && distance < r_current_snapped_dist) {
				snapped_point = point;
				r_current_snapped_dist = distance;
			}
		}
	}

	r_point = snapped_point;
}

void RGenericTilePolygonEditor::_snap_to_half_pixel(Point2 &r_point) {
	r_point = (r_point * 2).round() / 2.0;
}

void RGenericTilePolygonEditor::_base_control_gui_input(Ref<InputEvent> p_event) {
	UndoRedo *undo_redo = use_undo_redo ? editor_undo_redo : memnew(UndoRedo);
	real_t grab_threshold = EDITOR_GET("editors/polygon_editor/point_grab_radius");

	hovered_polygon_index = -1;
	hovered_point_index = -1;
	hovered_segment_index = -1;
	hovered_segment_point = Vector2();

	Transform2D xform;
	xform.set_origin(base_control->get_size() / 2 + panning);
	xform.set_scale(Vector2(editor_zoom_widget->get_zoom(), editor_zoom_widget->get_zoom()));

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		if (drag_type == DRAG_TYPE_DRAG_POINT) {
			ERR_FAIL_INDEX(drag_polygon_index, (int)polygons.size());
			ERR_FAIL_INDEX(drag_point_index, polygons[drag_polygon_index].size());
			Point2 point = xform.affine_inverse().xform(mm->get_position());
			float distance = grab_threshold * 2.0;
			_snap_to_tile_shape(point, distance, grab_threshold / editor_zoom_widget->get_zoom());
			if (button_pixel_snap->is_pressed()) {
				_snap_to_half_pixel(point);
			}
			polygons[drag_polygon_index].write[drag_point_index] = point;
		} else if (drag_type == DRAG_TYPE_PAN) {
			panning += mm->get_position() - drag_last_pos;
			drag_last_pos = mm->get_position();
			button_center_view->set_disabled(panning.is_equal_approx(Vector2()));
		} else {
			// Update hovered point.
			_grab_polygon_point(mm->get_position(), xform, hovered_polygon_index, hovered_point_index);

			// If we have no hovered point, check if we hover a segment.
			if (hovered_point_index == -1) {
				_grab_polygon_segment_point(mm->get_position(), xform, hovered_polygon_index, hovered_segment_index, hovered_segment_point);
			}
		}
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		if (mb->get_button_index() == BUTTON_WHEEL_UP && mb->get_control()) {
			editor_zoom_widget->set_zoom_by_increments(1);
			_zoom_changed();
			accept_event();
		} else if (mb->get_button_index() == BUTTON_WHEEL_DOWN && mb->get_control()) {
			editor_zoom_widget->set_zoom_by_increments(-1);
			_zoom_changed();
			accept_event();
		} else if (mb->get_button_index() == BUTTON_LEFT) {
			if (mb->is_pressed()) {
				if (tools_button_group->get_pressed_button() != button_create) {
					in_creation_polygon.clear();
				}
				if (tools_button_group->get_pressed_button() == button_create) {
					// Create points.
					if (in_creation_polygon.size() >= 3 && mb->get_position().distance_to(xform.xform(in_creation_polygon[0])) < grab_threshold) {
						// Closes and create polygon.
						if (!multiple_polygon_mode) {
							clear_polygons();
						}
						int added = add_polygon(in_creation_polygon);

						in_creation_polygon.clear();
						button_edit->set_pressed(true);
						undo_redo->create_action(TTR("Edit Polygons"));
						if (!multiple_polygon_mode) {
							undo_redo->add_do_method(this, "clear_polygons");
						}
						undo_redo->add_do_method(this, "add_polygon", in_creation_polygon);
						undo_redo->add_do_method(base_control, "update");
						undo_redo->add_undo_method(this, "remove_polygon", added);
						undo_redo->add_undo_method(base_control, "update");
						undo_redo->commit_action();
						emit_signal(("polygons_changed"));
					} else {
						// Create a new point.
						drag_type = DRAG_TYPE_CREATE_POINT;
					}
				} else if (tools_button_group->get_pressed_button() == button_edit) {
					// Edit points.
					int closest_polygon;
					int closest_point;
					_grab_polygon_point(mb->get_position(), xform, closest_polygon, closest_point);
					if (closest_polygon >= 0) {
						drag_type = DRAG_TYPE_DRAG_POINT;
						drag_polygon_index = closest_polygon;
						drag_point_index = closest_point;
						drag_old_polygon = polygons[drag_polygon_index];
					} else {
						// Create a point.
						Vector2 point_to_create;
						_grab_polygon_segment_point(mb->get_position(), xform, closest_polygon, closest_point, point_to_create);
						if (closest_polygon >= 0) {
							polygons[closest_polygon].insert(closest_point + 1, point_to_create);
							drag_type = DRAG_TYPE_DRAG_POINT;
							drag_polygon_index = closest_polygon;
							drag_point_index = closest_point + 1;
							drag_old_polygon = polygons[closest_polygon];
						}
					}
				} else if (tools_button_group->get_pressed_button() == button_delete) {
					// Remove point.
					int closest_polygon;
					int closest_point;
					_grab_polygon_point(mb->get_position(), xform, closest_polygon, closest_point);
					if (closest_polygon >= 0) {
						Vector<Vector2> old_polygon = polygons[closest_polygon];
						polygons[closest_polygon].remove(closest_point);
						undo_redo->create_action(TTR("Edit Polygons"));
						if (polygons[closest_polygon].size() < 3) {
							remove_polygon(closest_polygon);
							undo_redo->add_do_method(this, "remove_polygon", closest_polygon);
							undo_redo->add_undo_method(this, "add_polygon", old_polygon, closest_polygon);
						} else {
							undo_redo->add_do_method(this, "set_polygon", closest_polygon, polygons[closest_polygon]);
							undo_redo->add_undo_method(this, "set_polygon", closest_polygon, old_polygon);
						}
						undo_redo->add_do_method(base_control, "update");
						undo_redo->add_undo_method(base_control, "update");
						undo_redo->commit_action();
						emit_signal(("polygons_changed"));
					}
				}
			} else {
				if (drag_type == DRAG_TYPE_DRAG_POINT) {
					undo_redo->create_action(TTR("Edit Polygons"));
					undo_redo->add_do_method(this, "set_polygon", drag_polygon_index, polygons[drag_polygon_index]);
					undo_redo->add_do_method(base_control, "update");
					undo_redo->add_undo_method(this, "set_polygon", drag_polygon_index, drag_old_polygon);
					undo_redo->add_undo_method(base_control, "update");
					undo_redo->commit_action();
					emit_signal(("polygons_changed"));
				} else if (drag_type == DRAG_TYPE_CREATE_POINT) {
					Point2 point = xform.affine_inverse().xform(mb->get_position());
					float distance = grab_threshold * 2;
					_snap_to_tile_shape(point, distance, grab_threshold / editor_zoom_widget->get_zoom());
					if (button_pixel_snap->is_pressed()) {
						_snap_to_half_pixel(point);
					}
					in_creation_polygon.push_back(point);
				}
				drag_type = DRAG_TYPE_NONE;
				drag_point_index = -1;
			}

		} else if (mb->get_button_index() == BUTTON_RIGHT) {
			if (mb->is_pressed()) {
				if (tools_button_group->get_pressed_button() == button_edit) {
					// Remove point or pan.
					int closest_polygon;
					int closest_point;
					_grab_polygon_point(mb->get_position(), xform, closest_polygon, closest_point);
					if (closest_polygon >= 0) {
						Vector<Vector2> old_polygon = polygons[closest_polygon];
						polygons[closest_polygon].remove(closest_point);
						undo_redo->create_action(TTR("Edit Polygons"));
						if (polygons[closest_polygon].size() < 3) {
							remove_polygon(closest_polygon);
							undo_redo->add_do_method(this, "remove_polygon", closest_polygon);
							undo_redo->add_undo_method(this, "add_polygon", old_polygon, closest_polygon);
						} else {
							undo_redo->add_do_method(this, "set_polygon", closest_polygon, polygons[closest_polygon]);
							undo_redo->add_undo_method(this, "set_polygon", closest_polygon, old_polygon);
						}
						undo_redo->add_do_method(base_control, "update");
						undo_redo->add_undo_method(base_control, "update");
						undo_redo->commit_action();
						emit_signal(("polygons_changed"));
					} else {
						drag_type = DRAG_TYPE_PAN;
						drag_last_pos = mb->get_position();
					}
				} else {
					drag_type = DRAG_TYPE_PAN;
					drag_last_pos = mb->get_position();
				}
			} else {
				drag_type = DRAG_TYPE_NONE;
			}
		} else if (mb->get_button_index() == BUTTON_MIDDLE) {
			if (mb->is_pressed()) {
				drag_type = DRAG_TYPE_PAN;
				drag_last_pos = mb->get_position();
			} else {
				drag_type = DRAG_TYPE_NONE;
			}
		}
	}

	base_control->update();

	if (!use_undo_redo) {
		memdelete(undo_redo);
	}
}

void RGenericTilePolygonEditor::set_use_undo_redo(bool p_use_undo_redo) {
	use_undo_redo = p_use_undo_redo;
}

void RGenericTilePolygonEditor::set_tile_set(Ref<RTileSet> p_tile_set) {
	ERR_FAIL_COND(!p_tile_set.is_valid());
	if (tile_set == p_tile_set) {
		return;
	}

	// Set the default tile shape
	clear_polygons();
	if (p_tile_set.is_valid()) {
		Vector<Vector2> polygon = p_tile_set->get_tile_shape_polygon();
		for (int i = 0; i < polygon.size(); i++) {
			polygon.write[i] = polygon[i] * p_tile_set->get_tile_size();
		}
		add_polygon(polygon);
	}

	tile_set = p_tile_set;

	// Set the default zoom value.
	int default_control_y_size = 200 * EDSCALE;
	Vector2 zoomed_tile = editor_zoom_widget->get_zoom() * tile_set->get_tile_size();
	while (zoomed_tile.y < default_control_y_size) {
		editor_zoom_widget->set_zoom_by_increments(6, false);
		zoomed_tile = editor_zoom_widget->get_zoom() * tile_set->get_tile_size();
	}
	while (zoomed_tile.y > default_control_y_size) {
		editor_zoom_widget->set_zoom_by_increments(-6, false);
		zoomed_tile = editor_zoom_widget->get_zoom() * tile_set->get_tile_size();
	}
	editor_zoom_widget->set_zoom_by_increments(-6, false);
	_zoom_changed();
}

void RGenericTilePolygonEditor::set_background(Ref<Texture> p_texture, Rect2 p_region, Vector2 p_offset, bool p_flip_h, bool p_flip_v, bool p_transpose, Color p_modulate) {
	background_texture = p_texture;
	background_region = p_region;
	background_offset = p_offset;
	background_h_flip = p_flip_h;
	background_v_flip = p_flip_v;
	background_transpose = p_transpose;
	background_modulate = p_modulate;
	base_control->update();
}

int RGenericTilePolygonEditor::get_polygon_count() {
	return polygons.size();
}

int RGenericTilePolygonEditor::add_polygon(Vector<Point2> p_polygon, int p_index) {
	ERR_FAIL_COND_V(p_polygon.size() < 3, -1);
	ERR_FAIL_COND_V(!multiple_polygon_mode && polygons.size() >= 1, -1);

	if (p_index < 0) {
		polygons.push_back(p_polygon);
		base_control->update();
		button_edit->set_pressed(true);
		return polygons.size() - 1;
	} else {
		polygons.insert(p_index, p_polygon);
		button_edit->set_pressed(true);
		base_control->update();
		return p_index;
	}
}

int RGenericTilePolygonEditor::add_polygon_poolvector(PoolVector2Array p_polygon, int p_index) {
	Vector<Point2> v;

	v.resize(p_polygon.size());

	for (int i = 0; i < v.size(); ++i) {
		v.set(i, p_polygon[i]);
	}

	return add_polygon(v, p_index);
}

void RGenericTilePolygonEditor::remove_polygon(int p_index) {
	ERR_FAIL_INDEX(p_index, (int)polygons.size());
	polygons.remove(p_index);

	if (polygons.size() == 0) {
		button_create->set_pressed(true);
	}
	base_control->update();
}

void RGenericTilePolygonEditor::clear_polygons() {
	polygons.clear();
	base_control->update();
}

void RGenericTilePolygonEditor::set_polygon(int p_polygon_index, Vector<Point2> p_polygon) {
	ERR_FAIL_INDEX(p_polygon_index, (int)polygons.size());
	ERR_FAIL_COND(p_polygon.size() < 3);
	polygons[p_polygon_index] = p_polygon;
	button_edit->set_pressed(true);
	base_control->update();
}

Vector<Point2> RGenericTilePolygonEditor::get_polygon(int p_polygon_index) {
	ERR_FAIL_INDEX_V(p_polygon_index, (int)polygons.size(), Vector<Point2>());
	return polygons[p_polygon_index];
}

PoolVector2Array RGenericTilePolygonEditor::get_polygon_poolvector(int p_polygon_index) {
	ERR_FAIL_INDEX_V(p_polygon_index, (int)polygons.size(), PoolVector2Array());

	Vector<Vector2> vo = polygons[p_polygon_index];

	PoolVector2Array v;
	v.resize(vo.size());

	for (int i = 0; i < v.size(); ++i) {
		v.set(i, vo[i]);
	}

	return v;
}

void RGenericTilePolygonEditor::set_polygons_color(Color p_color) {
	polygon_color = p_color;
	base_control->update();
}

void RGenericTilePolygonEditor::set_multiple_polygon_mode(bool p_multiple_polygon_mode) {
	multiple_polygon_mode = p_multiple_polygon_mode;
}

void RGenericTilePolygonEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
			button_create->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon(("CurveCreate"), ("EditorIcons")));
			button_edit->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon(("CurveEdit"), ("EditorIcons")));
			button_delete->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon(("CurveDelete"), ("EditorIcons")));
			button_center_view->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon(("CenterView"), ("EditorIcons")));
			button_pixel_snap->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon(("Snap"), ("EditorIcons")));
			button_advanced_menu->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon(("GuiTabMenuHl"), ("EditorIcons")));

			PopupMenu *p = button_advanced_menu->get_popup();
			p->set_item_icon(p->get_item_index(ROTATE_RIGHT), get_icon(("RotateRight"), ("EditorIcons")));
			p->set_item_icon(p->get_item_index(ROTATE_LEFT), get_icon(("RotateLeft"), ("EditorIcons")));
			p->set_item_icon(p->get_item_index(FLIP_HORIZONTALLY), get_icon(("MirrorX"), ("EditorIcons")));
			p->set_item_icon(p->get_item_index(FLIP_VERTICALLY), get_icon(("MirrorY"), ("EditorIcons")));
			break;
	}
}

void RGenericTilePolygonEditor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_polygon_count"), &RGenericTilePolygonEditor::get_polygon_count);
	ClassDB::bind_method(D_METHOD("add_polygon", "polygon", "index"), &RGenericTilePolygonEditor::add_polygon, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("remove_polygon", "index"), &RGenericTilePolygonEditor::remove_polygon);
	ClassDB::bind_method(D_METHOD("clear_polygons"), &RGenericTilePolygonEditor::clear_polygons);
	ClassDB::bind_method(D_METHOD("set_polygon", "index", "polygon"), &RGenericTilePolygonEditor::set_polygon);
	ClassDB::bind_method(D_METHOD("get_polygon", "index"), &RGenericTilePolygonEditor::set_polygon);

	ADD_SIGNAL(MethodInfo("polygons_changed"));

	ClassDB::bind_method(D_METHOD("_advanced_menu_item_pressed", "pressed"), &RGenericTilePolygonEditor::_advanced_menu_item_pressed);
	ClassDB::bind_method(D_METHOD("_base_control_draw"), &RGenericTilePolygonEditor::_base_control_draw);
	ClassDB::bind_method(D_METHOD("_base_control_gui_input", "event"), &RGenericTilePolygonEditor::_base_control_gui_input);
	ClassDB::bind_method(D_METHOD("_zoom_changed"), &RGenericTilePolygonEditor::_zoom_changed);
	ClassDB::bind_method(D_METHOD("_center_view"), &RGenericTilePolygonEditor::_center_view);
}

RGenericTilePolygonEditor::RGenericTilePolygonEditor() {
	toolbar = memnew(HBoxContainer);
	add_child(toolbar);

	tools_button_group.instance();

	button_create = memnew(Button);
	button_create->set_flat(true);
	button_create->set_toggle_mode(true);
	button_create->set_button_group(tools_button_group);
	button_create->set_pressed(true);
	button_create->set_tooltip(TTR("Add polygon tool"));
	toolbar->add_child(button_create);

	button_edit = memnew(Button);
	button_edit->set_flat(true);
	button_edit->set_toggle_mode(true);
	button_edit->set_button_group(tools_button_group);
	button_edit->set_tooltip(TTR("Edit points tool"));
	toolbar->add_child(button_edit);

	button_delete = memnew(Button);
	button_delete->set_flat(true);
	button_delete->set_toggle_mode(true);
	button_delete->set_button_group(tools_button_group);
	button_delete->set_tooltip(TTR("Delete points tool"));
	toolbar->add_child(button_delete);

	button_advanced_menu = memnew(MenuButton);
	button_advanced_menu->set_flat(true);
	button_advanced_menu->set_toggle_mode(true);
	button_advanced_menu->get_popup()->add_item(TTR("Reset to default tile shape"), RESET_TO_DEFAULT_TILE);
	button_advanced_menu->get_popup()->add_item(TTR("Clear"), CLEAR_TILE);
	button_advanced_menu->get_popup()->add_separator();
	button_advanced_menu->get_popup()->add_icon_item(get_icon(("RotateRight"), ("EditorIcons")), TTR("Rotate Right"), ROTATE_RIGHT);
	button_advanced_menu->get_popup()->add_icon_item(get_icon(("RotateLeft"), ("EditorIcons")), TTR("Rotate Left"), ROTATE_LEFT);
	button_advanced_menu->get_popup()->add_icon_item(get_icon(("MirrorX"), ("EditorIcons")), TTR("Flip Horizontally"), FLIP_HORIZONTALLY);
	button_advanced_menu->get_popup()->add_icon_item(get_icon(("MirrorY"), ("EditorIcons")), TTR("Flip Vertically"), FLIP_VERTICALLY);
	button_advanced_menu->get_popup()->connect("id_pressed", this, "_advanced_menu_item_pressed");
	button_advanced_menu->set_focus_mode(FOCUS_ALL);
	toolbar->add_child(button_advanced_menu);

	toolbar->add_child(memnew(VSeparator));

	button_pixel_snap = memnew(Button);
	button_pixel_snap->set_flat(true);
	button_pixel_snap->set_toggle_mode(true);
	button_pixel_snap->set_pressed(true);
	button_pixel_snap->set_tooltip(TTR("Snap to half-pixel"));
	toolbar->add_child(button_pixel_snap);

	Control *root = memnew(Control);
	root->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root->set_custom_minimum_size(Size2(0, 200 * EDSCALE));
	root->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	add_child(root);

	panel = memnew(Panel);
	panel->set_anchors_and_margins_preset(Control::PRESET_WIDE);
	panel->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	root->add_child(panel);

	base_control = memnew(Control);
	//base_control->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	base_control->set_anchors_and_margins_preset(Control::PRESET_WIDE);
	base_control->connect("draw", this, "_base_control_draw");
	base_control->connect("gui_input", this, "_base_control_gui_input");
	base_control->set_clip_contents(true);
	base_control->set_focus_mode(Control::FOCUS_CLICK);
	root->add_child(base_control);

	editor_zoom_widget = memnew(EditorZoomWidget);
	editor_zoom_widget->set_position(Vector2(5, 5));
	editor_zoom_widget->connect("zoom_changed", this, "_zoom_changed");
	root->add_child(editor_zoom_widget);

	button_center_view = memnew(Button);
	button_center_view->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon(("CenterView"), ("EditorIcons")));
	button_center_view->set_anchors_and_margins_preset(Control::PRESET_TOP_RIGHT, Control::PRESET_MODE_MINSIZE, 5);
	button_center_view->connect("pressed", this, "_center_view");
	button_center_view->set_flat(true);
	button_center_view->set_disabled(true);
	root->add_child(button_center_view);
}

void RTileDataDefaultEditor::_property_value_changed(StringName p_property, Variant p_value, StringName p_field) {
	ERR_FAIL_COND(!dummy_object);
	dummy_object->set(p_property, p_value);
}

Variant RTileDataDefaultEditor::_get_painted_value() {
	ERR_FAIL_COND_V(!dummy_object, Variant());
	return dummy_object->get(property);
}

void RTileDataDefaultEditor::_set_painted_value(RTileSetAtlasSource *p_tile_set_atlas_source, Vector2 p_coords, int p_alternative_tile) {
	RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(p_coords, p_alternative_tile));
	ERR_FAIL_COND(!tile_data);
	Variant value = tile_data->get(property);
	dummy_object->set(property, value);
	if (property_editor) {
		property_editor->update_property();
	}
}

void RTileDataDefaultEditor::_set_value(RTileSetAtlasSource *p_tile_set_atlas_source, Vector2 p_coords, int p_alternative_tile, Variant p_value) {
	RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(p_coords, p_alternative_tile));
	ERR_FAIL_COND(!tile_data);
	tile_data->set(property, p_value);
}

Variant RTileDataDefaultEditor::_get_value(RTileSetAtlasSource *p_tile_set_atlas_source, Vector2 p_coords, int p_alternative_tile) {
	RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(p_coords, p_alternative_tile));
	ERR_FAIL_COND_V(!tile_data, Variant());
	return tile_data->get(property);
}

void RTileDataDefaultEditor::_setup_undo_redo_action(RTileSetAtlasSource *p_tile_set_atlas_source, Map<RTileMapCell, Variant> p_previous_values, Variant p_new_value) {
	for (const Map<RTileMapCell, Variant>::Element *E = p_previous_values.front(); E; E = E->next()) {
		Vector2i coords = E->key().get_atlas_coords();
		undo_redo->add_undo_property(p_tile_set_atlas_source, vformat("%d:%d/%d/%s", coords.x, coords.y, E->key().alternative_tile, property), E->value());
		undo_redo->add_do_property(p_tile_set_atlas_source, vformat("%d:%d/%d/%s", coords.x, coords.y, E->key().alternative_tile, property), p_new_value);
	}
}

void RTileDataDefaultEditor::forward_draw_over_atlas(RTileAtlasView *p_tile_atlas_view, RTileSetAtlasSource *p_tile_set_atlas_source, CanvasItem *p_canvas_item, Transform2D p_transform) {
	if (drag_type == DRAG_TYPE_PAINT_RECT) {
		Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
		Color selection_color = Color().from_hsv(Math::fposmod(grid_color.get_h() + 0.5, 1.0), grid_color.get_s(), grid_color.get_v(), 1.0);

		p_canvas_item->draw_set_transform_matrix(p_transform);

		Rect2i rect;
		rect.set_position(p_tile_atlas_view->get_atlas_tile_coords_at_pos(drag_start_pos));
		rect.set_size(p_tile_atlas_view->get_atlas_tile_coords_at_pos(p_transform.affine_inverse().xform(p_canvas_item->get_local_mouse_position())) - rect.get_position());
		rect = Rect2i(Point2i(rect.get_position().x + MIN(rect.get_size().x, 0), rect.get_position().y + MIN(rect.get_size().y, 0)), Size2i(Math::abs(rect.get_size().x), Math::abs(rect.get_size().y)));

		Set<RTileMapCell> edited;
		Point2i rect_end = rect.get_position() + rect.get_size();
		for (int x = rect.get_position().x; x <= rect_end.x; x++) {
			for (int y = rect.get_position().y; y <= rect_end.y; y++) {
				Vector2i coords = Vector2i(x, y);
				coords = p_tile_set_atlas_source->get_tile_at_coords(coords);
				if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
					RTileMapCell cell;
					cell.source_id = 0;
					cell.set_atlas_coords(coords);
					cell.alternative_tile = 0;
					edited.insert(cell);
				}
			}
		}

		for (Set<RTileMapCell>::Element *E = edited.front(); E; E = E->next()) {
			Vector2i coords = E->get().get_atlas_coords();
			p_canvas_item->draw_rect(p_tile_set_atlas_source->get_tile_texture_region(coords), selection_color, false);
		}
		p_canvas_item->draw_set_transform_matrix(Transform2D());
	}
};

void RTileDataDefaultEditor::forward_draw_over_alternatives(RTileAtlasView *p_tile_atlas_view, RTileSetAtlasSource *p_tile_set_atlas_source, CanvasItem *p_canvas_item, Transform2D p_transform){

};

void RTileDataDefaultEditor::forward_painting_atlas_gui_input(RTileAtlasView *p_tile_atlas_view, RTileSetAtlasSource *p_tile_set_atlas_source, const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		if (drag_type == DRAG_TYPE_PAINT) {
			Vector<Vector2i> line = Geometry2D::bresenham_line(p_tile_atlas_view->get_atlas_tile_coords_at_pos(drag_last_pos), p_tile_atlas_view->get_atlas_tile_coords_at_pos(mm->get_position()));
			for (int i = 0; i < line.size(); i++) {
				Vector2i coords = p_tile_set_atlas_source->get_tile_at_coords(line[i]);
				if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
					RTileMapCell cell;
					cell.source_id = 0;
					cell.set_atlas_coords(coords);
					cell.alternative_tile = 0;
					if (!drag_modified.has(cell)) {
						drag_modified[cell] = _get_value(p_tile_set_atlas_source, coords, 0);
					}
					_set_value(p_tile_set_atlas_source, coords, 0, drag_painted_value);
				}
			}
			drag_last_pos = mm->get_position();
		}
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		if (mb->get_button_index() == BUTTON_LEFT) {
			if (mb->is_pressed()) {
				if (picker_button->is_pressed()) {
					Vector2i coords = p_tile_atlas_view->get_atlas_tile_coords_at_pos(mb->get_position());
					coords = p_tile_set_atlas_source->get_tile_at_coords(coords);
					if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
						_set_painted_value(p_tile_set_atlas_source, coords, 0);
						picker_button->set_pressed(false);
					}
				} else if (mb->get_control()) {
					drag_type = DRAG_TYPE_PAINT_RECT;
					drag_modified.clear();
					drag_painted_value = _get_painted_value();
					drag_start_pos = mb->get_position();
				} else {
					drag_type = DRAG_TYPE_PAINT;
					drag_modified.clear();
					drag_painted_value = _get_painted_value();
					Vector2i coords = p_tile_atlas_view->get_atlas_tile_coords_at_pos(mb->get_position());
					coords = p_tile_set_atlas_source->get_tile_at_coords(coords);
					if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
						RTileMapCell cell;
						cell.source_id = 0;
						cell.set_atlas_coords(coords);
						cell.alternative_tile = 0;
						drag_modified[cell] = _get_value(p_tile_set_atlas_source, coords, 0);
						_set_value(p_tile_set_atlas_source, coords, 0, drag_painted_value);
					}
					drag_last_pos = mb->get_position();
				}
			} else {
				if (drag_type == DRAG_TYPE_PAINT_RECT) {
					Rect2i rect;
					rect.set_position(p_tile_atlas_view->get_atlas_tile_coords_at_pos(drag_start_pos));
					rect.set_size(p_tile_atlas_view->get_atlas_tile_coords_at_pos(mb->get_position()) - rect.get_position());
					rect = Rect2i(Point2i(rect.get_position().x + MIN(rect.get_size().x, 0), rect.get_position().y + MIN(rect.get_size().y, 0)), Size2i(Math::abs(rect.get_size().x), Math::abs(rect.get_size().y)));

					drag_modified.clear();
					Point2i rect_end = rect.get_position() + rect.get_size();
					for (int x = rect.get_position().x; x <= rect_end.x; x++) {
						for (int y = rect.get_position().y; y <= rect_end.y; y++) {
							Vector2i coords = Vector2i(x, y);
							coords = p_tile_set_atlas_source->get_tile_at_coords(coords);
							if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
								RTileMapCell cell;
								cell.source_id = 0;
								cell.set_atlas_coords(coords);
								cell.alternative_tile = 0;
								drag_modified[cell] = _get_value(p_tile_set_atlas_source, coords, 0);
							}
						}
					}
					undo_redo->create_action(TTR("Painting Tiles Property"));
					_setup_undo_redo_action(p_tile_set_atlas_source, drag_modified, drag_painted_value);
					undo_redo->commit_action();
					drag_type = DRAG_TYPE_NONE;
				} else if (drag_type == DRAG_TYPE_PAINT) {
					undo_redo->create_action(TTR("Painting Tiles Property"));
					_setup_undo_redo_action(p_tile_set_atlas_source, drag_modified, drag_painted_value);
					undo_redo->commit_action();
					drag_type = DRAG_TYPE_NONE;
				}
			}
		}
	}
}

void RTileDataDefaultEditor::forward_painting_alternatives_gui_input(RTileAtlasView *p_tile_atlas_view, RTileSetAtlasSource *p_tile_set_atlas_source, const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		if (drag_type == DRAG_TYPE_PAINT) {
			Vector3i tile = p_tile_atlas_view->get_alternative_tile_at_pos(mm->get_position());
			Vector2i coords = Vector2i(tile.x, tile.y);
			int alternative_tile = tile.z;

			if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
				RTileMapCell cell;
				cell.source_id = 0;
				cell.set_atlas_coords(coords);
				cell.alternative_tile = alternative_tile;
				if (!drag_modified.has(cell)) {
					drag_modified[cell] = _get_value(p_tile_set_atlas_source, coords, alternative_tile);
				}
				_set_value(p_tile_set_atlas_source, coords, alternative_tile, drag_painted_value);
			}

			drag_last_pos = mm->get_position();
		}
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		if (mb->get_button_index() == BUTTON_LEFT) {
			if (mb->is_pressed()) {
				if (picker_button->is_pressed()) {
					Vector3i tile = p_tile_atlas_view->get_alternative_tile_at_pos(mb->get_position());
					Vector2i coords = Vector2i(tile.x, tile.y);
					int alternative_tile = tile.z;
					if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
						_set_painted_value(p_tile_set_atlas_source, coords, alternative_tile);
						picker_button->set_pressed(false);
					}
				} else {
					drag_type = DRAG_TYPE_PAINT;
					drag_modified.clear();
					drag_painted_value = _get_painted_value();

					Vector3i tile = p_tile_atlas_view->get_alternative_tile_at_pos(mb->get_position());
					Vector2i coords = Vector2i(tile.x, tile.y);
					int alternative_tile = tile.z;

					if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
						RTileMapCell cell;
						cell.source_id = 0;
						cell.set_atlas_coords(coords);
						cell.alternative_tile = alternative_tile;
						drag_modified[cell] = _get_value(p_tile_set_atlas_source, coords, alternative_tile);
						_set_value(p_tile_set_atlas_source, coords, alternative_tile, drag_painted_value);
					}
					drag_last_pos = mb->get_position();
				}
			} else {
				undo_redo->create_action(TTR("Painting Tiles Property"));
				_setup_undo_redo_action(p_tile_set_atlas_source, drag_modified, drag_painted_value);
				undo_redo->commit_action();
				drag_type = DRAG_TYPE_NONE;
			}
		}
	}
}

void RTileDataDefaultEditor::draw_over_tile(CanvasItem *p_canvas_item, Transform2D p_transform, RTileMapCell p_cell, bool p_selected) {
	RTileData *tile_data = _get_tile_data(p_cell);
	ERR_FAIL_COND(!tile_data);

	bool valid;
	Variant value = tile_data->get(property, &valid);
	if (!valid) {
		return;
	}

	if (value.get_type() == Variant::BOOL) {
		Ref<Texture> texture = (bool)value ? tile_bool_checked : tile_bool_unchecked;
		int size = MIN(tile_set->get_tile_size().x, tile_set->get_tile_size().y) / 3;
		Rect2 rect = p_transform.xform(Rect2(Vector2(-size / 2, -size / 2), Vector2(size, size)));
		p_canvas_item->draw_texture_rect(texture, rect);
	} else if (value.get_type() == Variant::COLOR) {
		int size = MIN(tile_set->get_tile_size().x, tile_set->get_tile_size().y) / 3;
		Rect2 rect = p_transform.xform(Rect2(Vector2(-size / 2, -size / 2), Vector2(size, size)));
		p_canvas_item->draw_rect(rect, value);
	} else {
		Ref<Font> font = RTileSetEditor::get_singleton()->get_font(("bold"), ("EditorFonts"));
		String text;
		switch (value.get_type()) {
			case Variant::INT:
				text = vformat("%d", value);
				break;
			case Variant::REAL:
				text = vformat("%.2f", value);
				break;
			case Variant::STRING:
				//case Variant::STRING_NAME:
				text = value;
				break;
			default:
				return;
				break;
		}

		Color color = Color(1, 1, 1);
		if (p_selected) {
			Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
			Color selection_color = Color().from_hsv(Math::fposmod(grid_color.get_h() + 0.5, 1.0), grid_color.get_s(), grid_color.get_v(), 1.0);
			selection_color.set_hsv(selection_color.get_h(), selection_color.get_s(), 0.9);

			color = selection_color;
		} else if (is_visible_in_tree()) {
			Variant painted_value = _get_painted_value();
			bool equal = (painted_value.get_type() == Variant::REAL && value.get_type() == Variant::REAL) ? Math::is_equal_approx(float(painted_value), float(value)) : painted_value == value;
			if (equal) {
				color = Color(0.7, 0.7, 0.7);
			}
		}

		Vector2 string_size = font->get_string_size(text);
		p_canvas_item->draw_string(font, p_transform.get_origin() + Vector2i(-string_size.x / 2, string_size.y / 2), text, color);
	}
}

void RTileDataDefaultEditor::setup_property_editor(Variant::Type p_type, String p_property, String p_label, Variant p_default_value) {
	ERR_FAIL_COND_MSG(!property.empty(), "Cannot setup RTileDataDefaultEditor twice");
	property = p_property;

	// Update everything.
	if (property_editor) {
		property_editor->queue_delete();
	}

	// Update the dummy object.
	dummy_object->add_dummy_property(p_property);

	// Get the default value for the type.
	if (p_default_value == Variant()) {
		Variant::CallError error;
		Variant painted_value;
		Variant::construct(p_type, nullptr, 0, error);
		dummy_object->set(p_property, painted_value);
	} else {
		dummy_object->set(p_property, p_default_value);
	}

	// Create and setup the property editor.
	property_editor = get_editor_for_property(dummy_object, p_type, p_property, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT);
	property_editor->set_object_and_property(dummy_object, p_property);
	if (p_label.empty()) {
		property_editor->set_label(p_property);
	} else {
		property_editor->set_label(p_label);
	}
	property_editor->connect("property_changed", this, "_property_value_changed");
	property_editor->update_property();
	add_child(property_editor);
}

struct EditorPropertyRangeHint {
	bool angle_in_degrees = false;
	bool greater = true;
	bool lesser = true;
	double min = -99999;
	double max = 99999;
	double step = 0;
	String suffix;
	bool exp_range = false;
	bool hide_slider = true;
	bool radians = false;
};

static EditorPropertyRangeHint _parse_range_hint(PropertyHint p_hint, const String &p_hint_text, double p_default_step) {
	EditorPropertyRangeHint hint;
	hint.step = p_default_step;
	bool degrees = false;
	if (p_hint == PROPERTY_HINT_RANGE && p_hint_text.get_slice_count(",") >= 2) {
		hint.greater = false; //if using ranged, assume false by default
		hint.lesser = false;

		hint.min = p_hint_text.get_slice(",", 0).to_float();
		hint.max = p_hint_text.get_slice(",", 1).to_float();
		if (p_hint_text.get_slice_count(",") >= 3) {
			hint.step = p_hint_text.get_slice(",", 2).to_float();
		}
		hint.hide_slider = false;
		for (int i = 2; i < p_hint_text.get_slice_count(","); i++) {
			String slice = p_hint_text.get_slice(",", i).strip_edges();
			if (slice == "radians") {
				hint.radians = true;
			} else if (slice == "degrees") {
				degrees = true;
			} else if (slice == "or_greater") {
				hint.greater = true;
			} else if (slice == "or_lesser") {
				hint.lesser = true;
			} else if (slice == "noslider") {
				hint.hide_slider = true;
			} else if (slice == "exp") {
				hint.exp_range = true;
			} else if (slice.begins_with("suffix:")) {
				hint.suffix = " " + slice.replace_first("suffix:", "").strip_edges();
			}
		}
	}

	//if ((hint.radians || degrees) && hint.suffix.empty()) {
	//hint.suffix = U"\u00B0";
	//}

	return hint;
}

EditorProperty *RTileDataDefaultEditor::get_editor_for_property(Object *p_object, const Variant::Type p_type, const String &p_path, const PropertyHint p_hint, const String &p_hint_text, const uint32_t p_usage, const bool p_wide) {
	double default_float_step = EDITOR_GET("interface/inspector/default_float_step");

	switch (p_type) {
		// atomic types
		case Variant::NIL: {
			EditorPropertyNil *editor = memnew(EditorPropertyNil);
			return editor;
		} break;
		case Variant::BOOL: {
			EditorPropertyCheck *editor = memnew(EditorPropertyCheck);
			return editor;
		} break;
		case Variant::INT: {
			if (p_hint == PROPERTY_HINT_ENUM) {
				EditorPropertyEnum *editor = memnew(EditorPropertyEnum);
				Vector<String> options = p_hint_text.split(",");
				editor->setup(options);
				return editor;

			} else if (p_hint == PROPERTY_HINT_FLAGS) {
				EditorPropertyFlags *editor = memnew(EditorPropertyFlags);
				Vector<String> options = p_hint_text.split(",");
				editor->setup(options);
				return editor;

			} else if (p_hint == PROPERTY_HINT_LAYERS_2D_PHYSICS ||
					p_hint == PROPERTY_HINT_LAYERS_2D_RENDER ||
					p_hint == PROPERTY_HINT_LAYERS_3D_PHYSICS ||
					p_hint == PROPERTY_HINT_LAYERS_3D_RENDER) {
				EditorPropertyLayers::LayerType lt = EditorPropertyLayers::LAYER_RENDER_2D;
				switch (p_hint) {
					case PROPERTY_HINT_LAYERS_2D_RENDER:
						lt = EditorPropertyLayers::LAYER_RENDER_2D;
						break;
					case PROPERTY_HINT_LAYERS_2D_PHYSICS:
						lt = EditorPropertyLayers::LAYER_PHYSICS_2D;
						break;
					case PROPERTY_HINT_LAYERS_3D_RENDER:
						lt = EditorPropertyLayers::LAYER_RENDER_3D;
						break;
					case PROPERTY_HINT_LAYERS_3D_PHYSICS:
						lt = EditorPropertyLayers::LAYER_PHYSICS_3D;
						break;
					default: {
					} //compiler could be smarter here and realize this can't happen
				}
				EditorPropertyLayers *editor = memnew(EditorPropertyLayers);
				editor->setup(lt);
				return editor;
			} else if (p_hint == PROPERTY_HINT_OBJECT_ID) {
				EditorPropertyObjectID *editor = memnew(EditorPropertyObjectID);
				editor->setup(p_hint_text);
				return editor;

			} else {
				EditorPropertyInteger *editor = memnew(EditorPropertyInteger);

				EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, 1);

				editor->setup(hint.min, hint.max, hint.step, hint.greater, hint.lesser);

				return editor;
			}
		} break;
		case Variant::REAL: {
			if (p_hint == PROPERTY_HINT_EXP_EASING) {
				EditorPropertyEasing *editor = memnew(EditorPropertyEasing);
				bool full = true;
				bool flip = false;
				Vector<String> hints = p_hint_text.split(",");
				for (int i = 0; i < hints.size(); i++) {
					String h = hints[i].strip_edges();
					if (h == "attenuation") {
						flip = true;
					}
					if (h == "inout") {
						full = true;
					}
				}

				editor->setup(full, flip);
				return editor;

			} else {
				EditorPropertyFloat *editor = memnew(EditorPropertyFloat);

				EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
				editor->setup(hint.min, hint.max, hint.step, hint.hide_slider, hint.exp_range, hint.greater, hint.lesser);

				return editor;
			}
		} break;
		case Variant::STRING: {
			if (p_hint == PROPERTY_HINT_ENUM || p_hint == PROPERTY_HINT_ENUM) {
				EditorPropertyTextEnum *editor = memnew(EditorPropertyTextEnum);
				Vector<String> options = p_hint_text.split(",", false);
				editor->setup(options);
				return editor;
			} else if (p_hint == PROPERTY_HINT_MULTILINE_TEXT) {
				EditorPropertyMultilineText *editor = memnew(EditorPropertyMultilineText);
				return editor;
			} else if (p_hint == PROPERTY_HINT_TYPE_STRING) {
				EditorPropertyClassName *editor = memnew(EditorPropertyClassName);
				editor->setup("Object", p_hint_text);
				return editor;
			} else if (p_hint == PROPERTY_HINT_DIR || p_hint == PROPERTY_HINT_FILE || p_hint == PROPERTY_HINT_SAVE_FILE || p_hint == PROPERTY_HINT_GLOBAL_DIR || p_hint == PROPERTY_HINT_GLOBAL_FILE) {
				Vector<String> extensions = p_hint_text.split(",");
				bool global = p_hint == PROPERTY_HINT_GLOBAL_DIR || p_hint == PROPERTY_HINT_GLOBAL_FILE;
				bool folder = p_hint == PROPERTY_HINT_DIR || p_hint == PROPERTY_HINT_GLOBAL_DIR;
				bool save = p_hint == PROPERTY_HINT_SAVE_FILE;
				EditorPropertyPath *editor = memnew(EditorPropertyPath);
				editor->setup(extensions, folder, global);
				if (save) {
					editor->set_save_mode();
				}
				return editor;
			} else if (p_hint == PROPERTY_HINT_METHOD_OF_VARIANT_TYPE ||
					p_hint == PROPERTY_HINT_METHOD_OF_BASE_TYPE ||
					p_hint == PROPERTY_HINT_METHOD_OF_INSTANCE ||
					p_hint == PROPERTY_HINT_METHOD_OF_SCRIPT ||
					p_hint == PROPERTY_HINT_PROPERTY_OF_VARIANT_TYPE ||
					p_hint == PROPERTY_HINT_PROPERTY_OF_BASE_TYPE ||
					p_hint == PROPERTY_HINT_PROPERTY_OF_INSTANCE ||
					p_hint == PROPERTY_HINT_PROPERTY_OF_SCRIPT) {
				EditorPropertyMember *editor = memnew(EditorPropertyMember);

				EditorPropertyMember::Type type = EditorPropertyMember::MEMBER_METHOD_OF_BASE_TYPE;
				switch (p_hint) {
					case PROPERTY_HINT_METHOD_OF_BASE_TYPE:
						type = EditorPropertyMember::MEMBER_METHOD_OF_BASE_TYPE;
						break;
					case PROPERTY_HINT_METHOD_OF_INSTANCE:
						type = EditorPropertyMember::MEMBER_METHOD_OF_INSTANCE;
						break;
					case PROPERTY_HINT_METHOD_OF_SCRIPT:
						type = EditorPropertyMember::MEMBER_METHOD_OF_SCRIPT;
						break;
					case PROPERTY_HINT_PROPERTY_OF_VARIANT_TYPE:
						type = EditorPropertyMember::MEMBER_PROPERTY_OF_VARIANT_TYPE;
						break;
					case PROPERTY_HINT_PROPERTY_OF_BASE_TYPE:
						type = EditorPropertyMember::MEMBER_PROPERTY_OF_BASE_TYPE;
						break;
					case PROPERTY_HINT_PROPERTY_OF_INSTANCE:
						type = EditorPropertyMember::MEMBER_PROPERTY_OF_INSTANCE;
						break;
					case PROPERTY_HINT_PROPERTY_OF_SCRIPT:
						type = EditorPropertyMember::MEMBER_PROPERTY_OF_SCRIPT;
						break;
					default: {
					}
				}
				editor->setup(type, p_hint_text);
				return editor;

			} else {
				EditorPropertyText *editor = memnew(EditorPropertyText);
				if (p_hint == PROPERTY_HINT_PLACEHOLDER_TEXT) {
					editor->set_placeholder(p_hint_text);
				}
				return editor;
			}
		} break;

			// math types

		case Variant::VECTOR2: {
			EditorPropertyVector2 *editor = memnew(EditorPropertyVector2());

			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider);
			return editor;

		} break;
		case Variant::RECT2: {
			EditorPropertyRect2 *editor = memnew(EditorPropertyRect2());
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider);
			return editor;
		} break;
		case Variant::VECTOR3: {
			EditorPropertyVector3 *editor = memnew(EditorPropertyVector3());
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider);
			return editor;

		} break;
		case Variant::TRANSFORM2D: {
			EditorPropertyTransform2D *editor = memnew(EditorPropertyTransform2D);
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider);
			return editor;
		} break;
		case Variant::PLANE: {
			EditorPropertyPlane *editor = memnew(EditorPropertyPlane());
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider);
			return editor;
		} break;
		case Variant::QUAT: {
			EditorPropertyQuat *editor = memnew(EditorPropertyQuat);
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider);
			return editor;
		} break;
		case Variant::AABB: {
			EditorPropertyAABB *editor = memnew(EditorPropertyAABB);
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider);
			return editor;
		} break;
		case Variant::BASIS: {
			EditorPropertyBasis *editor = memnew(EditorPropertyBasis);
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider);
			return editor;
		} break;
		case Variant::TRANSFORM: {
			EditorPropertyTransform *editor = memnew(EditorPropertyTransform);
			EditorPropertyRangeHint hint = _parse_range_hint(p_hint, p_hint_text, default_float_step);
			editor->setup(hint.min, hint.max, hint.step, hint.hide_slider);
			return editor;

		} break;

		// misc types
		case Variant::COLOR: {
			EditorPropertyColor *editor = memnew(EditorPropertyColor);
			editor->setup(p_hint != PROPERTY_HINT_COLOR_NO_ALPHA);
			return editor;
		} break;
		case Variant::NODE_PATH: {
			EditorPropertyNodePath *editor = memnew(EditorPropertyNodePath);
			if (p_hint == PROPERTY_HINT_NODE_PATH_TO_EDITED_NODE && !p_hint_text.empty()) {
				editor->setup(p_hint_text, Vector<StringName>(), (p_usage & PROPERTY_USAGE_NODE_PATH_FROM_SCENE_ROOT));
			}
			if (p_hint == PROPERTY_HINT_NODE_PATH_VALID_TYPES && !p_hint_text.empty()) {
				Vector<String> types = p_hint_text.split(",", false);
				Vector<StringName> sn = Variant(types); //convert via variant
				editor->setup(NodePath(), sn, (p_usage & PROPERTY_USAGE_NODE_PATH_FROM_SCENE_ROOT));
			}
			return editor;

		} break;
		case Variant::_RID: {
			EditorPropertyRID *editor = memnew(EditorPropertyRID);
			return editor;
		} break;
		case Variant::OBJECT: {
			EditorPropertyResource *editor = memnew(EditorPropertyResource);
			editor->setup(p_object, p_path, p_hint == PROPERTY_HINT_RESOURCE_TYPE ? p_hint_text : "Resource");

			if (p_hint == PROPERTY_HINT_RESOURCE_TYPE) {
				String open_in_new = EDITOR_GET("interface/inspector/resources_to_open_in_new_inspector");
				for (int i = 0; i < open_in_new.get_slice_count(","); i++) {
					String type = open_in_new.get_slicec(',', i).strip_edges();
					for (int j = 0; j < p_hint_text.get_slice_count(","); j++) {
						String inherits = p_hint_text.get_slicec(',', j);
						if (ClassDB::is_parent_class(inherits, type)) {
							editor->set_use_sub_inspector(false);
						}
					}
				}
			}

			return editor;

		} break;
		case Variant::DICTIONARY: {
			EditorPropertyDictionary *editor = memnew(EditorPropertyDictionary);
			return editor;
		} break;
		case Variant::ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::ARRAY, p_hint_text);
			return editor;
		} break;
		case Variant::POOL_BYTE_ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::POOL_BYTE_ARRAY);
			return editor;
		} break;
		case Variant::POOL_INT_ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::POOL_INT_ARRAY);
			return editor;
		} break;
		case Variant::POOL_REAL_ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::POOL_REAL_ARRAY);
			return editor;
		} break;
		case Variant::POOL_STRING_ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::POOL_STRING_ARRAY);
			return editor;
		} break;
		case Variant::POOL_VECTOR2_ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::POOL_VECTOR2_ARRAY);
			return editor;
		} break;
		case Variant::POOL_VECTOR3_ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::POOL_VECTOR3_ARRAY);
			return editor;
		} break;
		case Variant::POOL_COLOR_ARRAY: {
			EditorPropertyArray *editor = memnew(EditorPropertyArray);
			editor->setup(Variant::POOL_COLOR_ARRAY);
			return editor;
		} break;
		default: {
		}
	}

	return nullptr;
}

void RTileDataDefaultEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_THEME_CHANGED:
			picker_button->set_icon(get_icon(("ColorPick"), ("EditorIcons")));
			tile_bool_checked = get_icon(("TileChecked"), ("EditorIcons"));
			tile_bool_unchecked = get_icon(("TileUnchecked"), ("EditorIcons"));
			break;
		default:
			break;
	}
}

RTileDataDefaultEditor::RTileDataDefaultEditor() {
	label = memnew(Label);
	label->set_text(TTR("Painting:"));
	add_child(label);

	toolbar->add_child(memnew(VSeparator));

	picker_button = memnew(Button);
	picker_button->set_flat(true);
	picker_button->set_toggle_mode(true);
	picker_button->set_shortcut(ED_SHORTCUT("tiles_editor/picker", "Picker", KEY_P));
	toolbar->add_child(picker_button);
}

RTileDataDefaultEditor::~RTileDataDefaultEditor() {
	toolbar->queue_delete();
	memdelete(dummy_object);
}

void RTileDataTextureOffsetEditor::draw_over_tile(CanvasItem *p_canvas_item, Transform2D p_transform, RTileMapCell p_cell, bool p_selected) {
	RTileData *tile_data = _get_tile_data(p_cell);
	ERR_FAIL_COND(!tile_data);

	Vector2i tile_set_tile_size = tile_set->get_tile_size();
	Color color = Color(1.0, 0.0, 0.0);
	if (p_selected) {
		Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
		Color selection_color = Color().from_hsv(Math::fposmod(grid_color.get_h() + 0.5, 1.0), grid_color.get_s(), grid_color.get_v(), 1.0);
		color = selection_color;
	}
	Transform2D tile_xform;
	tile_xform.set_scale(tile_set_tile_size);
	tile_set->draw_tile_shape(p_canvas_item, p_transform * tile_xform, color);
}

void RTileDataPositionEditor::draw_over_tile(CanvasItem *p_canvas_item, Transform2D p_transform, RTileMapCell p_cell, bool p_selected) {
	RTileData *tile_data = _get_tile_data(p_cell);
	ERR_FAIL_COND(!tile_data);

	bool valid;
	Variant value = tile_data->get(property, &valid);
	if (!valid) {
		return;
	}
	ERR_FAIL_COND(value.get_type() != Variant::VECTOR2 && value.get_type() != Variant::VECTOR2);

	Color color = Color(1.0, 1.0, 1.0);
	if (p_selected) {
		Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
		Color selection_color = Color().from_hsv(Math::fposmod(grid_color.get_h() + 0.5, 1.0), grid_color.get_s(), grid_color.get_v(), 1.0);
		color = selection_color;
	}
	Ref<Texture> position_icon = RTileSetEditor::get_singleton()->get_icon(("EditorPosition"), ("EditorIcons"));
	p_canvas_item->draw_texture(position_icon, p_transform.xform(Vector2(value)) - position_icon->get_size() / 2, color);
}

void RTileDataYSortEditor::draw_over_tile(CanvasItem *p_canvas_item, Transform2D p_transform, RTileMapCell p_cell, bool p_selected) {
	RTileData *tile_data = _get_tile_data(p_cell);
	ERR_FAIL_COND(!tile_data);

	Color color = Color(1.0, 1.0, 1.0);
	if (p_selected) {
		Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
		Color selection_color = Color().from_hsv(Math::fposmod(grid_color.get_h() + 0.5, 1.0), grid_color.get_s(), grid_color.get_v(), 1.0);
		color = selection_color;
	}
	Ref<Texture> position_icon = RTileSetEditor::get_singleton()->get_icon(("EditorPosition"), ("EditorIcons"));
	p_canvas_item->draw_texture(position_icon, p_transform.xform(Vector2(0, tile_data->get_y_sort_origin())) - position_icon->get_size() / 2, color);
}

void RTileDataOcclusionShapeEditor::draw_over_tile(CanvasItem *p_canvas_item, Transform2D p_transform, RTileMapCell p_cell, bool p_selected) {
	RTileData *tile_data = _get_tile_data(p_cell);
	ERR_FAIL_COND(!tile_data);

	Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
	Color selection_color = Color().from_hsv(Math::fposmod(grid_color.get_h() + 0.5, 1.0), grid_color.get_s(), grid_color.get_v(), 1.0);
	Color color = grid_color.darkened(0.2);
	if (p_selected) {
		color = selection_color.darkened(0.2);
	}
	color.a *= 0.5;

	Vector<Color> debug_occlusion_color;
	debug_occlusion_color.push_back(color);

	VisualServer::get_singleton()->canvas_item_add_set_transform(p_canvas_item->get_canvas_item(), p_transform);
	Ref<OccluderPolygon2D> occluder = tile_data->get_occluder(occlusion_layer);
	if (occluder.is_valid() && occluder->get_polygon().size() >= 3) {
		p_canvas_item->draw_polygon(Variant(occluder->get_polygon()), debug_occlusion_color);
	}
	VisualServer::get_singleton()->canvas_item_add_set_transform(p_canvas_item->get_canvas_item(), Transform2D());
}

Variant RTileDataOcclusionShapeEditor::_get_painted_value() {
	Ref<OccluderPolygon2D> occluder_polygon;
	occluder_polygon.instance();
	if (polygon_editor->get_polygon_count() >= 1) {
		occluder_polygon->set_polygon(polygon_editor->get_polygon_poolvector(0));
	}
	return occluder_polygon;
}

void RTileDataOcclusionShapeEditor::_set_painted_value(RTileSetAtlasSource *p_tile_set_atlas_source, Vector2 p_coords, int p_alternative_tile) {
	RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(p_coords, p_alternative_tile));
	ERR_FAIL_COND(!tile_data);

	Ref<OccluderPolygon2D> occluder_polygon = tile_data->get_occluder(occlusion_layer);
	polygon_editor->clear_polygons();
	if (occluder_polygon.is_valid()) {
		polygon_editor->add_polygon_poolvector(occluder_polygon->get_polygon());
	}
	polygon_editor->set_background(p_tile_set_atlas_source->get_texture(), p_tile_set_atlas_source->get_tile_texture_region(p_coords), p_tile_set_atlas_source->get_tile_effective_texture_offset(p_coords, p_alternative_tile), tile_data->get_flip_h(), tile_data->get_flip_v(), tile_data->get_transpose(), tile_data->get_modulate());
}

void RTileDataOcclusionShapeEditor::_set_value(RTileSetAtlasSource *p_tile_set_atlas_source, Vector2 p_coords, int p_alternative_tile, Variant p_value) {
	RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(p_coords, p_alternative_tile));
	ERR_FAIL_COND(!tile_data);
	Ref<OccluderPolygon2D> occluder_polygon = p_value;
	tile_data->set_occluder(occlusion_layer, occluder_polygon);

	polygon_editor->set_background(p_tile_set_atlas_source->get_texture(), p_tile_set_atlas_source->get_tile_texture_region(p_coords), p_tile_set_atlas_source->get_tile_effective_texture_offset(p_coords, p_alternative_tile), tile_data->get_flip_h(), tile_data->get_flip_v(), tile_data->get_transpose(), tile_data->get_modulate());
}

Variant RTileDataOcclusionShapeEditor::_get_value(RTileSetAtlasSource *p_tile_set_atlas_source, Vector2 p_coords, int p_alternative_tile) {
	RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(p_coords, p_alternative_tile));
	ERR_FAIL_COND_V(!tile_data, Variant());
	return tile_data->get_occluder(occlusion_layer);
}

void RTileDataOcclusionShapeEditor::_setup_undo_redo_action(RTileSetAtlasSource *p_tile_set_atlas_source, Map<RTileMapCell, Variant> p_previous_values, Variant p_new_value) {
	for (Map<RTileMapCell, Variant>::Element *E = p_previous_values.front(); E; E = E->next()) {
		Vector2i coords = E->key().get_atlas_coords();
		undo_redo->add_undo_property(p_tile_set_atlas_source, vformat("%d:%d/%d/occlusion_layer_%d/polygon", coords.x, coords.y, E->key().alternative_tile, occlusion_layer), E->value());
		undo_redo->add_do_property(p_tile_set_atlas_source, vformat("%d:%d/%d/occlusion_layer_%d/polygon", coords.x, coords.y, E->key().alternative_tile, occlusion_layer), p_new_value);
	}
}

void RTileDataOcclusionShapeEditor::_tile_set_changed() {
	polygon_editor->set_tile_set(tile_set);
}

void RTileDataOcclusionShapeEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			polygon_editor->set_polygons_color(get_tree()->get_debug_collisions_color());
			break;
		default:
			break;
	}
}

RTileDataOcclusionShapeEditor::RTileDataOcclusionShapeEditor() {
	polygon_editor = memnew(RGenericTilePolygonEditor);
	add_child(polygon_editor);
}

void RTileDataCollisionEditor::_property_value_changed(StringName p_property, Variant p_value, StringName p_field) {
	dummy_object->set(p_property, p_value);
}

void RTileDataCollisionEditor::_polygons_changed() {
	// Update the dummy object properties and their editors.
	for (int i = 0; i < polygon_editor->get_polygon_count(); i++) {
		StringName one_way_property = vformat("polygon_%d_one_way", i);
		StringName one_way_margin_property = vformat("polygon_%d_one_way_margin", i);

		if (!dummy_object->has_dummy_property(one_way_property)) {
			dummy_object->add_dummy_property(one_way_property);
			dummy_object->set(one_way_property, false);
		}

		if (!dummy_object->has_dummy_property(one_way_margin_property)) {
			dummy_object->add_dummy_property(one_way_margin_property);
			dummy_object->set(one_way_margin_property, 1.0);
		}

		if (!property_editors.has(one_way_property)) {
			EditorProperty *one_way_property_editor = get_editor_for_property(dummy_object, Variant::BOOL, one_way_property, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT);
			one_way_property_editor->set_object_and_property(dummy_object, one_way_property);
			one_way_property_editor->set_label(one_way_property);
			one_way_property_editor->connect("property_changed", this, "_property_value_changed");
			one_way_property_editor->update_property();
			add_child(one_way_property_editor);
			property_editors[one_way_property] = one_way_property_editor;
		}

		if (!property_editors.has(one_way_margin_property)) {
			EditorProperty *one_way_margin_property_editor = get_editor_for_property(dummy_object, Variant::REAL, one_way_margin_property, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT);
			one_way_margin_property_editor->set_object_and_property(dummy_object, one_way_margin_property);
			one_way_margin_property_editor->set_label(one_way_margin_property);
			one_way_margin_property_editor->connect("property_changed", this, "_property_value_changed");
			one_way_margin_property_editor->update_property();
			add_child(one_way_margin_property_editor);
			property_editors[one_way_margin_property] = one_way_margin_property_editor;
		}
	}

	// Remove unneeded properties and their editors.
	for (int i = polygon_editor->get_polygon_count(); dummy_object->has_dummy_property(vformat("polygon_%d_one_way", i)); i++) {
		dummy_object->remove_dummy_property(vformat("polygon_%d_one_way", i));
	}
	for (int i = polygon_editor->get_polygon_count(); dummy_object->has_dummy_property(vformat("polygon_%d_one_way_margin", i)); i++) {
		dummy_object->remove_dummy_property(vformat("polygon_%d_one_way_margin", i));
	}
	for (int i = polygon_editor->get_polygon_count(); property_editors.has(vformat("polygon_%d_one_way", i)); i++) {
		property_editors[vformat("polygon_%d_one_way", i)]->queue_delete();
		property_editors.erase(vformat("polygon_%d_one_way", i));
	}
	for (int i = polygon_editor->get_polygon_count(); property_editors.has(vformat("polygon_%d_one_way_margin", i)); i++) {
		property_editors[vformat("polygon_%d_one_way_margin", i)]->queue_delete();
		property_editors.erase(vformat("polygon_%d_one_way_margin", i));
	}
}

Variant RTileDataCollisionEditor::_get_painted_value() {
	Dictionary dict;
	dict["linear_velocity"] = dummy_object->get("linear_velocity");
	dict["angular_velocity"] = dummy_object->get("angular_velocity");
	Array array;
	for (int i = 0; i < polygon_editor->get_polygon_count(); i++) {
		ERR_FAIL_COND_V(polygon_editor->get_polygon(i).size() < 3, Variant());
		Dictionary polygon_dict;
		polygon_dict["points"] = polygon_editor->get_polygon(i);
		polygon_dict["one_way"] = dummy_object->get(vformat("polygon_%d_one_way", i));
		polygon_dict["one_way_margin"] = dummy_object->get(vformat("polygon_%d_one_way_margin", i));
		array.push_back(polygon_dict);
	}
	dict["polygons"] = array;

	return dict;
}

void RTileDataCollisionEditor::_set_painted_value(RTileSetAtlasSource *p_tile_set_atlas_source, Vector2 p_coords, int p_alternative_tile) {
	RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(p_coords, p_alternative_tile));
	ERR_FAIL_COND(!tile_data);

	polygon_editor->clear_polygons();
	for (int i = 0; i < tile_data->get_collision_polygons_count(physics_layer); i++) {
		Vector<Vector2> polygon = tile_data->get_collision_polygon_points(physics_layer, i);
		if (polygon.size() >= 3) {
			polygon_editor->add_polygon(polygon);
		}
	}

	_polygons_changed();
	dummy_object->set("linear_velocity", tile_data->get_constant_linear_velocity(physics_layer));
	dummy_object->set("angular_velocity", tile_data->get_constant_angular_velocity(physics_layer));
	for (int i = 0; i < tile_data->get_collision_polygons_count(physics_layer); i++) {
		dummy_object->set(vformat("polygon_%d_one_way", i), tile_data->is_collision_polygon_one_way(physics_layer, i));
		dummy_object->set(vformat("polygon_%d_one_way_margin", i), tile_data->get_collision_polygon_one_way_margin(physics_layer, i));
	}
	for (Map<StringName, EditorProperty *>::Element *E = property_editors.front(); E; E = E->next()) {
		E->value()->update_property();
	}

	polygon_editor->set_background(p_tile_set_atlas_source->get_texture(), p_tile_set_atlas_source->get_tile_texture_region(p_coords), p_tile_set_atlas_source->get_tile_effective_texture_offset(p_coords, p_alternative_tile), tile_data->get_flip_h(), tile_data->get_flip_v(), tile_data->get_transpose(), tile_data->get_modulate());
}

void RTileDataCollisionEditor::_set_value(RTileSetAtlasSource *p_tile_set_atlas_source, Vector2 p_coords, int p_alternative_tile, Variant p_value) {
	RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(p_coords, p_alternative_tile));
	ERR_FAIL_COND(!tile_data);

	Dictionary dict = p_value;
	tile_data->set_constant_linear_velocity(physics_layer, dict["linear_velocity"]);
	tile_data->set_constant_angular_velocity(physics_layer, dict["angular_velocity"]);
	Array array = dict["polygons"];
	tile_data->set_collision_polygons_count(physics_layer, array.size());
	for (int i = 0; i < array.size(); i++) {
		Dictionary polygon_dict = array[i];
		tile_data->set_collision_polygon_points(physics_layer, i, polygon_dict["points"]);
		tile_data->set_collision_polygon_one_way(physics_layer, i, polygon_dict["one_way"]);
		tile_data->set_collision_polygon_one_way_margin(physics_layer, i, polygon_dict["one_way_margin"]);
	}

	polygon_editor->set_background(p_tile_set_atlas_source->get_texture(), p_tile_set_atlas_source->get_tile_texture_region(p_coords), p_tile_set_atlas_source->get_tile_effective_texture_offset(p_coords, p_alternative_tile), tile_data->get_flip_h(), tile_data->get_flip_v(), tile_data->get_transpose(), tile_data->get_modulate());
}

Variant RTileDataCollisionEditor::_get_value(RTileSetAtlasSource *p_tile_set_atlas_source, Vector2 p_coords, int p_alternative_tile) {
	RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(p_coords, p_alternative_tile));
	ERR_FAIL_COND_V(!tile_data, Variant());

	Dictionary dict;
	dict["linear_velocity"] = tile_data->get_constant_linear_velocity(physics_layer);
	dict["angular_velocity"] = tile_data->get_constant_angular_velocity(physics_layer);
	Array array;
	for (int i = 0; i < tile_data->get_collision_polygons_count(physics_layer); i++) {
		Dictionary polygon_dict;
		polygon_dict["points"] = tile_data->get_collision_polygon_points(physics_layer, i);
		polygon_dict["one_way"] = tile_data->is_collision_polygon_one_way(physics_layer, i);
		polygon_dict["one_way_margin"] = tile_data->get_collision_polygon_one_way_margin(physics_layer, i);
		array.push_back(polygon_dict);
	}
	dict["polygons"] = array;
	return dict;
}

void RTileDataCollisionEditor::_setup_undo_redo_action(RTileSetAtlasSource *p_tile_set_atlas_source, Map<RTileMapCell, Variant> p_previous_values, Variant p_new_value) {
	Array new_array = p_new_value;
	for (Map<RTileMapCell, Variant>::Element *E = p_previous_values.front(); E; E = E->next()) {
		Array old_array = E->value();

		Vector2i coords = E->key().get_atlas_coords();
		undo_redo->add_undo_property(p_tile_set_atlas_source, vformat("%d:%d/%d/physics_layer_%d/polygons_count", coords.x, coords.y, E->key().alternative_tile, physics_layer), old_array.size());
		for (int i = 0; i < old_array.size(); i++) {
			Dictionary dict = old_array[i];
			undo_redo->add_undo_property(p_tile_set_atlas_source, vformat("%d:%d/%d/physics_layer_%d/polygon_%d/points", coords.x, coords.y, E->key().alternative_tile, physics_layer, i), dict["points"]);
			undo_redo->add_undo_property(p_tile_set_atlas_source, vformat("%d:%d/%d/physics_layer_%d/polygon_%d/one_way", coords.x, coords.y, E->key().alternative_tile, physics_layer, i), dict["one_way"]);
			undo_redo->add_undo_property(p_tile_set_atlas_source, vformat("%d:%d/%d/physics_layer_%d/polygon_%d/one_way_margin", coords.x, coords.y, E->key().alternative_tile, physics_layer, i), dict["one_way_margin"]);
		}

		undo_redo->add_do_property(p_tile_set_atlas_source, vformat("%d:%d/%d/physics_layer_%d/polygons_count", coords.x, coords.y, E->key().alternative_tile, physics_layer), new_array.size());
		for (int i = 0; i < new_array.size(); i++) {
			Dictionary dict = new_array[i];
			undo_redo->add_do_property(p_tile_set_atlas_source, vformat("%d:%d/%d/physics_layer_%d/polygon_%d/points", coords.x, coords.y, E->key().alternative_tile, physics_layer, i), dict["points"]);
			undo_redo->add_do_property(p_tile_set_atlas_source, vformat("%d:%d/%d/physics_layer_%d/polygon_%d/one_way", coords.x, coords.y, E->key().alternative_tile, physics_layer, i), dict["one_way"]);
			undo_redo->add_do_property(p_tile_set_atlas_source, vformat("%d:%d/%d/physics_layer_%d/polygon_%d/one_way_margin", coords.x, coords.y, E->key().alternative_tile, physics_layer, i), dict["one_way_margin"]);
		}
	}
}

void RTileDataCollisionEditor::_tile_set_changed() {
	polygon_editor->set_tile_set(tile_set);
	_polygons_changed();
}

void RTileDataCollisionEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			polygon_editor->set_polygons_color(get_tree()->get_debug_collisions_color());
			break;
		default:
			break;
	}
}

void RTileDataCollisionEditor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_polygons_changed"), &RTileDataCollisionEditor::_polygons_changed);
}

RTileDataCollisionEditor::RTileDataCollisionEditor() {
	polygon_editor = memnew(RGenericTilePolygonEditor);
	polygon_editor->set_multiple_polygon_mode(true);
	polygon_editor->connect("polygons_changed", this, "_polygons_changed");
	add_child(polygon_editor);

	dummy_object->add_dummy_property("linear_velocity");
	dummy_object->set("linear_velocity", Vector2());
	dummy_object->add_dummy_property("angular_velocity");
	dummy_object->set("angular_velocity", 0.0);

	EditorProperty *linear_velocity_editor = get_editor_for_property(dummy_object, Variant::VECTOR2, "linear_velocity", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT);
	linear_velocity_editor->set_object_and_property(dummy_object, "linear_velocity");
	linear_velocity_editor->set_label("linear_velocity");
	linear_velocity_editor->connect("property_changed", this, "_property_value_changed");
	linear_velocity_editor->update_property();
	add_child(linear_velocity_editor);
	property_editors["linear_velocity"] = linear_velocity_editor;

	EditorProperty *angular_velocity_editor = get_editor_for_property(dummy_object, Variant::REAL, "angular_velocity", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT);
	angular_velocity_editor->set_object_and_property(dummy_object, "angular_velocity");
	angular_velocity_editor->set_label("angular_velocity");
	angular_velocity_editor->connect("property_changed", this, "_property_value_changed");
	angular_velocity_editor->update_property();
	add_child(angular_velocity_editor);
	property_editors["angular_velocity"] = angular_velocity_editor;

	_polygons_changed();
}

RTileDataCollisionEditor::~RTileDataCollisionEditor() {
	memdelete(dummy_object);
}

void RTileDataCollisionEditor::draw_over_tile(CanvasItem *p_canvas_item, Transform2D p_transform, RTileMapCell p_cell, bool p_selected) {
	RTileData *tile_data = _get_tile_data(p_cell);
	ERR_FAIL_COND(!tile_data);

	// Draw all shapes.
	Vector<Color> color;
	if (p_selected) {
		Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
		Color selection_color = Color().from_hsv(Math::fposmod(grid_color.get_h() + 0.5, 1.0), grid_color.get_s(), grid_color.get_v(), 1.0);
		selection_color.a = 0.7;
		color.push_back(selection_color);
	} else {
		Color debug_collision_color = p_canvas_item->get_tree()->get_debug_collisions_color();
		color.push_back(debug_collision_color);
	}

	VisualServer::get_singleton()->canvas_item_add_set_transform(p_canvas_item->get_canvas_item(), p_transform);
	for (int i = 0; i < tile_data->get_collision_polygons_count(physics_layer); i++) {
		Vector<Vector2> polygon = tile_data->get_collision_polygon_points(physics_layer, i);
		if (polygon.size() >= 3) {
			p_canvas_item->draw_polygon(polygon, color);
		}
	}
	VisualServer::get_singleton()->canvas_item_add_set_transform(p_canvas_item->get_canvas_item(), Transform2D());
}

void RTileDataTerrainsEditor::_update_terrain_selector() {
	ERR_FAIL_COND(!tile_set.is_valid());

	// Update the terrain set selector.
	Vector<String> options;
	options.push_back(String(TTR("No terrains")) + String(":-1"));
	for (int i = 0; i < tile_set->get_terrain_sets_count(); i++) {
		options.push_back(vformat("Terrain Set %d", i));
	}
	terrain_set_property_editor->setup(options);
	terrain_set_property_editor->update_property();

	// Update the terrain selector.
	int terrain_set = int(dummy_object->get("terrain_set"));
	if (terrain_set == -1) {
		terrain_property_editor->hide();
	} else {
		options.clear();
		Vector<Vector<Ref<Texture>>> icons = tile_set->generate_terrains_icons(Size2(16, 16) * EDSCALE);
		options.push_back(String(TTR("No terrain")) + String(":-1"));
		for (int i = 0; i < tile_set->get_terrains_count(terrain_set); i++) {
			String name = tile_set->get_terrain_name(terrain_set, i);
			if (name.empty()) {
				options.push_back(vformat("Terrain %d", i));
			} else {
				options.push_back(name);
			}
		}
		terrain_property_editor->setup(options);
		terrain_property_editor->update_property();

		// Kind of a hack to set icons.
		// We could provide a way to modify that in the EditorProperty.
		OptionButton *option_button = Object::cast_to<OptionButton>(terrain_property_editor->get_child(0));
		for (int terrain = 0; terrain < tile_set->get_terrains_count(terrain_set); terrain++) {
			option_button->set_item_icon(terrain + 1, icons[terrain_set][terrain]);
		}
		terrain_property_editor->show();
	}
}

void RTileDataTerrainsEditor::_property_value_changed(StringName p_property, Variant p_value, StringName p_field) {
	Variant old_value = dummy_object->get(p_property);
	dummy_object->set(p_property, p_value);
	if (p_property == "terrain_set") {
		if (p_value != old_value) {
			dummy_object->set("terrain", -1);
		}
		_update_terrain_selector();
	}
	emit_signal(("needs_redraw"));
}

void RTileDataTerrainsEditor::_tile_set_changed() {
	ERR_FAIL_COND(!tile_set.is_valid());

	// Fix if wrong values are selected.
	int terrain_set = int(dummy_object->get("terrain_set"));
	if (terrain_set >= tile_set->get_terrain_sets_count()) {
		terrain_set = -1;
		dummy_object->set("terrain_set", -1);
	}
	if (terrain_set >= 0) {
		if (int(dummy_object->get("terrain")) >= tile_set->get_terrains_count(terrain_set)) {
			dummy_object->set("terrain", -1);
		}
	}

	_update_terrain_selector();
}

void RTileDataTerrainsEditor::forward_draw_over_atlas(RTileAtlasView *p_tile_atlas_view, RTileSetAtlasSource *p_tile_set_atlas_source, CanvasItem *p_canvas_item, Transform2D p_transform) {
	ERR_FAIL_COND(!tile_set.is_valid());

	// Draw the hovered terrain bit, or the whole tile if it has the wrong terrain set.
	Vector2i hovered_coords = RTileSetSource::INVALID_ATLAS_COORDS;
	if (drag_type == DRAG_TYPE_NONE) {
		Vector2i mouse_pos = p_transform.affine_inverse().xform(p_canvas_item->get_local_mouse_position());
		hovered_coords = p_tile_atlas_view->get_atlas_tile_coords_at_pos(mouse_pos);
		hovered_coords = p_tile_set_atlas_source->get_tile_at_coords(hovered_coords);
		if (hovered_coords != RTileSetSource::INVALID_ATLAS_COORDS) {
			RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(hovered_coords, 0));
			int terrain_set = tile_data->get_terrain_set();
			Rect2i texture_region = p_tile_set_atlas_source->get_tile_texture_region(hovered_coords);
			Vector2i position = texture_region.get_center() + p_tile_set_atlas_source->get_tile_effective_texture_offset(hovered_coords, 0);

			if (terrain_set >= 0 && terrain_set == int(dummy_object->get("terrain_set"))) {
				// Draw hovered bit.
				Transform2D xform;
				xform.set_origin(position);

				Vector<Color> color;
				color.push_back(Color(1.0, 1.0, 1.0, 0.5));

				for (int i = 0; i < RTileSet::CELL_NEIGHBOR_MAX; i++) {
					RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
					if (tile_set->is_valid_peering_bit_terrain(terrain_set, bit)) {
						Vector<Vector2> polygon = tile_set->get_terrain_bit_polygon(terrain_set, bit);
						if (Geometry2D::is_point_in_polygon(xform.affine_inverse().xform(mouse_pos), polygon)) {
							p_canvas_item->draw_set_transform_matrix(p_transform * xform);
							p_canvas_item->draw_polygon(polygon, color);
						}
					}
				}
			} else {
				// Draw hovered tile.
				Transform2D tile_xform;
				tile_xform.set_origin(position);
				tile_xform.set_scale(tile_set->get_tile_size());
				tile_set->draw_tile_shape(p_canvas_item, p_transform * tile_xform, Color(1.0, 1.0, 1.0, 0.5), true);
			}
		}
	}

	// Dim terrains with wrong terrain set.
	Ref<Font> font = RTileSetEditor::get_singleton()->get_font(("bold"), ("EditorFonts"));
	for (int i = 0; i < p_tile_set_atlas_source->get_tiles_count(); i++) {
		Vector2i coords = p_tile_set_atlas_source->get_tile_id(i);
		if (coords != hovered_coords) {
			RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(coords, 0));
			if (tile_data->get_terrain_set() != int(dummy_object->get("terrain_set"))) {
				// Dimming
				p_canvas_item->draw_set_transform_matrix(p_transform);
				Rect2i rect = p_tile_set_atlas_source->get_tile_texture_region(coords);
				p_canvas_item->draw_rect(rect, Color(0.0, 0.0, 0.0, 0.3));

				// Text
				p_canvas_item->draw_set_transform_matrix(Transform2D());
				Rect2i texture_region = p_tile_set_atlas_source->get_tile_texture_region(coords);
				Vector2i position = texture_region.get_center() + p_tile_set_atlas_source->get_tile_effective_texture_offset(coords, 0);

				Color color = Color(1, 1, 1);
				String text;
				if (tile_data->get_terrain_set() >= 0) {
					text = vformat("%d", tile_data->get_terrain_set());
				} else {
					text = "-";
				}
				Vector2 string_size = font->get_string_size(text);
				p_canvas_item->draw_string(font, p_transform.xform(position) + Vector2i(-string_size.x / 2, string_size.y / 2), text, color);
			}
		}
	}
	p_canvas_item->draw_set_transform_matrix(Transform2D());

	if (drag_type == DRAG_TYPE_PAINT_TERRAIN_SET_RECT) {
		// Draw selection rectangle.
		Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
		Color selection_color = Color().from_hsv(Math::fposmod(grid_color.get_h() + 0.5, 1.0), grid_color.get_s(), grid_color.get_v(), 1.0);

		p_canvas_item->draw_set_transform_matrix(p_transform);

		Rect2i rect;
		rect.set_position(p_tile_atlas_view->get_atlas_tile_coords_at_pos(drag_start_pos));
		rect.set_size(p_tile_atlas_view->get_atlas_tile_coords_at_pos(p_transform.affine_inverse().xform(p_canvas_item->get_local_mouse_position())) - rect.get_position());
		rect = Rect2i(Point2i(rect.get_position().x + MIN(rect.get_size().x, 0), rect.get_position().y + MIN(rect.get_size().y, 0)), Size2i(Math::abs(rect.get_size().x), Math::abs(rect.get_size().y)));

		Set<RTileMapCell> edited;
		Point2i rect_end = rect.get_position() + rect.get_size();
		for (int x = rect.get_position().x; x <= rect_end.x; x++) {
			for (int y = rect.get_position().y; y <= rect_end.y; y++) {
				Vector2i coords = Vector2i(x, y);
				coords = p_tile_set_atlas_source->get_tile_at_coords(coords);
				if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
					RTileMapCell cell;
					cell.source_id = 0;
					cell.set_atlas_coords(coords);
					cell.alternative_tile = 0;
					edited.insert(cell);
				}
			}
		}

		for (Set<RTileMapCell>::Element *E = edited.front(); E; E = E->next()) {
			Vector2i coords = E->get().get_atlas_coords();
			p_canvas_item->draw_rect(p_tile_set_atlas_source->get_tile_texture_region(coords), selection_color, false);
		}
		p_canvas_item->draw_set_transform_matrix(Transform2D());
	} else if (drag_type == DRAG_TYPE_PAINT_TERRAIN_BITS_RECT) {
		// Highlight selected peering bits.
		Dictionary painted = Dictionary(drag_painted_value);
		int terrain_set = int(painted["terrain_set"]);

		Rect2i rect;
		rect.set_position(p_tile_atlas_view->get_atlas_tile_coords_at_pos(drag_start_pos));
		rect.set_size(p_tile_atlas_view->get_atlas_tile_coords_at_pos(p_transform.affine_inverse().xform(p_canvas_item->get_local_mouse_position())) - rect.get_position());
		rect = Rect2i(Point2i(rect.get_position().x + MIN(rect.get_size().x, 0), rect.get_position().y + MIN(rect.get_size().y, 0)), Size2i(Math::abs(rect.get_size().x), Math::abs(rect.get_size().y)));

		Set<RTileMapCell> edited;
		Point2i rect_end = rect.get_position() + rect.get_size();
		for (int x = rect.get_position().x; x <= rect_end.x; x++) {
			for (int y = rect.get_position().y; y <= rect_end.y; y++) {
				Vector2i coords = Vector2i(x, y);
				coords = p_tile_set_atlas_source->get_tile_at_coords(coords);
				if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
					RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(coords, 0));
					if (tile_data->get_terrain_set() == terrain_set) {
						RTileMapCell cell;
						cell.source_id = 0;
						cell.set_atlas_coords(coords);
						cell.alternative_tile = 0;
						edited.insert(cell);
					}
				}
			}
		}

		Vector2 end = p_transform.affine_inverse().xform(p_canvas_item->get_local_mouse_position());
		Vector<Point2> mouse_pos_rect_polygon;
		mouse_pos_rect_polygon.push_back(drag_start_pos);
		mouse_pos_rect_polygon.push_back(Vector2(end.x, drag_start_pos.y));
		mouse_pos_rect_polygon.push_back(end);
		mouse_pos_rect_polygon.push_back(Vector2(drag_start_pos.x, end.y));

		Vector<Color> color;
		color.push_back(Color(1.0, 1.0, 1.0, 0.5));

		p_canvas_item->draw_set_transform_matrix(p_transform);

		for (Set<RTileMapCell>::Element *E = edited.front(); E; E = E->next()) {
			Vector2i coords = E->get().get_atlas_coords();

			Rect2i texture_region = p_tile_set_atlas_source->get_tile_texture_region(coords);
			Vector2i position = texture_region.get_center() + p_tile_set_atlas_source->get_tile_effective_texture_offset(coords, 0);

			for (int i = 0; i < RTileSet::CELL_NEIGHBOR_MAX; i++) {
				RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
				if (tile_set->is_valid_peering_bit_terrain(terrain_set, bit)) {
					Vector<Vector2> polygon = tile_set->get_terrain_bit_polygon(terrain_set, bit);
					for (int j = 0; j < polygon.size(); j++) {
						polygon.write[j] += position;
					}
					if (!Geometry2D::intersect_polygons(polygon, mouse_pos_rect_polygon).empty()) {
						// Draw bit.
						p_canvas_item->draw_polygon(polygon, color);
					}
				}
			}
		}

		p_canvas_item->draw_set_transform_matrix(Transform2D());
	}
}

void RTileDataTerrainsEditor::forward_draw_over_alternatives(RTileAtlasView *p_tile_atlas_view, RTileSetAtlasSource *p_tile_set_atlas_source, CanvasItem *p_canvas_item, Transform2D p_transform) {
	ERR_FAIL_COND(!tile_set.is_valid());

	// Draw the hovered terrain bit, or the whole tile if it has the wrong terrain set.
	Vector2i hovered_coords = RTileSetSource::INVALID_ATLAS_COORDS;
	int hovered_alternative = RTileSetSource::INVALID_TILE_ALTERNATIVE;
	if (drag_type == DRAG_TYPE_NONE) {
		Vector2i mouse_pos = p_transform.affine_inverse().xform(p_canvas_item->get_local_mouse_position());
		Vector3i hovered = p_tile_atlas_view->get_alternative_tile_at_pos(mouse_pos);
		hovered_coords = Vector2i(hovered.x, hovered.y);
		hovered_alternative = hovered.z;
		if (hovered_coords != RTileSetSource::INVALID_ATLAS_COORDS) {
			RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(hovered_coords, hovered_alternative));
			int terrain_set = tile_data->get_terrain_set();
			Rect2i texture_region = p_tile_atlas_view->get_alternative_tile_rect(hovered_coords, hovered_alternative);
			Vector2i position = texture_region.get_center() + p_tile_set_atlas_source->get_tile_effective_texture_offset(hovered_coords, hovered_alternative);

			if (terrain_set == int(dummy_object->get("terrain_set"))) {
				// Draw hovered bit.
				Transform2D xform;
				xform.set_origin(position);

				Vector<Color> color;
				color.push_back(Color(1.0, 1.0, 1.0, 0.5));

				for (int i = 0; i < RTileSet::CELL_NEIGHBOR_MAX; i++) {
					RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
					if (tile_set->is_valid_peering_bit_terrain(terrain_set, bit)) {
						Vector<Vector2> polygon = tile_set->get_terrain_bit_polygon(terrain_set, bit);
						if (Geometry2D::is_point_in_polygon(xform.affine_inverse().xform(mouse_pos), polygon)) {
							p_canvas_item->draw_set_transform_matrix(p_transform * xform);
							p_canvas_item->draw_polygon(polygon, color);
						}
					}
				}
			} else {
				// Draw hovered tile.
				Transform2D tile_xform;
				tile_xform.set_origin(position);
				tile_xform.set_scale(tile_set->get_tile_size());
				tile_set->draw_tile_shape(p_canvas_item, p_transform * tile_xform, Color(1.0, 1.0, 1.0, 0.5), true);
			}
		}
	}

	// Dim terrains with wrong terrain set.
	Ref<Font> font = RTileSetEditor::get_singleton()->get_font(("bold"), ("EditorFonts"));
	for (int i = 0; i < p_tile_set_atlas_source->get_tiles_count(); i++) {
		Vector2i coords = p_tile_set_atlas_source->get_tile_id(i);
		for (int j = 1; j < p_tile_set_atlas_source->get_alternative_tiles_count(coords); j++) {
			int alternative_tile = p_tile_set_atlas_source->get_alternative_tile_id(coords, j);
			if (coords != hovered_coords || alternative_tile != hovered_alternative) {
				RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(coords, alternative_tile));
				if (tile_data->get_terrain_set() != int(dummy_object->get("terrain_set"))) {
					// Dimming
					p_canvas_item->draw_set_transform_matrix(p_transform);
					Rect2i rect = p_tile_atlas_view->get_alternative_tile_rect(coords, alternative_tile);
					p_canvas_item->draw_rect(rect, Color(0.0, 0.0, 0.0, 0.3));

					// Text
					p_canvas_item->draw_set_transform_matrix(Transform2D());
					Rect2i texture_region = p_tile_atlas_view->get_alternative_tile_rect(coords, alternative_tile);
					Vector2i position = texture_region.get_center() + p_tile_set_atlas_source->get_tile_effective_texture_offset(coords, 0);

					Color color = Color(1, 1, 1);
					String text;
					if (tile_data->get_terrain_set() >= 0) {
						text = vformat("%d", tile_data->get_terrain_set());
					} else {
						text = "-";
					}
					Vector2 string_size = font->get_string_size(text);
					p_canvas_item->draw_string(font, p_transform.xform(position) + Vector2i(-string_size.x / 2, string_size.y / 2), text, color);
				}
			}
		}
	}

	p_canvas_item->draw_set_transform_matrix(Transform2D());
}

void RTileDataTerrainsEditor::forward_painting_atlas_gui_input(RTileAtlasView *p_tile_atlas_view, RTileSetAtlasSource *p_tile_set_atlas_source, const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		if (drag_type == DRAG_TYPE_PAINT_TERRAIN_SET) {
			Vector<Vector2i> line = Geometry2D::bresenham_line(p_tile_atlas_view->get_atlas_tile_coords_at_pos(drag_last_pos), p_tile_atlas_view->get_atlas_tile_coords_at_pos(mm->get_position()));
			for (int i = 0; i < line.size(); i++) {
				Vector2i coords = p_tile_set_atlas_source->get_tile_at_coords(line[i]);
				if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
					int terrain_set = drag_painted_value;
					RTileMapCell cell;
					cell.source_id = 0;
					cell.set_atlas_coords(coords);
					cell.alternative_tile = 0;

					// Save the old terrain_set and terrains bits.
					RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(coords, 0));
					if (!drag_modified.has(cell)) {
						Dictionary dict;
						dict["terrain_set"] = tile_data->get_terrain_set();
						Array array;
						for (int j = 0; j < RTileSet::CELL_NEIGHBOR_MAX; j++) {
							RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(j);
							array.push_back(tile_data->is_valid_peering_bit_terrain(bit) ? tile_data->get_peering_bit_terrain(bit) : -1);
						}
						dict["terrain_peering_bits"] = array;
						drag_modified[cell] = dict;
					}

					// Set the terrain_set.
					tile_data->set_terrain_set(terrain_set);
				}
			}
			drag_last_pos = mm->get_position();
		} else if (drag_type == DRAG_TYPE_PAINT_TERRAIN_BITS) {
			int terrain_set = Dictionary(drag_painted_value)["terrain_set"];
			int terrain = Dictionary(drag_painted_value)["terrain"];
			Vector<Vector2i> line = Geometry2D::bresenham_line(p_tile_atlas_view->get_atlas_tile_coords_at_pos(drag_last_pos), p_tile_atlas_view->get_atlas_tile_coords_at_pos(mm->get_position()));
			for (int i = 0; i < line.size(); i++) {
				Vector2i coords = p_tile_set_atlas_source->get_tile_at_coords(line[i]);
				if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
					RTileMapCell cell;
					cell.source_id = 0;
					cell.set_atlas_coords(coords);
					cell.alternative_tile = 0;

					RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(coords, 0));
					if (tile_data->get_terrain_set() == terrain_set) {
						// Save the old terrain_set and terrains bits.
						if (!drag_modified.has(cell)) {
							Dictionary dict;
							dict["terrain_set"] = tile_data->get_terrain_set();
							Array array;
							for (int j = 0; j < RTileSet::CELL_NEIGHBOR_MAX; j++) {
								RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(j);
								array.push_back(tile_data->is_valid_peering_bit_terrain(bit) ? tile_data->get_peering_bit_terrain(bit) : -1);
							}
							dict["terrain_peering_bits"] = array;
							drag_modified[cell] = dict;
						}

						// Set the terrains bits.
						Rect2i texture_region = p_tile_set_atlas_source->get_tile_texture_region(coords);
						Vector2i position = texture_region.get_center() + p_tile_set_atlas_source->get_tile_effective_texture_offset(coords, 0);
						for (int j = 0; j < RTileSet::CELL_NEIGHBOR_MAX; j++) {
							RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(j);
							if (tile_data->is_valid_peering_bit_terrain(bit)) {
								Vector<Vector2> polygon = tile_set->get_terrain_bit_polygon(tile_data->get_terrain_set(), bit);
								if (Geometry2D::is_segment_intersecting_polygon(mm->get_position() - position, drag_last_pos - position, polygon)) {
									tile_data->set_peering_bit_terrain(bit, terrain);
								}
							}
						}
					}
				}
			}
			drag_last_pos = mm->get_position();
		}
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		if (mb->get_button_index() == BUTTON_LEFT) {
			if (mb->is_pressed()) {
				if (picker_button->is_pressed()) {
					Vector2i coords = p_tile_atlas_view->get_atlas_tile_coords_at_pos(mb->get_position());
					coords = p_tile_set_atlas_source->get_tile_at_coords(coords);
					if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
						RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(coords, 0));
						int terrain_set = tile_data->get_terrain_set();
						Rect2i texture_region = p_tile_set_atlas_source->get_tile_texture_region(coords);
						Vector2i position = texture_region.get_center() + p_tile_set_atlas_source->get_tile_effective_texture_offset(coords, 0);
						dummy_object->set("terrain_set", terrain_set);
						dummy_object->set("terrain", -1);
						for (int i = 0; i < RTileSet::CELL_NEIGHBOR_MAX; i++) {
							RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
							if (tile_set->is_valid_peering_bit_terrain(terrain_set, bit)) {
								Vector<Vector2> polygon = tile_set->get_terrain_bit_polygon(terrain_set, bit);
								if (Geometry2D::is_point_in_polygon(mb->get_position() - position, polygon)) {
									dummy_object->set("terrain", tile_data->get_peering_bit_terrain(bit));
								}
							}
						}
						terrain_set_property_editor->update_property();
						_update_terrain_selector();
						picker_button->set_pressed(false);
					}
				} else {
					Vector2i coords = p_tile_atlas_view->get_atlas_tile_coords_at_pos(mb->get_position());
					coords = p_tile_set_atlas_source->get_tile_at_coords(coords);
					RTileData *tile_data = nullptr;
					if (coords != RTileSetAtlasSource::INVALID_ATLAS_COORDS) {
						tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(coords, 0));
					}
					int terrain_set = int(dummy_object->get("terrain_set"));
					int terrain = int(dummy_object->get("terrain"));
					if (terrain_set == -1 || !tile_data || tile_data->get_terrain_set() != terrain_set) {
						if (mb->get_control()) {
							// Paint terrain set with rect.
							drag_type = DRAG_TYPE_PAINT_TERRAIN_SET_RECT;
							drag_modified.clear();
							drag_painted_value = terrain_set;
							drag_start_pos = mb->get_position();
						} else {
							// Paint terrain set.
							drag_type = DRAG_TYPE_PAINT_TERRAIN_SET;
							drag_modified.clear();
							drag_painted_value = terrain_set;

							if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
								RTileMapCell cell;
								cell.source_id = 0;
								cell.set_atlas_coords(coords);
								cell.alternative_tile = 0;

								// Save the old terrain_set and terrains bits.
								Dictionary dict;
								dict["terrain_set"] = tile_data->get_terrain_set();
								Array array;
								for (int i = 0; i < RTileSet::CELL_NEIGHBOR_MAX; i++) {
									RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
									array.push_back(tile_data->is_valid_peering_bit_terrain(bit) ? tile_data->get_peering_bit_terrain(bit) : -1);
								}
								dict["terrain_peering_bits"] = array;
								drag_modified[cell] = dict;

								// Set the terrain_set.
								tile_data->set_terrain_set(terrain_set);
							}
							drag_last_pos = mb->get_position();
						}
					} else if (tile_data && tile_data->get_terrain_set() == terrain_set) {
						if (mb->get_control()) {
							// Paint terrain set with rect.
							drag_type = DRAG_TYPE_PAINT_TERRAIN_BITS_RECT;
							drag_modified.clear();
							Dictionary painted_dict;
							painted_dict["terrain_set"] = terrain_set;
							painted_dict["terrain"] = terrain;
							drag_painted_value = painted_dict;
							drag_start_pos = mb->get_position();
						} else {
							// Paint terrain bits.
							drag_type = DRAG_TYPE_PAINT_TERRAIN_BITS;
							drag_modified.clear();
							Dictionary painted_dict;
							painted_dict["terrain_set"] = terrain_set;
							painted_dict["terrain"] = terrain;
							drag_painted_value = painted_dict;

							if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
								RTileMapCell cell;
								cell.source_id = 0;
								cell.set_atlas_coords(coords);
								cell.alternative_tile = 0;

								// Save the old terrain_set and terrains bits.
								Dictionary dict;
								dict["terrain_set"] = tile_data->get_terrain_set();
								Array array;
								for (int i = 0; i < RTileSet::CELL_NEIGHBOR_MAX; i++) {
									RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
									array.push_back(tile_data->is_valid_peering_bit_terrain(bit) ? tile_data->get_peering_bit_terrain(bit) : -1);
								}
								dict["terrain_peering_bits"] = array;
								drag_modified[cell] = dict;

								// Set the terrain bit.
								Rect2i texture_region = p_tile_set_atlas_source->get_tile_texture_region(coords);
								Vector2i position = texture_region.get_center() + p_tile_set_atlas_source->get_tile_effective_texture_offset(coords, 0);

								for (int i = 0; i < RTileSet::CELL_NEIGHBOR_MAX; i++) {
									RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
									if (tile_set->is_valid_peering_bit_terrain(terrain_set, bit)) {
										Vector<Vector2> polygon = tile_set->get_terrain_bit_polygon(terrain_set, bit);
										if (Geometry2D::is_point_in_polygon(mb->get_position() - position, polygon)) {
											tile_data->set_peering_bit_terrain(bit, terrain);
										}
									}
								}
							}
							drag_last_pos = mb->get_position();
						}
					}
				}
			} else {
				if (drag_type == DRAG_TYPE_PAINT_TERRAIN_SET_RECT) {
					Rect2i rect;
					rect.set_position(p_tile_atlas_view->get_atlas_tile_coords_at_pos(drag_start_pos));
					rect.set_size(p_tile_atlas_view->get_atlas_tile_coords_at_pos(mb->get_position()) - rect.get_position());
					rect = Rect2i(Point2i(rect.get_position().x + MIN(rect.get_size().x, 0), rect.get_position().y + MIN(rect.get_size().y, 0)), Size2i(Math::abs(rect.get_size().x), Math::abs(rect.get_size().y)));

					Set<RTileMapCell> edited;
					Point2i rect_end = rect.get_position() + rect.get_size();
					for (int x = rect.get_position().x; x <= rect_end.x; x++) {
						for (int y = rect.get_position().y; y <= rect_end.y; y++) {
							Vector2i coords = Vector2i(x, y);
							coords = p_tile_set_atlas_source->get_tile_at_coords(coords);
							if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
								RTileMapCell cell;
								cell.source_id = 0;
								cell.set_atlas_coords(coords);
								cell.alternative_tile = 0;
								edited.insert(cell);
							}
						}
					}
					undo_redo->create_action(TTR("Painting Terrain Set"));
					for (Set<RTileMapCell>::Element *E = edited.front(); E; E = E->next()) {
						Vector2i coords = E->get().get_atlas_coords();
						RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(coords, 0));
						undo_redo->add_undo_property(p_tile_set_atlas_source, vformat("%d:%d/%d/terrain_set", coords.x, coords.y, E->get().alternative_tile), tile_data->get_terrain_set());
						undo_redo->add_do_property(p_tile_set_atlas_source, vformat("%d:%d/%d/terrain_set", coords.x, coords.y, E->get().alternative_tile), drag_painted_value);
						for (int i = 0; i < RTileSet::CELL_NEIGHBOR_MAX; i++) {
							RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
							if (tile_data->is_valid_peering_bit_terrain(bit)) {
								undo_redo->add_undo_property(p_tile_set_atlas_source, vformat("%d:%d/%d/terrains_peering_bit/" + String(RTileSet::CELL_NEIGHBOR_ENUM_TO_TEXT[i]), coords.x, coords.y, E->get().alternative_tile), tile_data->get_peering_bit_terrain(bit));
							}
						}
					}
					undo_redo->commit_action();
					drag_type = DRAG_TYPE_NONE;
				} else if (drag_type == DRAG_TYPE_PAINT_TERRAIN_SET) {
					undo_redo->create_action(TTR("Painting Terrain Set"));
					for (Map<RTileMapCell, Variant>::Element *E = drag_modified.front(); E; E = E->next()) {
						Dictionary dict = E->value();
						Vector2i coords = E->key().get_atlas_coords();
						undo_redo->add_do_property(p_tile_set_atlas_source, vformat("%d:%d/%d/terrain_set", coords.x, coords.y, E->key().alternative_tile), drag_painted_value);
						undo_redo->add_undo_property(p_tile_set_atlas_source, vformat("%d:%d/%d/terrain_set", coords.x, coords.y, E->key().alternative_tile), dict["terrain_set"]);
						Array array = dict["terrain_peering_bits"];
						for (int i = 0; i < array.size(); i++) {
							RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
							if (tile_set->is_valid_peering_bit_terrain(dict["terrain_set"], bit)) {
								undo_redo->add_undo_property(p_tile_set_atlas_source, vformat("%d:%d/%d/terrains_peering_bit/" + String(RTileSet::CELL_NEIGHBOR_ENUM_TO_TEXT[i]), coords.x, coords.y, E->key().alternative_tile), array[i]);
							}
						}
					}
					undo_redo->commit_action();
					drag_type = DRAG_TYPE_NONE;
				} else if (drag_type == DRAG_TYPE_PAINT_TERRAIN_BITS) {
					Dictionary painted = Dictionary(drag_painted_value);
					int terrain_set = int(painted["terrain_set"]);
					int terrain = int(painted["terrain"]);
					undo_redo->create_action(TTR("Painting Terrain"));
					for (Map<RTileMapCell, Variant>::Element *E = drag_modified.front(); E; E = E->next()) {
						Dictionary dict = E->value();
						Vector2i coords = E->key().get_atlas_coords();
						Array array = dict["terrain_peering_bits"];
						for (int i = 0; i < array.size(); i++) {
							RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
							if (tile_set->is_valid_peering_bit_terrain(terrain_set, bit)) {
								undo_redo->add_do_property(p_tile_set_atlas_source, vformat("%d:%d/%d/terrains_peering_bit/" + String(RTileSet::CELL_NEIGHBOR_ENUM_TO_TEXT[i]), coords.x, coords.y, E->key().alternative_tile), terrain);
							}
							if (tile_set->is_valid_peering_bit_terrain(dict["terrain_set"], bit)) {
								undo_redo->add_undo_property(p_tile_set_atlas_source, vformat("%d:%d/%d/terrains_peering_bit/" + String(RTileSet::CELL_NEIGHBOR_ENUM_TO_TEXT[i]), coords.x, coords.y, E->key().alternative_tile), array[i]);
							}
						}
					}
					undo_redo->commit_action();
					drag_type = DRAG_TYPE_NONE;
				} else if (drag_type == DRAG_TYPE_PAINT_TERRAIN_BITS_RECT) {
					Dictionary painted = Dictionary(drag_painted_value);
					int terrain_set = int(painted["terrain_set"]);
					int terrain = int(painted["terrain"]);

					Rect2i rect;
					rect.set_position(p_tile_atlas_view->get_atlas_tile_coords_at_pos(drag_start_pos));
					rect.set_size(p_tile_atlas_view->get_atlas_tile_coords_at_pos(mb->get_position()) - rect.get_position());
					rect = Rect2i(Point2i(rect.get_position().x + MIN(rect.get_size().x, 0), rect.get_position().y + MIN(rect.get_size().y, 0)), Size2i(Math::abs(rect.get_size().x), Math::abs(rect.get_size().y)));

					Set<RTileMapCell> edited;
					Point2i rect_end = rect.get_position() + rect.get_size();
					for (int x = rect.get_position().x; x <= rect_end.x; x++) {
						for (int y = rect.get_position().y; y <= rect_end.y; y++) {
							Vector2i coords = Vector2i(x, y);
							coords = p_tile_set_atlas_source->get_tile_at_coords(coords);
							if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
								RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(coords, 0));
								if (tile_data->get_terrain_set() == terrain_set) {
									RTileMapCell cell;
									cell.source_id = 0;
									cell.set_atlas_coords(coords);
									cell.alternative_tile = 0;
									edited.insert(cell);
								}
							}
						}
					}

					Vector<Point2> mouse_pos_rect_polygon;
					mouse_pos_rect_polygon.push_back(drag_start_pos);
					mouse_pos_rect_polygon.push_back(Vector2(mb->get_position().x, drag_start_pos.y));
					mouse_pos_rect_polygon.push_back(mb->get_position());
					mouse_pos_rect_polygon.push_back(Vector2(drag_start_pos.x, mb->get_position().y));

					undo_redo->create_action(TTR("Painting Terrain"));
					for (Set<RTileMapCell>::Element *E = edited.front(); E; E = E->next()) {
						Vector2i coords = E->get().get_atlas_coords();
						RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(coords, 0));

						for (int i = 0; i < RTileSet::CELL_NEIGHBOR_MAX; i++) {
							RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
							if (tile_set->is_valid_peering_bit_terrain(terrain_set, bit)) {
								Rect2i texture_region = p_tile_set_atlas_source->get_tile_texture_region(coords);
								Vector2i position = texture_region.get_center() + p_tile_set_atlas_source->get_tile_effective_texture_offset(coords, 0);

								Vector<Vector2> polygon = tile_set->get_terrain_bit_polygon(terrain_set, bit);
								for (int j = 0; j < polygon.size(); j++) {
									polygon.write[j] += position;
								}
								if (!Geometry2D::intersect_polygons(polygon, mouse_pos_rect_polygon).empty()) {
									// Draw bit.
									undo_redo->add_do_property(p_tile_set_atlas_source, vformat("%d:%d/%d/terrains_peering_bit/" + String(RTileSet::CELL_NEIGHBOR_ENUM_TO_TEXT[i]), coords.x, coords.y, E->get().alternative_tile), terrain);
									undo_redo->add_undo_property(p_tile_set_atlas_source, vformat("%d:%d/%d/terrains_peering_bit/" + String(RTileSet::CELL_NEIGHBOR_ENUM_TO_TEXT[i]), coords.x, coords.y, E->get().alternative_tile), tile_data->get_peering_bit_terrain(bit));
								}
							}
						}
					}
					undo_redo->commit_action();
					drag_type = DRAG_TYPE_NONE;
				}
			}
		}
	}
}

void RTileDataTerrainsEditor::forward_painting_alternatives_gui_input(RTileAtlasView *p_tile_atlas_view, RTileSetAtlasSource *p_tile_set_atlas_source, const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		if (drag_type == DRAG_TYPE_PAINT_TERRAIN_SET) {
			Vector3i tile = p_tile_atlas_view->get_alternative_tile_at_pos(mm->get_position());
			Vector2i coords = Vector2i(tile.x, tile.y);
			int alternative_tile = tile.z;

			if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
				RTileMapCell cell;
				cell.source_id = 0;
				cell.set_atlas_coords(coords);
				cell.alternative_tile = alternative_tile;
				RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(coords, alternative_tile));
				if (!drag_modified.has(cell)) {
					Dictionary dict;
					dict["terrain_set"] = tile_data->get_terrain_set();
					Array array;
					for (int j = 0; j < RTileSet::CELL_NEIGHBOR_MAX; j++) {
						RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(j);
						array.push_back(tile_data->is_valid_peering_bit_terrain(bit) ? tile_data->get_peering_bit_terrain(bit) : -1);
					}
					dict["terrain_peering_bits"] = array;
					drag_modified[cell] = dict;
				}
				tile_data->set_terrain_set(drag_painted_value);
			}

			drag_last_pos = mm->get_position();
		} else if (drag_type == DRAG_TYPE_PAINT_TERRAIN_BITS) {
			Dictionary painted = Dictionary(drag_painted_value);
			int terrain_set = int(painted["terrain_set"]);
			int terrain = int(painted["terrain"]);

			Vector3i tile = p_tile_atlas_view->get_alternative_tile_at_pos(mm->get_position());
			Vector2i coords = Vector2i(tile.x, tile.y);
			int alternative_tile = tile.z;

			if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
				RTileMapCell cell;
				cell.source_id = 0;
				cell.set_atlas_coords(coords);
				cell.alternative_tile = alternative_tile;

				// Save the old terrain_set and terrains bits.
				RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(coords, alternative_tile));
				if (tile_data->get_terrain_set() == terrain_set) {
					if (!drag_modified.has(cell)) {
						Dictionary dict;
						dict["terrain_set"] = tile_data->get_terrain_set();
						Array array;
						for (int j = 0; j < RTileSet::CELL_NEIGHBOR_MAX; j++) {
							RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(j);
							array.push_back(tile_data->is_valid_peering_bit_terrain(bit) ? tile_data->get_peering_bit_terrain(bit) : -1);
						}
						dict["terrain_peering_bits"] = array;
						drag_modified[cell] = dict;
					}

					// Set the terrains bits.
					Rect2i texture_region = p_tile_atlas_view->get_alternative_tile_rect(coords, alternative_tile);
					Vector2i position = texture_region.get_center() + p_tile_set_atlas_source->get_tile_effective_texture_offset(coords, alternative_tile);
					for (int j = 0; j < RTileSet::CELL_NEIGHBOR_MAX; j++) {
						RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(j);
						if (tile_data->is_valid_peering_bit_terrain(bit)) {
							Vector<Vector2> polygon = tile_set->get_terrain_bit_polygon(tile_data->get_terrain_set(), bit);
							if (Geometry2D::is_segment_intersecting_polygon(mm->get_position() - position, drag_last_pos - position, polygon)) {
								tile_data->set_peering_bit_terrain(bit, terrain);
							}
						}
					}
				}
			}
			drag_last_pos = mm->get_position();
		}
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		if (mb->get_button_index() == BUTTON_LEFT) {
			if (mb->is_pressed()) {
				if (picker_button->is_pressed()) {
					Vector3i tile = p_tile_atlas_view->get_alternative_tile_at_pos(mb->get_position());
					Vector2i coords = Vector2i(tile.x, tile.y);
					int alternative_tile = tile.z;

					if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
						RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(coords, alternative_tile));
						int terrain_set = tile_data->get_terrain_set();
						Rect2i texture_region = p_tile_atlas_view->get_alternative_tile_rect(coords, alternative_tile);
						Vector2i position = texture_region.get_center() + p_tile_set_atlas_source->get_tile_effective_texture_offset(coords, alternative_tile);
						dummy_object->set("terrain_set", terrain_set);
						dummy_object->set("terrain", -1);
						for (int i = 0; i < RTileSet::CELL_NEIGHBOR_MAX; i++) {
							RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
							if (tile_set->is_valid_peering_bit_terrain(terrain_set, bit)) {
								Vector<Vector2> polygon = tile_set->get_terrain_bit_polygon(terrain_set, bit);
								if (Geometry2D::is_point_in_polygon(mb->get_position() - position, polygon)) {
									dummy_object->set("terrain", tile_data->get_peering_bit_terrain(bit));
								}
							}
						}
						terrain_set_property_editor->update_property();
						_update_terrain_selector();
						picker_button->set_pressed(false);
					}
				} else {
					int terrain_set = int(dummy_object->get("terrain_set"));
					int terrain = int(dummy_object->get("terrain"));

					Vector3i tile = p_tile_atlas_view->get_alternative_tile_at_pos(mb->get_position());
					Vector2i coords = Vector2i(tile.x, tile.y);
					int alternative_tile = tile.z;

					RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(coords, alternative_tile));

					if (terrain_set == -1 || !tile_data || tile_data->get_terrain_set() != terrain_set) {
						drag_type = DRAG_TYPE_PAINT_TERRAIN_SET;
						drag_modified.clear();
						drag_painted_value = int(dummy_object->get("terrain_set"));
						if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
							RTileMapCell cell;
							cell.source_id = 0;
							cell.set_atlas_coords(coords);
							cell.alternative_tile = alternative_tile;
							Dictionary dict;
							dict["terrain_set"] = tile_data->get_terrain_set();
							Array array;
							for (int i = 0; i < RTileSet::CELL_NEIGHBOR_MAX; i++) {
								RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
								array.push_back(tile_data->is_valid_peering_bit_terrain(bit) ? tile_data->get_peering_bit_terrain(bit) : -1);
							}
							dict["terrain_peering_bits"] = array;
							drag_modified[cell] = dict;
							tile_data->set_terrain_set(drag_painted_value);
						}
						drag_last_pos = mb->get_position();
					} else if (tile_data && tile_data->get_terrain_set() == terrain_set) {
						// Paint terrain bits.
						drag_type = DRAG_TYPE_PAINT_TERRAIN_BITS;
						drag_modified.clear();
						Dictionary painted_dict;
						painted_dict["terrain_set"] = terrain_set;
						painted_dict["terrain"] = terrain;
						drag_painted_value = painted_dict;

						if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
							RTileMapCell cell;
							cell.source_id = 0;
							cell.set_atlas_coords(coords);
							cell.alternative_tile = alternative_tile;

							// Save the old terrain_set and terrains bits.
							Dictionary dict;
							dict["terrain_set"] = tile_data->get_terrain_set();
							Array array;
							for (int i = 0; i < RTileSet::CELL_NEIGHBOR_MAX; i++) {
								RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
								array.push_back(tile_data->is_valid_peering_bit_terrain(bit) ? tile_data->get_peering_bit_terrain(bit) : -1);
							}
							dict["terrain_peering_bits"] = array;
							drag_modified[cell] = dict;

							// Set the terrain bit.
							Rect2i texture_region = p_tile_atlas_view->get_alternative_tile_rect(coords, alternative_tile);
							Vector2i position = texture_region.get_center() + p_tile_set_atlas_source->get_tile_effective_texture_offset(coords, alternative_tile);
							for (int i = 0; i < RTileSet::CELL_NEIGHBOR_MAX; i++) {
								RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
								if (tile_set->is_valid_peering_bit_terrain(terrain_set, bit)) {
									Vector<Vector2> polygon = tile_set->get_terrain_bit_polygon(terrain_set, bit);
									if (Geometry2D::is_point_in_polygon(mb->get_position() - position, polygon)) {
										tile_data->set_peering_bit_terrain(bit, terrain);
									}
								}
							}
						}
						drag_last_pos = mb->get_position();
					}
				}
			} else {
				if (drag_type == DRAG_TYPE_PAINT_TERRAIN_SET) {
					undo_redo->create_action(TTR("Painting Tiles Property"));
					for (Map<RTileMapCell, Variant>::Element *E = drag_modified.front(); E; E = E->next()) {
						Dictionary dict = E->value();
						Vector2i coords = E->key().get_atlas_coords();
						undo_redo->add_undo_property(p_tile_set_atlas_source, vformat("%d:%d/%d/terrain_set", coords.x, coords.y, E->key().alternative_tile), dict["terrain_set"]);
						undo_redo->add_do_property(p_tile_set_atlas_source, vformat("%d:%d/%d/terrain_set", coords.x, coords.y, E->key().alternative_tile), drag_painted_value);
						Array array = dict["terrain_peering_bits"];
						for (int i = 0; i < array.size(); i++) {
							undo_redo->add_undo_property(p_tile_set_atlas_source, vformat("%d:%d/%d/terrains_peering_bit/" + String(RTileSet::CELL_NEIGHBOR_ENUM_TO_TEXT[i]), coords.x, coords.y, E->key().alternative_tile), array[i]);
						}
					}
					undo_redo->commit_action();
					drag_type = DRAG_TYPE_NONE;
				} else if (drag_type == DRAG_TYPE_PAINT_TERRAIN_BITS) {
					Dictionary painted = Dictionary(drag_painted_value);
					int terrain_set = int(painted["terrain_set"]);
					int terrain = int(painted["terrain"]);
					undo_redo->create_action(TTR("Painting Terrain"));
					for (Map<RTileMapCell, Variant>::Element *E = drag_modified.front(); E; E = E->next()) {
						Dictionary dict = E->value();
						Vector2i coords = E->key().get_atlas_coords();
						Array array = dict["terrain_peering_bits"];
						for (int i = 0; i < array.size(); i++) {
							RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
							if (tile_set->is_valid_peering_bit_terrain(terrain_set, bit)) {
								undo_redo->add_do_property(p_tile_set_atlas_source, vformat("%d:%d/%d/terrains_peering_bit/" + String(RTileSet::CELL_NEIGHBOR_ENUM_TO_TEXT[i]), coords.x, coords.y, E->key().alternative_tile), terrain);
							}
							if (tile_set->is_valid_peering_bit_terrain(dict["terrain_set"], bit)) {
								undo_redo->add_undo_property(p_tile_set_atlas_source, vformat("%d:%d/%d/terrains_peering_bit/" + String(RTileSet::CELL_NEIGHBOR_ENUM_TO_TEXT[i]), coords.x, coords.y, E->key().alternative_tile), array[i]);
							}
						}
					}
					undo_redo->commit_action();
					drag_type = DRAG_TYPE_NONE;
				}
			}
		}
	}
}

void RTileDataTerrainsEditor::draw_over_tile(CanvasItem *p_canvas_item, Transform2D p_transform, RTileMapCell p_cell, bool p_selected) {
	RTileData *tile_data = _get_tile_data(p_cell);
	ERR_FAIL_COND(!tile_data);

	tile_set->draw_terrains(p_canvas_item, p_transform, tile_data);
}

void RTileDataTerrainsEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_THEME_CHANGED:
			picker_button->set_icon(get_icon(("ColorPick"), ("EditorIcons")));
			break;
		default:
			break;
	}
}

RTileDataTerrainsEditor::RTileDataTerrainsEditor() {
	label = memnew(Label);
	label->set_text("Painting:");
	add_child(label);

	// Toolbar
	toolbar->add_child(memnew(VSeparator));

	picker_button = memnew(Button);
	picker_button->set_flat(true);
	picker_button->set_toggle_mode(true);
	picker_button->set_shortcut(ED_SHORTCUT("tiles_editor/picker", "Picker", KEY_P));
	toolbar->add_child(picker_button);

	// Setup
	dummy_object->add_dummy_property("terrain_set");
	dummy_object->set("terrain_set", -1);
	dummy_object->add_dummy_property("terrain");
	dummy_object->set("terrain", -1);

	// Get the default value for the type.
	terrain_set_property_editor = memnew(EditorPropertyEnum);
	terrain_set_property_editor->set_object_and_property(dummy_object, "terrain_set");
	terrain_set_property_editor->set_label("Terrain Set");
	terrain_set_property_editor->connect("property_changed", this, "_property_value_changed");
	add_child(terrain_set_property_editor);

	terrain_property_editor = memnew(EditorPropertyEnum);
	terrain_property_editor->set_object_and_property(dummy_object, "terrain");
	terrain_property_editor->set_label("Terrain");
	terrain_property_editor->connect("property_changed", this, "_property_value_changed");
	add_child(terrain_property_editor);
}

RTileDataTerrainsEditor::~RTileDataTerrainsEditor() {
	toolbar->queue_delete();
	memdelete(dummy_object);
}

Variant RTileDataNavigationEditor::_get_painted_value() {
	Ref<NavigationPolygon> navigation_polygon;
	navigation_polygon.instance();

	for (int i = 0; i < polygon_editor->get_polygon_count(); i++) {
		PoolVector2Array polygon = polygon_editor->get_polygon_poolvector(i);
		navigation_polygon->add_outline(polygon);
	}

	navigation_polygon->make_polygons_from_outlines();
	return navigation_polygon;
}

void RTileDataNavigationEditor::_set_painted_value(RTileSetAtlasSource *p_tile_set_atlas_source, Vector2 p_coords, int p_alternative_tile) {
	RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(p_coords, p_alternative_tile));
	ERR_FAIL_COND(!tile_data);

	Ref<NavigationPolygon> navigation_polygon = tile_data->get_navigation_polygon(navigation_layer);
	polygon_editor->clear_polygons();
	if (navigation_polygon.is_valid()) {
		for (int i = 0; i < navigation_polygon->get_outline_count(); i++) {
			polygon_editor->add_polygon_poolvector(navigation_polygon->get_outline(i));
		}
	}
	polygon_editor->set_background(p_tile_set_atlas_source->get_texture(), p_tile_set_atlas_source->get_tile_texture_region(p_coords), p_tile_set_atlas_source->get_tile_effective_texture_offset(p_coords, p_alternative_tile), tile_data->get_flip_h(), tile_data->get_flip_v(), tile_data->get_transpose(), tile_data->get_modulate());
}

void RTileDataNavigationEditor::_set_value(RTileSetAtlasSource *p_tile_set_atlas_source, Vector2 p_coords, int p_alternative_tile, Variant p_value) {
	RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(p_coords, p_alternative_tile));
	ERR_FAIL_COND(!tile_data);
	Ref<NavigationPolygon> navigation_polygon = p_value;
	tile_data->set_navigation_polygon(navigation_layer, navigation_polygon);

	polygon_editor->set_background(p_tile_set_atlas_source->get_texture(), p_tile_set_atlas_source->get_tile_texture_region(p_coords), p_tile_set_atlas_source->get_tile_effective_texture_offset(p_coords, p_alternative_tile), tile_data->get_flip_h(), tile_data->get_flip_v(), tile_data->get_transpose(), tile_data->get_modulate());
}

Variant RTileDataNavigationEditor::_get_value(RTileSetAtlasSource *p_tile_set_atlas_source, Vector2 p_coords, int p_alternative_tile) {
	RTileData *tile_data = Object::cast_to<RTileData>(p_tile_set_atlas_source->get_tile_data(p_coords, p_alternative_tile));
	ERR_FAIL_COND_V(!tile_data, Variant());
	return tile_data->get_navigation_polygon(navigation_layer);
}

void RTileDataNavigationEditor::_setup_undo_redo_action(RTileSetAtlasSource *p_tile_set_atlas_source, Map<RTileMapCell, Variant> p_previous_values, Variant p_new_value) {
	for (Map<RTileMapCell, Variant>::Element *E = p_previous_values.front(); E; E = E->next()) {
		Vector2i coords = E->key().get_atlas_coords();
		undo_redo->add_undo_property(p_tile_set_atlas_source, vformat("%d:%d/%d/navigation_layer_%d/polygon", coords.x, coords.y, E->key().alternative_tile, navigation_layer), E->value());
		undo_redo->add_do_property(p_tile_set_atlas_source, vformat("%d:%d/%d/navigation_layer_%d/polygon", coords.x, coords.y, E->key().alternative_tile, navigation_layer), p_new_value);
	}
}

void RTileDataNavigationEditor::_tile_set_changed() {
	polygon_editor->set_tile_set(tile_set);
}

void RTileDataNavigationEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			polygon_editor->set_polygons_color(get_tree()->get_debug_navigation_color());
			break;
		default:
			break;
	}
}

RTileDataNavigationEditor::RTileDataNavigationEditor() {
	polygon_editor = memnew(RGenericTilePolygonEditor);
	polygon_editor->set_multiple_polygon_mode(true);
	add_child(polygon_editor);
}

void RTileDataNavigationEditor::draw_over_tile(CanvasItem *p_canvas_item, Transform2D p_transform, RTileMapCell p_cell, bool p_selected) {
	RTileData *tile_data = _get_tile_data(p_cell);
	ERR_FAIL_COND(!tile_data);

	// Draw all shapes.
	VisualServer::get_singleton()->canvas_item_add_set_transform(p_canvas_item->get_canvas_item(), p_transform);

	Ref<NavigationPolygon> navigation_polygon = tile_data->get_navigation_polygon(navigation_layer);
	if (navigation_polygon.is_valid()) {
		PoolVector2Array verts = navigation_polygon->get_vertices();
		if (verts.size() < 3) {
			return;
		}

		Color color = p_canvas_item->get_tree()->get_debug_navigation_color();
		if (p_selected) {
			Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
			Color selection_color = Color().from_hsv(Math::fposmod(grid_color.get_h() + 0.5, 1.0), grid_color.get_s(), grid_color.get_v(), 1.0);
			selection_color.a = 0.7;
			color = selection_color;
		}

		RandomPCG rand;
		for (int i = 0; i < navigation_polygon->get_polygon_count(); i++) {
			// An array of vertices for this polygon.
			Vector<int> polygon = navigation_polygon->get_polygon(i);
			Vector<Vector2> vertices;
			vertices.resize(polygon.size());
			for (int j = 0; j < polygon.size(); j++) {
				ERR_FAIL_INDEX(polygon[j], verts.size());
				vertices.write[j] = verts[polygon[j]];
			}

			// Generate the polygon color, slightly randomly modified from the settings one.
			Color random_variation_color;
			random_variation_color.set_hsv(color.get_h() + rand.random(-1.0, 1.0) * 0.05, color.get_s(), color.get_v() + rand.random(-1.0, 1.0) * 0.1);
			random_variation_color.a = color.a;
			Vector<Color> colors;
			colors.push_back(random_variation_color);

			VisualServer::get_singleton()->canvas_item_add_polygon(p_canvas_item->get_canvas_item(), vertices, colors);
		}
	}

	VisualServer::get_singleton()->canvas_item_add_set_transform(p_canvas_item->get_canvas_item(), Transform2D());
}
