/*************************************************************************/
/*  tile_map_editor.cpp                                                  */
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

#include "tile_map_editor.h"

#include "tiles_editor_plugin.h"

#include "editor/editor_resource_preview.h"
#include "editor/editor_scale.h"
#include "editor/plugins/canvas_item_editor_plugin.h"

#include "scene/2d/camera_2d.h"
#include "scene/gui/center_container.h"
#include "scene/gui/split_container.h"

#include "core/os/input.h"
#include "../geometry_2d.h"
#include "core/os/keyboard.h"
#include "../math_ext.h"

void RTileMapEditorTilesPlugin::tile_set_changed() {
	_update_fix_selected_and_hovered();
	_update_tile_set_sources_list();
	_update_source_display();
	_update_patterns_list();
}

void RTileMapEditorTilesPlugin::_on_random_tile_checkbox_toggled(bool p_pressed) {
	scatter_spinbox->set_editable(p_pressed);
}

void RTileMapEditorTilesPlugin::_on_scattering_spinbox_changed(double p_value) {
	scattering = p_value;
}

void RTileMapEditorTilesPlugin::_update_toolbar() {
	// Stop draggig if needed.
	_stop_dragging();

	// Hide all settings.
	for (int i = 0; i < tools_settings->get_child_count(); i++) {
		Object::cast_to<CanvasItem>(tools_settings->get_child(i))->hide();
	}

	// Show only the correct settings.
	if (tool_buttons_group->get_pressed_button() == select_tool_button) {
	} else if (tool_buttons_group->get_pressed_button() == paint_tool_button) {
		tools_settings_vsep->show();
		picker_button->show();
		erase_button->show();
		tools_settings_vsep_2->show();
		random_tile_checkbox->show();
		scatter_label->show();
		scatter_spinbox->show();
	} else if (tool_buttons_group->get_pressed_button() == line_tool_button) {
		tools_settings_vsep->show();
		picker_button->show();
		erase_button->show();
		tools_settings_vsep_2->show();
		random_tile_checkbox->show();
		scatter_label->show();
		scatter_spinbox->show();
	} else if (tool_buttons_group->get_pressed_button() == rect_tool_button) {
		tools_settings_vsep->show();
		picker_button->show();
		erase_button->show();
		tools_settings_vsep_2->show();
		random_tile_checkbox->show();
		scatter_label->show();
		scatter_spinbox->show();
	} else if (tool_buttons_group->get_pressed_button() == bucket_tool_button) {
		tools_settings_vsep->show();
		picker_button->show();
		erase_button->show();
		tools_settings_vsep_2->show();
		bucket_contiguous_checkbox->show();
		random_tile_checkbox->show();
		scatter_label->show();
		scatter_spinbox->show();
	}
}

Vector<RTileMapEditorPlugin::TabData> RTileMapEditorTilesPlugin::get_tabs() const {
	Vector<RTileMapEditorPlugin::TabData> tabs;
	tabs.push_back({ toolbar, tiles_bottom_panel });
	tabs.push_back({ toolbar, patterns_bottom_panel });
	return tabs;
}

void RTileMapEditorTilesPlugin::_tab_changed() {
	if (tiles_bottom_panel->is_visible_in_tree()) {
		_update_selection_pattern_from_tileset_tiles_selection();
	} else if (patterns_bottom_panel->is_visible_in_tree()) {
		_update_selection_pattern_from_tileset_pattern_selection();
	}
}

void RTileMapEditorTilesPlugin::_update_tile_set_sources_list() {
	// Update the sources.
	int old_current = sources_list->get_current();
	sources_list->clear();

	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	for (int i = 0; i < tile_set->get_source_count(); i++) {
		int source_id = tile_set->get_source_id(i);

		RTileSetSource *source = *tile_set->get_source(source_id);

		Ref<Texture> texture;
		String item_text;

		// Common to all type of sources.
		if (!source->get_name().empty()) {
			item_text = vformat(TTR("%s (id:%d)"), source->get_name(), source_id);
		}

		// Atlas source.
		RTileSetAtlasSource *atlas_source = Object::cast_to<RTileSetAtlasSource>(source);
		if (atlas_source) {
			texture = atlas_source->get_texture();
			if (item_text.empty()) {
				if (texture.is_valid()) {
					item_text = vformat("%s (ID: %d)", texture->get_path().get_file(), source_id);
				} else {
					item_text = vformat("No Texture Atlas Source (ID: %d)", source_id);
				}
			}
		}

		// Scene collection source.
		RTileSetScenesCollectionSource *scene_collection_source = Object::cast_to<RTileSetScenesCollectionSource>(source);
		if (scene_collection_source) {
			texture = tiles_bottom_panel->get_icon(("PackedScene"), ("EditorIcons"));
			if (item_text.empty()) {
				item_text = vformat(TTR("Scene Collection Source (ID: %d)"), source_id);
			}
		}

		// Use default if not valid.
		if (item_text.empty()) {
			item_text = vformat(TTR("Unknown Type Source (ID: %d)"), source_id);
		}
		if (!texture.is_valid()) {
			texture = missing_atlas_texture_icon;
		}

		sources_list->add_item(item_text, texture);
		sources_list->set_item_metadata(i, source_id);
	}

	if (sources_list->get_item_count() > 0) {
		if (old_current > 0) {
			// Keep the current selected item if needed.
			sources_list->set_current(CLAMP(old_current, 0, sources_list->get_item_count() - 1));
		} else {
			sources_list->set_current(0);
		}
		sources_list->emit_signal(("item_selected"), sources_list->get_current());
	}

	// Synchronize
	RTilesEditorPlugin::get_singleton()->set_sources_lists_current(sources_list->get_current());
}

void RTileMapEditorTilesPlugin::_update_source_display(const int index) {
	// Update the atlas display.
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	int source_index = sources_list->get_current();
	if (source_index >= 0 && source_index < sources_list->get_item_count()) {
		atlas_sources_split_container->show();
		missing_source_label->hide();

		int source_id = sources_list->get_item_metadata(source_index);
		RTileSetSource *source = *tile_set->get_source(source_id);
		RTileSetAtlasSource *atlas_source = Object::cast_to<RTileSetAtlasSource>(source);
		RTileSetScenesCollectionSource *scenes_collection_source = Object::cast_to<RTileSetScenesCollectionSource>(source);

		if (atlas_source) {
			tile_atlas_view->show();
			scene_tiles_list->hide();
			invalid_source_label->hide();
			_update_atlas_view();
		} else if (scenes_collection_source) {
			tile_atlas_view->hide();
			scene_tiles_list->show();
			invalid_source_label->hide();
			_update_scenes_collection_view();
		} else {
			tile_atlas_view->hide();
			scene_tiles_list->hide();
			invalid_source_label->show();
		}
	} else {
		atlas_sources_split_container->hide();
		missing_source_label->show();

		tile_atlas_view->hide();
		scene_tiles_list->hide();
		invalid_source_label->hide();
	}
}

void RTileMapEditorTilesPlugin::_patterns_item_list_gui_input(const Ref<InputEvent> &p_event) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	if (ED_IS_SHORTCUT("tiles_editor/paste", p_event) && p_event->is_pressed() && !p_event->is_echo()) {
		select_last_pattern = true;
		int new_pattern_index = tile_set->get_patterns_count();
		undo_redo->create_action(TTR("Add RTileSet pattern"));
		undo_redo->add_do_method(*tile_set, "add_pattern", tile_map_clipboard, new_pattern_index);
		undo_redo->add_undo_method(*tile_set, "remove_pattern", new_pattern_index);
		undo_redo->commit_action();
		patterns_item_list->accept_event();
	}

	if (ED_IS_SHORTCUT("tiles_editor/delete", p_event) && p_event->is_pressed() && !p_event->is_echo()) {
		Vector<int> selected = patterns_item_list->get_selected_items();
		undo_redo->create_action(TTR("Remove RTileSet patterns"));
		for (int i = 0; i < selected.size(); i++) {
			int pattern_index = selected[i];
			undo_redo->add_do_method(*tile_set, "remove_pattern", pattern_index);
			undo_redo->add_undo_method(*tile_set, "add_pattern", tile_set->get_pattern(pattern_index), pattern_index);
		}
		undo_redo->commit_action();
		patterns_item_list->accept_event();
	}
}

void RTileMapEditorTilesPlugin::_pattern_preview_done(Ref<RTileMapPattern> p_pattern, Ref<Texture> p_texture) {
	// TODO optimize ?
	for (int i = 0; i < patterns_item_list->get_item_count(); i++) {
		if (patterns_item_list->get_item_metadata(i) == p_pattern) {
			patterns_item_list->set_item_icon(i, p_texture);
			break;
		}
	}
}

void RTileMapEditorTilesPlugin::_update_patterns_list() {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	// Recreate the items.
	patterns_item_list->clear();
	for (int i = 0; i < tile_set->get_patterns_count(); i++) {
		patterns_item_list->add_item("");
		int id = patterns_item_list->get_item_count() - 1;
		patterns_item_list->set_item_metadata(id, tile_set->get_pattern(i));
		RTilesEditorPlugin::get_singleton()->queue_pattern_preview(tile_set, tile_set->get_pattern(i), this, "_pattern_preview_done");
	}

	// Update the label visibility.
	patterns_help_label->set_visible(patterns_item_list->get_item_count() == 0);

	// Added a new pattern, thus select the last one.
	if (select_last_pattern) {
		patterns_item_list->select(tile_set->get_patterns_count() - 1);
		patterns_item_list->grab_focus();
		_update_selection_pattern_from_tileset_pattern_selection();
	}
	select_last_pattern = false;
}

void RTileMapEditorTilesPlugin::_update_atlas_view() {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	int source_id = sources_list->get_item_metadata(sources_list->get_current());
	RTileSetSource *source = *tile_set->get_source(source_id);
	RTileSetAtlasSource *atlas_source = Object::cast_to<RTileSetAtlasSource>(source);
	ERR_FAIL_COND(!atlas_source);

	tile_atlas_view->set_atlas_source(*tile_map->get_tileset(), atlas_source, source_id);
	RTilesEditorPlugin::get_singleton()->synchronize_atlas_view(tile_atlas_view);
	tile_atlas_control->update();
}

void RTileMapEditorTilesPlugin::_update_scenes_collection_view() {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	int source_id = sources_list->get_item_metadata(sources_list->get_current());
	RTileSetSource *source = *tile_set->get_source(source_id);
	RTileSetScenesCollectionSource *scenes_collection_source = Object::cast_to<RTileSetScenesCollectionSource>(source);
	ERR_FAIL_COND(!scenes_collection_source);

	// Clear the list.
	scene_tiles_list->clear();

	// Rebuild the list.
	for (int i = 0; i < scenes_collection_source->get_scene_tiles_count(); i++) {
		int scene_id = scenes_collection_source->get_scene_tile_id(i);

		Ref<PackedScene> scene = scenes_collection_source->get_scene_tile_scene(scene_id);

		int item_index = 0;
		if (scene.is_valid()) {
			scene_tiles_list->add_item(vformat("%s (Path: %s, ID: %d)", scene->get_path().get_file().get_basename(), scene->get_path(), scene_id));
			item_index = scene_tiles_list->get_item_count() - 1;
			Variant udata = i;
			EditorResourcePreview::get_singleton()->queue_edited_resource_preview(scene, this, "_scene_thumbnail_done", udata);
		} else {
			scene_tiles_list->add_item(TTR("Tile with Invalid Scene"), tiles_bottom_panel->get_icon(("PackedScene"), ("EditorIcons")));
			item_index = scene_tiles_list->get_item_count() - 1;
		}
		scene_tiles_list->set_item_metadata(item_index, scene_id);

		// Check if in selection.
		if (tile_set_selection.has(RTileMapCell(source_id, Vector2i(), scene_id))) {
			scene_tiles_list->select(item_index, false);
		}
	}

	// Icon size update.
	int int_size = int(EditorSettings::get_singleton()->get("filesystem/file_dialog/thumbnail_size")) * EDSCALE;
	scene_tiles_list->set_fixed_icon_size(Vector2(int_size, int_size));
}

void RTileMapEditorTilesPlugin::_scene_thumbnail_done(const String &p_path, const Ref<Texture> &p_preview, const Ref<Texture> &p_small_preview, Variant p_ud) {
	int index = p_ud;

	if (index >= 0 && index < scene_tiles_list->get_item_count()) {
		scene_tiles_list->set_item_icon(index, p_preview);
	}
}

void RTileMapEditorTilesPlugin::_scenes_list_multi_selected(int p_index, bool p_selected) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	// Add or remove the Tile form the selection.
	int scene_id = scene_tiles_list->get_item_metadata(p_index);
	int source_id = sources_list->get_item_metadata(sources_list->get_current());
	RTileSetSource *source = *tile_set->get_source(source_id);
	RTileSetScenesCollectionSource *scenes_collection_source = Object::cast_to<RTileSetScenesCollectionSource>(source);
	ERR_FAIL_COND(!scenes_collection_source);

	RTileMapCell selected = RTileMapCell(source_id, Vector2i(), scene_id);

	// Clear the selection if shift is not pressed.
	if (!Input::get_singleton()->is_key_pressed(KEY_SHIFT)) {
		tile_set_selection.clear();
	}

	if (p_selected) {
		tile_set_selection.insert(selected);
	} else {
		if (tile_set_selection.has(selected)) {
			tile_set_selection.erase(selected);
		}
	}

	_update_selection_pattern_from_tileset_tiles_selection();
}

void RTileMapEditorTilesPlugin::_scenes_list_nothing_selected() {
	scene_tiles_list->unselect_all();
	tile_set_selection.clear();
	tile_map_selection.clear();
	selection_pattern.instance();
	_update_selection_pattern_from_tileset_tiles_selection();
}

void RTileMapEditorTilesPlugin::_update_theme() {
	select_tool_button->set_icon(tiles_bottom_panel->get_icon(("ToolSelect"), ("EditorIcons")));
	paint_tool_button->set_icon(tiles_bottom_panel->get_icon(("Edit"), ("EditorIcons")));
	line_tool_button->set_icon(tiles_bottom_panel->get_icon(("CurveLinear"), ("EditorIcons")));
	rect_tool_button->set_icon(tiles_bottom_panel->get_icon(("Rectangle"), ("EditorIcons")));
	bucket_tool_button->set_icon(tiles_bottom_panel->get_icon(("Bucket"), ("EditorIcons")));

	picker_button->set_icon(tiles_bottom_panel->get_icon(("ColorPick"), ("EditorIcons")));
	erase_button->set_icon(tiles_bottom_panel->get_icon(("Eraser"), ("EditorIcons")));

	missing_atlas_texture_icon = tiles_bottom_panel->get_icon(("RTileSet"), ("EditorIcons"));
}

bool RTileMapEditorTilesPlugin::forward_canvas_gui_input(const Ref<InputEvent> &p_event) {
	if (!(tiles_bottom_panel->is_visible_in_tree() || patterns_bottom_panel->is_visible_in_tree())) {
		// If the bottom editor is not visible, we ignore inputs.
		return false;
	}

	if (CanvasItemEditor::get_singleton()->get_current_tool() != CanvasItemEditor::TOOL_SELECT) {
		return false;
	}

	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return false;
	}

	if (tile_map_layer < 0) {
		return false;
	}
	ERR_FAIL_INDEX_V(tile_map_layer, tile_map->get_layers_count(), false);

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return false;
	}

	// Shortcuts
	if (ED_IS_SHORTCUT("tiles_editor/cut", p_event) || ED_IS_SHORTCUT("tiles_editor/copy", p_event)) {
		// Fill in the clipboard.
		if (!tile_map_selection.empty()) {
			tile_map_clipboard.instance();
			Vector<Vector2> coords_array;
			for (Set<Vector2i>::Element *E = tile_map_selection.front(); E; E = E->next()) {
				coords_array.push_back(Vector2(E->get()));
			}
			tile_map_clipboard = tile_map->get_pattern(tile_map_layer, coords_array);
		}

		if (ED_IS_SHORTCUT("tiles_editor/cut", p_event)) {
			// Delete selected tiles.
			if (!tile_map_selection.empty()) {
				undo_redo->create_action(TTR("Delete tiles"));
				for (Set<Vector2i>::Element *E = tile_map_selection.front(); E; E = E->next()) {
					undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, Vector2(E->get()), RTileSet::INVALID_SOURCE, RTileSetSource::INVALID_ATLAS_COORDSV, RTileSetSource::INVALID_TILE_ALTERNATIVE);
					undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, Vector2(E->get()), tile_map->get_cell_source_id(tile_map_layer, E->get()), tile_map->get_cell_atlas_coords(tile_map_layer, E->get()), tile_map->get_cell_alternative_tile(tile_map_layer, E->get()));
				}
				//undo_redo->add_undo_method(this, "_set_tile_map_selection", _get_tile_map_selection());
				tile_map_selection.clear();
				_set_tile_map_selection(_get_tile_map_selection());
				//undo_redo->add_do_method(this, "_set_tile_map_selection", _get_tile_map_selection());
				undo_redo->commit_action();
			}
		}

		return true;
	}
	if (ED_IS_SHORTCUT("tiles_editor/paste", p_event)) {
		if (drag_type == DRAG_TYPE_NONE) {
			drag_type = DRAG_TYPE_CLIPBOARD_PASTE;
		}
		CanvasItemEditor::get_singleton()->update_viewport();
		return true;
	}
	if (ED_IS_SHORTCUT("tiles_editor/cancel", p_event)) {
		if (drag_type == DRAG_TYPE_CLIPBOARD_PASTE) {
			drag_type = DRAG_TYPE_NONE;
			CanvasItemEditor::get_singleton()->update_viewport();
			return true;
		}
	}
	if (ED_IS_SHORTCUT("tiles_editor/delete", p_event)) {
		// Delete selected tiles.
		if (!tile_map_selection.empty()) {
			undo_redo->create_action(TTR("Delete tiles"));
			for (Set<Vector2i>::Element *E = tile_map_selection.front(); E; E = E->next()) {
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, Vector2(E->get()), RTileSet::INVALID_SOURCE, RTileSetSource::INVALID_ATLAS_COORDSV, RTileSetSource::INVALID_TILE_ALTERNATIVE);
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, Vector2(E->get()), tile_map->get_cell_source_id(tile_map_layer, Vector2(E->get())), tile_map->get_cell_atlas_coords(tile_map_layer, Vector2(E->get())), tile_map->get_cell_alternative_tile(tile_map_layer, Vector2(E->get())));
			}
			//undo_redo->add_undo_method(this, "_set_tile_map_selection", _get_tile_map_selection());
			tile_map_selection.clear();
			_set_tile_map_selection(_get_tile_map_selection());
			//undo_redo->add_do_method(this, "_set_tile_map_selection", _get_tile_map_selection());
			undo_redo->commit_action();
		}
		return true;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		has_mouse = true;
		Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
		Vector2 mpos = xform.affine_inverse().xform(mm->get_position());

		switch (drag_type) {
			case DRAG_TYPE_PAINT: {
				Map<Vector2i, RTileMapCell> to_draw = _draw_line(drag_start_mouse_pos, drag_last_mouse_pos, mpos, drag_erasing);
				for (Map<Vector2i, RTileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
					if (!drag_erasing && E->value().source_id == RTileSet::INVALID_SOURCE) {
						continue;
					}
					Vector2i coords = E->key();
					if (!drag_modified.has(coords)) {
						drag_modified.insert(coords, tile_map->get_cell(tile_map_layer, coords));
					}
					tile_map->set_cell(tile_map_layer, coords, E->value().source_id, E->value().get_atlas_coords(), E->value().alternative_tile);
				}
				_fix_invalid_tiles_in_tile_map_selection();
			} break;
			case DRAG_TYPE_BUCKET: {
				Vector<Vector2i> line = RTileMapEditor::get_line(tile_map, tile_map->world_to_map(drag_last_mouse_pos), tile_map->world_to_map(mpos));
				for (int i = 0; i < line.size(); i++) {
					if (!drag_modified.has(line[i])) {
						Map<Vector2i, RTileMapCell> to_draw = _draw_bucket_fill(line[i], bucket_contiguous_checkbox->is_pressed(), drag_erasing);
						for (Map<Vector2i, RTileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
							if (!drag_erasing && E->value().source_id == RTileSet::INVALID_SOURCE) {
								continue;
							}
							Vector2i coords = E->key();
							if (!drag_modified.has(coords)) {
								drag_modified.insert(coords, tile_map->get_cell(tile_map_layer, coords));
							}
							tile_map->set_cell(tile_map_layer, coords, E->value().source_id, E->value().get_atlas_coords(), E->value().alternative_tile);
						}
					}
				}
				_fix_invalid_tiles_in_tile_map_selection();
			} break;
			default:
				break;
		}
		drag_last_mouse_pos = mpos;
		CanvasItemEditor::get_singleton()->update_viewport();

		return true;
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		has_mouse = true;
		Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
		Vector2 mpos = xform.affine_inverse().xform(mb->get_position());

		if (mb->get_button_index() == BUTTON_LEFT || mb->get_button_index() == BUTTON_RIGHT) {
			if (mb->is_pressed()) {
				// Pressed
				if (erase_button->is_pressed() || mb->get_button_index() == BUTTON_RIGHT) {
					drag_erasing = true;
				}

				if (drag_type == DRAG_TYPE_CLIPBOARD_PASTE) {
					// Cancel tile pasting on right-click
					if (mb->get_button_index() == BUTTON_RIGHT) {
						drag_type = DRAG_TYPE_NONE;
					}
				} else if (tool_buttons_group->get_pressed_button() == select_tool_button) {
					drag_start_mouse_pos = mpos;
					if (tile_map_selection.has(tile_map->world_to_map(drag_start_mouse_pos)) && !mb->get_shift()) {
						// Move the selection
						_update_selection_pattern_from_tilemap_selection(); // Make sure the pattern is up to date before moving.
						drag_type = DRAG_TYPE_MOVE;
						drag_modified.clear();
						for (Set<Vector2i>::Element *E = tile_map_selection.front(); E; E = E->next()) {
							Vector2i coords = E->get();
							drag_modified.insert(coords, tile_map->get_cell(tile_map_layer, coords));
							tile_map->set_cell(tile_map_layer, coords, RTileSet::INVALID_SOURCE, RTileSetSource::INVALID_ATLAS_COORDS, RTileSetSource::INVALID_TILE_ALTERNATIVE);
						}
					} else {
						// Select tiles
						drag_type = DRAG_TYPE_SELECT;
					}
				} else {
					// Check if we are picking a tile.
					if (picker_button->is_pressed() || (Input::get_singleton()->is_key_pressed(KEY_CONTROL) && !Input::get_singleton()->is_key_pressed(KEY_SHIFT))) {
						drag_type = DRAG_TYPE_PICK;
						drag_start_mouse_pos = mpos;
					} else {
						// Paint otherwise.
						if (tool_buttons_group->get_pressed_button() == paint_tool_button && !Input::get_singleton()->is_key_pressed(KEY_CONTROL) && !Input::get_singleton()->is_key_pressed(KEY_SHIFT)) {
							drag_type = DRAG_TYPE_PAINT;
							drag_start_mouse_pos = mpos;
							drag_modified.clear();
							Map<Vector2i, RTileMapCell> to_draw = _draw_line(drag_start_mouse_pos, mpos, mpos, drag_erasing);
							for (Map<Vector2i, RTileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
								if (!drag_erasing && E->value().source_id == RTileSet::INVALID_SOURCE) {
									continue;
								}
								Vector2i coords = E->key();
								if (!drag_modified.has(coords)) {
									drag_modified.insert(coords, tile_map->get_cell(tile_map_layer, coords));
								}
								tile_map->set_cell(tile_map_layer, coords, E->value().source_id, E->value().get_atlas_coords(), E->value().alternative_tile);
							}
							_fix_invalid_tiles_in_tile_map_selection();
						} else if (tool_buttons_group->get_pressed_button() == line_tool_button || (tool_buttons_group->get_pressed_button() == paint_tool_button && Input::get_singleton()->is_key_pressed(KEY_SHIFT) && !Input::get_singleton()->is_key_pressed(KEY_CONTROL))) {
							drag_type = DRAG_TYPE_LINE;
							drag_start_mouse_pos = mpos;
							drag_modified.clear();
						} else if (tool_buttons_group->get_pressed_button() == rect_tool_button || (tool_buttons_group->get_pressed_button() == paint_tool_button && Input::get_singleton()->is_key_pressed(KEY_SHIFT) && Input::get_singleton()->is_key_pressed(KEY_CONTROL))) {
							drag_type = DRAG_TYPE_RECT;
							drag_start_mouse_pos = mpos;
							drag_modified.clear();
						} else if (tool_buttons_group->get_pressed_button() == bucket_tool_button) {
							drag_type = DRAG_TYPE_BUCKET;
							drag_start_mouse_pos = mpos;
							drag_modified.clear();
							Vector<Vector2i> line = RTileMapEditor::get_line(tile_map, tile_map->world_to_map(drag_last_mouse_pos), tile_map->world_to_map(mpos));
							for (int i = 0; i < line.size(); i++) {
								if (!drag_modified.has(line[i])) {
									Map<Vector2i, RTileMapCell> to_draw = _draw_bucket_fill(line[i], bucket_contiguous_checkbox->is_pressed(), drag_erasing);
									for (Map<Vector2i, RTileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
										if (!drag_erasing && E->value().source_id == RTileSet::INVALID_SOURCE) {
											continue;
										}
										Vector2i coords = E->key();
										if (!drag_modified.has(coords)) {
											drag_modified.insert(coords, tile_map->get_cell(tile_map_layer, coords));
										}
										tile_map->set_cell(tile_map_layer, coords, E->value().source_id, E->value().get_atlas_coords(), E->value().alternative_tile);
									}
								}
							}
							_fix_invalid_tiles_in_tile_map_selection();
						}
					}
				}

			} else {
				// Released
				_stop_dragging();
				drag_erasing = false;
			}

			CanvasItemEditor::get_singleton()->update_viewport();

			return true;
		}
		drag_last_mouse_pos = mpos;
	}

	return false;
}

