/*************************************************************************/
/*  tile_proxies_manager_dialog.cpp                                      */
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

#include "tile_proxies_manager_dialog.h"

#include "editor/editor_scale.h"

void RTileProxiesManagerDialog::_right_clicked(int p_item, Vector2 p_local_mouse_pos, Object *p_item_list) {
	ItemList *item_list = Object::cast_to<ItemList>(p_item_list);
	//popup_menu->reset_size();
	popup_menu->set_position(get_position() + item_list->get_global_mouse_position());
	popup_menu->popup();
}

void RTileProxiesManagerDialog::_menu_id_pressed(int p_id) {
	if (p_id == 0) {
		// Delete.
		_delete_selected_bindings();
	}
}

void RTileProxiesManagerDialog::_delete_selected_bindings() {
	undo_redo->create_action(TTR("Remove Tile Proxies"));

	Vector<int> source_level_selected = source_level_list->get_selected_items();
	for (int i = 0; i < source_level_selected.size(); i++) {
		int key = source_level_list->get_item_metadata(source_level_selected[i]);
		int val = tile_set->get_source_level_tile_proxy(key);
		undo_redo->add_do_method(*tile_set, "remove_source_level_tile_proxy", key);
		undo_redo->add_undo_method(*tile_set, "set_source_level_tile_proxy", key, val);
	}

	Vector<int> coords_level_selected = coords_level_list->get_selected_items();
	for (int i = 0; i < coords_level_selected.size(); i++) {
		Array key = coords_level_list->get_item_metadata(coords_level_selected[i]);
		Array val = tile_set->get_coords_level_tile_proxy(key[0], key[1]);
		undo_redo->add_do_method(*tile_set, "remove_coords_level_tile_proxy", key[0], key[1]);
		undo_redo->add_undo_method(*tile_set, "set_coords_level_tile_proxy", key[0], key[1], val[0], val[1]);
	}

	Vector<int> alternative_level_selected = alternative_level_list->get_selected_items();
	for (int i = 0; i < alternative_level_selected.size(); i++) {
		Array key = alternative_level_list->get_item_metadata(alternative_level_selected[i]);
		Array val = tile_set->get_coords_level_tile_proxy(key[0], key[1]);
		//TODO
		//undo_redo->add_do_method(*tile_set, "remove_alternative_level_tile_proxy", key[0], key[1], key[2]);
		tile_set->remove_alternative_level_tile_proxy(key[0], key[1], key[2]);
		//undo_redo->add_undo_method(*tile_set, "set_alternative_level_tile_proxy", key[0], key[1], key[2], val[0], val[1], val[2]);
	}
	undo_redo->add_do_method(this, "_update_lists");
	undo_redo->add_undo_method(this, "_update_lists");
	undo_redo->commit_action();

	commited_actions_count += 1;
}

void RTileProxiesManagerDialog::_update_lists() {
	source_level_list->clear();
	coords_level_list->clear();
	alternative_level_list->clear();

	Array proxies = tile_set->get_source_level_tile_proxies();
	for (int i = 0; i < proxies.size(); i++) {
		Array proxy = proxies[i];
		String text = vformat("%s", proxy[0]).rpad(5) + "-> " + vformat("%s", proxy[1]);
		source_level_list->add_item(text);
		int id = source_level_list->get_item_count() - 1;
		source_level_list->set_item_metadata(id, proxy[0]);
	}

	proxies = tile_set->get_coords_level_tile_proxies();
	for (int i = 0; i < proxies.size(); i++) {
		Array proxy = proxies[i];
		String text = vformat("%s, %s", proxy[0], proxy[1]).rpad(17) + "-> " + vformat("%s, %s", proxy[2], proxy[3]);
		coords_level_list->add_item(text);
		int id = coords_level_list->get_item_count() - 1;
		coords_level_list->set_item_metadata(id, proxy.slice(0, 2));
	}

	proxies = tile_set->get_alternative_level_tile_proxies();
	for (int i = 0; i < proxies.size(); i++) {
		Array proxy = proxies[i];
		String text = vformat("%s, %s, %s", proxy[0], proxy[1], proxy[2]).rpad(24) + "-> " + vformat("%s, %s, %s", proxy[3], proxy[4], proxy[5]);
		alternative_level_list->add_item(text);
		int id = alternative_level_list->get_item_count() - 1;
		alternative_level_list->set_item_metadata(id, proxy.slice(0, 3));
	}
}

