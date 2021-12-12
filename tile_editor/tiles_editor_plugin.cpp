/*************************************************************************/
/*  tiles_editor_plugin.cpp                                              */
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

#include "tiles_editor_plugin.h"

#include "core/os/mutex.h"

#include "editor/editor_node.h"
#include "editor/editor_scale.h"
#include "editor/plugins/canvas_item_editor_plugin.h"
#include "scene/main/viewport.h"

#include "../rtile_map.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/control.h"
#include "scene/gui/separator.h"
#include "../rtile_set.h"

#include "tile_set_editor.h"

RTilesEditorPlugin *RTilesEditorPlugin::singleton = nullptr;

void RTilesEditorPlugin::_preview_frame_started() {
	VS::get_singleton()->request_frame_drawn_callback(this, "_pattern_preview_done", Variant());
}

void RTilesEditorPlugin::_pattern_preview_done(const Variant &p_userdata) {
	pattern_preview_done.post();
}

void RTilesEditorPlugin::_thread_func(void *ud) {
	RTilesEditorPlugin *te = (RTilesEditorPlugin *)ud;
	te->_thread();
}

void RTilesEditorPlugin::_thread() {
	pattern_thread_exited.clear();
	while (!pattern_thread_exit.is_set()) {
		pattern_preview_sem.wait();

		pattern_preview_mutex.lock();
		if (pattern_preview_queue.size()) {
			QueueItem item = pattern_preview_queue.front()->get();
			pattern_preview_queue.pop_front();
			pattern_preview_mutex.unlock();

			int thumbnail_size = EditorSettings::get_singleton()->get("filesystem/file_dialog/thumbnail_size");
			thumbnail_size *= EDSCALE;
			Vector2 thumbnail_size2 = Vector2(thumbnail_size, thumbnail_size);

			if (item.pattern.is_valid() && !item.pattern->is_empty()) {
				// Generate the pattern preview
				Viewport *viewport = memnew(Viewport);
				viewport->set_size(thumbnail_size2);
				viewport->set_disable_input(true);
				viewport->set_transparent_background(true);
				viewport->set_update_mode(Viewport::UPDATE_ONCE);

				RTileMap *tile_map = memnew(RTileMap);
				tile_map->set_tileset(item.tile_set);
				tile_map->set_pattern(0, Vector2(), item.pattern);
				viewport->add_child(tile_map);

				Vector<Vector2> used_cells = tile_map->get_used_cells(0);

				Rect2 encompassing_rect = Rect2();
				encompassing_rect.set_position(tile_map->map_to_world(used_cells[0]));
				for (int i = 0; i < used_cells.size(); i++) {
					Vector2i cell = used_cells[i];
					Vector2 world_pos = tile_map->map_to_world(cell);
					encompassing_rect.expand_to(world_pos);

					// Texture.
					Ref<RTileSetAtlasSource> atlas_source = tile_set->get_source(tile_map->get_cell_source_id(0, cell));
					if (atlas_source.is_valid()) {
						Vector2i coords = tile_map->get_cell_atlas_coords(0, cell);
						int alternative = tile_map->get_cell_alternative_tile(0, cell);

						Vector2 center = world_pos - atlas_source->get_tile_effective_texture_offset(coords, alternative);
						encompassing_rect.expand_to(center - atlas_source->get_tile_texture_region(coords).size / 2);
						encompassing_rect.expand_to(center + atlas_source->get_tile_texture_region(coords).size / 2);
					}
				}

				Vector2 scale = thumbnail_size2 / MAX(encompassing_rect.size.x, encompassing_rect.size.y);
				tile_map->set_scale(scale);
				tile_map->set_position(-(scale * encompassing_rect.get_center()) + thumbnail_size2 / 2);

				// Add the viewport at the lasst moment to avoid rendering too early.
				EditorNode::get_singleton()->add_child(viewport);

				VS::get_singleton()->connect(("frame_pre_draw"), this, "_preview_frame_started", Vector<Variant>(), Object::CONNECT_ONESHOT);

				pattern_preview_done.wait();

				Ref<Image> image = viewport->get_texture()->get_data();
				Ref<ImageTexture> image_texture;
				image_texture.instance();
				image_texture->create_from_image(image);

				// Find the index for the given pattern. TODO: optimize.
				Variant args[] = { item.pattern, image_texture };
				const Variant *args_ptr[] = { &args[0], &args[1] };
				Variant r;
				Variant::CallError error;
				r = item.obj->call(item.callback, args_ptr, 2, error);

				viewport->queue_delete();
			} else {
				pattern_preview_mutex.unlock();
			}
		}
	}
	pattern_thread_exited.set();
}