void RTileMapEditorTilesPlugin::forward_canvas_draw_over_viewport(Control *p_overlay) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	if (tile_map_layer < 0) {
		return;
	}
	ERR_FAIL_INDEX(tile_map_layer, tile_map->get_layers_count());

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	if (!tile_map->is_visible_in_tree()) {
		return;
	}

	Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
	Vector2i tile_shape_size = tile_set->get_tile_size();

	// Draw the selection.
	if ((tiles_bottom_panel->is_visible_in_tree() || patterns_bottom_panel->is_visible_in_tree()) && tool_buttons_group->get_pressed_button() == select_tool_button) {
		// In select mode, we only draw the current selection if we are modifying it (pressing control or shift).
		if (drag_type == DRAG_TYPE_MOVE || (drag_type == DRAG_TYPE_SELECT && !Input::get_singleton()->is_key_pressed(KEY_CONTROL) && !Input::get_singleton()->is_key_pressed(KEY_SHIFT))) {
			// Do nothing
		} else {
			Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
			Color selection_color = Color().from_hsv(Math::fposmod(grid_color.get_h() + 0.5, 1.0), grid_color.get_s(), grid_color.get_v(), 1.0);
			tile_map->draw_cells_outline(p_overlay, tile_map_selection, selection_color, xform);
		}
	}

	// Handle the preview of the tiles to be placed.
	if ((tiles_bottom_panel->is_visible_in_tree() || patterns_bottom_panel->is_visible_in_tree()) && has_mouse) { // Only if the tilemap editor is opened and the viewport is hovered.
		Map<Vector2i, RTileMapCell> preview;
		Rect2i drawn_grid_rect;

		if (drag_type == DRAG_TYPE_PICK) {
			// Draw the area being picked.
			Rect2i rect = MathExt::rect2i_abs(Rect2i(tile_map->world_to_map(drag_start_mouse_pos), tile_map->world_to_map(drag_last_mouse_pos) - tile_map->world_to_map(drag_start_mouse_pos)));
			rect.size += Vector2i(1, 1);
			Point2i rect_end = MathExt::rect2i_get_end(rect);
			for (int x = rect.position.x; x < rect_end.x; x++) {
				for (int y = rect.position.y; y < rect_end.y; y++) {
					Vector2i coords = Vector2i(x, y);
					if (tile_map->get_cell_source_id(tile_map_layer, coords) != RTileSet::INVALID_SOURCE) {
						Transform2D tile_xform;
						tile_xform.set_origin(tile_map->map_to_world(coords));
						tile_xform.set_scale(tile_shape_size);
						tile_set->draw_tile_shape(p_overlay, xform * tile_xform, Color(1.0, 1.0, 1.0), false);
					}
				}
			}
		} else if (drag_type == DRAG_TYPE_SELECT) {
			// Draw the area being selected.
			Rect2i rect = MathExt::rect2i_abs(Rect2i(tile_map->world_to_map(drag_start_mouse_pos), tile_map->world_to_map(drag_last_mouse_pos) - tile_map->world_to_map(drag_start_mouse_pos)));
			rect.size += Vector2i(1, 1);
			Set<Vector2i> to_draw;
			Point2i rect_end = MathExt::rect2i_get_end(rect);
			for (int x = rect.position.x; x < rect_end.x; x++) {
				for (int y = rect.position.y; y < rect_end.y; y++) {
					Vector2i coords = Vector2i(x, y);
					if (tile_map->get_cell_source_id(tile_map_layer, coords) != RTileSet::INVALID_SOURCE) {
						to_draw.insert(coords);
					}
				}
			}
			tile_map->draw_cells_outline(p_overlay, to_draw, Color(1.0, 1.0, 1.0), xform);
		} else if (drag_type == DRAG_TYPE_MOVE) {
			if (!(patterns_item_list->is_visible_in_tree() && patterns_item_list->has_point(patterns_item_list->get_local_mouse_position()))) {
				// Preview when moving.
				Vector2i top_left;
				if (!tile_map_selection.empty()) {
					top_left = tile_map_selection.front()->get();
				}
				for (Set<Vector2i>::Element *E = tile_map_selection.front(); E; E = E->next()) {
					top_left = MathExt::vector2i_min(top_left, E->get());
				}
				Vector2i offset = drag_start_mouse_pos - tile_map->map_to_world(top_left);
				offset = tile_map->world_to_map(drag_last_mouse_pos - offset) - tile_map->world_to_map(drag_start_mouse_pos - offset);

				PoolVector2Array selection_used_cells = selection_pattern->get_used_cells();
				for (int i = 0; i < selection_used_cells.size(); i++) {
					Vector2i coords = tile_map->map_pattern(offset + top_left, selection_used_cells[i], selection_pattern);
					preview[coords] = RTileMapCell(selection_pattern->get_cell_source_id(selection_used_cells[i]), selection_pattern->get_cell_atlas_coords(selection_used_cells[i]), selection_pattern->get_cell_alternative_tile(selection_used_cells[i]));
				}
			}
		} else if (drag_type == DRAG_TYPE_CLIPBOARD_PASTE) {
			// Preview when pasting.
			Vector2 mouse_offset = (Vector2(tile_map_clipboard->get_size()) / 2.0 - Vector2(0.5, 0.5)) * tile_set->get_tile_size();
			PoolVector2Array clipboard_used_cells = tile_map_clipboard->get_used_cells();
			for (int i = 0; i < clipboard_used_cells.size(); i++) {
				Vector2i coords = tile_map->map_pattern(tile_map->world_to_map(drag_last_mouse_pos - mouse_offset), clipboard_used_cells[i], tile_map_clipboard);
				preview[coords] = RTileMapCell(tile_map_clipboard->get_cell_source_id(clipboard_used_cells[i]), tile_map_clipboard->get_cell_atlas_coords(clipboard_used_cells[i]), tile_map_clipboard->get_cell_alternative_tile(clipboard_used_cells[i]));
			}
		} else if (!picker_button->is_pressed() && !(drag_type == DRAG_TYPE_NONE && Input::get_singleton()->is_key_pressed(KEY_CONTROL) && !Input::get_singleton()->is_key_pressed(KEY_SHIFT))) {
			bool expand_grid = false;
			if (tool_buttons_group->get_pressed_button() == paint_tool_button && drag_type == DRAG_TYPE_NONE) {
				// Preview for a single pattern.
				preview = _draw_line(drag_last_mouse_pos, drag_last_mouse_pos, drag_last_mouse_pos, erase_button->is_pressed());
				expand_grid = true;
			} else if (tool_buttons_group->get_pressed_button() == line_tool_button || drag_type == DRAG_TYPE_LINE) {
				if (drag_type == DRAG_TYPE_NONE) {
					// Preview for a single pattern.
					preview = _draw_line(drag_last_mouse_pos, drag_last_mouse_pos, drag_last_mouse_pos, erase_button->is_pressed());
					expand_grid = true;
				} else if (drag_type == DRAG_TYPE_LINE) {
					// Preview for a line pattern.
					preview = _draw_line(drag_start_mouse_pos, drag_start_mouse_pos, drag_last_mouse_pos, drag_erasing);
					expand_grid = true;
				}
			} else if (drag_type == DRAG_TYPE_RECT) {
				// Preview for a rect pattern.
				preview = _draw_rect(tile_map->world_to_map(drag_start_mouse_pos), tile_map->world_to_map(drag_last_mouse_pos), drag_erasing);
				expand_grid = true;
			} else if (tool_buttons_group->get_pressed_button() == bucket_tool_button && drag_type == DRAG_TYPE_NONE) {
				// Preview for a fill pattern.
				preview = _draw_bucket_fill(tile_map->world_to_map(drag_last_mouse_pos), bucket_contiguous_checkbox->is_pressed(), erase_button->is_pressed());
			}

			// Expand the grid if needed
			if (expand_grid && !preview.empty()) {
				drawn_grid_rect = Rect2i(preview.front()->key(), Vector2i(1, 1));
				for (Map<Vector2i, RTileMapCell>::Element *E = preview.front(); E; E = E->next()) {
					drawn_grid_rect.expand_to(E->key());
				}
			}
		}

		if (!preview.empty()) {
			const int fading = 5;

			// Draw the lines of the grid behind the preview.
			bool display_grid = EditorSettings::get_singleton()->get("editors/tiles_editor/display_grid");
			if (display_grid) {
				Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
				if (drawn_grid_rect.size.x > 0 && drawn_grid_rect.size.y > 0) {
					drawn_grid_rect = drawn_grid_rect.grow(fading);
					for (int x = drawn_grid_rect.position.x; x < (drawn_grid_rect.position.x + drawn_grid_rect.size.x); x++) {
						for (int y = drawn_grid_rect.position.y; y < (drawn_grid_rect.position.y + drawn_grid_rect.size.y); y++) {
							Vector2i pos_in_rect = Vector2i(x, y) - drawn_grid_rect.position;

							// Fade out the border of the grid.
							float left_opacity = CLAMP(Math::inverse_lerp(0.0f, (float)fading, (float)pos_in_rect.x), 0.0f, 1.0f);
							float right_opacity = CLAMP(Math::inverse_lerp((float)drawn_grid_rect.size.x, (float)(drawn_grid_rect.size.x - fading), (float)pos_in_rect.x), 0.0f, 1.0f);
							float top_opacity = CLAMP(Math::inverse_lerp(0.0f, (float)fading, (float)pos_in_rect.y), 0.0f, 1.0f);
							float bottom_opacity = CLAMP(Math::inverse_lerp((float)drawn_grid_rect.size.y, (float)(drawn_grid_rect.size.y - fading), (float)pos_in_rect.y), 0.0f, 1.0f);
							float opacity = CLAMP(MIN(left_opacity, MIN(right_opacity, MIN(top_opacity, bottom_opacity))) + 0.1, 0.0f, 1.0f);

							Transform2D tile_xform;
							tile_xform.set_origin(tile_map->map_to_world(Vector2(x, y)));
							tile_xform.set_scale(tile_shape_size);
							Color color = grid_color;
							color.a = color.a * opacity;
							tile_set->draw_tile_shape(p_overlay, xform * tile_xform, color, false);
						}
					}
				}
			}

			// Draw the preview.
			for (Map<Vector2i, RTileMapCell>::Element *E = preview.front(); E; E = E->next()) {
				Transform2D tile_xform;
				tile_xform.set_origin(tile_map->map_to_world(E->key()));
				tile_xform.set_scale(tile_set->get_tile_size());
				if (!(drag_erasing || erase_button->is_pressed()) && random_tile_checkbox->is_pressed()) {
					tile_set->draw_tile_shape(p_overlay, xform * tile_xform, Color(1.0, 1.0, 1.0, 0.5), true);
				} else {
					if (tile_set->has_source(E->value().source_id)) {
						RTileSetSource *source = *tile_set->get_source(E->value().source_id);
						RTileSetAtlasSource *atlas_source = Object::cast_to<RTileSetAtlasSource>(source);
						if (atlas_source) {
							// Get tile data.
							RTileData *tile_data = Object::cast_to<RTileData>(atlas_source->get_tile_data(E->value().get_atlas_coords(), E->value().alternative_tile));

							// Compute the offset
							Rect2i source_rect = atlas_source->get_tile_texture_region(E->value().get_atlas_coords());
							Vector2i tile_offset = atlas_source->get_tile_effective_texture_offset(E->value().get_atlas_coords(), E->value().alternative_tile);

							// Compute the destination rectangle in the CanvasItem.
							Rect2 dest_rect;
							dest_rect.size = source_rect.size;

							bool transpose = tile_data->get_transpose();
							if (transpose) {
								dest_rect.position = (tile_map->map_to_world(E->key()) - Vector2(dest_rect.size.y, dest_rect.size.x) / 2 - tile_offset);
							} else {
								dest_rect.position = (tile_map->map_to_world(E->key()) - dest_rect.size / 2 - tile_offset);
							}

							dest_rect = xform.xform(dest_rect);

							if (tile_data->get_flip_h()) {
								dest_rect.size.x = -dest_rect.size.x;
							}

							if (tile_data->get_flip_v()) {
								dest_rect.size.y = -dest_rect.size.y;
							}

							// Get the tile modulation.
							Color modulate = tile_data->get_modulate();
							Color self_modulate = tile_map->get_self_modulate();
							modulate *= self_modulate;

							// Draw the tile.
							p_overlay->draw_texture_rect_region(atlas_source->get_texture(), dest_rect, source_rect, modulate * Color(1.0, 1.0, 1.0, 0.5), transpose, Ref<Texture>(), tile_set->is_uv_clipping());
						} else {
							tile_set->draw_tile_shape(p_overlay, xform * tile_xform, Color(1.0, 1.0, 1.0, 0.5), true);
						}
					} else {
						tile_set->draw_tile_shape(p_overlay, xform * tile_xform, Color(0.0, 0.0, 0.0, 0.5), true);
					}
				}
			}
		}
	}
}

void RTileMapEditorTilesPlugin::_mouse_exited_viewport() {
	has_mouse = false;
	CanvasItemEditor::get_singleton()->update_viewport();
}

RTileMapCell RTileMapEditorTilesPlugin::_pick_random_tile(Ref<RTileMapPattern> p_pattern) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return RTileMapCell();
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return RTileMapCell();
	}

	PoolVector2Array used_cells = p_pattern->get_used_cells();
	double sum = 0.0;
	for (int i = 0; i < used_cells.size(); i++) {
		int source_id = p_pattern->get_cell_source_id(used_cells[i]);
		Vector2i atlas_coords = p_pattern->get_cell_atlas_coords(used_cells[i]);
		int alternative_tile = p_pattern->get_cell_alternative_tile(used_cells[i]);

		RTileSetSource *source = *tile_set->get_source(source_id);
		RTileSetAtlasSource *atlas_source = Object::cast_to<RTileSetAtlasSource>(source);
		if (atlas_source) {
			RTileData *tile_data = Object::cast_to<RTileData>(atlas_source->get_tile_data(atlas_coords, alternative_tile));
			ERR_FAIL_COND_V(!tile_data, RTileMapCell());
			sum += tile_data->get_probability();
		} else {
			sum += 1.0;
		}
	}

	double empty_probability = sum * scattering;
	double current = 0.0;
	double rand = Math::random(0.0, sum + empty_probability);
	for (int i = 0; i < used_cells.size(); i++) {
		int source_id = p_pattern->get_cell_source_id(used_cells[i]);
		Vector2i atlas_coords = p_pattern->get_cell_atlas_coords(used_cells[i]);
		int alternative_tile = p_pattern->get_cell_alternative_tile(used_cells[i]);

		RTileSetSource *source = *tile_set->get_source(source_id);
		RTileSetAtlasSource *atlas_source = Object::cast_to<RTileSetAtlasSource>(source);
		if (atlas_source) {
			current += Object::cast_to<RTileData>(atlas_source->get_tile_data(atlas_coords, alternative_tile))->get_probability();
		} else {
			current += 1.0;
		}

		if (current >= rand) {
			return RTileMapCell(source_id, atlas_coords, alternative_tile);
		}
	}
	return RTileMapCell();
}

Map<Vector2i, RTileMapCell> RTileMapEditorTilesPlugin::_draw_line(Vector2 p_start_drag_mouse_pos, Vector2 p_from_mouse_pos, Vector2 p_to_mouse_pos, bool p_erase) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return Map<Vector2i, RTileMapCell>();
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return Map<Vector2i, RTileMapCell>();
	}

	// Get or create the pattern.
	Ref<RTileMapPattern> erase_pattern;
	erase_pattern.instance();
	erase_pattern->set_cell(Vector2i(0, 0), RTileSet::INVALID_SOURCE, RTileSetSource::INVALID_ATLAS_COORDS, RTileSetSource::INVALID_TILE_ALTERNATIVE);
	Ref<RTileMapPattern> pattern = p_erase ? erase_pattern : selection_pattern;

	Map<Vector2i, RTileMapCell> output;

	ERR_FAIL_COND_V(!pattern.is_valid(), output);

	if (!pattern->is_empty()) {
		// Paint the tiles on the tile map.
		if (!p_erase && random_tile_checkbox->is_pressed()) {
			// Paint a random tile.
			Vector<Vector2i> line = RTileMapEditor::get_line(tile_map, tile_map->world_to_map(p_from_mouse_pos), tile_map->world_to_map(p_to_mouse_pos));
			for (int i = 0; i < line.size(); i++) {
				output.insert(line[i], _pick_random_tile(pattern));
			}
		} else {
			// Paint the pattern.
			// If we paint several tiles, we virtually move the mouse as if it was in the center of the "brush"
			Vector2 mouse_offset = (Vector2(pattern->get_size()) / 2.0 - Vector2(0.5, 0.5)) * tile_set->get_tile_size();
			Vector2i last_hovered_cell = tile_map->world_to_map(p_from_mouse_pos - mouse_offset);
			Vector2i new_hovered_cell = tile_map->world_to_map(p_to_mouse_pos - mouse_offset);
			Vector2i drag_start_cell = tile_map->world_to_map(p_start_drag_mouse_pos - mouse_offset);
			Vector2i pattern_size = pattern->get_size();

			#warning TODO
			if (pattern_size.x == 0) {
				pattern_size.x = 1;
			}

			if (pattern_size.y == 0) {
				pattern_size.y = 1;
			}

			PoolVector2Array used_cells = pattern->get_used_cells();
			Vector2i offset = Vector2i(Math::posmod(drag_start_cell.x, pattern_size.x), Math::posmod(drag_start_cell.y, pattern_size.y)); // Note: no posmodv for Vector2i for now. Meh.s
			Vector<Vector2i> line = RTileMapEditor::get_line(tile_map, (last_hovered_cell - offset) / pattern_size, (new_hovered_cell - offset) / pattern_size);
			for (int i = 0; i < line.size(); i++) {
				Vector2i top_left = line[i] * pattern_size + offset;
				for (int j = 0; j < used_cells.size(); j++) {
					Vector2i coords = tile_map->map_pattern(top_left, used_cells[j], pattern);
					output.insert(coords, RTileMapCell(pattern->get_cell_source_id(used_cells[j]), pattern->get_cell_atlas_coords(used_cells[j]), pattern->get_cell_alternative_tile(used_cells[j])));
				}
			}
		}
	}
	return output;
}

Map<Vector2i, RTileMapCell> RTileMapEditorTilesPlugin::_draw_rect(Vector2i p_start_cell, Vector2i p_end_cell, bool p_erase) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return Map<Vector2i, RTileMapCell>();
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return Map<Vector2i, RTileMapCell>();
	}

	// Create the rect to draw.
	Rect2i rect = MathExt::rect2i_abs(Rect2i(p_start_cell, p_end_cell - p_start_cell));
	rect.size += Vector2i(1, 1);

	// Get or create the pattern.
	Ref<RTileMapPattern> erase_pattern;
	erase_pattern.instance();
	erase_pattern->set_cell(Vector2i(0, 0), RTileSet::INVALID_SOURCE, RTileSetSource::INVALID_ATLAS_COORDS, RTileSetSource::INVALID_TILE_ALTERNATIVE);
	Ref<RTileMapPattern> pattern = p_erase ? erase_pattern : selection_pattern;

	Map<Vector2i, RTileMapCell> err_output;
	ERR_FAIL_COND_V(pattern->is_empty(), err_output);

	// Compute the offset to align things to the bottom or right.
	bool aligned_right = p_end_cell.x < p_start_cell.x;
	bool valigned_bottom = p_end_cell.y < p_start_cell.y;
	Vector2i offset = Vector2i(aligned_right ? -(pattern->get_size().x - (rect.get_size().x % static_cast<int>(pattern->get_size().x))) : 0, valigned_bottom ? -(pattern->get_size().y - (rect.get_size().y % static_cast<int>(pattern->get_size().y))) : 0);

	Map<Vector2i, RTileMapCell> output;
	if (!pattern->is_empty()) {
		if (!p_erase && random_tile_checkbox->is_pressed()) {
			// Paint a random tile.
			for (int x = 0; x < rect.size.x; x++) {
				for (int y = 0; y < rect.size.y; y++) {
					Vector2i coords = rect.position + Vector2i(x, y);
					output.insert(coords, _pick_random_tile(pattern));
				}
			}
		} else {
			// Paint the pattern.
			PoolVector2Array used_cells = pattern->get_used_cells();
			for (int x = 0; x <= rect.size.x / pattern->get_size().x; x++) {
				for (int y = 0; y <= rect.size.y / pattern->get_size().y; y++) {
					Vector2i pattern_coords = rect.position + Vector2i(x, y) * pattern->get_size() + offset;
					for (int j = 0; j < used_cells.size(); j++) {
						Vector2i coords = pattern_coords + used_cells[j];
						if (rect.has_point(coords)) {
							output.insert(coords, RTileMapCell(pattern->get_cell_source_id(used_cells[j]), pattern->get_cell_atlas_coords(used_cells[j]), pattern->get_cell_alternative_tile(used_cells[j])));
						}
					}
				}
			}
		}
	}

	return output;
}