void RTileProxiesManagerDialog::_update_enabled_property_editors() {
	if (from.source_id == RTileSet::INVALID_SOURCE) {
		from.set_atlas_coords(RTileSetSource::INVALID_ATLAS_COORDS);
		to.set_atlas_coords(RTileSetSource::INVALID_ATLAS_COORDS);
		from.alternative_tile = RTileSetSource::INVALID_TILE_ALTERNATIVE;
		to.alternative_tile = RTileSetSource::INVALID_TILE_ALTERNATIVE;
		coords_from_property_editor->hide();
		coords_to_property_editor->hide();
		alternative_from_property_editor->hide();
		alternative_to_property_editor->hide();
	} else if (from.get_atlas_coords().x == -1 || from.get_atlas_coords().y == -1) {
		from.alternative_tile = RTileSetSource::INVALID_TILE_ALTERNATIVE;
		to.alternative_tile = RTileSetSource::INVALID_TILE_ALTERNATIVE;
		coords_from_property_editor->show();
		coords_to_property_editor->show();
		alternative_from_property_editor->hide();
		alternative_to_property_editor->hide();
	} else {
		coords_from_property_editor->show();
		coords_to_property_editor->show();
		alternative_from_property_editor->show();
		alternative_to_property_editor->show();
	}

	source_from_property_editor->update_property();
	source_to_property_editor->update_property();
	coords_from_property_editor->update_property();
	coords_to_property_editor->update_property();
	alternative_from_property_editor->update_property();
	alternative_to_property_editor->update_property();
}

void RTileProxiesManagerDialog::_property_changed(const String &p_path, const Variant &p_value, const String &p_name, bool p_changing) {
	_set(p_path, p_value);
}

void RTileProxiesManagerDialog::_add_button_pressed() {
	if (from.source_id != RTileSet::INVALID_SOURCE && to.source_id != RTileSet::INVALID_SOURCE) {
		Vector2i from_coords = from.get_atlas_coords();
		Vector2i to_coords = to.get_atlas_coords();
		if (from_coords.x >= 0 && from_coords.y >= 0 && to_coords.x >= 0 && to_coords.y >= 0) {
			if (from.alternative_tile != RTileSetSource::INVALID_TILE_ALTERNATIVE && to.alternative_tile != RTileSetSource::INVALID_TILE_ALTERNATIVE) {
				undo_redo->create_action(TTR("Create Alternative-level Tile Proxy"));
				//TODO
				//undo_redo->add_do_method(*tile_set, "set_alternative_level_tile_proxy", from.source_id, from.get_atlas_coords(), from.alternative_tile, to.source_id, to.get_atlas_coords(), to.alternative_tile);
				if (tile_set->has_alternative_level_tile_proxy(from.source_id, from.get_atlas_coords(), from.alternative_tile)) {
					Array a = tile_set->get_alternative_level_tile_proxy(from.source_id, from.get_atlas_coords(), from.alternative_tile);
					//TODO
					//undo_redo->add_undo_method(*tile_set, "set_alternative_level_tile_proxy", to.source_id, to.get_atlas_coords(), to.alternative_tile, a[0], a[1], a[2]);
				} else {
					//TODO
					//undo_redo->add_undo_method(*tile_set, "remove_alternative_level_tile_proxy", from.source_id, from.get_atlas_coords(), from.alternative_tile);
				}
			} else {
				undo_redo->create_action(TTR("Create Coords-level Tile Proxy"));
				undo_redo->add_do_method(*tile_set, "set_coords_level_tile_proxy", from.source_id, from.get_atlas_coords(), to.source_id, to.get_atlas_coords());
				if (tile_set->has_coords_level_tile_proxy(from.source_id, from.get_atlas_coords())) {
					Array a = tile_set->get_coords_level_tile_proxy(from.source_id, from.get_atlas_coords());
					undo_redo->add_undo_method(*tile_set, "set_coords_level_tile_proxy", to.source_id, to.get_atlas_coords(), a[0], a[1]);
				} else {
					undo_redo->add_undo_method(*tile_set, "remove_coords_level_tile_proxy", from.source_id, from.get_atlas_coords());
				}
			}
		} else {
			undo_redo->create_action(TTR("Create source-level Tile Proxy"));
			undo_redo->add_do_method(*tile_set, "set_source_level_tile_proxy", from.source_id, to.source_id);
			if (tile_set->has_source_level_tile_proxy(from.source_id)) {
				undo_redo->add_undo_method(*tile_set, "set_source_level_tile_proxy", to.source_id, tile_set->get_source_level_tile_proxy(from.source_id));
			} else {
				undo_redo->add_undo_method(*tile_set, "remove_source_level_tile_proxy", from.source_id);
			}
		}
		undo_redo->add_do_method(this, "_update_lists");
		undo_redo->add_undo_method(this, "_update_lists");
		undo_redo->commit_action();
		commited_actions_count++;
	}
}