void RTilesEditorPlugin::_tile_map_changed() {
	tile_map_changed_needs_update = true;
}

void RTilesEditorPlugin::_update_editors() {
	// If tile_map is not edited, we change the edited only if we are not editing a tile_set.
	tileset_editor->edit(tile_set);
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (tile_map) {
		tilemap_editor->edit(tile_map);
	} else {
		tilemap_editor->edit(nullptr);
	}

	// Update the viewport.
	CanvasItemEditor::get_singleton()->update_viewport();
}

void RTilesEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (tile_map_changed_needs_update) {
				RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
				if (tile_map) {
					tile_set = tile_map->get_tileset();
				}
				_update_editors();
				tile_map_changed_needs_update = false;
			}
		} break;
	}
}

void RTilesEditorPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_atlas_view_transform"), &RTilesEditorPlugin::set_atlas_view_transform);
	ClassDB::bind_method(D_METHOD("set_sources_lists_current"), &RTilesEditorPlugin::set_sources_lists_current);
	ClassDB::bind_method(D_METHOD("synchronize_sources_list"), &RTilesEditorPlugin::synchronize_sources_list);

	ClassDB::bind_method(D_METHOD("_tile_map_changed"), &RTilesEditorPlugin::_tile_map_changed);
	ClassDB::bind_method(D_METHOD("_pattern_preview_done"), &RTilesEditorPlugin::_pattern_preview_done);

	ClassDB::bind_method(D_METHOD("_preview_frame_started"), &RTilesEditorPlugin::_preview_frame_started);
}

void RTilesEditorPlugin::make_visible(bool p_visible) {
	if (p_visible) {
		// Disable and hide invalid editors.
		RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
		tileset_editor_button->set_visible(tile_set.is_valid());
		tilemap_editor_button->set_visible(tile_map);
		if (tile_map) {
			editor_node->make_bottom_panel_item_visible(tilemap_editor);
		} else {
			editor_node->make_bottom_panel_item_visible(tileset_editor);
		}

	} else {
		tileset_editor_button->hide();
		tilemap_editor_button->hide();
		editor_node->hide_bottom_panel();
	}
}

void RTilesEditorPlugin::queue_pattern_preview(Ref<RTileSet> p_tile_set, Ref<RTileMapPattern> p_pattern, Object *obj, String p_callback) {
	ERR_FAIL_COND(!p_tile_set.is_valid());
	ERR_FAIL_COND(!p_pattern.is_valid());
	{
		MutexLock lock(pattern_preview_mutex);
		pattern_preview_queue.push_back({ p_tile_set, p_pattern, obj, p_callback });
	}
	pattern_preview_sem.post();
}

void RTilesEditorPlugin::set_sources_lists_current(int p_current) {
	atlas_sources_lists_current = p_current;
}

void RTilesEditorPlugin::synchronize_sources_list(Object *p_current) {
	ItemList *item_list = Object::cast_to<ItemList>(p_current);
	ERR_FAIL_COND(!item_list);

	if (item_list->is_visible_in_tree()) {
		if (atlas_sources_lists_current < 0 || atlas_sources_lists_current >= item_list->get_item_count()) {
			item_list->unselect_all();
		} else {
			item_list->set_current(atlas_sources_lists_current);
			item_list->emit_signal(("item_selected"), atlas_sources_lists_current);
		}
	}
}