Map<Vector2i, RTileMapCell> RTileMapEditorTilesPlugin::_draw_bucket_fill(Vector2i p_coords, bool p_contiguous, bool p_erase) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return Map<Vector2i, RTileMapCell>();
	}

	if (tile_map_layer < 0) {
		return Map<Vector2i, RTileMapCell>();
	}
	Map<Vector2i, RTileMapCell> output;
	ERR_FAIL_INDEX_V(tile_map_layer, tile_map->get_layers_count(), output);

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return Map<Vector2i, RTileMapCell>();
	}

	// Get or create the pattern.
	Ref<RTileMapPattern> erase_pattern;
	erase_pattern.instance();
	erase_pattern->set_cell(Vector2i(0, 0), RTileSet::INVALID_SOURCE, RTileSetSource::INVALID_ATLAS_COORDS, RTileSetSource::INVALID_TILE_ALTERNATIVE);
	Ref<RTileMapPattern> pattern = p_erase ? erase_pattern : selection_pattern;

	if (!pattern->is_empty()) {
		RTileMapCell source_cell = tile_map->get_cell(tile_map_layer, p_coords);

		// If we are filling empty tiles, compute the tilemap boundaries.
		Rect2i boundaries;
		if (source_cell.source_id == RTileSet::INVALID_SOURCE) {
			boundaries = tile_map->get_used_rect();
		}

		if (p_contiguous) {
			// Replace continuous tiles like the source.
			Set<Vector2i> already_checked;
			List<Vector2i> to_check;
			to_check.push_back(p_coords);
			while (!to_check.empty()) {
				Vector2i coords = to_check.back()->get();
				to_check.pop_back();
				if (!already_checked.has(coords)) {
					if (source_cell.source_id == tile_map->get_cell_source_id(tile_map_layer, coords) &&
							source_cell.get_atlas_coords() == tile_map->get_cell_atlas_coords(tile_map_layer, coords) &&
							source_cell.alternative_tile == tile_map->get_cell_alternative_tile(tile_map_layer, coords) &&
							(source_cell.source_id != RTileSet::INVALID_SOURCE || boundaries.has_point(coords))) {
						if (!p_erase && random_tile_checkbox->is_pressed()) {
							// Paint a random tile.
							output.insert(coords, _pick_random_tile(pattern));
						} else {
							// Paint the pattern.
							Vector2i psi = pattern->get_size();
							Vector2i pattern_coords = (coords - p_coords); // Note: it would be good to have posmodv for Vector2i.

							pattern_coords.x = pattern_coords.x % psi.x;
							pattern_coords.y = pattern_coords.y % psi.y;

							pattern_coords.x = pattern_coords.x < 0 ? pattern_coords.x + pattern->get_size().x : pattern_coords.x;
							pattern_coords.y = pattern_coords.y < 0 ? pattern_coords.y + pattern->get_size().y : pattern_coords.y;
							if (pattern->has_cell(pattern_coords)) {
								output.insert(coords, RTileMapCell(pattern->get_cell_source_id(pattern_coords), pattern->get_cell_atlas_coords(pattern_coords), pattern->get_cell_alternative_tile(pattern_coords)));
							} else {
								output.insert(coords, RTileMapCell());
							}
						}

						// Get surrounding tiles (handles different tile shapes).
						Vector<Vector2> around = tile_map->get_surrounding_tiles(coords);
						for (int i = 0; i < around.size(); i++) {
							to_check.push_back(around[i]);
						}
					}
					already_checked.insert(coords);
				}
			}
		} else {
			// Replace all tiles like the source.
			Vector<Vector2> to_check;
			if (source_cell.source_id == RTileSet::INVALID_SOURCE) {
				Rect2i rect = tile_map->get_used_rect();
				if (rect.has_no_area()) {
					rect = Rect2i(p_coords, Vector2i(1, 1));
				}

				Point2i boundaries_end = MathExt::rect2i_get_end(boundaries);
				for (int x = boundaries.position.x; x < boundaries_end.x; x++) {
					for (int y = boundaries.position.y; y < boundaries_end.y; y++) {
						to_check.push_back(Vector2i(x, y));
					}
				}
			} else {
				to_check = tile_map->get_used_cells(tile_map_layer);
			}
			for (int i = 0; i < to_check.size(); i++) {
				Vector2i coords = to_check[i];
				if (source_cell.source_id == tile_map->get_cell_source_id(tile_map_layer, coords) &&
						source_cell.get_atlas_coords() == tile_map->get_cell_atlas_coords(tile_map_layer, coords) &&
						source_cell.alternative_tile == tile_map->get_cell_alternative_tile(tile_map_layer, coords) &&
						(source_cell.source_id != RTileSet::INVALID_SOURCE || boundaries.has_point(coords))) {
					if (!p_erase && random_tile_checkbox->is_pressed()) {
						// Paint a random tile.
						output.insert(coords, _pick_random_tile(pattern));
					} else {
						// Paint the pattern.
						Vector2i psi = Vector2i(pattern->get_size());
						Vector2i pattern_coords = (coords - p_coords); // Note: it would be good to have posmodv for Vector2i.
						pattern_coords.x = pattern_coords.x % psi.x;
						pattern_coords.y = pattern_coords.y % psi.y;

						pattern_coords.x = pattern_coords.x < 0 ? pattern_coords.x + pattern->get_size().x : pattern_coords.x;
						pattern_coords.y = pattern_coords.y < 0 ? pattern_coords.y + pattern->get_size().y : pattern_coords.y;
						if (pattern->has_cell(pattern_coords)) {
							output.insert(coords, RTileMapCell(pattern->get_cell_source_id(pattern_coords), pattern->get_cell_atlas_coords(pattern_coords), pattern->get_cell_alternative_tile(pattern_coords)));
						} else {
							output.insert(coords, RTileMapCell());
						}
					}
				}
			}
		}
	}
	return output;
}

void RTileMapEditorTilesPlugin::_stop_dragging() {
	if (drag_type == DRAG_TYPE_NONE) {
		return;
	}

	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	if (tile_map_layer < 0) {
		return;
	}
	ERR_FAIL_INDEX(tile_map_layer, tile_map->get_layers_count());

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
	Vector2 mpos = xform.affine_inverse().xform(CanvasItemEditor::get_singleton()->get_viewport_control()->get_local_mouse_position());

	switch (drag_type) {
		case DRAG_TYPE_SELECT: {
			undo_redo->create_action(TTR("Change selection"));
			//undo_redo->add_undo_method(this, "_set_tile_map_selection", _get_tile_map_selection());

			if (!Input::get_singleton()->is_key_pressed(KEY_SHIFT) && !Input::get_singleton()->is_key_pressed(KEY_CONTROL)) {
				tile_map_selection.clear();
			}
			Rect2i rect = MathExt::rect2i_abs(Rect2i(tile_map->world_to_map(drag_start_mouse_pos), tile_map->world_to_map(mpos) - tile_map->world_to_map(drag_start_mouse_pos)));

			Point2i rect_end = MathExt::rect2i_get_end(rect);
			for (int x = rect.position.x; x <= rect_end.x; x++) {
				for (int y = rect.position.y; y <= rect_end.y; y++) {
					Vector2i coords = Vector2i(x, y);
					if (Input::get_singleton()->is_key_pressed(KEY_CONTROL)) {
						if (tile_map_selection.has(coords)) {
							tile_map_selection.erase(coords);
						}
					} else {
						if (tile_map->get_cell_source_id(tile_map_layer, coords) != RTileSet::INVALID_SOURCE) {
							tile_map_selection.insert(coords);
						}
					}
				}
			}
			//undo_redo->add_do_method(this, "_set_tile_map_selection", _get_tile_map_selection());
			undo_redo->commit_action();
			_set_tile_map_selection(_get_tile_map_selection());

			_update_selection_pattern_from_tilemap_selection();
			_update_tileset_selection_from_selection_pattern();
		} break;
		case DRAG_TYPE_MOVE: {
			if (patterns_item_list->is_visible_in_tree() && patterns_item_list->has_point(patterns_item_list->get_local_mouse_position())) {
				// Restore the cells.
				for (Map<Vector2i, RTileMapCell>::Element *kv = drag_modified.front(); kv; kv = kv->next()) {
					tile_map->set_cell(tile_map_layer, kv->key(), kv->value().source_id, kv->value().get_atlas_coords(), kv->value().alternative_tile);
				}

				// Creating a pattern in the pattern list.
				select_last_pattern = true;
				int new_pattern_index = tile_set->get_patterns_count();
				undo_redo->create_action(TTR("Add RTileSet pattern"));
				undo_redo->add_do_method(*tile_set, "add_pattern", selection_pattern, new_pattern_index);
				undo_redo->add_undo_method(*tile_set, "remove_pattern", new_pattern_index);
				undo_redo->commit_action();
			} else {
				// Get the top-left cell.
				Vector2i top_left;
				if (!tile_map_selection.empty()) {
					top_left = tile_map_selection.front()->get();
				}
				for (Set<Vector2i>::Element *E = tile_map_selection.front(); E; E = E->next()) {
					top_left = MathExt::vector2i_min(top_left, E->get());
				}

				// Get the offset from the mouse.
				Vector2i offset = drag_start_mouse_pos - tile_map->map_to_world(top_left);
				offset = tile_map->world_to_map(mpos - offset) - tile_map->world_to_map(drag_start_mouse_pos - offset);

				PoolVector2Array selection_used_cells = selection_pattern->get_used_cells();

				// Build the list of cells to undo.
				Vector2i coords;
				Map<Vector2i, RTileMapCell> cells_undo;
				for (int i = 0; i < selection_used_cells.size(); i++) {
					coords = tile_map->map_pattern(top_left, selection_used_cells[i], selection_pattern);
					cells_undo[coords] = RTileMapCell(drag_modified[coords].source_id, drag_modified[coords].get_atlas_coords(), drag_modified[coords].alternative_tile);
					coords = tile_map->map_pattern(top_left + offset, selection_used_cells[i], selection_pattern);
					cells_undo[coords] = RTileMapCell(tile_map->get_cell_source_id(tile_map_layer, coords), tile_map->get_cell_atlas_coords(tile_map_layer, coords), tile_map->get_cell_alternative_tile(tile_map_layer, coords));
				}

				// Build the list of cells to do.
				Map<Vector2i, RTileMapCell> cells_do;
				for (int i = 0; i < selection_used_cells.size(); i++) {
					coords = tile_map->map_pattern(top_left, selection_used_cells[i], selection_pattern);
					cells_do[coords] = RTileMapCell();
				}
				for (int i = 0; i < selection_used_cells.size(); i++) {
					coords = tile_map->map_pattern(top_left + offset, selection_used_cells[i], selection_pattern);
					cells_do[coords] = RTileMapCell(selection_pattern->get_cell_source_id(selection_used_cells[i]), selection_pattern->get_cell_atlas_coords(selection_used_cells[i]), selection_pattern->get_cell_alternative_tile(selection_used_cells[i]));
				}

				// Move the tiles.
				undo_redo->create_action(TTR("Move tiles"));
				for (Map<Vector2i, RTileMapCell>::Element *E = cells_do.front(); E; E = E->next()) {
					undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), E->get().source_id, E->get().get_atlas_coords(), E->get().alternative_tile);
				}
				for (Map<Vector2i, RTileMapCell>::Element *E = cells_undo.front(); E; E = E->next()) {
					undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), E->get().source_id, E->get().get_atlas_coords(), E->get().alternative_tile);
				}

				// Update the selection.
				//undo_redo->add_undo_method(this, "_set_tile_map_selection", _get_tile_map_selection());
				tile_map_selection.clear();
				for (int i = 0; i < selection_used_cells.size(); i++) {
					coords = tile_map->map_pattern(top_left + offset, selection_used_cells[i], selection_pattern);
					tile_map_selection.insert(coords);
				}
				//undo_redo->add_do_method(this, "_set_tile_map_selection", _get_tile_map_selection());
				
				undo_redo->commit_action();
				_set_tile_map_selection(_get_tile_map_selection());
			}
		} break;
		case DRAG_TYPE_PICK: {
			Rect2i rect = MathExt::rect2i_abs(Rect2i(tile_map->world_to_map(drag_start_mouse_pos), tile_map->world_to_map(mpos) - tile_map->world_to_map(drag_start_mouse_pos)));
			rect.size += Vector2i(1, 1);

			Vector<Vector2> coords_array;
			Point2i rect_end = MathExt::rect2i_get_end(rect);
			for (int x = rect.position.x; x < rect_end.x; x++) {
				for (int y = rect.position.y; y < rect_end.y; y++) {
					Vector2i coords = Vector2i(x, y);
					if (tile_map->get_cell_source_id(tile_map_layer, coords) != RTileSet::INVALID_SOURCE) {
						coords_array.push_back(coords);
					}
				}
			}
			Ref<RTileMapPattern> new_selection_pattern = tile_map->get_pattern(tile_map_layer, coords_array);
			if (!new_selection_pattern->is_empty()) {
				selection_pattern = new_selection_pattern;
				_update_tileset_selection_from_selection_pattern();
			}
			picker_button->set_pressed(false);
		} break;
		case DRAG_TYPE_PAINT: {
			undo_redo->create_action(TTR("Paint tiles"));
			for (Map<Vector2i, RTileMapCell>::Element *E = drag_modified.front(); E; E = E->next()) {
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), tile_map->get_cell_source_id(tile_map_layer, E->key()), tile_map->get_cell_atlas_coords(tile_map_layer, E->key()), tile_map->get_cell_alternative_tile(tile_map_layer, E->key()));
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), E->value().source_id, E->value().get_atlas_coords(), E->value().alternative_tile);
			}
			undo_redo->commit_action();
		} break;
		case DRAG_TYPE_LINE: {
			Map<Vector2i, RTileMapCell> to_draw = _draw_line(drag_start_mouse_pos, drag_start_mouse_pos, mpos, drag_erasing);
			undo_redo->create_action(TTR("Paint tiles"));
			for (Map<Vector2i, RTileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
				if (!drag_erasing && E->value().source_id == RTileSet::INVALID_SOURCE) {
					continue;
				}
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), E->value().source_id, E->value().get_atlas_coords(), E->value().alternative_tile);
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), tile_map->get_cell_source_id(tile_map_layer, E->key()), tile_map->get_cell_atlas_coords(tile_map_layer, E->key()), tile_map->get_cell_alternative_tile(tile_map_layer, E->key()));
			}
			undo_redo->commit_action();
		} break;
		case DRAG_TYPE_RECT: {
			Map<Vector2i, RTileMapCell> to_draw = _draw_rect(tile_map->world_to_map(drag_start_mouse_pos), tile_map->world_to_map(mpos), drag_erasing);
			undo_redo->create_action(TTR("Paint tiles"));
			for (Map<Vector2i, RTileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
				if (!drag_erasing && E->value().source_id == RTileSet::INVALID_SOURCE) {
					continue;
				}
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), E->value().source_id, E->value().get_atlas_coords(), E->value().alternative_tile);
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), tile_map->get_cell_source_id(tile_map_layer, E->key()), tile_map->get_cell_atlas_coords(tile_map_layer, E->key()), tile_map->get_cell_alternative_tile(tile_map_layer, E->key()));
			}
			undo_redo->commit_action();
		} break;
		case DRAG_TYPE_BUCKET: {
			undo_redo->create_action(TTR("Paint tiles"));
			for (Map<Vector2i, RTileMapCell>::Element *E = drag_modified.front(); E; E = E->next()) {
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), tile_map->get_cell_source_id(tile_map_layer, E->key()), tile_map->get_cell_atlas_coords(tile_map_layer, E->key()), tile_map->get_cell_alternative_tile(tile_map_layer, E->key()));
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), E->value().source_id, E->value().get_atlas_coords(), E->value().alternative_tile);
			}
			undo_redo->commit_action();
		} break;
		case DRAG_TYPE_CLIPBOARD_PASTE: {
			Vector2 mouse_offset = (Vector2(tile_map_clipboard->get_size()) / 2.0 - Vector2(0.5, 0.5)) * tile_set->get_tile_size();
			undo_redo->create_action(TTR("Paste tiles"));
			PoolVector2Array used_cells = tile_map_clipboard->get_used_cells();
			for (int i = 0; i < used_cells.size(); i++) {
				Vector2i coords = tile_map->map_pattern(tile_map->world_to_map(mpos - mouse_offset), used_cells[i], tile_map_clipboard);
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, Vector2(coords), tile_map_clipboard->get_cell_source_id(used_cells[i]), tile_map_clipboard->get_cell_atlas_coords(used_cells[i]), tile_map_clipboard->get_cell_alternative_tile(used_cells[i]));
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, Vector2(coords), tile_map->get_cell_source_id(tile_map_layer, coords), tile_map->get_cell_atlas_coords(tile_map_layer, coords), tile_map->get_cell_alternative_tile(tile_map_layer, Vector2(coords)));
			}
			undo_redo->commit_action();
		} break;
		default:
			break;
	}
	drag_type = DRAG_TYPE_NONE;
}

void RTileMapEditorTilesPlugin::_update_fix_selected_and_hovered(int i) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		hovered_tile.source_id = RTileSet::INVALID_SOURCE;
		hovered_tile.set_atlas_coords(RTileSetSource::INVALID_ATLAS_COORDS);
		hovered_tile.alternative_tile = RTileSetSource::INVALID_TILE_ALTERNATIVE;
		tile_set_selection.clear();
		patterns_item_list->unselect_all();
		tile_map_selection.clear();
		selection_pattern.instance();
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		hovered_tile.source_id = RTileSet::INVALID_SOURCE;
		hovered_tile.set_atlas_coords(RTileSetSource::INVALID_ATLAS_COORDS);
		hovered_tile.alternative_tile = RTileSetSource::INVALID_TILE_ALTERNATIVE;
		tile_set_selection.clear();
		patterns_item_list->unselect_all();
		tile_map_selection.clear();
		selection_pattern.instance();
		return;
	}

	int source_index = sources_list->get_current();
	if (source_index < 0 || source_index >= sources_list->get_item_count()) {
		hovered_tile.source_id = RTileSet::INVALID_SOURCE;
		hovered_tile.set_atlas_coords(RTileSetSource::INVALID_ATLAS_COORDS);
		hovered_tile.alternative_tile = RTileSetSource::INVALID_TILE_ALTERNATIVE;
		tile_set_selection.clear();
		patterns_item_list->unselect_all();
		tile_map_selection.clear();
		selection_pattern.instance();
		return;
	}

	int source_id = sources_list->get_item_metadata(source_index);

	// Clear hovered if needed.
	if (source_id != hovered_tile.source_id ||
			!tile_set->has_source(hovered_tile.source_id) ||
			!tile_set->get_source(hovered_tile.source_id)->has_tile(hovered_tile.get_atlas_coords()) ||
			!tile_set->get_source(hovered_tile.source_id)->has_alternative_tile(hovered_tile.get_atlas_coords(), hovered_tile.alternative_tile)) {
		hovered_tile.source_id = RTileSet::INVALID_SOURCE;
		hovered_tile.set_atlas_coords(RTileSetSource::INVALID_ATLAS_COORDS);
		hovered_tile.alternative_tile = RTileSetSource::INVALID_TILE_ALTERNATIVE;
	}

	// Selection if needed.
	for (Set<RTileMapCell>::Element *E = tile_set_selection.front(); E; E = E->next()) {
		const RTileMapCell *selected = &(E->get());
		if (!tile_set->has_source(selected->source_id) ||
				!tile_set->get_source(selected->source_id)->has_tile(selected->get_atlas_coords()) ||
				!tile_set->get_source(selected->source_id)->has_alternative_tile(selected->get_atlas_coords(), selected->alternative_tile)) {
			tile_set_selection.erase(E);
		}
	}

	if (!tile_map_selection.empty()) {
		_update_selection_pattern_from_tilemap_selection();
	} else if (tiles_bottom_panel->is_visible_in_tree()) {
		_update_selection_pattern_from_tileset_tiles_selection();
	} else {
		_update_selection_pattern_from_tileset_pattern_selection();
	}
}

void RTileMapEditorTilesPlugin::_fix_invalid_tiles_in_tile_map_selection() {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Set<Vector2i> to_remove;
	for (Set<Vector2i>::Element *selected = tile_map_selection.front(); selected; selected = selected->next()) {
		RTileMapCell cell = tile_map->get_cell(tile_map_layer, selected->get());
		if (cell.source_id == RTileSet::INVALID_SOURCE && cell.get_atlas_coords() == RTileSetSource::INVALID_ATLAS_COORDS && cell.alternative_tile == RTileSetAtlasSource::INVALID_TILE_ALTERNATIVE) {
			to_remove.insert(selected->get());
		}
	}

	for (Set<Vector2i>::Element *cell = to_remove.front(); cell; cell = cell->next()) {
		tile_map_selection.erase(cell);
	}
}

void RTileMapEditorTilesPlugin::_update_selection_pattern_from_tilemap_selection() {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	ERR_FAIL_INDEX(tile_map_layer, tile_map->get_layers_count());

	selection_pattern.instance();

	Vector<Vector2> coords_array;
	for (Set<Vector2i>::Element *E = tile_map_selection.front(); E; E = E->next()) {
		coords_array.push_back(E->get());
	}
	selection_pattern = tile_map->get_pattern(tile_map_layer, coords_array);
}