void RTileProxiesManagerDialog::_clear_invalid_button_pressed() {
	undo_redo->create_action(TTR("Delete All Invalid Tile Proxies"));

	undo_redo->add_do_method(*tile_set, "cleanup_invalid_tile_proxies");

	Array proxies = tile_set->get_source_level_tile_proxies();
	for (int i = 0; i < proxies.size(); i++) {
		Array proxy = proxies[i];
		undo_redo->add_undo_method(*tile_set, "set_source_level_tile_proxy", proxy[0], proxy[1]);
	}

	proxies = tile_set->get_coords_level_tile_proxies();
	for (int i = 0; i < proxies.size(); i++) {
		Array proxy = proxies[i];
		undo_redo->add_undo_method(*tile_set, "set_coords_level_tile_proxy", proxy[0], proxy[1], proxy[2], proxy[3]);
	}

	proxies = tile_set->get_alternative_level_tile_proxies();
	for (int i = 0; i < proxies.size(); i++) {
		Array proxy = proxies[i];
		//TODO
		//undo_redo->add_undo_method(*tile_set, "set_alternative_level_tile_proxy", proxy[0], proxy[1], proxy[2], proxy[3], proxy[4], proxy[5]);
	}
	undo_redo->add_do_method(this, "_update_lists");
	undo_redo->add_undo_method(this, "_update_lists");
	undo_redo->commit_action();
}

void RTileProxiesManagerDialog::_clear_all_button_pressed() {
	undo_redo->create_action(TTR("Delete All Tile Proxies"));

	undo_redo->add_do_method(*tile_set, "clear_tile_proxies");

	Array proxies = tile_set->get_source_level_tile_proxies();
	for (int i = 0; i < proxies.size(); i++) {
		Array proxy = proxies[i];
		undo_redo->add_undo_method(*tile_set, "set_source_level_tile_proxy", proxy[0], proxy[1]);
	}

	proxies = tile_set->get_coords_level_tile_proxies();
	for (int i = 0; i < proxies.size(); i++) {
		Array proxy = proxies[i];
		undo_redo->add_undo_method(*tile_set, "set_coords_level_tile_proxy", proxy[0], proxy[1], proxy[2], proxy[3]);
	}

	proxies = tile_set->get_alternative_level_tile_proxies();
	for (int i = 0; i < proxies.size(); i++) {
		Array proxy = proxies[i];
		//TODO
		//undo_redo->add_undo_method(*tile_set, "set_alternative_level_tile_proxy", proxy[0], proxy[1], proxy[2], proxy[3], proxy[4], proxy[5]);
	}
	undo_redo->add_do_method(this, "_update_lists");
	undo_redo->add_undo_method(this, "_update_lists");
	undo_redo->commit_action();
}