void RTilesEditorPlugin::set_atlas_view_transform(float p_zoom, Vector2 p_scroll) {
	atlas_view_zoom = p_zoom;
	atlas_view_scroll = p_scroll;
}

void RTilesEditorPlugin::synchronize_atlas_view(Object *p_current) {
	RTileAtlasView *tile_atlas_view = Object::cast_to<RTileAtlasView>(p_current);
	ERR_FAIL_COND(!tile_atlas_view);

	if (tile_atlas_view->is_visible_in_tree()) {
		tile_atlas_view->set_transform(atlas_view_zoom, atlas_view_scroll);
	}
}

void RTilesEditorPlugin::edit(Object *p_object) {
	// Disconnect to changes.
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (tile_map) {
		tile_map->disconnect("changed", this, "_tile_map_changed");
	}

	// Update edited objects.
	tile_set = Ref<RTileSet>();
	if (p_object) {
		if (p_object->is_class("RTileMap")) {
			tile_map_id = p_object->get_instance_id();
			tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
			tile_set = tile_map->get_tileset();
			editor_node->make_bottom_panel_item_visible(tilemap_editor);
		} else if (p_object->is_class("RTileSet")) {
			tile_set = Ref<RTileSet>(p_object);
			if (tile_map) {
				if (tile_map->get_tileset() != tile_set || !tile_map->is_inside_tree()) {
					tile_map = nullptr;
					tile_map_id = ObjectID();
				}
			}
			editor_node->make_bottom_panel_item_visible(tileset_editor);
		}
	}

	// Update the editors.
	_update_editors();

	// Add change listener.
	if (tile_map) {
		tile_map->connect("changed", this, "_tile_map_changed");
	}
}

bool RTilesEditorPlugin::handles(Object *p_object) const {
	return p_object->is_class("RTileMap") || p_object->is_class("RTileSet");
}

RTilesEditorPlugin::RTilesEditorPlugin(EditorNode *p_node) {
	set_process_internal(true);

	EDITOR_DEF("editors/tiles_editor/display_grid", true);
	EDITOR_DEF("editors/tiles_editor/grid_color", Color(1, 0.5, 0.2, 0.5));

	// Update the singleton.
	singleton = this;

	editor_node = p_node;

	// Tileset editor.
	tileset_editor = memnew(RTileSetEditor);
	tileset_editor->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_editor->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_editor->set_custom_minimum_size(Size2(0, 200) * EDSCALE);
	tileset_editor->hide();

	// Tilemap editor.
	tilemap_editor = memnew(RTileMapEditor);
	tilemap_editor->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tilemap_editor->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tilemap_editor->set_custom_minimum_size(Size2(0, 200) * EDSCALE);
	tilemap_editor->hide();

	// Pattern preview generation thread.
	pattern_preview_thread.start(_thread_func, this);

	// Bottom buttons.
	tileset_editor_button = p_node->add_bottom_panel_item(TTR("RTileSet"), tileset_editor);
	tileset_editor_button->hide();
	tilemap_editor_button = p_node->add_bottom_panel_item(TTR("RTileMap"), tilemap_editor);
	tilemap_editor_button->hide();

	// Initialization.
	_update_editors();
}

RTilesEditorPlugin::~RTilesEditorPlugin() {
	if (pattern_preview_thread.is_started()) {
		pattern_thread_exit.set();
		pattern_preview_sem.post();
		while (!pattern_thread_exited.is_set()) {
			OS::get_singleton()->delay_usec(10000);
			VisualServer::get_singleton()->sync(); //sync pending stuff, as thread may be blocked on visual server
		}
		pattern_preview_thread.wait_to_finish();
	}
}