void RTileMapEditorTilesPlugin::_update_selection_pattern_from_tileset_tiles_selection() {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	// Clear the tilemap selection.
	tile_map_selection.clear();

	// Clear the selected pattern.
	selection_pattern.instance();

	// Group per source.
	Map<int, List<const RTileMapCell *>> per_source;
	for (Set<RTileMapCell>::Element *E = tile_set_selection.front(); E; E = E->next()) {
		per_source[E->get().source_id].push_back(&(E->get()));
	}

	int vertical_offset = 0;
	for (Map<int, List<const RTileMapCell *>>::Element *E_source = per_source.front(); E_source; E_source = E_source->next()) {
		// Per source.
		List<const RTileMapCell *> unorganized;
		Rect2i encompassing_rect_coords;
		Map<Vector2i, const RTileMapCell *> organized_pattern;

		RTileSetSource *source = *tile_set->get_source(E_source->key());
		RTileSetAtlasSource *atlas_source = Object::cast_to<RTileSetAtlasSource>(source);
		if (atlas_source) {
			// Organize using coordinates.
			for (List<const RTileMapCell *>::Element *current = E_source->value().front(); current; current = current->next()) {
			//for (const RTileMapCell *current : E_source->value()) {
				if (current->get()->alternative_tile == 0) {
					organized_pattern[current->get()->get_atlas_coords()] = current->get();
				} else {
					unorganized.push_back(current->get());
				}
			}

			// Compute the encompassing rect for the organized pattern.
			Map<Vector2i, const RTileMapCell *>::Element *E_cell = organized_pattern.front();
			if (E_cell) {
				encompassing_rect_coords = Rect2i(E_cell->key(), Vector2i(1, 1));
				for (; E_cell; E_cell = E_cell->next()) {
					encompassing_rect_coords.expand_to(E_cell->key() + Vector2i(1, 1));
					encompassing_rect_coords.expand_to(E_cell->key());
				}
			}
		} else {
			// Add everything unorganized.
			for (List<const RTileMapCell *>::Element *cell = E_source->value().front(); cell; cell = cell->next()) {
				unorganized.push_back(cell->get());
			}
		}

		// Now add everything to the output pattern.
		for (Map<Vector2i, const RTileMapCell *>::Element *E_cell = organized_pattern.front(); E_cell; E_cell = E_cell->next()) {
			selection_pattern->set_cell(E_cell->key() - encompassing_rect_coords.position + Vector2i(0, vertical_offset), E_cell->value()->source_id, E_cell->value()->get_atlas_coords(), E_cell->value()->alternative_tile);
		}
		Vector2i organized_size = selection_pattern->get_size();
		int unorganized_index = 0;

		for (List<const RTileMapCell *>::Element *cell = unorganized.front(); cell; cell = cell->next()) {
			selection_pattern->set_cell(Vector2(organized_size.x + unorganized_index, vertical_offset), cell->get()->source_id, cell->get()->get_atlas_coords(), cell->get()->alternative_tile);
			unorganized_index++;
		}
		vertical_offset += MAX(organized_size.y, 1);
	}
	CanvasItemEditor::get_singleton()->update_viewport();
}

void RTileMapEditorTilesPlugin::_update_selection_pattern_from_tileset_pattern_selection() {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	// Clear the tilemap selection.
	tile_map_selection.clear();

	// Clear the selected pattern.
	selection_pattern.instance();

	if (patterns_item_list->get_selected_items().size() >= 1) {
		selection_pattern = patterns_item_list->get_item_metadata(patterns_item_list->get_selected_items()[0]);
	}

	CanvasItemEditor::get_singleton()->update_viewport();
}

void RTileMapEditorTilesPlugin::_update_tileset_selection_from_selection_pattern() {
	tile_set_selection.clear();
	PoolVector2Array used_cells = selection_pattern->get_used_cells();
	for (int i = 0; i < used_cells.size(); i++) {
		Vector2i coords = used_cells[i];
		if (selection_pattern->get_cell_source_id(coords) != RTileSet::INVALID_SOURCE) {
			tile_set_selection.insert(RTileMapCell(selection_pattern->get_cell_source_id(coords), selection_pattern->get_cell_atlas_coords(coords), selection_pattern->get_cell_alternative_tile(coords)));
		}
	}
	_update_source_display();
	tile_atlas_control->update();
	alternative_tiles_control->update();
}

void RTileMapEditorTilesPlugin::_tile_atlas_control_draw() {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	int source_index = sources_list->get_current();
	if (source_index < 0 || source_index >= sources_list->get_item_count()) {
		return;
	}

	int source_id = sources_list->get_item_metadata(source_index);
	if (!tile_set->has_source(source_id)) {
		return;
	}

	RTileSetAtlasSource *atlas = Object::cast_to<RTileSetAtlasSource>(*tile_set->get_source(source_id));
	if (!atlas) {
		return;
	}

	// Draw the selection.
	Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
	Color selection_color = Color().from_hsv(Math::fposmod(grid_color.get_h() + 0.5, 1.0), grid_color.get_s(), grid_color.get_v(), 1.0);
	for (Set<RTileMapCell>::Element *E = tile_set_selection.front(); E; E = E->next()) {
		if (E->get().source_id == source_id && E->get().alternative_tile == 0) {
			for (int frame = 0; frame < atlas->get_tile_animation_frames_count(E->get().get_atlas_coords()); frame++) {
				Color color = selection_color;
				if (frame > 0) {
					color.a *= 0.3;
				}
				tile_atlas_control->draw_rect(atlas->get_tile_texture_region(E->get().get_atlas_coords(), frame), color, false);
			}
		}
	}

	// Draw the hovered tile.
	if (hovered_tile.get_atlas_coords() != RTileSetSource::INVALID_ATLAS_COORDS && hovered_tile.alternative_tile == 0 && !tile_set_dragging_selection) {
		for (int frame = 0; frame < atlas->get_tile_animation_frames_count(hovered_tile.get_atlas_coords()); frame++) {
			Color color = Color(1.0, 1.0, 1.0);
			if (frame > 0) {
				color.a *= 0.3;
			}
			tile_atlas_control->draw_rect(atlas->get_tile_texture_region(hovered_tile.get_atlas_coords(), frame), color, false);
		}
	}

	// Draw the selection rect.
	if (tile_set_dragging_selection) {
		Vector2i start_tile = tile_atlas_view->get_atlas_tile_coords_at_pos(tile_set_drag_start_mouse_pos);
		Vector2i end_tile = tile_atlas_view->get_atlas_tile_coords_at_pos(tile_atlas_control->get_local_mouse_position());

		Rect2i region = MathExt::rect2i_abs(Rect2i(start_tile, end_tile - start_tile));
		region.size += Vector2i(1, 1);

		Set<Vector2i> to_draw;
		Point2i region_end = MathExt::rect2i_get_end(region);
		for (int x = region.position.x; x < region_end.x; x++) {
			for (int y = region.position.y; y < region_end.y; y++) {
				Vector2i tile = atlas->get_tile_at_coords(Vector2i(x, y));
				if (tile != RTileSetSource::INVALID_ATLAS_COORDS) {
					to_draw.insert(tile);
				}
			}
		}
		Color selection_rect_color = selection_color.lightened(0.2);
		for (Set<Vector2i>::Element *E = to_draw.front(); E; E = E->next()) {
			tile_atlas_control->draw_rect(atlas->get_tile_texture_region(E->get()), selection_rect_color, false);
		}
	}
}

void RTileMapEditorTilesPlugin::_tile_atlas_control_mouse_exited() {
	hovered_tile.source_id = RTileSet::INVALID_SOURCE;
	hovered_tile.set_atlas_coords(RTileSetSource::INVALID_ATLAS_COORDS);
	hovered_tile.alternative_tile = RTileSetSource::INVALID_TILE_ALTERNATIVE;
	tile_set_dragging_selection = false;
	tile_atlas_control->update();
}

void RTileMapEditorTilesPlugin::_tile_atlas_control_gui_input(const Ref<InputEvent> &p_event) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	int source_index = sources_list->get_current();
	if (source_index < 0 || source_index >= sources_list->get_item_count()) {
		return;
	}

	int source_id = sources_list->get_item_metadata(source_index);
	if (!tile_set->has_source(source_id)) {
		return;
	}

	RTileSetAtlasSource *atlas = Object::cast_to<RTileSetAtlasSource>(*tile_set->get_source(source_id));
	if (!atlas) {
		return;
	}

	// Update the hovered tile
	hovered_tile.source_id = source_id;
	hovered_tile.set_atlas_coords(RTileSetSource::INVALID_ATLAS_COORDS);
	hovered_tile.alternative_tile = RTileSetSource::INVALID_TILE_ALTERNATIVE;
	Vector2i coords = tile_atlas_view->get_atlas_tile_coords_at_pos(tile_atlas_control->get_local_mouse_position());
	if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
		coords = atlas->get_tile_at_coords(coords);
		if (coords != RTileSetSource::INVALID_ATLAS_COORDS) {
			hovered_tile.set_atlas_coords(coords);
			hovered_tile.alternative_tile = 0;
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		tile_atlas_control->update();
		alternative_tiles_control->update();
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->get_button_index() == BUTTON_LEFT) {
		if (mb->is_pressed()) { // Pressed
			tile_set_dragging_selection = true;
			tile_set_drag_start_mouse_pos = tile_atlas_control->get_local_mouse_position();
			if (!mb->get_shift()) {
				tile_set_selection.clear();
			}

			if (hovered_tile.get_atlas_coords() != RTileSetSource::INVALID_ATLAS_COORDS && hovered_tile.alternative_tile == 0) {
				if (mb->get_shift() && tile_set_selection.has(RTileMapCell(source_id, hovered_tile.get_atlas_coords(), 0))) {
					tile_set_selection.erase(RTileMapCell(source_id, hovered_tile.get_atlas_coords(), 0));
				} else {
					tile_set_selection.insert(RTileMapCell(source_id, hovered_tile.get_atlas_coords(), 0));
				}
			}
			_update_selection_pattern_from_tileset_tiles_selection();
		} else { // Released
			if (tile_set_dragging_selection) {
				if (!mb->get_shift()) {
					tile_set_selection.clear();
				}
				// Compute the covered area.
				Vector2i start_tile = tile_atlas_view->get_atlas_tile_coords_at_pos(tile_set_drag_start_mouse_pos);
				Vector2i end_tile = tile_atlas_view->get_atlas_tile_coords_at_pos(tile_atlas_control->get_local_mouse_position());
				if (start_tile != RTileSetSource::INVALID_ATLAS_COORDS && end_tile != RTileSetSource::INVALID_ATLAS_COORDS) {
					Rect2i region = MathExt::rect2i_abs(Rect2i(start_tile, end_tile - start_tile));
					region.size += Vector2i(1, 1);

					// To update the selection, we copy the selected/not selected status of the tiles we drag from.
					Vector2i start_coords = atlas->get_tile_at_coords(start_tile);
					if (mb->get_shift() && start_coords != RTileSetSource::INVALID_ATLAS_COORDS && !tile_set_selection.has(RTileMapCell(source_id, start_coords, 0))) {
						// Remove from the selection.
						Point2i region_end = MathExt::rect2i_get_end(region);
						for (int x = region.position.x; x < region_end.x; x++) {
							for (int y = region.position.y; y < region_end.y; y++) {
								Vector2i tile_coords = atlas->get_tile_at_coords(Vector2i(x, y));
								if (tile_coords != RTileSetSource::INVALID_ATLAS_COORDS && tile_set_selection.has(RTileMapCell(source_id, tile_coords, 0))) {
									tile_set_selection.erase(RTileMapCell(source_id, tile_coords, 0));
								}
							}
						}
					} else {
						// Insert in the selection.
						Point2i region_end = MathExt::rect2i_get_end(region);
						for (int x = region.position.x; x < region_end.x; x++) {
							for (int y = region.position.y; y < region_end.y; y++) {
								Vector2i tile_coords = atlas->get_tile_at_coords(Vector2i(x, y));
								if (tile_coords != RTileSetSource::INVALID_ATLAS_COORDS) {
									tile_set_selection.insert(RTileMapCell(source_id, tile_coords, 0));
								}
							}
						}
					}
				}
				_update_selection_pattern_from_tileset_tiles_selection();
			}
			tile_set_dragging_selection = false;
		}
		tile_atlas_control->update();
	}
}

void RTileMapEditorTilesPlugin::_tile_alternatives_control_draw() {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	int source_index = sources_list->get_current();
	if (source_index < 0 || source_index >= sources_list->get_item_count()) {
		return;
	}

	int source_id = sources_list->get_item_metadata(source_index);
	if (!tile_set->has_source(source_id)) {
		return;
	}

	RTileSetAtlasSource *atlas = Object::cast_to<RTileSetAtlasSource>(*tile_set->get_source(source_id));
	if (!atlas) {
		return;
	}

	// Draw the selection.
	for (Set<RTileMapCell>::Element *E = tile_set_selection.front(); E; E = E->next()) {
		if (E->get().source_id == source_id && E->get().get_atlas_coords() != RTileSetSource::INVALID_ATLAS_COORDS && E->get().alternative_tile > 0) {
			Rect2i rect = tile_atlas_view->get_alternative_tile_rect(E->get().get_atlas_coords(), E->get().alternative_tile);
			if (rect != Rect2i()) {
				alternative_tiles_control->draw_rect(rect, Color(0.2, 0.2, 1.0), false);
			}
		}
	}

	// Draw hovered tile.
	if (hovered_tile.get_atlas_coords() != RTileSetSource::INVALID_ATLAS_COORDS && hovered_tile.alternative_tile > 0) {
		Rect2i rect = tile_atlas_view->get_alternative_tile_rect(hovered_tile.get_atlas_coords(), hovered_tile.alternative_tile);
		if (rect != Rect2i()) {
			alternative_tiles_control->draw_rect(rect, Color(1.0, 1.0, 1.0), false);
		}
	}
}

void RTileMapEditorTilesPlugin::_tile_alternatives_control_mouse_exited() {
	hovered_tile.source_id = RTileSet::INVALID_SOURCE;
	hovered_tile.set_atlas_coords(RTileSetSource::INVALID_ATLAS_COORDS);
	hovered_tile.alternative_tile = RTileSetSource::INVALID_TILE_ALTERNATIVE;
	tile_set_dragging_selection = false;
	alternative_tiles_control->update();
}

void RTileMapEditorTilesPlugin::_tile_alternatives_control_gui_input(const Ref<InputEvent> &p_event) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	int source_index = sources_list->get_current();
	if (source_index < 0 || source_index >= sources_list->get_item_count()) {
		return;
	}

	int source_id = sources_list->get_item_metadata(source_index);
	if (!tile_set->has_source(source_id)) {
		return;
	}

	RTileSetAtlasSource *atlas = Object::cast_to<RTileSetAtlasSource>(*tile_set->get_source(source_id));
	if (!atlas) {
		return;
	}

	// Update the hovered tile
	hovered_tile.source_id = source_id;
	hovered_tile.set_atlas_coords(RTileSetSource::INVALID_ATLAS_COORDS);
	hovered_tile.alternative_tile = RTileSetSource::INVALID_TILE_ALTERNATIVE;
	Vector3i alternative_coords = tile_atlas_view->get_alternative_tile_at_pos(alternative_tiles_control->get_local_mouse_position());
	Vector2i coords = Vector2i(alternative_coords.x, alternative_coords.y);
	int alternative = alternative_coords.z;
	if (coords != RTileSetSource::INVALID_ATLAS_COORDS && alternative != RTileSetSource::INVALID_TILE_ALTERNATIVE) {
		hovered_tile.set_atlas_coords(coords);
		hovered_tile.alternative_tile = alternative;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		tile_atlas_control->update();
		alternative_tiles_control->update();
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->get_button_index() == BUTTON_LEFT) {
		if (mb->is_pressed()) { // Pressed
			// Left click pressed.
			if (!mb->get_shift()) {
				tile_set_selection.clear();
			}

			if (coords != RTileSetSource::INVALID_ATLAS_COORDS && alternative != RTileSetAtlasSource::INVALID_TILE_ALTERNATIVE) {
				if (mb->get_shift() && tile_set_selection.has(RTileMapCell(source_id, hovered_tile.get_atlas_coords(), hovered_tile.alternative_tile))) {
					tile_set_selection.erase(RTileMapCell(source_id, hovered_tile.get_atlas_coords(), hovered_tile.alternative_tile));
				} else {
					tile_set_selection.insert(RTileMapCell(source_id, hovered_tile.get_atlas_coords(), hovered_tile.alternative_tile));
				}
			}
			_update_selection_pattern_from_tileset_tiles_selection();
		}
		tile_atlas_control->update();
		alternative_tiles_control->update();
	}
}

void RTileMapEditorTilesPlugin::_set_tile_map_selection(const Vector<Vector2i> &p_selection) {
	tile_map_selection.clear();
	for (int i = 0; i < p_selection.size(); i++) {
		tile_map_selection.insert(p_selection[i]);
	}
	_update_selection_pattern_from_tilemap_selection();
	_update_tileset_selection_from_selection_pattern();
	CanvasItemEditor::get_singleton()->update_viewport();
}

Vector<Vector2i> RTileMapEditorTilesPlugin::_get_tile_map_selection() const {
	Vector<Vector2i> output;
	for (Set<Vector2i>::Element *E = tile_map_selection.front(); E; E = E->next()) {
		output.push_back(E->get());
	}
	return output;
}

void RTileMapEditorTilesPlugin::edit(ObjectID p_tile_map_id, int p_tile_map_layer) {
	_stop_dragging(); // Avoids staying in a wrong drag state.

	if (tile_map_id != p_tile_map_id) {
		tile_map_id = p_tile_map_id;

		// Clear the selection.
		tile_set_selection.clear();
		patterns_item_list->unselect_all();
		tile_map_selection.clear();
		selection_pattern.instance();
	}

	tile_map_layer = p_tile_map_layer;
}

void RTileMapEditorTilesPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_scene_thumbnail_done"), &RTileMapEditorTilesPlugin::_scene_thumbnail_done);
	//ClassDB::bind_method(D_METHOD("_set_tile_map_selection", "selection"), &RTileMapEditorTilesPlugin::_set_tile_map_selection);
	//ClassDB::bind_method(D_METHOD("_get_tile_map_selection"), &RTileMapEditorTilesPlugin::_get_tile_map_selection);
	ClassDB::bind_method(D_METHOD("_pattern_preview_done"), &RTileMapEditorTilesPlugin::_pattern_preview_done);

	ClassDB::bind_method(D_METHOD("_mouse_exited_viewport"), &RTileMapEditorTilesPlugin::_mouse_exited_viewport);
	ClassDB::bind_method(D_METHOD("_update_toolbar"), &RTileMapEditorTilesPlugin::_update_toolbar);
	ClassDB::bind_method(D_METHOD("_on_random_tile_checkbox_toggled"), &RTileMapEditorTilesPlugin::_on_random_tile_checkbox_toggled);
	ClassDB::bind_method(D_METHOD("_on_scattering_spinbox_changed"), &RTileMapEditorTilesPlugin::_on_scattering_spinbox_changed);
	ClassDB::bind_method(D_METHOD("_update_theme"), &RTileMapEditorTilesPlugin::_update_theme);
	ClassDB::bind_method(D_METHOD("_stop_dragging"), &RTileMapEditorTilesPlugin::_stop_dragging);
	ClassDB::bind_method(D_METHOD("_tab_changed"), &RTileMapEditorTilesPlugin::_tab_changed);
	ClassDB::bind_method(D_METHOD("_update_fix_selected_and_hovered"), &RTileMapEditorTilesPlugin::_update_fix_selected_and_hovered);
	ClassDB::bind_method(D_METHOD("_update_source_display"), &RTileMapEditorTilesPlugin::_update_source_display);

	ClassDB::bind_method(D_METHOD("_tile_atlas_control_draw"), &RTileMapEditorTilesPlugin::_tile_atlas_control_draw);
	ClassDB::bind_method(D_METHOD("_tile_atlas_control_mouse_exited"), &RTileMapEditorTilesPlugin::_tile_atlas_control_mouse_exited);
	ClassDB::bind_method(D_METHOD("_tile_atlas_control_gui_input"), &RTileMapEditorTilesPlugin::_tile_atlas_control_gui_input);
	ClassDB::bind_method(D_METHOD("_tile_alternatives_control_draw"), &RTileMapEditorTilesPlugin::_tile_alternatives_control_draw);
	ClassDB::bind_method(D_METHOD("_tile_alternatives_control_mouse_exited"), &RTileMapEditorTilesPlugin::_tile_alternatives_control_mouse_exited);
	ClassDB::bind_method(D_METHOD("_tile_alternatives_control_gui_input"), &RTileMapEditorTilesPlugin::_tile_alternatives_control_gui_input);
	ClassDB::bind_method(D_METHOD("_scenes_list_multi_selected"), &RTileMapEditorTilesPlugin::_scenes_list_multi_selected);
	ClassDB::bind_method(D_METHOD("_scenes_list_nothing_selected"), &RTileMapEditorTilesPlugin::_scenes_list_nothing_selected);
	ClassDB::bind_method(D_METHOD("_patterns_item_list_gui_input"), &RTileMapEditorTilesPlugin::_patterns_item_list_gui_input);
	ClassDB::bind_method(D_METHOD("_update_selection_pattern_from_tileset_pattern_selection"), &RTileMapEditorTilesPlugin::_update_selection_pattern_from_tileset_pattern_selection);
}