bool RTileProxiesManagerDialog::_set(const StringName &p_name, const Variant &p_value) {
	if (p_name == "from_source") {
		from.source_id = MAX(int(p_value), -1);
	} else if (p_name == "from_coords") {
		Vector2i v = Vector2i(Vector2(p_value));

		from.set_atlas_coords(Vector2i(MAX(v.x, -1), MAX(v.y, -1)));
	} else if (p_name == "from_alternative") {
		from.alternative_tile = MAX(int(p_value), -1);
	} else if (p_name == "to_source") {
		to.source_id = MAX(int(p_value), 0);
	} else if (p_name == "to_coords") {
		Vector2i v = Vector2i(Vector2(p_value));

		to.set_atlas_coords(Vector2i(MAX(v.x, 0), MAX(v.y, 0)));
	} else if (p_name == "to_alternative") {
		to.alternative_tile = MAX(int(p_value), 0);
	} else {
		return false;
	}
	_update_enabled_property_editors();
	return true;
}

bool RTileProxiesManagerDialog::_get(const StringName &p_name, Variant &r_ret) const {
	if (p_name == "from_source") {
		r_ret = from.source_id;
	} else if (p_name == "from_coords") {
		r_ret = from.get_atlas_coords();
	} else if (p_name == "from_alternative") {
		r_ret = from.alternative_tile;
	} else if (p_name == "to_source") {
		r_ret = to.source_id;
	} else if (p_name == "to_coords") {
		r_ret = to.get_atlas_coords();
	} else if (p_name == "to_alternative") {
		r_ret = to.alternative_tile;
	} else {
		return false;
	}
	return true;
}

void RTileProxiesManagerDialog::_unhandled_key_input(Ref<InputEvent> p_event) {
	ERR_FAIL_COND(p_event.is_null());

	if (p_event->is_pressed() && !p_event->is_echo() && (Object::cast_to<InputEventKey>(p_event.ptr()) || Object::cast_to<InputEventJoypadButton>(p_event.ptr()) || Object::cast_to<InputEventAction>(*p_event))) {
		if (!is_inside_tree() || !is_visible()) {
			return;
		}

		if (popup_menu->activate_item_by_event(p_event, false)) {
			get_tree()->set_input_as_handled();
		}
	}
}

void RTileProxiesManagerDialog::cancel_pressed() {
	for (int i = 0; i < commited_actions_count; i++) {
		undo_redo->undo();
	}
	commited_actions_count = 0;
}

void RTileProxiesManagerDialog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_update_lists"), &RTileProxiesManagerDialog::_update_lists);
	ClassDB::bind_method(D_METHOD("_unhandled_key_input"), &RTileProxiesManagerDialog::_unhandled_key_input);
	ClassDB::bind_method(D_METHOD("_right_clicked"), &RTileProxiesManagerDialog::_right_clicked);
	ClassDB::bind_method(D_METHOD("_menu_id_pressed"), &RTileProxiesManagerDialog::_menu_id_pressed);
	ClassDB::bind_method(D_METHOD("_property_changed"), &RTileProxiesManagerDialog::_property_changed);

	ClassDB::bind_method(D_METHOD("_add_button_pressed"), &RTileProxiesManagerDialog::_add_button_pressed);
	ClassDB::bind_method(D_METHOD("_clear_invalid_button_pressed"), &RTileProxiesManagerDialog::_clear_invalid_button_pressed);
	ClassDB::bind_method(D_METHOD("_clear_all_button_pressed"), &RTileProxiesManagerDialog::_clear_all_button_pressed);
}