RTileMapEditorTilesPlugin::RTileMapEditorTilesPlugin() {
	CanvasItemEditor::get_singleton()->get_viewport_control()->connect("mouse_exited", this, "_mouse_exited_viewport");

	// --- Shortcuts ---
	ED_SHORTCUT("tiles_editor/cut", TTR("Cut"), KEY_MASK_CMD | KEY_X);
	ED_SHORTCUT("tiles_editor/copy", TTR("Copy"), KEY_MASK_CMD | KEY_C);
	ED_SHORTCUT("tiles_editor/paste", TTR("Paste"), KEY_MASK_CMD | KEY_V);
	ED_SHORTCUT("tiles_editor/cancel", TTR("Cancel"), KEY_ESCAPE);
	ED_SHORTCUT("tiles_editor/delete", TTR("Delete"), KEY_DELETE);

	// --- Initialize references ---
	tile_map_clipboard.instance();
	selection_pattern.instance();

	// --- Toolbar ---
	toolbar = memnew(HBoxContainer);
	toolbar->set_h_size_flags(Control::SIZE_EXPAND_FILL);

	HBoxContainer *tilemap_tiles_tools_buttons = memnew(HBoxContainer);

	tool_buttons_group.instance();

	select_tool_button = memnew(Button);
	select_tool_button->set_flat(true);
	select_tool_button->set_toggle_mode(true);
	select_tool_button->set_button_group(tool_buttons_group);
	select_tool_button->set_shortcut(ED_SHORTCUT("tiles_editor/selection_tool", "Selection", KEY_S));
	select_tool_button->connect("pressed", this, "_update_toolbar");
	tilemap_tiles_tools_buttons->add_child(select_tool_button);

	paint_tool_button = memnew(Button);
	paint_tool_button->set_flat(true);
	paint_tool_button->set_toggle_mode(true);
	paint_tool_button->set_button_group(tool_buttons_group);
	paint_tool_button->set_shortcut(ED_SHORTCUT("tiles_editor/paint_tool", "Paint", KEY_D));
	paint_tool_button->set_tooltip(("Shift: Draw line. \nShift+Ctrl: Draw rectangle."));
	paint_tool_button->connect("pressed", this, "_update_toolbar");
	tilemap_tiles_tools_buttons->add_child(paint_tool_button);

	line_tool_button = memnew(Button);
	line_tool_button->set_flat(true);
	line_tool_button->set_toggle_mode(true);
	line_tool_button->set_button_group(tool_buttons_group);
	line_tool_button->set_shortcut(ED_SHORTCUT("tiles_editor/line_tool", "Line", KEY_L));
	line_tool_button->connect("pressed", this, "_update_toolbar");
	tilemap_tiles_tools_buttons->add_child(line_tool_button);

	rect_tool_button = memnew(Button);
	rect_tool_button->set_flat(true);
	rect_tool_button->set_toggle_mode(true);
	rect_tool_button->set_button_group(tool_buttons_group);
	rect_tool_button->set_shortcut(ED_SHORTCUT("tiles_editor/rect_tool", "Rect", KEY_R));
	rect_tool_button->connect("pressed", this, "_update_toolbar");
	tilemap_tiles_tools_buttons->add_child(rect_tool_button);

	bucket_tool_button = memnew(Button);
	bucket_tool_button->set_flat(true);
	bucket_tool_button->set_toggle_mode(true);
	bucket_tool_button->set_button_group(tool_buttons_group);
	bucket_tool_button->set_shortcut(ED_SHORTCUT("tiles_editor/bucket_tool", "Bucket", KEY_B));
	bucket_tool_button->connect("pressed", this, "_update_toolbar");
	tilemap_tiles_tools_buttons->add_child(bucket_tool_button);
	toolbar->add_child(tilemap_tiles_tools_buttons);

	// -- TileMap tool settings --
	tools_settings = memnew(HBoxContainer);
	toolbar->add_child(tools_settings);

	tools_settings_vsep = memnew(VSeparator);
	tools_settings->add_child(tools_settings_vsep);

	// Picker
	picker_button = memnew(Button);
	picker_button->set_flat(true);
	picker_button->set_toggle_mode(true);
	picker_button->set_shortcut(ED_SHORTCUT("tiles_editor/picker", "Picker", KEY_P));
	picker_button->set_tooltip(TTR("Alternatively hold Ctrl with other tools to pick tile."));
	picker_button->connect("pressed", CanvasItemEditor::get_singleton(), "update_viewport");
	tools_settings->add_child(picker_button);

	// Erase button.
	erase_button = memnew(Button);
	erase_button->set_flat(true);
	erase_button->set_toggle_mode(true);
	erase_button->set_shortcut(ED_SHORTCUT("tiles_editor/eraser", "Eraser", KEY_E));
	erase_button->set_tooltip(TTR("Alternatively use RMB to erase tiles."));
	erase_button->connect("pressed", CanvasItemEditor::get_singleton(), "update_viewport");
	tools_settings->add_child(erase_button);

	// Separator 2.
	tools_settings_vsep_2 = memnew(VSeparator);
	tools_settings->add_child(tools_settings_vsep_2);

	// Continuous checkbox.
	bucket_contiguous_checkbox = memnew(CheckBox);
	bucket_contiguous_checkbox->set_flat(true);
	bucket_contiguous_checkbox->set_text(TTR("Contiguous"));
	bucket_contiguous_checkbox->set_pressed(true);
	tools_settings->add_child(bucket_contiguous_checkbox);

	// Random tile checkbox.
	random_tile_checkbox = memnew(CheckBox);
	random_tile_checkbox->set_flat(true);
	random_tile_checkbox->set_text(TTR("Place Random Tile"));
	random_tile_checkbox->connect("toggled", this, "_on_random_tile_checkbox_toggled");
	tools_settings->add_child(random_tile_checkbox);

	// Random tile scattering.
	scatter_label = memnew(Label);
	scatter_label->set_tooltip(TTR("Defines the probability of painting nothing instead of a randomly selected tile."));
	scatter_label->set_text(TTR("Scattering:"));
	tools_settings->add_child(scatter_label);

	scatter_spinbox = memnew(SpinBox);
	scatter_spinbox->set_min(0.0);
	scatter_spinbox->set_max(1000);
	scatter_spinbox->set_step(0.001);
	scatter_spinbox->set_tooltip(TTR("Defines the probability of painting nothing instead of a randomly selected tile."));
	scatter_spinbox->get_line_edit()->add_constant_override("minimum_character_width", 4);
	scatter_spinbox->connect("value_changed", this, "_on_scattering_spinbox_changed");
	tools_settings->add_child(scatter_spinbox);

	_on_random_tile_checkbox_toggled(false);

	// Default tool.
	paint_tool_button->set_pressed(true);
	_update_toolbar();

	// --- Bottom panel tiles ---
	tiles_bottom_panel = memnew(VBoxContainer);
	tiles_bottom_panel->connect("tree_entered", this, "_update_theme");
	//tiles_bottom_panel->connect("theme_changed", this, "_update_theme");
	tiles_bottom_panel->connect("visibility_changed", this, "_stop_dragging");
	tiles_bottom_panel->connect("visibility_changed", this, "_tab_changed");
	tiles_bottom_panel->set_name(TTR("Tiles"));

	missing_source_label = memnew(Label);
	missing_source_label->set_text(TTR("This TileMap's RTileSet has no source configured. Edit the RTileSet resource to add one."));
	missing_source_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	missing_source_label->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	missing_source_label->set_align(Label::ALIGN_CENTER);
	missing_source_label->set_valign(Label::VALIGN_CENTER);
	missing_source_label->hide();
	tiles_bottom_panel->add_child(missing_source_label);

	atlas_sources_split_container = memnew(HSplitContainer);
	atlas_sources_split_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	atlas_sources_split_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tiles_bottom_panel->add_child(atlas_sources_split_container);

	sources_list = memnew(ItemList);
	sources_list->set_fixed_icon_size(Size2i(60, 60) * EDSCALE);
	sources_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sources_list->set_stretch_ratio(0.25);
	sources_list->set_custom_minimum_size(Size2i(70, 0) * EDSCALE);
	//sources_list->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	sources_list->connect("item_selected", this, "_update_fix_selected_and_hovered");
	sources_list->connect("item_selected", this, "_update_source_display");
	sources_list->connect("item_selected", RTilesEditorPlugin::get_singleton(), "set_sources_lists_current");
	Vector<Variant> sources_list_arr;
	sources_list_arr.push_back(sources_list);
	sources_list->connect("visibility_changed", RTilesEditorPlugin::get_singleton(), "synchronize_sources_list", sources_list_arr);
	atlas_sources_split_container->add_child(sources_list);

	// Tile atlas source.
	tile_atlas_view = memnew(RTileAtlasView);
	tile_atlas_view->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tile_atlas_view->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tile_atlas_view->set_texture_grid_visible(false);
	tile_atlas_view->set_tile_shape_grid_visible(false);
	tile_atlas_view->connect("transform_changed", RTilesEditorPlugin::get_singleton(), "set_atlas_view_transform");
	atlas_sources_split_container->add_child(tile_atlas_view);

	tile_atlas_control = memnew(Control);
	tile_atlas_control->connect("draw", this, "_tile_atlas_control_draw");
	tile_atlas_control->connect("mouse_exited", this, "_tile_atlas_control_mouse_exited");
	tile_atlas_control->connect("gui_input", this, "_tile_atlas_control_gui_input");
	tile_atlas_view->add_control_over_atlas_tiles(tile_atlas_control);

	alternative_tiles_control = memnew(Control);
	alternative_tiles_control->connect("draw", this, "_tile_alternatives_control_draw");
	alternative_tiles_control->connect("mouse_exited", this, "_tile_alternatives_control_mouse_exited");
	alternative_tiles_control->connect("gui_input", this, "_tile_alternatives_control_gui_input");
	tile_atlas_view->add_control_over_alternative_tiles(alternative_tiles_control);

	// Scenes collection source.
	scene_tiles_list = memnew(ItemList);
	scene_tiles_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	scene_tiles_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	//scene_tiles_list->set_drag_forwarding(this);
	scene_tiles_list->set_select_mode(ItemList::SELECT_MULTI);
	scene_tiles_list->connect("multi_selected", this, "_scenes_list_multi_selected");
	scene_tiles_list->connect("nothing_selected", this, "_scenes_list_nothing_selected");
	//scene_tiles_list->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	atlas_sources_split_container->add_child(scene_tiles_list);

	// Invalid source label.
	invalid_source_label = memnew(Label);
	invalid_source_label->set_text(TTR("Invalid source selected."));
	invalid_source_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	invalid_source_label->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	invalid_source_label->set_align(Label::ALIGN_CENTER);
	invalid_source_label->set_valign(Label::VALIGN_CENTER);
	invalid_source_label->hide();
	atlas_sources_split_container->add_child(invalid_source_label);

	// --- Bottom panel patterns ---
	patterns_bottom_panel = memnew(VBoxContainer);
	patterns_bottom_panel->set_name(TTR("Patterns"));
	patterns_bottom_panel->connect("visibility_changed", this, "_tab_changed");

	int thumbnail_size = 64;
	patterns_item_list = memnew(ItemList);
	patterns_item_list->set_max_columns(0);
	patterns_item_list->set_icon_mode(ItemList::ICON_MODE_TOP);
	patterns_item_list->set_fixed_column_width(thumbnail_size * 3 / 2);
	patterns_item_list->set_max_text_lines(2);
	patterns_item_list->set_fixed_icon_size(Size2(thumbnail_size, thumbnail_size));
	patterns_item_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	patterns_item_list->connect("gui_input", this, "_patterns_item_list_gui_input");
	patterns_item_list->connect("item_selected", this, "_update_selection_pattern_from_tileset_pattern_selection");
	patterns_item_list->connect("item_activated", this, "_update_selection_pattern_from_tileset_pattern_selection");
	patterns_item_list->connect("nothing_selected", this, "_update_selection_pattern_from_tileset_pattern_selection");
	patterns_bottom_panel->add_child(patterns_item_list);

	patterns_help_label = memnew(Label);
	patterns_help_label->set_text(TTR("Drag and drop or paste a TileMap selection here to store a pattern."));
	patterns_help_label->set_anchors_and_margins_preset(Control::PRESET_CENTER);
	patterns_item_list->add_child(patterns_help_label);

	// Update.
	_update_source_display();
}

RTileMapEditorTilesPlugin::~RTileMapEditorTilesPlugin() {
}

void RTileMapEditorTerrainsPlugin::tile_set_changed() {
	_update_terrains_cache();
	_update_terrains_tree();
	_update_tiles_list();
}

void RTileMapEditorTerrainsPlugin::_update_toolbar() {
	// Hide all settings.
	for (int i = 0; i < tools_settings->get_child_count(); i++) {
		Object::cast_to<CanvasItem>(tools_settings->get_child(i))->hide();
	}

	// Show only the correct settings.
	if (tool_buttons_group->get_pressed_button() == paint_tool_button) {
		tools_settings_vsep->show();
		picker_button->show();
		erase_button->show();
		tools_settings_vsep_2->hide();
		bucket_contiguous_checkbox->hide();
	} else if (tool_buttons_group->get_pressed_button() == line_tool_button) {
		tools_settings_vsep->show();
		picker_button->show();
		erase_button->show();
		tools_settings_vsep_2->hide();
		bucket_contiguous_checkbox->hide();
	} else if (tool_buttons_group->get_pressed_button() == rect_tool_button) {
		tools_settings_vsep->show();
		picker_button->show();
		erase_button->show();
		tools_settings_vsep_2->hide();
		bucket_contiguous_checkbox->hide();
	} else if (tool_buttons_group->get_pressed_button() == bucket_tool_button) {
		tools_settings_vsep->show();
		picker_button->show();
		erase_button->show();
		tools_settings_vsep_2->show();
		bucket_contiguous_checkbox->show();
	}
}

Vector<RTileMapEditorPlugin::TabData> RTileMapEditorTerrainsPlugin::get_tabs() const {
	Vector<RTileMapEditorPlugin::TabData> tabs;
	tabs.push_back({ toolbar, main_vbox_container });
	return tabs;
}

Map<Vector2i, RTileMapCell> RTileMapEditorTerrainsPlugin::_draw_terrains(const Map<Vector2i, RTileSet::TerrainsPattern> &p_to_paint, int p_terrain_set) const {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return Map<Vector2i, RTileMapCell>();
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return Map<Vector2i, RTileMapCell>();
	}

	Map<Vector2i, RTileMapCell> output;

	// Add the constraints from the added tiles.
	Set<RTileMap::TerrainConstraint> added_tiles_constraints_set;
	for (Map<Vector2i, RTileSet::TerrainsPattern>::Element *E_to_paint = p_to_paint.front(); E_to_paint; E_to_paint = E_to_paint->next()) {
		Vector2i coords = E_to_paint->key();
		RTileSet::TerrainsPattern terrains_pattern = E_to_paint->value();

		Set<RTileMap::TerrainConstraint> cell_constraints = tile_map->get_terrain_constraints_from_added_tile(coords, p_terrain_set, terrains_pattern);
		for (Set<RTileMap::TerrainConstraint>::Element *E = cell_constraints.front(); E; E = E->next()) {
			added_tiles_constraints_set.insert(E->get());
		}
	}

	// Build the list of potential tiles to replace.
	Set<Vector2i> potential_to_replace;
	for (Map<Vector2i, RTileSet::TerrainsPattern>::Element *E_to_paint = p_to_paint.front(); E_to_paint; E_to_paint = E_to_paint->next()) {
		Vector2i coords = E_to_paint->key();
		for (int i = 0; i < RTileSet::CELL_NEIGHBOR_MAX; i++) {
			if (tile_map->is_existing_neighbor(RTileSet::CellNeighbor(i))) {
				Vector2i neighbor = tile_map->get_neighbor_cell(coords, RTileSet::CellNeighbor(i));
				if (!p_to_paint.has(neighbor)) {
					potential_to_replace.insert(neighbor);
				}
			}
		}
	}

	// Set of tiles to replace
	Set<Vector2i> to_replace;

	// Add the central tiles to the one to replace.
	for (Map<Vector2i, RTileSet::TerrainsPattern>::Element *E_to_paint = p_to_paint.front(); E_to_paint; E_to_paint = E_to_paint->next()) {
		to_replace.insert(E_to_paint->key());
	}

	// Add the constraints from the surroundings of the modified areas.
	Set<RTileMap::TerrainConstraint> removed_cells_constraints_set;
	bool to_replace_modified = true;
	while (to_replace_modified) {
		// Get the constraints from the removed cells.
		removed_cells_constraints_set = tile_map->get_terrain_constraints_from_removed_cells_list(tile_map_layer, to_replace, p_terrain_set);

		// Filter the sources to make sure they are in the potential_to_replace.
		Map<RTileMap::TerrainConstraint, Set<Vector2i>> per_constraint_tiles;
		for (Set<RTileMap::TerrainConstraint>::Element *E = removed_cells_constraints_set.front(); E; E = E->next()) {
			Map<Vector2i, RTileSet::CellNeighbor> sources_of_constraint = E->get().get_overlapping_coords_and_peering_bits();
			for (Map<Vector2i, RTileSet::CellNeighbor>::Element *E_source_tile_of_constraint = sources_of_constraint.front(); E_source_tile_of_constraint; E_source_tile_of_constraint = E_source_tile_of_constraint->next()) {
				if (potential_to_replace.has(E_source_tile_of_constraint->key())) {
					per_constraint_tiles[E->get()].insert(E_source_tile_of_constraint->key());
				}
			}
		}

		to_replace_modified = false;
		for (Set<RTileMap::TerrainConstraint>::Element *E = added_tiles_constraints_set.front(); E; E = E->next()) {
			RTileMap::TerrainConstraint c = E->get();
			// Check if we have a conflict in constraints.
			if (removed_cells_constraints_set.has(c) && removed_cells_constraints_set.find(c)->get().get_terrain() != c.get_terrain()) {
				// If we do, we search for a neighbor to remove.
				if (per_constraint_tiles.has(c) && !per_constraint_tiles[c].empty()) {
					// Remove it.
					Vector2i to_add_to_remove = per_constraint_tiles[c].front()->get();
					potential_to_replace.erase(to_add_to_remove);
					to_replace.insert(to_add_to_remove);
					to_replace_modified = true;
					for (Map<RTileMap::TerrainConstraint, Set<Vector2i>>::Element *E_source_tiles_of_constraint = per_constraint_tiles.front(); E_source_tiles_of_constraint; E_source_tiles_of_constraint = E_source_tiles_of_constraint->next()) {
						E_source_tiles_of_constraint->value().erase(to_add_to_remove);
					}
					break;
				}
			}
		}
	}

	// Combine all constraints together.
	Set<RTileMap::TerrainConstraint> constraints = removed_cells_constraints_set;
	for (Set<RTileMap::TerrainConstraint>::Element *E = added_tiles_constraints_set.front(); E; E = E->next()) {
		constraints.insert(E->get());
	}

	// Remove the central tiles from the ones to replace.
	for (Map<Vector2i, RTileSet::TerrainsPattern>::Element *E_to_paint = p_to_paint.front(); E_to_paint; E_to_paint = E_to_paint->next()) {
		to_replace.erase(E_to_paint->key());
	}

	// Run WFC to fill the holes with the constraints.
	Map<Vector2i, RTileSet::TerrainsPattern> wfc_output = tile_map->terrain_wave_function_collapse(to_replace, p_terrain_set, constraints);

	// Actually paint the tiles.
	for (Map<Vector2i, RTileSet::TerrainsPattern>::Element *E_to_paint = p_to_paint.front(); E_to_paint; E_to_paint = E_to_paint->next()) {
		output[E_to_paint->key()] = tile_set->get_random_tile_from_terrains_pattern(p_terrain_set, E_to_paint->value());
	}

	// Use the WFC run for the output.
	for (Map<Vector2i, RTileSet::TerrainsPattern>::Element *E = wfc_output.front(); E; E = E->next()) {
		output[E->key()] = tile_set->get_random_tile_from_terrains_pattern(p_terrain_set, E->value());
	}

	return output;
}

Map<Vector2i, RTileMapCell> RTileMapEditorTerrainsPlugin::_draw_line(Vector2i p_start_cell, Vector2i p_end_cell, bool p_erase) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return Map<Vector2i, RTileMapCell>();
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return Map<Vector2i, RTileMapCell>();
	}

	RTileSet::TerrainsPattern terrains_pattern;
	if (p_erase) {
		terrains_pattern = RTileSet::TerrainsPattern(*tile_set, selected_terrain_set);
	} else {
		terrains_pattern = selected_terrains_pattern;
	}

	Vector<Vector2i> line = RTileMapEditor::get_line(tile_map, p_start_cell, p_end_cell);
	Map<Vector2i, RTileSet::TerrainsPattern> to_draw;
	for (int i = 0; i < line.size(); i++) {
		to_draw[line[i]] = terrains_pattern;
	}
	return _draw_terrains(to_draw, selected_terrain_set);
}

Map<Vector2i, RTileMapCell> RTileMapEditorTerrainsPlugin::_draw_rect(Vector2i p_start_cell, Vector2i p_end_cell, bool p_erase) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return Map<Vector2i, RTileMapCell>();
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return Map<Vector2i, RTileMapCell>();
	}

	RTileSet::TerrainsPattern terrains_pattern;
	if (p_erase) {
		terrains_pattern = RTileSet::TerrainsPattern(*tile_set, selected_terrain_set);
	} else {
		terrains_pattern = selected_terrains_pattern;
	}

	Rect2i rect;
	rect.set_position(p_start_cell);
	MathExt::rect2i_set_end(&rect, p_end_cell);
	rect = MathExt::rect2i_abs(rect);

	Map<Vector2i, RTileSet::TerrainsPattern> to_draw;
	Point2i rect_end = MathExt::rect2i_get_end(rect);
	for (int x = rect.position.x; x <= rect_end.x; x++) {
		for (int y = rect.position.y; y <= rect_end.y; y++) {
			to_draw[Vector2i(x, y)] = terrains_pattern;
		}
	}
	return _draw_terrains(to_draw, selected_terrain_set);
}

Set<Vector2i> RTileMapEditorTerrainsPlugin::_get_cells_for_bucket_fill(Vector2i p_coords, bool p_contiguous) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return Set<Vector2i>();
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return Set<Vector2i>();
	}

	RTileMapCell source_cell = tile_map->get_cell(tile_map_layer, p_coords);

	RTileSet::TerrainsPattern source_pattern(*tile_set, selected_terrain_set);
	if (source_cell.source_id != RTileSet::INVALID_SOURCE) {
		RTileData *tile_data = nullptr;
		Ref<RTileSetSource> source = tile_set->get_source(source_cell.source_id);
		Ref<RTileSetAtlasSource> atlas_source = source;
		if (atlas_source.is_valid()) {
			tile_data = Object::cast_to<RTileData>(atlas_source->get_tile_data(source_cell.get_atlas_coords(), source_cell.alternative_tile));
		}
		if (!tile_data) {
			return Set<Vector2i>();
		}
		source_pattern = tile_data->get_terrains_pattern();
	}

	// If we are filling empty tiles, compute the tilemap boundaries.
	Rect2i boundaries;
	if (source_cell.source_id == RTileSet::INVALID_SOURCE) {
		boundaries = tile_map->get_used_rect();
	}

	Set<Vector2i> output;
	if (p_contiguous) {
		// Replace continuous tiles like the source.
		Set<Vector2i> already_checked;
		List<Vector2i> to_check;
		to_check.push_back(p_coords);
		while (!to_check.empty()) {
			Vector2i coords = to_check.back()->get();
			to_check.pop_back();
			if (!already_checked.has(coords)) {
				// Get the candidate cell pattern.
				RTileSet::TerrainsPattern candidate_pattern(*tile_set, selected_terrain_set);
				if (tile_map->get_cell_source_id(tile_map_layer, coords) != RTileSet::INVALID_SOURCE) {
					RTileData *tile_data = nullptr;
					Ref<RTileSetSource> source = tile_set->get_source(tile_map->get_cell_source_id(tile_map_layer, coords));
					Ref<RTileSetAtlasSource> atlas_source = source;
					if (atlas_source.is_valid()) {
						tile_data = Object::cast_to<RTileData>(atlas_source->get_tile_data(tile_map->get_cell_atlas_coords(tile_map_layer, coords), tile_map->get_cell_alternative_tile(tile_map_layer, coords)));
					}
					if (tile_data) {
						candidate_pattern = tile_data->get_terrains_pattern();
					}
				}

				// Draw.
				if (candidate_pattern == source_pattern && (!source_pattern.is_erase_pattern() || boundaries.has_point(coords))) {
					output.insert(coords);

					// Get surrounding tiles (handles different tile shapes).
					Vector<Vector2> around = tile_map->get_surrounding_tiles(coords);
					for (int i = 0; i < around.size(); i++) {
						to_check.push_back(around[i]);
					}
				}
				already_checked.insert(coords);
			}
		}
	} else {
		// Replace all tiles like the source.
		Vector<Vector2> to_check;
		if (source_cell.source_id == RTileSet::INVALID_SOURCE) {
			Rect2i rect = tile_map->get_used_rect();
			if (rect.has_no_area()) {
				rect = Rect2i(p_coords, Vector2i(1, 1));
			}

			Point2i boundaries_end = MathExt::rect2i_get_end(boundaries);
			for (int x = boundaries.position.x; x < boundaries_end.x; x++) {
				for (int y = boundaries.position.y; y < boundaries_end.y; y++) {
					to_check.push_back(Vector2(x, y));
				}
			}
		} else {
			to_check = tile_map->get_used_cells(tile_map_layer);
		}
		for (int i = 0; i < to_check.size(); i++) {
			Vector2i coords = to_check[i];
			// Get the candidate cell pattern.
			RTileSet::TerrainsPattern candidate_pattern;
			if (tile_map->get_cell_source_id(tile_map_layer, coords) != RTileSet::INVALID_SOURCE) {
				RTileData *tile_data = nullptr;
				Ref<RTileSetSource> source = tile_set->get_source(tile_map->get_cell_source_id(tile_map_layer, coords));
				Ref<RTileSetAtlasSource> atlas_source = source;
				if (atlas_source.is_valid()) {
					tile_data = Object::cast_to<RTileData>(atlas_source->get_tile_data(tile_map->get_cell_atlas_coords(tile_map_layer, coords), tile_map->get_cell_alternative_tile(tile_map_layer, coords)));
				}
				if (tile_data) {
					candidate_pattern = tile_data->get_terrains_pattern();
				}
			}

			// Draw.
			if (candidate_pattern == source_pattern && (!source_pattern.is_erase_pattern() || boundaries.has_point(coords))) {
				output.insert(coords);
			}
		}
	}
	return output;
}

Map<Vector2i, RTileMapCell> RTileMapEditorTerrainsPlugin::_draw_bucket_fill(Vector2i p_coords, bool p_contiguous, bool p_erase) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return Map<Vector2i, RTileMapCell>();
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return Map<Vector2i, RTileMapCell>();
	}

	RTileSet::TerrainsPattern terrains_pattern;
	if (p_erase) {
		terrains_pattern = RTileSet::TerrainsPattern(*tile_set, selected_terrain_set);
	} else {
		terrains_pattern = selected_terrains_pattern;
	}

	Set<Vector2i> cells_to_draw = _get_cells_for_bucket_fill(p_coords, p_contiguous);
	Map<Vector2i, RTileSet::TerrainsPattern> to_draw;
	for (Set<Vector2i>::Element *coords = cells_to_draw.front(); coords; coords = coords->next()) {
		to_draw[coords->get()] = terrains_pattern;
	}

	return _draw_terrains(to_draw, selected_terrain_set);
}

void RTileMapEditorTerrainsPlugin::_stop_dragging() {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
	Vector2 mpos = xform.affine_inverse().xform(CanvasItemEditor::get_singleton()->get_viewport_control()->get_local_mouse_position());

	switch (drag_type) {
		case DRAG_TYPE_PICK: {
			Vector2i coords = tile_map->world_to_map(mpos);
			RTileMapCell cell = tile_map->get_cell(tile_map_layer, coords);
			RTileData *tile_data = nullptr;

			Ref<RTileSetSource> source = tile_set->get_source(cell.source_id);
			Ref<RTileSetAtlasSource> atlas_source = source;
			if (atlas_source.is_valid()) {
				tile_data = Object::cast_to<RTileData>(atlas_source->get_tile_data(cell.get_atlas_coords(), cell.alternative_tile));
			}

			if (tile_data) {
				RTileSet::TerrainsPattern terrains_pattern = tile_data->get_terrains_pattern();

				// Find the tree item for the right terrain set.
				bool need_tree_item_switch = true;
				TreeItem *tree_item = terrains_tree->get_selected();
				int new_terrain_set = -1;
				if (tree_item) {
					Dictionary metadata_dict = tree_item->get_metadata(0);
					if (metadata_dict.has("terrain_set") && metadata_dict.has("terrain_id")) {
						int terrain_set = metadata_dict["terrain_set"];
						int terrain_id = metadata_dict["terrain_id"];
						if (per_terrain_terrains_patterns[terrain_set][terrain_id].has(terrains_pattern)) {
							new_terrain_set = terrain_set;
							need_tree_item_switch = false;
						}
					}
				}

				if (need_tree_item_switch) {
					for (tree_item = terrains_tree->get_root()->get_children(); tree_item; tree_item = tree_item->get_next_visible()) {
						Dictionary metadata_dict = tree_item->get_metadata(0);
						if (metadata_dict.has("terrain_set") && metadata_dict.has("terrain_id")) {
							int terrain_set = metadata_dict["terrain_set"];
							int terrain_id = metadata_dict["terrain_id"];
							if (per_terrain_terrains_patterns[terrain_set][terrain_id].has(terrains_pattern)) {
								// Found
								new_terrain_set = terrain_set;
								tree_item->select(0);
								_update_tiles_list();
								break;
							}
						}
					}
				}

				// Find the list item for the given tile.
				if (tree_item) {
					for (int i = 0; i < terrains_tile_list->get_item_count(); i++) {
						Dictionary metadata_dict = terrains_tile_list->get_item_metadata(i);
						RTileSet::TerrainsPattern in_meta_terrains_pattern(*tile_set, new_terrain_set);
						in_meta_terrains_pattern.set_terrains_from_array(metadata_dict["terrains_pattern"]);
						if (in_meta_terrains_pattern == terrains_pattern) {
							terrains_tile_list->select(i);
							break;
						}
					}
				} else {
					ERR_PRINT("Terrain tile not found.");
				}
			}
			picker_button->set_pressed(false);
		} break;
		case DRAG_TYPE_PAINT: {
			undo_redo->create_action(TTR("Paint terrain"));
			for (Map<Vector2i, RTileMapCell>::Element *E = drag_modified.front(); E; E = E->next()) {
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), tile_map->get_cell_source_id(tile_map_layer, E->key()), tile_map->get_cell_atlas_coords(tile_map_layer, E->key()), tile_map->get_cell_alternative_tile(tile_map_layer, E->key()));
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), E->value().source_id, E->value().get_atlas_coords(), E->value().alternative_tile);
			}
			undo_redo->commit_action();
		} break;
		case DRAG_TYPE_LINE: {
			Map<Vector2i, RTileMapCell> to_draw = _draw_line(tile_map->world_to_map(drag_start_mouse_pos), tile_map->world_to_map(mpos), drag_erasing);
			undo_redo->create_action(TTR("Paint terrain"));
			for (Map<Vector2i, RTileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
				if (!drag_erasing && E->value().source_id == RTileSet::INVALID_SOURCE) {
					continue;
				}
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), E->value().source_id, E->value().get_atlas_coords(), E->value().alternative_tile);
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), tile_map->get_cell_source_id(tile_map_layer, E->key()), tile_map->get_cell_atlas_coords(tile_map_layer, E->key()), tile_map->get_cell_alternative_tile(tile_map_layer, E->key()));
			}
			undo_redo->commit_action();
		} break;
		case DRAG_TYPE_RECT: {
			Map<Vector2i, RTileMapCell> to_draw = _draw_rect(tile_map->world_to_map(drag_start_mouse_pos), tile_map->world_to_map(mpos), drag_erasing);
			undo_redo->create_action(TTR("Paint terrain"));
			for (Map<Vector2i, RTileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
				if (!drag_erasing && E->value().source_id == RTileSet::INVALID_SOURCE) {
					continue;
				}
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), E->value().source_id, E->value().get_atlas_coords(), E->value().alternative_tile);
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), tile_map->get_cell_source_id(tile_map_layer, E->key()), tile_map->get_cell_atlas_coords(tile_map_layer, E->key()), tile_map->get_cell_alternative_tile(tile_map_layer, E->key()));
			}
			undo_redo->commit_action();
		} break;
		case DRAG_TYPE_BUCKET: {
			undo_redo->create_action(TTR("Paint terrain"));
			for (Map<Vector2i, RTileMapCell>::Element *E = drag_modified.front(); E; E = E->next()) {
				undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), tile_map->get_cell_source_id(tile_map_layer, E->key()), tile_map->get_cell_atlas_coords(tile_map_layer, E->key()), tile_map->get_cell_alternative_tile(tile_map_layer, E->key()));
				undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, Vector2(E->key()), E->value().source_id, E->value().get_atlas_coords(), E->value().alternative_tile);
			}
			undo_redo->commit_action();
		} break;

		default:
			break;
	}
	drag_type = DRAG_TYPE_NONE;
}

void RTileMapEditorTerrainsPlugin::_mouse_exited_viewport() {
	has_mouse = false;
	CanvasItemEditor::get_singleton()->update_viewport();
}

void RTileMapEditorTerrainsPlugin::_update_selection() {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	// Get the selected terrain.
	selected_terrains_pattern = RTileSet::TerrainsPattern();
	selected_terrain_set = -1;

	TreeItem *selected_tree_item = terrains_tree->get_selected();
	if (selected_tree_item && selected_tree_item->get_metadata(0)) {
		Dictionary metadata_dict = selected_tree_item->get_metadata(0);
		// Selected terrain
		selected_terrain_set = metadata_dict["terrain_set"];

		// Selected tile
		if (erase_button->is_pressed()) {
			selected_terrains_pattern = RTileSet::TerrainsPattern(*tile_set, selected_terrain_set);
		} else if (terrains_tile_list->is_anything_selected()) {
			metadata_dict = terrains_tile_list->get_item_metadata(terrains_tile_list->get_selected_items()[0]);
			selected_terrains_pattern = RTileSet::TerrainsPattern(*tile_set, selected_terrain_set);
			selected_terrains_pattern.set_terrains_from_array(metadata_dict["terrains_pattern"]);
		}
	}
}

bool RTileMapEditorTerrainsPlugin::forward_canvas_gui_input(const Ref<InputEvent> &p_event) {
	if (!main_vbox_container->is_visible_in_tree()) {
		// If the bottom editor is not visible, we ignore inputs.
		return false;
	}

	if (CanvasItemEditor::get_singleton()->get_current_tool() != CanvasItemEditor::TOOL_SELECT) {
		return false;
	}

	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return false;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return false;
	}

	if (tile_map_layer < 0) {
		return false;
	}
	ERR_FAIL_COND_V(tile_map_layer >= tile_map->get_layers_count(), false);

	_update_selection();

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		has_mouse = true;
		Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
		Vector2 mpos = xform.affine_inverse().xform(mm->get_position());

		switch (drag_type) {
			case DRAG_TYPE_PAINT: {
				if (selected_terrain_set >= 0) {
					Map<Vector2i, RTileMapCell> to_draw = _draw_line(tile_map->world_to_map(drag_last_mouse_pos), tile_map->world_to_map(mpos), drag_erasing);
					for (Map<Vector2i, RTileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
						if (!drag_modified.has(E->key())) {
							drag_modified[E->key()] = tile_map->get_cell(tile_map_layer, E->key());
						}
						tile_map->set_cell(tile_map_layer, E->key(), E->value().source_id, E->value().get_atlas_coords(), E->value().alternative_tile);
					}
				}
			} break;
			default:
				break;
		}
		drag_last_mouse_pos = mpos;
		CanvasItemEditor::get_singleton()->update_viewport();

		return true;
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		has_mouse = true;
		Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
		Vector2 mpos = xform.affine_inverse().xform(mb->get_position());

		if (mb->get_button_index() == BUTTON_LEFT || mb->get_button_index() == BUTTON_RIGHT) {
			if (mb->is_pressed()) {
				// Pressed
				if (erase_button->is_pressed() || mb->get_button_index() == BUTTON_RIGHT) {
					drag_erasing = true;
				}

				if (picker_button->is_pressed()) {
					drag_type = DRAG_TYPE_PICK;
				} else {
					// Paint otherwise.
					if (tool_buttons_group->get_pressed_button() == paint_tool_button && !Input::get_singleton()->is_key_pressed(KEY_CONTROL) && !Input::get_singleton()->is_key_pressed(KEY_SHIFT)) {
						if (selected_terrain_set < 0 || !selected_terrains_pattern.is_valid()) {
							return true;
						}

						drag_type = DRAG_TYPE_PAINT;
						drag_start_mouse_pos = mpos;

						drag_modified.clear();
						Vector2i cell = tile_map->world_to_map(mpos);
						Map<Vector2i, RTileMapCell> to_draw = _draw_line(cell, cell, drag_erasing);
						for (Map<Vector2i, RTileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
							drag_modified[E->key()] = tile_map->get_cell(tile_map_layer, E->key());
							tile_map->set_cell(tile_map_layer, E->key(), E->value().source_id, E->value().get_atlas_coords(), E->value().alternative_tile);
						}
					} else if (tool_buttons_group->get_pressed_button() == line_tool_button || (tool_buttons_group->get_pressed_button() == paint_tool_button && Input::get_singleton()->is_key_pressed(KEY_SHIFT) && !Input::get_singleton()->is_key_pressed(KEY_CONTROL))) {
						if (selected_terrain_set < 0 || !selected_terrains_pattern.is_valid()) {
							return true;
						}
						drag_type = DRAG_TYPE_LINE;
						drag_start_mouse_pos = mpos;
						drag_modified.clear();
					} else if (tool_buttons_group->get_pressed_button() == rect_tool_button || (tool_buttons_group->get_pressed_button() == paint_tool_button && Input::get_singleton()->is_key_pressed(KEY_SHIFT) && Input::get_singleton()->is_key_pressed(KEY_CONTROL))) {
						if (selected_terrain_set < 0 || !selected_terrains_pattern.is_valid()) {
							return true;
						}
						drag_type = DRAG_TYPE_RECT;
						drag_start_mouse_pos = mpos;
						drag_modified.clear();
					} else if (tool_buttons_group->get_pressed_button() == bucket_tool_button) {
						if (selected_terrain_set < 0 || !selected_terrains_pattern.is_valid()) {
							return true;
						}
						drag_type = DRAG_TYPE_BUCKET;
						drag_start_mouse_pos = mpos;
						drag_modified.clear();
						Vector<Vector2i> line = RTileMapEditor::get_line(tile_map, tile_map->world_to_map(drag_last_mouse_pos), tile_map->world_to_map(mpos));
						for (int i = 0; i < line.size(); i++) {
							if (!drag_modified.has(line[i])) {
								Map<Vector2i, RTileMapCell> to_draw = _draw_bucket_fill(line[i], bucket_contiguous_checkbox->is_pressed(), drag_erasing);
								for (Map<Vector2i, RTileMapCell>::Element *E = to_draw.front(); E; E = E->next()) {
									if (!drag_erasing && E->value().source_id == RTileSet::INVALID_SOURCE) {
										continue;
									}
									Vector2i coords = E->key();
									if (!drag_modified.has(coords)) {
										drag_modified.insert(coords, tile_map->get_cell(tile_map_layer, coords));
									}
									tile_map->set_cell(tile_map_layer, coords, E->value().source_id, E->value().get_atlas_coords(), E->value().alternative_tile);
								}
							}
						}
					}
				}
			} else {
				// Released
				_stop_dragging();
				drag_erasing = false;
			}

			CanvasItemEditor::get_singleton()->update_viewport();

			return true;
		}
		drag_last_mouse_pos = mpos;
	}

	return false;
}

void RTileMapEditorTerrainsPlugin::forward_canvas_draw_over_viewport(Control *p_overlay) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	if (tile_map_layer < 0) {
		return;
	}
	ERR_FAIL_INDEX(tile_map_layer, tile_map->get_layers_count());

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	if (!tile_map->is_visible_in_tree()) {
		return;
	}

	Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
	Vector2i tile_shape_size = tile_set->get_tile_size();

	// Handle the preview of the tiles to be placed.
	if (main_vbox_container->is_visible_in_tree() && has_mouse) { // Only if the tilemap editor is opened and the viewport is hovered.
		Set<Vector2i> preview;
		Rect2i drawn_grid_rect;

		if (drag_type == DRAG_TYPE_PICK) {
			// Draw the area being picked.
			Vector2i coords = tile_map->world_to_map(drag_last_mouse_pos);
			if (tile_map->get_cell_source_id(tile_map_layer, coords) != RTileSet::INVALID_SOURCE) {
				Transform2D tile_xform;
				tile_xform.set_origin(tile_map->map_to_world(coords));
				tile_xform.set_scale(tile_shape_size);
				tile_set->draw_tile_shape(p_overlay, xform * tile_xform, Color(1.0, 1.0, 1.0), false);
			}
		} else if (!picker_button->is_pressed() && !(drag_type == DRAG_TYPE_NONE && Input::get_singleton()->is_key_pressed(KEY_CONTROL) && !Input::get_singleton()->is_key_pressed(KEY_SHIFT))) {
			bool expand_grid = false;
			if (tool_buttons_group->get_pressed_button() == paint_tool_button && drag_type == DRAG_TYPE_NONE) {
				// Preview for a single tile.
				preview.insert(tile_map->world_to_map(drag_last_mouse_pos));
				expand_grid = true;
			} else if (tool_buttons_group->get_pressed_button() == line_tool_button || drag_type == DRAG_TYPE_LINE) {
				if (drag_type == DRAG_TYPE_NONE) {
					// Preview for a single tile.
					preview.insert(tile_map->world_to_map(drag_last_mouse_pos));
				} else if (drag_type == DRAG_TYPE_LINE) {
					// Preview for a line.
					Vector<Vector2i> line = RTileMapEditor::get_line(tile_map, tile_map->world_to_map(drag_start_mouse_pos), tile_map->world_to_map(drag_last_mouse_pos));
					for (int i = 0; i < line.size(); i++) {
						preview.insert(line[i]);
					}
					expand_grid = true;
				}
			} else if (drag_type == DRAG_TYPE_RECT) {
				// Preview for a rect.
				Rect2i rect;
				rect.set_position(tile_map->world_to_map(drag_start_mouse_pos));
				MathExt::rect2i_set_end(&rect, tile_map->world_to_map(drag_last_mouse_pos));
				rect = MathExt::rect2i_abs(rect);

				Map<Vector2i, RTileSet::TerrainsPattern> to_draw;
				Point2i rect_end = MathExt::rect2i_get_end(rect);
				for (int x = rect.position.x; x <= rect_end.x; x++) {
					for (int y = rect.position.y; y <= rect_end.y; y++) {
						preview.insert(Vector2i(x, y));
					}
				}
				expand_grid = true;
			} else if (tool_buttons_group->get_pressed_button() == bucket_tool_button && drag_type == DRAG_TYPE_NONE) {
				// Preview for a fill.
				preview = _get_cells_for_bucket_fill(tile_map->world_to_map(drag_last_mouse_pos), bucket_contiguous_checkbox->is_pressed());
			}

			// Expand the grid if needed
			if (expand_grid && !preview.empty()) {
				drawn_grid_rect = Rect2i(preview.front()->get(), Vector2i(1, 1));

				for (Set<Vector2i>::Element *E = preview.front(); E; E = E->next()) {
					drawn_grid_rect.expand_to(E->get());
				}
			}
		}

		if (!preview.empty()) {
			const int fading = 5;

			// Draw the lines of the grid behind the preview.
			bool display_grid = EditorSettings::get_singleton()->get("editors/tiles_editor/display_grid");
			if (display_grid) {
				Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
				if (drawn_grid_rect.size.x > 0 && drawn_grid_rect.size.y > 0) {
					drawn_grid_rect = drawn_grid_rect.grow(fading);
					for (int x = drawn_grid_rect.position.x; x < (drawn_grid_rect.position.x + drawn_grid_rect.size.x); x++) {
						for (int y = drawn_grid_rect.position.y; y < (drawn_grid_rect.position.y + drawn_grid_rect.size.y); y++) {
							Vector2i pos_in_rect = Vector2i(x, y) - drawn_grid_rect.position;

							// Fade out the border of the grid.
							float left_opacity = CLAMP(Math::inverse_lerp(0.0f, (float)fading, (float)pos_in_rect.x), 0.0f, 1.0f);
							float right_opacity = CLAMP(Math::inverse_lerp((float)drawn_grid_rect.size.x, (float)(drawn_grid_rect.size.x - fading), (float)pos_in_rect.x), 0.0f, 1.0f);
							float top_opacity = CLAMP(Math::inverse_lerp(0.0f, (float)fading, (float)pos_in_rect.y), 0.0f, 1.0f);
							float bottom_opacity = CLAMP(Math::inverse_lerp((float)drawn_grid_rect.size.y, (float)(drawn_grid_rect.size.y - fading), (float)pos_in_rect.y), 0.0f, 1.0f);
							float opacity = CLAMP(MIN(left_opacity, MIN(right_opacity, MIN(top_opacity, bottom_opacity))) + 0.1, 0.0f, 1.0f);

							Transform2D tile_xform;
							tile_xform.set_origin(tile_map->map_to_world(Vector2(x, y)));
							tile_xform.set_scale(tile_shape_size);
							Color color = grid_color;
							color.a = color.a * opacity;
							tile_set->draw_tile_shape(p_overlay, xform * tile_xform, color, false);
						}
					}
				}
			}

			// Draw the preview.
			for (Set<Vector2i>::Element *E = preview.front(); E; E = E->next()) {
				Transform2D tile_xform;
				tile_xform.set_origin(tile_map->map_to_world(E->get()));
				tile_xform.set_scale(tile_set->get_tile_size());
				if (drag_erasing || erase_button->is_pressed()) {
					tile_set->draw_tile_shape(p_overlay, xform * tile_xform, Color(0.0, 0.0, 0.0, 0.5), true);
				} else {
					tile_set->draw_tile_shape(p_overlay, xform * tile_xform, Color(1.0, 1.0, 1.0, 0.5), true);
				}
			}
		}
	}
}