void RTileProxiesManagerDialog::update_tile_set(Ref<RTileSet> p_tile_set) {
	ERR_FAIL_COND(!p_tile_set.is_valid());
	tile_set = p_tile_set;
	commited_actions_count = 0;
	_update_lists();
}

RTileProxiesManagerDialog::RTileProxiesManagerDialog() {
	// Tile proxy management window.
	set_title(TTR("Tile Proxies Management"));
	set_process_unhandled_key_input(true);

	to.source_id = 0;
	to.set_atlas_coords(Vector2i());
	to.alternative_tile = 0;

	VBoxContainer *vbox_container = memnew(VBoxContainer);
	vbox_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	vbox_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(vbox_container);

	Label *source_level_label = memnew(Label);
	source_level_label->set_text(TTR("Source-level proxies"));
	vbox_container->add_child(source_level_label);

	source_level_list = memnew(ItemList);
	source_level_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	source_level_list->set_select_mode(ItemList::SELECT_MULTI);
	source_level_list->set_allow_rmb_select(true);
	Vector<Variant> source_level_list_arr;
	source_level_list_arr.push_back(source_level_list);
	source_level_list->connect("item_rmb_selected", this, "_right_clicked", source_level_list_arr);
	vbox_container->add_child(source_level_list);

	Label *coords_level_label = memnew(Label);
	coords_level_label->set_text(TTR("Coords-level proxies"));
	vbox_container->add_child(coords_level_label);

	coords_level_list = memnew(ItemList);
	coords_level_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	coords_level_list->set_select_mode(ItemList::SELECT_MULTI);
	coords_level_list->set_allow_rmb_select(true);
	Vector<Variant> coords_level_list_arr;
	coords_level_list_arr.push_back(coords_level_list);
	coords_level_list->connect("item_rmb_selected", this, "_right_clicked", coords_level_list_arr);
	vbox_container->add_child(coords_level_list);

	Label *alternative_level_label = memnew(Label);
	alternative_level_label->set_text(TTR("Alternative-level proxies"));
	vbox_container->add_child(alternative_level_label);

	alternative_level_list = memnew(ItemList);
	alternative_level_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	alternative_level_list->set_select_mode(ItemList::SELECT_MULTI);
	alternative_level_list->set_allow_rmb_select(true);
	Vector<Variant> alternative_level_list_arr;
	alternative_level_list_arr.push_back(alternative_level_list);
	alternative_level_list->connect("item_rmb_selected", this, "_right_clicked", alternative_level_list_arr);
	vbox_container->add_child(alternative_level_list);

	popup_menu = memnew(PopupMenu);
	//popup_menu->add_shortcut(ED_GET_SHORTCUT("ui_text_delete"));
	popup_menu->connect("id_pressed", this, "_menu_id_pressed");
	add_child(popup_menu);

	// Add proxy panel.
	HSeparator *h_separator = memnew(HSeparator);
	vbox_container->add_child(h_separator);

	Label *add_label = memnew(Label);
	add_label->set_text(TTR("Add a new tile proxy:"));
	vbox_container->add_child(add_label);

	HBoxContainer *hboxcontainer = memnew(HBoxContainer);
	vbox_container->add_child(hboxcontainer);

	// From
	VBoxContainer *vboxcontainer_from = memnew(VBoxContainer);
	vboxcontainer_from->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hboxcontainer->add_child(vboxcontainer_from);

	source_from_property_editor = memnew(EditorPropertyInteger);
	source_from_property_editor->set_label(TTR("From Source"));
	source_from_property_editor->set_object_and_property(this, "from_source");
	source_from_property_editor->connect("property_changed", this, "_property_changed");
	source_from_property_editor->set_selectable(false);
	source_from_property_editor->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	source_from_property_editor->setup(-1, 99999, 1, true, false);
	vboxcontainer_from->add_child(source_from_property_editor);

	coords_from_property_editor = memnew(EditorPropertyVector2);
	coords_from_property_editor->set_label(TTR("From Coords"));
	coords_from_property_editor->set_object_and_property(this, "from_coords");
	coords_from_property_editor->connect("property_changed", this, "_property_changed");
	coords_from_property_editor->set_selectable(false);
	coords_from_property_editor->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	coords_from_property_editor->setup(-1, 99999, 1, true);
	coords_from_property_editor->hide();
	vboxcontainer_from->add_child(coords_from_property_editor);

	alternative_from_property_editor = memnew(EditorPropertyInteger);
	alternative_from_property_editor->set_label(TTR("From Alternative"));
	alternative_from_property_editor->set_object_and_property(this, "from_alternative");
	alternative_from_property_editor->connect("property_changed", this, "_property_changed");
	alternative_from_property_editor->set_selectable(false);
	alternative_from_property_editor->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	alternative_from_property_editor->setup(-1, 99999, 1, true, false);
	alternative_from_property_editor->hide();
	vboxcontainer_from->add_child(alternative_from_property_editor);

	// To
	VBoxContainer *vboxcontainer_to = memnew(VBoxContainer);
	vboxcontainer_to->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hboxcontainer->add_child(vboxcontainer_to);

	source_to_property_editor = memnew(EditorPropertyInteger);
	source_to_property_editor->set_label(TTR("To Source"));
	source_to_property_editor->set_object_and_property(this, "to_source");
	source_to_property_editor->connect("property_changed", this, "_property_changed");
	source_to_property_editor->set_selectable(false);
	source_to_property_editor->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	source_to_property_editor->setup(-1, 99999, 1, true, false);
	vboxcontainer_to->add_child(source_to_property_editor);

	coords_to_property_editor = memnew(EditorPropertyVector2);
	coords_to_property_editor->set_label(TTR("To Coords"));
	coords_to_property_editor->set_object_and_property(this, "to_coords");
	coords_to_property_editor->connect("property_changed", this, "_property_changed");
	coords_to_property_editor->set_selectable(false);
	coords_to_property_editor->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	coords_to_property_editor->setup(-1, 99999, 1, true);
	coords_to_property_editor->hide();
	vboxcontainer_to->add_child(coords_to_property_editor);

	alternative_to_property_editor = memnew(EditorPropertyInteger);
	alternative_to_property_editor->set_label(TTR("To Alternative"));
	alternative_to_property_editor->set_object_and_property(this, "to_alternative");
	alternative_to_property_editor->connect("property_changed", this, "_property_changed");
	alternative_to_property_editor->set_selectable(false);
	alternative_to_property_editor->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	alternative_to_property_editor->setup(-1, 99999, 1, true, false);
	alternative_to_property_editor->hide();
	vboxcontainer_to->add_child(alternative_to_property_editor);

	Button *add_button = memnew(Button);
	add_button->set_text(TTR("Add"));
	add_button->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
	add_button->connect("pressed", this, "_add_button_pressed");
	vbox_container->add_child(add_button);

	h_separator = memnew(HSeparator);
	vbox_container->add_child(h_separator);

	// Generic actions.
	Label *generic_actions_label = memnew(Label);
	generic_actions_label->set_text(TTR("Global actions:"));
	vbox_container->add_child(generic_actions_label);

	hboxcontainer = memnew(HBoxContainer);
	vbox_container->add_child(hboxcontainer);

	Button *clear_invalid_button = memnew(Button);
	clear_invalid_button->set_text(TTR("Clear Invalid"));
	clear_invalid_button->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
	clear_invalid_button->connect("pressed", this, "_clear_invalid_button_pressed");
	hboxcontainer->add_child(clear_invalid_button);

	Button *clear_all_button = memnew(Button);
	clear_all_button->set_text(TTR("Clear All"));
	clear_all_button->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
	clear_all_button->connect("pressed", this, "_clear_all_button_pressed");
	hboxcontainer->add_child(clear_all_button);

	h_separator = memnew(HSeparator);
	vbox_container->add_child(h_separator);
}