void RTileMapEditorTerrainsPlugin::_update_terrains_cache() {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	// Organizes tiles into structures.
	per_terrain_terrains_patterns.resize(tile_set->get_terrain_sets_count());
	for (int i = 0; i < tile_set->get_terrain_sets_count(); i++) {
		per_terrain_terrains_patterns[i].resize(tile_set->get_terrains_count(i));
		for (int j = 0; j < (int)per_terrain_terrains_patterns[i].size(); j++) {
			per_terrain_terrains_patterns[i][j].clear();
		}
	}

	for (int source_index = 0; source_index < tile_set->get_source_count(); source_index++) {
		int source_id = tile_set->get_source_id(source_index);
		Ref<RTileSetSource> source = tile_set->get_source(source_id);

		Ref<RTileSetAtlasSource> atlas_source = source;
		if (atlas_source.is_valid()) {
			for (int tile_index = 0; tile_index < source->get_tiles_count(); tile_index++) {
				Vector2i tile_id = source->get_tile_id(tile_index);
				for (int alternative_index = 0; alternative_index < source->get_alternative_tiles_count(tile_id); alternative_index++) {
					int alternative_id = source->get_alternative_tile_id(tile_id, alternative_index);

					RTileData *tile_data = Object::cast_to<RTileData>(atlas_source->get_tile_data(tile_id, alternative_id));
					int terrain_set = tile_data->get_terrain_set();
					if (terrain_set >= 0) {
						ERR_FAIL_INDEX(terrain_set, (int)per_terrain_terrains_patterns.size());

						RTileMapCell cell;
						cell.source_id = source_id;
						cell.set_atlas_coords(tile_id);
						cell.alternative_tile = alternative_id;

						RTileSet::TerrainsPattern terrains_pattern = tile_data->get_terrains_pattern();
						// Terrain bits.
						for (int i = 0; i < RTileSet::CELL_NEIGHBOR_MAX; i++) {
							RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
							if (tile_set->is_valid_peering_bit_terrain(terrain_set, bit)) {
								int terrain = terrains_pattern.get_terrain(bit);
								if (terrain >= 0 && terrain < (int)per_terrain_terrains_patterns[terrain_set].size()) {
									per_terrain_terrains_patterns[terrain_set][terrain].insert(terrains_pattern);
								}
							}
						}
					}
				}
			}
		}
	}
}

void RTileMapEditorTerrainsPlugin::_update_terrains_tree() {
	terrains_tree->clear();
	terrains_tree->create_item();

	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	// Fill in the terrain list.
	Vector<Vector<Ref<Texture>>> icons = tile_set->generate_terrains_icons(Size2(16, 16) * EDSCALE);
	for (int terrain_set_index = 0; terrain_set_index < tile_set->get_terrain_sets_count(); terrain_set_index++) {
		// Add an item for the terrain set.
		TreeItem *terrain_set_tree_item = terrains_tree->create_item();
		String matches;
		if (tile_set->get_terrain_set_mode(terrain_set_index) == RTileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES) {
			terrain_set_tree_item->set_icon(0, main_vbox_container->get_icon(("TerrainMatchCornersAndSides"), ("EditorIcons")));
			matches = String(TTR("Matches Corners and Sides"));
		} else if (tile_set->get_terrain_set_mode(terrain_set_index) == RTileSet::TERRAIN_MODE_MATCH_CORNERS) {
			terrain_set_tree_item->set_icon(0, main_vbox_container->get_icon(("TerrainMatchCorners"), ("EditorIcons")));
			matches = String(TTR("Matches Corners Only"));
		} else {
			terrain_set_tree_item->set_icon(0, main_vbox_container->get_icon(("TerrainMatchSides"), ("EditorIcons")));
			matches = String(TTR("Matches Sides Only"));
		}
		terrain_set_tree_item->set_text(0, vformat("Terrain Set %d (%s)", terrain_set_index, matches));
		terrain_set_tree_item->set_selectable(0, false);

		for (int terrain_index = 0; terrain_index < tile_set->get_terrains_count(terrain_set_index); terrain_index++) {
			// Add the item to the terrain list.
			TreeItem *terrain_tree_item = terrains_tree->create_item(terrain_set_tree_item);
			terrain_tree_item->set_text(0, tile_set->get_terrain_name(terrain_set_index, terrain_index));
			terrain_tree_item->set_icon_max_width(0, 32 * EDSCALE);
			terrain_tree_item->set_icon(0, icons[terrain_set_index][terrain_index]);

			Dictionary metadata_dict;
			metadata_dict["terrain_set"] = terrain_set_index;
			metadata_dict["terrain_id"] = terrain_index;
			terrain_tree_item->set_metadata(0, metadata_dict);
		}
	}
}

void RTileMapEditorTerrainsPlugin::_update_tiles_list() {
	terrains_tile_list->clear();

	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	TreeItem *selected_tree_item = terrains_tree->get_selected();
	if (selected_tree_item && selected_tree_item->get_metadata(0)) {
		Dictionary metadata_dict = selected_tree_item->get_metadata(0);
		int selected_terrain_set = metadata_dict["terrain_set"];
		int selected_terrain_id = metadata_dict["terrain_id"];
		ERR_FAIL_INDEX(selected_terrain_set, tile_set->get_terrain_sets_count());
		ERR_FAIL_INDEX(selected_terrain_id, tile_set->get_terrains_count(selected_terrain_set));

		// Sort the items in a map by the number of corresponding terrains.
		Map<int, Set<RTileSet::TerrainsPattern>> sorted;

		for (Set<RTileSet::TerrainsPattern>::Element *E = per_terrain_terrains_patterns[selected_terrain_set][selected_terrain_id].front(); E; E = E->next()) {
			// Count the number of matching sides/terrains.
			int count = 0;

			for (int i = 0; i < RTileSet::CELL_NEIGHBOR_MAX; i++) {
				RTileSet::CellNeighbor bit = RTileSet::CellNeighbor(i);
				if (tile_set->is_valid_peering_bit_terrain(selected_terrain_set, bit) && E->get().get_terrain(bit) == selected_terrain_id) {
					count++;
				}
			}
			sorted[count].insert(E->get());
		}

		for (Map<int, Set<RTileSet::TerrainsPattern>>::Element *E_set = sorted.back(); E_set; E_set = E_set->prev()) {
			for (Set<RTileSet::TerrainsPattern>::Element *E = E_set->get().front(); E; E = E->next()) {
				RTileSet::TerrainsPattern terrains_pattern = E->get();

				// Get the icon.
				Ref<Texture> icon;
				Rect2 region;
				bool transpose = false;

				double max_probability = -1.0;
				for (Set<RTileMapCell>::Element *cell = tile_set->get_tiles_for_terrains_pattern(selected_terrain_set, terrains_pattern).front(); cell; cell = cell->next()) {
					Ref<RTileSetSource> source = tile_set->get_source(cell->get().source_id);

					Ref<RTileSetAtlasSource> atlas_source = source;
					if (atlas_source.is_valid()) {
						RTileData *tile_data = Object::cast_to<RTileData>(atlas_source->get_tile_data(cell->get().get_atlas_coords(), cell->get().alternative_tile));
						if (tile_data->get_probability() > max_probability) {
							icon = atlas_source->get_texture();
							region = atlas_source->get_tile_texture_region(cell->get().get_atlas_coords());
							if (tile_data->get_flip_h()) {
								region.position.x += region.size.x;
								region.size.x = -region.size.x;
							}
							if (tile_data->get_flip_v()) {
								region.position.y += region.size.y;
								region.size.y = -region.size.y;
							}
							transpose = tile_data->get_transpose();
							max_probability = tile_data->get_probability();
						}
					}
				}

				// Create the ItemList's item.
				terrains_tile_list->add_item("");
				int item_index = terrains_tile_list->get_item_count() - 1;
				terrains_tile_list->set_item_icon(item_index, icon);
				terrains_tile_list->set_item_icon_region(item_index, region);
				terrains_tile_list->set_item_icon_transposed(item_index, transpose);
				Dictionary list_metadata_dict;
				list_metadata_dict["terrains_pattern"] = terrains_pattern.get_terrains_as_array();
				terrains_tile_list->set_item_metadata(item_index, list_metadata_dict);
			}
		}
		if (terrains_tile_list->get_item_count() > 0) {
			terrains_tile_list->select(0);
		}
	}
}

void RTileMapEditorTerrainsPlugin::_update_theme() {
	paint_tool_button->set_icon(main_vbox_container->get_icon(("Edit"), ("EditorIcons")));
	line_tool_button->set_icon(main_vbox_container->get_icon(("CurveLinear"), ("EditorIcons")));
	rect_tool_button->set_icon(main_vbox_container->get_icon(("Rectangle"), ("EditorIcons")));
	bucket_tool_button->set_icon(main_vbox_container->get_icon(("Bucket"), ("EditorIcons")));

	picker_button->set_icon(main_vbox_container->get_icon(("ColorPick"), ("EditorIcons")));
	erase_button->set_icon(main_vbox_container->get_icon(("Eraser"), ("EditorIcons")));
}

void RTileMapEditorTerrainsPlugin::edit(ObjectID p_tile_map_id, int p_tile_map_layer) {
	_stop_dragging(); // Avoids staying in a wrong drag state.

	tile_map_id = p_tile_map_id;
	tile_map_layer = p_tile_map_layer;

	_update_terrains_cache();
	_update_terrains_tree();
	_update_tiles_list();
}

RTileMapEditorTerrainsPlugin::RTileMapEditorTerrainsPlugin() {
	main_vbox_container = memnew(VBoxContainer);
	main_vbox_container->connect("tree_entered", this, "_update_theme");
	//main_vbox_container->connect("theme_changed", this, "_update_theme");
	main_vbox_container->set_name("Terrains");

	HSplitContainer *tilemap_tab_terrains = memnew(HSplitContainer);
	tilemap_tab_terrains->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tilemap_tab_terrains->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_vbox_container->add_child(tilemap_tab_terrains);

	terrains_tree = memnew(Tree);
	terrains_tree->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	terrains_tree->set_stretch_ratio(0.25);
	terrains_tree->set_custom_minimum_size(Size2i(70, 0) * EDSCALE);
	//terrains_tree->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	terrains_tree->set_hide_root(true);
	terrains_tree->connect("item_selected", this, "_update_tiles_list");
	tilemap_tab_terrains->add_child(terrains_tree);

	terrains_tile_list = memnew(ItemList);
	terrains_tile_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	terrains_tile_list->set_max_columns(0);
	terrains_tile_list->set_same_column_width(true);
	terrains_tile_list->set_fixed_icon_size(Size2(30, 30) * EDSCALE);
	//terrains_tile_list->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	tilemap_tab_terrains->add_child(terrains_tile_list);

	// --- Toolbar ---
	toolbar = memnew(HBoxContainer);

	HBoxContainer *tilemap_tiles_tools_buttons = memnew(HBoxContainer);

	tool_buttons_group.instance();

	paint_tool_button = memnew(Button);
	paint_tool_button->set_flat(true);
	paint_tool_button->set_toggle_mode(true);
	paint_tool_button->set_button_group(tool_buttons_group);
	paint_tool_button->set_pressed(true);
	paint_tool_button->set_shortcut(ED_SHORTCUT("tiles_editor/paint_tool", "Paint", KEY_D));
	paint_tool_button->connect("pressed", this, "_update_toolbar");
	tilemap_tiles_tools_buttons->add_child(paint_tool_button);

	line_tool_button = memnew(Button);
	line_tool_button->set_flat(true);
	line_tool_button->set_toggle_mode(true);
	line_tool_button->set_button_group(tool_buttons_group);
	line_tool_button->set_shortcut(ED_SHORTCUT("tiles_editor/line_tool", "Line", KEY_L));
	line_tool_button->connect("pressed", this, "_update_toolbar");
	tilemap_tiles_tools_buttons->add_child(line_tool_button);

	rect_tool_button = memnew(Button);
	rect_tool_button->set_flat(true);
	rect_tool_button->set_toggle_mode(true);
	rect_tool_button->set_button_group(tool_buttons_group);
	rect_tool_button->set_shortcut(ED_SHORTCUT("tiles_editor/rect_tool", "Rect", KEY_R));
	rect_tool_button->connect("pressed", this, "_update_toolbar");
	tilemap_tiles_tools_buttons->add_child(rect_tool_button);

	bucket_tool_button = memnew(Button);
	bucket_tool_button->set_flat(true);
	bucket_tool_button->set_toggle_mode(true);
	bucket_tool_button->set_button_group(tool_buttons_group);
	bucket_tool_button->set_shortcut(ED_SHORTCUT("tiles_editor/bucket_tool", "Bucket", KEY_B));
	bucket_tool_button->connect("pressed", this, "_update_toolbar");
	tilemap_tiles_tools_buttons->add_child(bucket_tool_button);

	toolbar->add_child(tilemap_tiles_tools_buttons);

	// -- TileMap tool settings --
	tools_settings = memnew(HBoxContainer);
	toolbar->add_child(tools_settings);

	tools_settings_vsep = memnew(VSeparator);
	tools_settings->add_child(tools_settings_vsep);

	// Picker
	picker_button = memnew(Button);
	picker_button->set_flat(true);
	picker_button->set_toggle_mode(true);
	picker_button->set_shortcut(ED_SHORTCUT("tiles_editor/picker", "Picker", KEY_P));
	picker_button->connect("pressed", CanvasItemEditor::get_singleton(), "update_viewport");
	tools_settings->add_child(picker_button);

	// Erase button.
	erase_button = memnew(Button);
	erase_button->set_flat(true);
	erase_button->set_toggle_mode(true);
	erase_button->set_shortcut(ED_SHORTCUT("tiles_editor/eraser", "Eraser", KEY_E));
	erase_button->connect("pressed", CanvasItemEditor::get_singleton(), "update_viewport");
	tools_settings->add_child(erase_button);

	// Separator 2.
	tools_settings_vsep_2 = memnew(VSeparator);
	tools_settings->add_child(tools_settings_vsep_2);

	// Continuous checkbox.
	bucket_contiguous_checkbox = memnew(CheckBox);
	bucket_contiguous_checkbox->set_flat(true);
	bucket_contiguous_checkbox->set_text(TTR("Contiguous"));
	bucket_contiguous_checkbox->set_pressed(true);
	tools_settings->add_child(bucket_contiguous_checkbox);
}

RTileMapEditorTerrainsPlugin::~RTileMapEditorTerrainsPlugin() {
}

void RTileMapEditorTerrainsPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_update_tiles_list"), &RTileMapEditorTerrainsPlugin::_update_tiles_list);
	ClassDB::bind_method(D_METHOD("_update_theme"), &RTileMapEditorTerrainsPlugin::_update_theme);
}

void RTileMapEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_THEME_CHANGED:
			missing_tile_texture = get_icon(("StatusWarning"), ("EditorIcons"));
			warning_pattern_texture = get_icon(("WarningPattern"), ("EditorIcons"));
			advanced_menu_button->set_icon(get_icon(("Tools"), ("EditorIcons")));
			toggle_grid_button->set_icon(get_icon(("Grid"), ("EditorIcons")));
			toggle_grid_button->set_pressed(EditorSettings::get_singleton()->get("editors/tiles_editor/display_grid"));
			toogle_highlight_selected_layer_button->set_icon(get_icon(("TileMapHighlightSelected"), ("EditorIcons")));
			break;
		case NOTIFICATION_INTERNAL_PROCESS:
			if (is_visible_in_tree() && tileset_changed_needs_update) {
				_update_bottom_panel();
				_update_layers_selection();
				tabs_plugins[tabs_bar->get_current_tab()]->tile_set_changed();
				CanvasItemEditor::get_singleton()->update_viewport();
				tileset_changed_needs_update = false;
			}
			break;
		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED:
			toggle_grid_button->set_pressed(EditorSettings::get_singleton()->get("editors/tiles_editor/display_grid"));
			break;
		case NOTIFICATION_VISIBILITY_CHANGED:
			RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
			if (tile_map) {
				if (is_visible_in_tree()) {
					tile_map->set_selected_layer(tile_map_layer);
				} else {
					tile_map->set_selected_layer(-1);
				}
			}
			break;
	}
}

void RTileMapEditor::_on_grid_toggled(bool p_pressed) {
	EditorSettings::get_singleton()->set("editors/tiles_editor/display_grid", p_pressed);
}

void RTileMapEditor::_layers_selection_button_draw() {
	if (!has_icon(("arrow"), ("OptionButton"))) {
		return;
	}

	RID ci = layers_selection_button->get_canvas_item();
	Ref<Texture> arrow = Control::get_icon(("arrow"), ("OptionButton"));

	Color clr = Color(1, 1, 1);
	if (get_constant(("modulate_arrow"))) {
		switch (layers_selection_button->get_draw_mode()) {
			case BaseButton::DRAW_PRESSED:
				clr = get_color(("font_pressed_color"));
				break;
			case BaseButton::DRAW_HOVER:
				clr = get_color(("font_hover_color"));
				break;
			case BaseButton::DRAW_DISABLED:
				clr = get_color(("font_disabled_color"));
				break;
			default:
				if (layers_selection_button->has_focus()) {
					clr = get_color(("font_focus_color"));
				} else {
					clr = get_color(("font_color"));
				}
		}
	}

	Size2 size = layers_selection_button->get_size();

	Point2 ofs;
	//if (is_layout_rtl()) {
	//	ofs = Point2(get_theme_constant(("arrow_margin"), ("OptionButton")), int(Math::abs((size.height - arrow->get_height()) / 2)));
	//} else {
		ofs = Point2(size.width - arrow->get_width() - get_constant(("arrow_margin"), ("OptionButton")), int(Math::abs((size.height - arrow->get_height()) / 2)));
	//}
	Rect2 dst_rect = Rect2(ofs, arrow->get_size());
	if (!layers_selection_button->is_pressed()) {
		dst_rect.size = -dst_rect.size;
	}
	arrow->draw_rect(ci, dst_rect, false, clr);
}

void RTileMapEditor::_layers_selection_button_pressed() {
	if (!layers_selection_popup->is_visible()) {
		Size2 size = layers_selection_popup->get_combined_minimum_size();
		size.x = MAX(size.x, layers_selection_button->get_size().x);
		layers_selection_popup->set_position(layers_selection_button->get_global_position() - Size2(0, size.y * get_global_transform().get_scale().y));
		layers_selection_popup->set_size(size);
		layers_selection_popup->popup();
	} else {
		layers_selection_popup->hide();
	}
}

void RTileMapEditor::_layers_selection_id_pressed(int p_id) {
	tile_map_layer = p_id;
	_update_layers_selection();
}

void RTileMapEditor::_advanced_menu_button_id_pressed(int p_id) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	if (p_id == 0) { // Replace Tile Proxies
		undo_redo->create_action(TTR("Replace Tiles with Proxies"));
		for (int layer_index = 0; layer_index < tile_map->get_layers_count(); layer_index++) {
			Vector<Vector2> used_cells = tile_map->get_used_cells(layer_index);
			for (int i = 0; i < used_cells.size(); i++) {
				Vector2i cell_coords = used_cells[i];
				RTileMapCell from = tile_map->get_cell(layer_index, cell_coords);
				Array to_array = tile_set->map_tile_proxy(from.source_id, from.get_atlas_coords(), from.alternative_tile);
				RTileMapCell to;
				to.source_id = to_array[0];
				to.set_atlas_coords(to_array[1]);
				to.alternative_tile = to_array[2];
				if (from != to) {
					undo_redo->add_do_method(tile_map, "set_cell", tile_map_layer, Vector2(cell_coords), to.source_id, to.get_atlas_coords(), to.alternative_tile);
					undo_redo->add_undo_method(tile_map, "set_cell", tile_map_layer, Vector2(cell_coords), from.source_id, from.get_atlas_coords(), from.alternative_tile);
				}
			}
		}
		undo_redo->commit_action();
	}
}

void RTileMapEditor::_update_bottom_panel() {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}
	Ref<RTileSet> tile_set = tile_map->get_tileset();

	// Update the visibility of controls.
	missing_tileset_label->set_visible(!tile_set.is_valid());
	for (unsigned int tab_index = 0; tab_index < tabs_data.size(); tab_index++) {
		tabs_data[tab_index].panel->hide();
	}
	if (tile_set.is_valid()) {
		tabs_data[tabs_bar->get_current_tab()].panel->show();
	}
}

Vector<Vector2i> RTileMapEditor::get_line(RTileMap *p_tile_map, Vector2i p_from_cell, Vector2i p_to_cell) {
	ERR_FAIL_COND_V(!p_tile_map, Vector<Vector2i>());

	Ref<RTileSet> tile_set = p_tile_map->get_tileset();
	ERR_FAIL_COND_V(!tile_set.is_valid(), Vector<Vector2i>());

	if (tile_set->get_tile_shape() == RTileSet::TILE_SHAPE_SQUARE) {
		return Geometry2D::bresenham_line(p_from_cell, p_to_cell);
	} else {
		// Adapt the bresenham line algorithm to half-offset shapes.
		// See this blog post: http://zvold.blogspot.com/2010/01/bresenhams-line-drawing-algorithm-on_26.html
		Vector<Point2i> points;

		bool transposed = tile_set->get_tile_offset_axis() == RTileSet::TILE_OFFSET_AXIS_VERTICAL;
		p_from_cell = RTileMap::transform_coords_layout(p_from_cell, tile_set->get_tile_offset_axis(), tile_set->get_tile_layout(), RTileSet::TILE_LAYOUT_STACKED);
		p_to_cell = RTileMap::transform_coords_layout(p_to_cell, tile_set->get_tile_offset_axis(), tile_set->get_tile_layout(), RTileSet::TILE_LAYOUT_STACKED);
		if (transposed) {
			SWAP(p_from_cell.x, p_from_cell.y);
			SWAP(p_to_cell.x, p_to_cell.y);
		}

		Vector2i delta = p_to_cell - p_from_cell;
		delta = Vector2i(2 * delta.x + ABS(p_to_cell.y % 2) - ABS(p_from_cell.y % 2), delta.y);
		Vector2i sign = MathExt::vector2i_sign(delta);

		Vector2i current = p_from_cell;
		points.push_back(RTileMap::transform_coords_layout(transposed ? Vector2i(current.y, current.x) : current, tile_set->get_tile_offset_axis(), RTileSet::TILE_LAYOUT_STACKED, tile_set->get_tile_layout()));

		int err = 0;
		if (ABS(delta.y) < ABS(delta.x)) {
			Vector2i err_step = 3 * MathExt::vector2i_abs(delta);
			while (current != p_to_cell) {
				err += err_step.y;
				if (err > ABS(delta.x)) {
					if (sign.x == 0) {
						current += Vector2(sign.y, 0);
					} else {
						current += Vector2(bool(current.y % 2) ^ (sign.x < 0) ? sign.x : 0, sign.y);
					}
					err -= err_step.x;
				} else {
					current += Vector2i(sign.x, 0);
					err += err_step.y;
				}
				points.push_back(RTileMap::transform_coords_layout(transposed ? Vector2i(current.y, current.x) : current, tile_set->get_tile_offset_axis(), RTileSet::TILE_LAYOUT_STACKED, tile_set->get_tile_layout()));
			}
		} else {
			Vector2i err_step = MathExt::vector2i_abs(delta);
			while (current != p_to_cell) {
				err += err_step.x;
				if (err > 0) {
					if (sign.x == 0) {
						current += Vector2(0, sign.y);
					} else {
						current += Vector2(bool(current.y % 2) ^ (sign.x < 0) ? sign.x : 0, sign.y);
					}
					err -= err_step.y;
				} else {
					if (sign.x == 0) {
						current += Vector2(0, sign.y);
					} else {
						current += Vector2(bool(current.y % 2) ^ (sign.x > 0) ? -sign.x : 0, sign.y);
					}
					err += err_step.y;
				}
				points.push_back(RTileMap::transform_coords_layout(transposed ? Vector2i(current.y, current.x) : current, tile_set->get_tile_offset_axis(), RTileSet::TILE_LAYOUT_STACKED, tile_set->get_tile_layout()));
			}
		}

		return points;
	}
}

void RTileMapEditor::_tile_map_changed() {
	tileset_changed_needs_update = true;
}

void RTileMapEditor::_tab_changed(int p_tab_id) {
	// Make the plugin edit the correct tilemap.
	tabs_plugins[tabs_bar->get_current_tab()]->edit(tile_map_id, tile_map_layer);

	// Update toolbar.
	for (unsigned int tab_index = 0; tab_index < tabs_data.size(); tab_index++) {
		tabs_data[tab_index].toolbar->hide();
	}
	tabs_data[p_tab_id].toolbar->show();

	// Update visible panel.
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	for (unsigned int tab_index = 0; tab_index < tabs_data.size(); tab_index++) {
		tabs_data[tab_index].panel->hide();
	}
	if (tile_map && tile_map->get_tileset().is_valid()) {
		tabs_data[tabs_bar->get_current_tab()].panel->show();
	}

	// Graphical update.
	tabs_data[tabs_bar->get_current_tab()].panel->update();
	CanvasItemEditor::get_singleton()->update_viewport();
}

void RTileMapEditor::_layers_select_next_or_previous(bool p_next) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	if (tile_map->get_layers_count() < 1) {
		return;
	}

	if (tile_map_layer < 0) {
		tile_map_layer = 0;
	}

	int inc = p_next ? 1 : -1;
	int origin_layer = tile_map_layer;
	tile_map_layer = Math::posmod((tile_map_layer + inc), tile_map->get_layers_count());
	while (tile_map_layer != origin_layer) {
		if (tile_map->is_layer_enabled(tile_map_layer)) {
			break;
		}
		tile_map_layer = Math::posmod((tile_map_layer + inc), tile_map->get_layers_count());
	}

	_update_layers_selection();
}

void RTileMapEditor::_update_layers_selection() {
	layers_selection_popup->clear();

	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	// Update the selected layer.
	if (is_visible_in_tree() && tile_map->get_layers_count() >= 1) {
		tile_map_layer = CLAMP(tile_map_layer, 0, tile_map->get_layers_count() - 1);

		// Search for an enabled layer if the current one is not.
		int origin_layer = tile_map_layer;
		while (tile_map_layer >= 0 && !tile_map->is_layer_enabled(tile_map_layer)) {
			tile_map_layer--;
		}
		if (tile_map_layer < 0) {
			tile_map_layer = origin_layer;
			while (tile_map_layer < tile_map->get_layers_count() && !tile_map->is_layer_enabled(tile_map_layer)) {
				tile_map_layer++;
			}
		}
		if (tile_map_layer >= tile_map->get_layers_count()) {
			tile_map_layer = -1;
		}
	} else {
		tile_map_layer = -1;
	}
	tile_map->set_selected_layer(toogle_highlight_selected_layer_button->is_pressed() ? tile_map_layer : -1);

	// Build the list of layers.
	for (int i = 0; i < tile_map->get_layers_count(); i++) {
		String name = tile_map->get_layer_name(i);
		layers_selection_popup->add_item(name.empty() ? vformat(TTR("Layer #%d"), i) : name, i);
		layers_selection_popup->set_item_as_radio_checkable(i, true);
		layers_selection_popup->set_item_disabled(i, !tile_map->is_layer_enabled(i));
		layers_selection_popup->set_item_checked(i, i == tile_map_layer);
	}

	// Update the button label.
	if (tile_map_layer >= 0) {
		layers_selection_button->set_text(layers_selection_popup->get_item_text(tile_map_layer));
	} else {
		layers_selection_button->set_text(TTR("Select a layer"));
	}

	// Set button minimum width.
	Size2 min_button_size = Size2(layers_selection_popup->get_combined_minimum_size().x, 0);
	if (has_icon(("arrow"), ("OptionButton"))) {
		Ref<Texture> arrow = Control::get_icon(("arrow"), ("OptionButton"));
		min_button_size.x += arrow->get_size().x;
	}
	layers_selection_button->set_custom_minimum_size(min_button_size);
	layers_selection_button->update();

	tabs_plugins[tabs_bar->get_current_tab()]->edit(tile_map_id, tile_map_layer);
}

void RTileMapEditor::_move_tile_map_array_element(Object *p_undo_redo, Object *p_edited, String p_array_prefix, int p_from_index, int p_to_pos) {
	UndoRedo *undo_redo = Object::cast_to<UndoRedo>(p_undo_redo);
	ERR_FAIL_COND(!undo_redo);

	RTileMap *tile_map = Object::cast_to<RTileMap>(p_edited);
	if (!tile_map) {
		return;
	}

	// Compute the array indices to save.
	int begin = 0;
	int end;
	if (p_array_prefix == "layer_") {
		end = tile_map->get_layers_count();
	} else {
		ERR_FAIL_MSG("Invalid array prefix for RTileSet.");
	}
	if (p_from_index < 0) {
		// Adding new.
		if (p_to_pos >= 0) {
			begin = p_to_pos;
		} else {
			end = 0; // Nothing to save when adding at the end.
		}
	} else if (p_to_pos < 0) {
		// Removing.
		begin = p_from_index;
	} else {
		// Moving.
		begin = MIN(p_from_index, p_to_pos);
		end = MIN(MAX(p_from_index, p_to_pos) + 1, end);
	}

#define ADD_UNDO(obj, property) undo_redo->add_undo_property(obj, property, obj->get(property));
	// Save layers' properties.
	if (p_from_index < 0) {
		undo_redo->add_undo_method(tile_map, "remove_layer", p_to_pos < 0 ? tile_map->get_layers_count() : p_to_pos);
	} else if (p_to_pos < 0) {
		undo_redo->add_undo_method(tile_map, "add_layer", p_from_index);
	}

	List<PropertyInfo> properties;
	tile_map->get_property_list(&properties);

	for (List<PropertyInfo>::Element *pi = properties.front(); pi; pi = pi->next()) {
		if (pi->get().name.begins_with(p_array_prefix)) {
			String str = pi->get().name.trim_prefix(p_array_prefix);
			int to_char_index = 0;
			while (to_char_index < str.length()) {
				if (str[to_char_index] < '0' || str[to_char_index] > '9') {
					break;
				}
				to_char_index++;
			}
			if (to_char_index > 0) {
				int array_index = str.left(to_char_index).to_int();
				if (array_index >= begin && array_index < end) {
					ADD_UNDO(tile_map, pi->get().name);
				}
			}
		}
	}
#undef ADD_UNDO

	if (p_from_index < 0) {
		undo_redo->add_do_method(tile_map, "add_layer", p_to_pos);
	} else if (p_to_pos < 0) {
		undo_redo->add_do_method(tile_map, "remove_layer", p_from_index);
	} else {
		undo_redo->add_do_method(tile_map, "move_layer", p_from_index, p_to_pos);
	}
}

bool RTileMapEditor::forward_canvas_gui_input(const Ref<InputEvent> &p_event) {
	if (ED_IS_SHORTCUT("tiles_editor/select_next_layer", p_event) && p_event->is_pressed()) {
		_layers_select_next_or_previous(true);
		return true;
	}

	if (ED_IS_SHORTCUT("tiles_editor/select_previous_layer", p_event) && p_event->is_pressed()) {
		_layers_select_next_or_previous(false);
		return true;
	}

	return tabs_plugins[tabs_bar->get_current_tab()]->forward_canvas_gui_input(p_event);
}

void RTileMapEditor::forward_canvas_draw_over_viewport(Control *p_overlay) {
	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (!tile_map) {
		return;
	}

	Ref<RTileSet> tile_set = tile_map->get_tileset();
	if (!tile_set.is_valid()) {
		return;
	}

	if (!tile_map->is_visible_in_tree()) {
		return;
	}

	Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * tile_map->get_global_transform();
	Transform2D xform_inv = xform.affine_inverse();
	Vector2i tile_shape_size = tile_set->get_tile_size();

	// Draw tiles with invalid IDs in the grid.
	if (tile_map_layer >= 0) {
		ERR_FAIL_COND(tile_map_layer >= tile_map->get_layers_count());
		Vector<Vector2> used_cells = tile_map->get_used_cells(tile_map_layer);
		for (int i = 0; i < used_cells.size(); i++) {
			Vector2i coords = used_cells[i];
			int tile_source_id = tile_map->get_cell_source_id(tile_map_layer, coords);
			if (tile_source_id >= 0) {
				Vector2i tile_atlas_coords = tile_map->get_cell_atlas_coords(tile_map_layer, coords);
				int tile_alternative_tile = tile_map->get_cell_alternative_tile(tile_map_layer, coords);

				RTileSetSource *source = nullptr;
				if (tile_set->has_source(tile_source_id)) {
					source = *tile_set->get_source(tile_source_id);
				}

				if (!source || !source->has_tile(tile_atlas_coords) || !source->has_alternative_tile(tile_atlas_coords, tile_alternative_tile)) {
					// Generate a random color from the hashed values of the tiles.
					Array a = tile_set->map_tile_proxy(tile_source_id, tile_atlas_coords, tile_alternative_tile);
					if (int(a[0]) == tile_source_id && Vector2i(a[1]) == tile_atlas_coords && int(a[2]) == tile_alternative_tile) {
						// Only display the pattern if we have no proxy tile.
						Array to_hash;
						to_hash.push_back(tile_source_id);
						to_hash.push_back(Vector2(tile_atlas_coords));
						to_hash.push_back(tile_alternative_tile);
						uint32_t hash = RandomPCG(to_hash.hash()).rand();

						Color color;
						color = color.from_hsv(
								(float)((hash >> 24) & 0xFF) / 256.0,
								Math::lerp(0.5, 1.0, (float)((hash >> 16) & 0xFF) / 256.0),
								Math::lerp(0.5, 1.0, (float)((hash >> 8) & 0xFF) / 256.0),
								0.8);

						// Draw the scaled tile.
						Transform2D tile_xform;
						tile_xform.set_origin(tile_map->map_to_world(coords));
						tile_xform.set_scale(tile_shape_size);
						tile_set->draw_tile_shape(p_overlay, xform * tile_xform, color, true, warning_pattern_texture);
					}

					// Draw the warning icon.
					int min_axis = missing_tile_texture->get_size().min_axis();
					Vector2 icon_size;
					icon_size[min_axis] = tile_set->get_tile_size()[min_axis] / 3;
					icon_size[(min_axis + 1) % 2] = (icon_size[min_axis] * missing_tile_texture->get_size()[(min_axis + 1) % 2] / missing_tile_texture->get_size()[min_axis]);
					Rect2 rect = Rect2(xform.xform(tile_map->map_to_world(coords)) - (icon_size * xform.get_scale() / 2), icon_size * xform.get_scale());
					p_overlay->draw_texture_rect(missing_tile_texture, rect);
				}
			}
		}
	}

	// Fading on the border.
	const int fading = 5;

	// Determine the drawn area.
	Size2 screen_size = p_overlay->get_size();
	Rect2i screen_rect;
	screen_rect.position = tile_map->world_to_map(xform_inv.xform(Vector2()));
	screen_rect.expand_to(tile_map->world_to_map(xform_inv.xform(Vector2(0, screen_size.height))));
	screen_rect.expand_to(tile_map->world_to_map(xform_inv.xform(Vector2(screen_size.width, 0))));
	screen_rect.expand_to(tile_map->world_to_map(xform_inv.xform(screen_size)));
	screen_rect = screen_rect.grow(1);

	Rect2i tilemap_used_rect = tile_map->get_used_rect();

	//Rect2i displayed_rect = tilemap_used_rect.intersection(screen_rect);
	Rect2i displayed_rect = MathExt::rect2i_intersection(tilemap_used_rect, screen_rect);
	displayed_rect = displayed_rect.grow(fading);

	// Reduce the drawn area to avoid crashes if needed.
	int max_size = 100;
	if (displayed_rect.size.x > max_size) {
		displayed_rect = displayed_rect.grow_individual(-(displayed_rect.size.x - max_size) / 2, 0, -(displayed_rect.size.x - max_size) / 2, 0);
	}
	if (displayed_rect.size.y > max_size) {
		displayed_rect = displayed_rect.grow_individual(0, -(displayed_rect.size.y - max_size) / 2, 0, -(displayed_rect.size.y - max_size) / 2);
	}

	// Draw the grid.
	bool display_grid = EditorSettings::get_singleton()->get("editors/tiles_editor/display_grid");
	if (display_grid) {
		Color grid_color = EditorSettings::get_singleton()->get("editors/tiles_editor/grid_color");
		for (int x = displayed_rect.position.x; x < (displayed_rect.position.x + displayed_rect.size.x); x++) {
			for (int y = displayed_rect.position.y; y < (displayed_rect.position.y + displayed_rect.size.y); y++) {
				Vector2i pos_in_rect = Vector2i(x, y) - displayed_rect.position;

				// Fade out the border of the grid.
				float left_opacity = CLAMP(Math::inverse_lerp(0.0f, (float)fading, (float)pos_in_rect.x), 0.0f, 1.0f);
				float right_opacity = CLAMP(Math::inverse_lerp((float)displayed_rect.size.x, (float)(displayed_rect.size.x - fading), (float)pos_in_rect.x), 0.0f, 1.0f);
				float top_opacity = CLAMP(Math::inverse_lerp(0.0f, (float)fading, (float)pos_in_rect.y), 0.0f, 1.0f);
				float bottom_opacity = CLAMP(Math::inverse_lerp((float)displayed_rect.size.y, (float)(displayed_rect.size.y - fading), (float)pos_in_rect.y), 0.0f, 1.0f);
				float opacity = CLAMP(MIN(left_opacity, MIN(right_opacity, MIN(top_opacity, bottom_opacity))) + 0.1, 0.0f, 1.0f);

				Transform2D tile_xform;
				tile_xform.set_origin(tile_map->map_to_world(Vector2(x, y)));
				tile_xform.set_scale(tile_shape_size);
				Color color = grid_color;
				color.a = color.a * opacity;
				tile_set->draw_tile_shape(p_overlay, xform * tile_xform, color, false);
			}
		}
	}

	// Draw the IDs for debug.
	/*Ref<Font> font = get_theme_font(("font"), ("Label"));
	for (int x = displayed_rect.position.x; x < (displayed_rect.position.x + displayed_rect.size.x); x++) {
		for (int y = displayed_rect.position.y; y < (displayed_rect.position.y + displayed_rect.size.y); y++) {
			p_overlay->draw_string(font, xform.xform(tile_map->map_to_world(Vector2(x, y))) + Vector2i(-tile_shape_size.x / 2, 0), vformat("%s", Vector2(x, y)));
		}
	}*/

	// Draw the plugins.
	tabs_plugins[tabs_bar->get_current_tab()]->forward_canvas_draw_over_viewport(p_overlay);
}

void RTileMapEditor::edit(RTileMap *p_tile_map) {
	if (p_tile_map && p_tile_map->get_instance_id() == tile_map_id) {
		return;
	}

	RTileMap *tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
	if (tile_map) {
		// Unselect layer if we are changing tile_map.
		if (tile_map != p_tile_map) {
			tile_map->set_selected_layer(-1);
		}

		// Disconnect to changes.
		tile_map->disconnect("changed", this, "_tile_map_changed");
	}

	if (p_tile_map) {
		// Change the edited object.
		tile_map_id = p_tile_map->get_instance_id();
		tile_map = Object::cast_to<RTileMap>(ObjectDB::get_instance(tile_map_id));
		// Connect to changes.
		if (!tile_map->is_connected("changed", this, "_tile_map_changed")) {
			tile_map->connect("changed", this, "_tile_map_changed");
		}
	} else {
		tile_map_id = ObjectID();
	}

	_update_layers_selection();

	// Call the plugins.
	tabs_plugins[tabs_bar->get_current_tab()]->edit(tile_map_id, tile_map_layer);

	_tile_map_changed();
}

void RTileMapEditor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_tile_map_changed"), &RTileMapEditor::_tile_map_changed);
	ClassDB::bind_method(D_METHOD("_tab_changed"), &RTileMapEditor::_tab_changed);

	ClassDB::bind_method(D_METHOD("_layers_selection_id_pressed"), &RTileMapEditor::_layers_selection_id_pressed);
	ClassDB::bind_method(D_METHOD("_layers_selection_button_draw"), &RTileMapEditor::_layers_selection_button_draw);
	ClassDB::bind_method(D_METHOD("_layers_selection_button_pressed"), &RTileMapEditor::_layers_selection_button_pressed);
	ClassDB::bind_method(D_METHOD("_update_layers_selection"), &RTileMapEditor::_update_layers_selection);
	ClassDB::bind_method(D_METHOD("_on_grid_toggled"), &RTileMapEditor::_on_grid_toggled);
	ClassDB::bind_method(D_METHOD("_advanced_menu_button_id_pressed"), &RTileMapEditor::_advanced_menu_button_id_pressed);
}

RTileMapEditor::RTileMapEditor() {
	set_process_internal(true);

	// Shortcuts.
	ED_SHORTCUT("tiles_editor/select_next_layer", TTR("Select Next Tile Map Layer"), KEY_PAGEUP);
	ED_SHORTCUT("tiles_editor/select_previous_layer", TTR("Select Previous Tile Map Layer"), KEY_PAGEDOWN);

	// TileMap editor plugins
	tile_map_editor_plugins.push_back(memnew(RTileMapEditorTilesPlugin));
	tile_map_editor_plugins.push_back(memnew(RTileMapEditorTerrainsPlugin));

	// TabBar.
	tabs_bar = memnew(Tabs);
	//tabs_bar->set_clip_tabs(false);
	for (int plugin_index = 0; plugin_index < tile_map_editor_plugins.size(); plugin_index++) {
		Vector<RTileMapEditorPlugin::TabData> tabs_vector = tile_map_editor_plugins[plugin_index]->get_tabs();
		for (int tab_index = 0; tab_index < tabs_vector.size(); tab_index++) {
			tabs_bar->add_tab(tabs_vector[tab_index].panel->get_name());
			tabs_data.push_back(tabs_vector[tab_index]);
			tabs_plugins.push_back(tile_map_editor_plugins[plugin_index]);
		}
	}
	tabs_bar->connect("tab_changed", this, "_tab_changed");

	// --- TileMap toolbar ---
	tile_map_toolbar = memnew(HBoxContainer);
	tile_map_toolbar->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(tile_map_toolbar);

	// Tabs.
	tile_map_toolbar->add_child(tabs_bar);

	// Tabs toolbars.
	for (unsigned int tab_index = 0; tab_index < tabs_data.size(); tab_index++) {
		tabs_data[tab_index].toolbar->hide();
		if (!tabs_data[tab_index].toolbar->get_parent()) {
			tile_map_toolbar->add_child(tabs_data[tab_index].toolbar);
		}
	}

	// Wide empty separation control.
	Control *h_empty_space = memnew(Control);
	h_empty_space->set_h_size_flags(SIZE_EXPAND_FILL);
	tile_map_toolbar->add_child(h_empty_space);

	// Layer selector.
	layers_selection_popup = memnew(PopupMenu);
	layers_selection_popup->connect("id_pressed", this, "_layers_selection_id_pressed");
	layers_selection_popup->set_hide_on_window_lose_focus(false);

	layers_selection_button = memnew(Button);
	layers_selection_button->set_toggle_mode(true);
	layers_selection_button->connect("draw", this, "_layers_selection_button_draw");
	layers_selection_button->connect("pressed", this, "_layers_selection_button_pressed");
	layers_selection_button->connect("hide", layers_selection_popup, "hide");
	layers_selection_button->set_tooltip(TTR("Tile Map Layer"));
	layers_selection_button->add_child(layers_selection_popup);
	tile_map_toolbar->add_child(layers_selection_button);

	toogle_highlight_selected_layer_button = memnew(Button);
	toogle_highlight_selected_layer_button->set_flat(true);
	toogle_highlight_selected_layer_button->set_toggle_mode(true);
	toogle_highlight_selected_layer_button->set_pressed(true);
	toogle_highlight_selected_layer_button->connect("pressed", this, "_update_layers_selection");
	toogle_highlight_selected_layer_button->set_tooltip(TTR("Highlight Selected TileMap Layer"));
	tile_map_toolbar->add_child(toogle_highlight_selected_layer_button);

	tile_map_toolbar->add_child(memnew(VSeparator));

	// Grid toggle.
	toggle_grid_button = memnew(Button);
	toggle_grid_button->set_flat(true);
	toggle_grid_button->set_toggle_mode(true);
	toggle_grid_button->set_tooltip(TTR("Toggle grid visibility."));
	toggle_grid_button->connect("toggled", this, "_on_grid_toggled");
	tile_map_toolbar->add_child(toggle_grid_button);

	// Advanced settings menu button.
	advanced_menu_button = memnew(MenuButton);
	advanced_menu_button->set_flat(true);
	advanced_menu_button->get_popup()->add_item(TTR("Automatically Replace Tiles with Proxies"));
	advanced_menu_button->get_popup()->connect("id_pressed", this, "_advanced_menu_button_id_pressed");
	tile_map_toolbar->add_child(advanced_menu_button);

	missing_tileset_label = memnew(Label);
	missing_tileset_label->set_text(TTR("The edited TileMap node has no RTileSet resource."));
	missing_tileset_label->set_h_size_flags(SIZE_EXPAND_FILL);
	missing_tileset_label->set_v_size_flags(SIZE_EXPAND_FILL);
	missing_tileset_label->set_align(Label::ALIGN_CENTER);
	missing_tileset_label->set_valign(Label::VALIGN_CENTER);
	missing_tileset_label->hide();
	add_child(missing_tileset_label);

	for (unsigned int tab_index = 0; tab_index < tabs_data.size(); tab_index++) {
		add_child(tabs_data[tab_index].panel);
		tabs_data[tab_index].panel->set_v_size_flags(SIZE_EXPAND_FILL);
		tabs_data[tab_index].panel->set_visible(tab_index == 0);
		tabs_data[tab_index].panel->set_h_size_flags(SIZE_EXPAND_FILL);
	}

	_tab_changed(0);

	// Registers UndoRedo inspector callback.
	//EditorNode::get_singleton()->get_editor_data().add_move_array_element_function(("TileMap"), callable_mp(this, &RTileMapEditor::_move_tile_map_array_element));
}

RTileMapEditor::~RTileMapEditor() {
	for (int i = 0; i < tile_map_editor_plugins.size(); i++) {
		memdelete(tile_map_editor_plugins[i]);
	}
}
