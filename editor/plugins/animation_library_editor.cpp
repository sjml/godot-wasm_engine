/*************************************************************************/
/*  animation_library_editor.cpp                                         */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "animation_library_editor.h"
#include "editor/editor_file_dialog.h"
#include "editor/editor_node.h"
#include "editor/editor_scale.h"

void AnimationLibraryEditor::set_animation_player(Object *p_player) {
	player = p_player;
}

void AnimationLibraryEditor::_add_library() {
	add_library_dialog->set_title(TTR("Library Name:"));
	add_library_name->set_text("");
	add_library_dialog->popup_centered();
	add_library_name->grab_focus();
	adding_animation = false;
	adding_animation_to_library = StringName();
	_add_library_validate("");
}

void AnimationLibraryEditor::_add_library_validate(const String &p_name) {
	String error;

	if (adding_animation) {
		Ref<AnimationLibrary> al = player->call("get_animation_library", adding_animation_to_library);
		ERR_FAIL_COND(al.is_null());
		if (p_name == "") {
			error = TTR("Animation name can't be empty.");
		} else if (!AnimationLibrary::is_valid_animation_name(p_name)) {
			error = TTR("Animation name contains invalid characters: '/', ':', ',' or '['.");
		} else if (al->has_animation(p_name)) {
			error = TTR("Animation with the same name already exists.");
		}
	} else {
		if (p_name == "" && bool(player->call("has_animation_library", ""))) {
			error = TTR("Enter a library name.");
		} else if (!AnimationLibrary::is_valid_library_name(p_name)) {
			error = TTR("Library name contains invalid characters: '/', ':', ',' or '['.");
		} else if (bool(player->call("has_animation_library", p_name))) {
			error = TTR("Library with the same name already exists.");
		}
	}

	if (error != "") {
		add_library_validate->add_theme_color_override("font_color", get_theme_color(SNAME("error_color"), SNAME("Editor")));
		add_library_validate->set_text(error);
		add_library_dialog->get_ok_button()->set_disabled(true);
	} else {
		add_library_validate->add_theme_color_override("font_color", get_theme_color(SNAME("success_color"), SNAME("Editor")));
		if (p_name == "") {
			add_library_validate->set_text(TTR("Global library will be created."));
		} else {
			add_library_validate->set_text(TTR("Library name is valid."));
		}
		add_library_dialog->get_ok_button()->set_disabled(false);
	}
}

void AnimationLibraryEditor::_add_library_confirm() {
	if (adding_animation) {
		String anim_name = add_library_name->get_text();
		UndoRedo *undo_redo = EditorNode::get_singleton()->get_undo_redo();

		Ref<AnimationLibrary> al = player->call("get_animation_library", adding_animation_to_library);
		ERR_FAIL_COND(!al.is_valid());

		Ref<Animation> anim;
		anim.instantiate();

		undo_redo->create_action(vformat(TTR("Add Animation to Library: %s"), anim_name));
		undo_redo->add_do_method(al.ptr(), "add_animation", anim_name, anim);
		undo_redo->add_undo_method(al.ptr(), "remove_animation", anim_name);
		undo_redo->add_do_method(this, "_update_editor", player);
		undo_redo->add_undo_method(this, "_update_editor", player);
		undo_redo->commit_action();

	} else {
		String lib_name = add_library_name->get_text();
		UndoRedo *undo_redo = EditorNode::get_singleton()->get_undo_redo();

		Ref<AnimationLibrary> al;
		al.instantiate();

		undo_redo->create_action(vformat(TTR("Add Animation Library: %s"), lib_name));
		undo_redo->add_do_method(player, "add_animation_library", lib_name, al);
		undo_redo->add_undo_method(player, "remove_animation_library", lib_name);
		undo_redo->add_do_method(this, "_update_editor", player);
		undo_redo->add_undo_method(this, "_update_editor", player);
		undo_redo->commit_action();
	}
}

void AnimationLibraryEditor::_load_library() {
	List<String> extensions;
	ResourceLoader::get_recognized_extensions_for_type("AnimationLibrary", &extensions);

	file_dialog->set_title(TTR("Load Animation"));
	file_dialog->clear_filters();
	for (const String &K : extensions) {
		file_dialog->add_filter("*." + K);
	}

	file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
	file_dialog->set_current_file("");
	file_dialog->popup_centered_ratio();

	file_dialog_action = FILE_DIALOG_ACTION_OPEN_LIBRARY;
}

void AnimationLibraryEditor::_file_popup_selected(int p_id) {
	Ref<AnimationLibrary> al = player->call("get_animation_library", file_dialog_library);
	Ref<Animation> anim;
	if (file_dialog_animation != StringName()) {
		anim = al->get_animation(file_dialog_animation);
		ERR_FAIL_COND(anim.is_null());
	}
	switch (p_id) {
		case FILE_MENU_SAVE_LIBRARY: {
			if (al->get_path().is_resource_file()) {
				EditorNode::get_singleton()->save_resource(al);
				break;
			}
			[[fallthrough]];
		}
		case FILE_MENU_SAVE_AS_LIBRARY: {
			file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
			file_dialog->set_title(TTR("Save Library"));
			if (al->get_path().is_resource_file()) {
				file_dialog->set_current_path(al->get_path());
			} else {
				file_dialog->set_current_file(String(file_dialog_library) + ".res");
			}
			file_dialog->clear_filters();
			List<String> exts;
			ResourceLoader::get_recognized_extensions_for_type("AnimationLibrary", &exts);
			for (const String &K : exts) {
				file_dialog->add_filter("*." + K);
			}

			file_dialog->popup_centered_ratio();
			file_dialog_action = FILE_DIALOG_ACTION_SAVE_LIBRARY;
		} break;
		case FILE_MENU_MAKE_LIBRARY_UNIQUE: {
			StringName lib_name = file_dialog_library;

			Ref<AnimationLibrary> ald = al->duplicate();

			UndoRedo *undo_redo = EditorNode::get_singleton()->get_undo_redo();
			undo_redo->create_action(vformat(TTR("Make Animation Library Unique: %s"), lib_name));
			undo_redo->add_do_method(player, "remove_animation_library", lib_name);
			undo_redo->add_do_method(player, "add_animation_library", lib_name, ald);
			undo_redo->add_undo_method(player, "remove_animation_library", lib_name);
			undo_redo->add_undo_method(player, "add_animation_library", lib_name, al);
			undo_redo->add_do_method(this, "_update_editor", player);
			undo_redo->add_undo_method(this, "_update_editor", player);
			undo_redo->commit_action();

		} break;
		case FILE_MENU_EDIT_LIBRARY: {
			EditorNode::get_singleton()->push_item(al.ptr());
		} break;

		case FILE_MENU_SAVE_ANIMATION: {
			if (anim->get_path().is_resource_file()) {
				EditorNode::get_singleton()->save_resource(anim);
				break;
			}
			[[fallthrough]];
		}
		case FILE_MENU_SAVE_AS_ANIMATION: {
			file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
			file_dialog->set_title(TTR("Save Animation"));
			if (anim->get_path().is_resource_file()) {
				file_dialog->set_current_path(anim->get_path());
			} else {
				file_dialog->set_current_file(String(file_dialog_animation) + ".res");
			}
			file_dialog->clear_filters();
			List<String> exts;
			ResourceLoader::get_recognized_extensions_for_type("Animation", &exts);
			for (const String &K : exts) {
				file_dialog->add_filter("*." + K);
			}

			file_dialog->popup_centered_ratio();
			file_dialog_action = FILE_DIALOG_ACTION_SAVE_ANIMATION;
		} break;
		case FILE_MENU_MAKE_ANIMATION_UNIQUE: {
			StringName anim_name = file_dialog_animation;

			Ref<Animation> animd = anim->duplicate();

			UndoRedo *undo_redo = EditorNode::get_singleton()->get_undo_redo();
			undo_redo->create_action(vformat(TTR("Make Animation Unique: %s"), anim_name));
			undo_redo->add_do_method(al.ptr(), "remove_animation", anim_name);
			undo_redo->add_do_method(al.ptr(), "add_animation", anim_name, animd);
			undo_redo->add_undo_method(al.ptr(), "remove_animation", anim_name);
			undo_redo->add_undo_method(al.ptr(), "add_animation", anim_name, anim);
			undo_redo->add_do_method(this, "_update_editor", player);
			undo_redo->add_undo_method(this, "_update_editor", player);
			undo_redo->commit_action();
		} break;
		case FILE_MENU_EDIT_ANIMATION: {
			EditorNode::get_singleton()->push_item(anim.ptr());
		} break;
	}
}
void AnimationLibraryEditor::_load_file(String p_path) {
	switch (file_dialog_action) {
		case FILE_DIALOG_ACTION_OPEN_LIBRARY: {
			Ref<AnimationLibrary> al = ResourceLoader::load(p_path);
			if (al.is_null()) {
				error_dialog->set_text(TTR("Invalid AnimationLibrary file."));
				error_dialog->popup_centered();
				return;
			}

			TypedArray<StringName> libs = player->call("get_animation_library_list");
			for (int i = 0; i < libs.size(); i++) {
				const StringName K = libs[i];
				Ref<AnimationLibrary> al2 = player->call("get_animation_library", K);
				if (al2 == al) {
					error_dialog->set_text(TTR("This library is already added to the player."));
					error_dialog->popup_centered();

					return;
				}
			}

			String name = AnimationLibrary::validate_library_name(p_path.get_file().get_basename());

			int attempt = 1;

			while (bool(player->call("has_animation_library", name))) {
				attempt++;
				name = p_path.get_file().get_basename() + " " + itos(attempt);
			}

			UndoRedo *undo_redo = EditorNode::get_singleton()->get_undo_redo();

			undo_redo->create_action(vformat(TTR("Add Animation Library: %s"), name));
			undo_redo->add_do_method(player, "add_animation_library", name, al);
			undo_redo->add_undo_method(player, "remove_animation_library", name);
			undo_redo->add_do_method(this, "_update_editor", player);
			undo_redo->add_undo_method(this, "_update_editor", player);
			undo_redo->commit_action();
		} break;
		case FILE_DIALOG_ACTION_OPEN_ANIMATION: {
			Ref<Animation> anim = ResourceLoader::load(p_path);
			if (anim.is_null()) {
				error_dialog->set_text(TTR("Invalid Animation file."));
				error_dialog->popup_centered();
				return;
			}

			Ref<AnimationLibrary> al = player->call("get_animation_library", adding_animation_to_library);
			List<StringName> anims;
			al->get_animation_list(&anims);
			for (const StringName &K : anims) {
				Ref<Animation> a2 = al->get_animation(K);
				if (a2 == anim) {
					error_dialog->set_text(TTR("This animation is already added to the library."));
					error_dialog->popup_centered();
					return;
				}
			}

			String name = p_path.get_file().get_basename();

			int attempt = 1;

			while (al->has_animation(name)) {
				attempt++;
				name = p_path.get_file().get_basename() + " " + itos(attempt);
			}

			UndoRedo *undo_redo = EditorNode::get_singleton()->get_undo_redo();

			undo_redo->create_action(vformat(TTR("Load Animation into Library: %s"), name));
			undo_redo->add_do_method(al.ptr(), "add_animation", name, anim);
			undo_redo->add_undo_method(al.ptr(), "remove_animation", name);
			undo_redo->add_do_method(this, "_update_editor", player);
			undo_redo->add_undo_method(this, "_update_editor", player);
			undo_redo->commit_action();
		} break;

		case FILE_DIALOG_ACTION_SAVE_LIBRARY: {
			Ref<AnimationLibrary> al = player->call("get_animation_library", file_dialog_library);
			String prev_path = al->get_path();
			EditorNode::get_singleton()->save_resource_in_path(al, p_path);

			if (al->get_path() != prev_path) { // Save successful.
				UndoRedo *undo_redo = EditorNode::get_singleton()->get_undo_redo();

				undo_redo->create_action(vformat(TTR("Save Animation library to File: %s"), file_dialog_library));
				undo_redo->add_do_method(al.ptr(), "set_path", al->get_path());
				undo_redo->add_undo_method(al.ptr(), "set_path", prev_path);
				undo_redo->add_do_method(this, "_update_editor", player);
				undo_redo->add_undo_method(this, "_update_editor", player);
				undo_redo->commit_action();
			}

		} break;
		case FILE_DIALOG_ACTION_SAVE_ANIMATION: {
			Ref<AnimationLibrary> al = player->call("get_animation_library", file_dialog_library);
			Ref<Animation> anim;
			if (file_dialog_animation != StringName()) {
				anim = al->get_animation(file_dialog_animation);
				ERR_FAIL_COND(anim.is_null());
			}
			String prev_path = anim->get_path();
			EditorNode::get_singleton()->save_resource_in_path(anim, p_path);
			if (anim->get_path() != prev_path) { // Save successful.
				UndoRedo *undo_redo = EditorNode::get_singleton()->get_undo_redo();

				undo_redo->create_action(vformat(TTR("Save Animation to File: %s"), file_dialog_animation));
				undo_redo->add_do_method(anim.ptr(), "set_path", anim->get_path());
				undo_redo->add_undo_method(anim.ptr(), "set_path", prev_path);
				undo_redo->add_do_method(this, "_update_editor", player);
				undo_redo->add_undo_method(this, "_update_editor", player);
				undo_redo->commit_action();
			}
		} break;
	}
}

void AnimationLibraryEditor::_item_renamed() {
	TreeItem *ti = tree->get_edited();
	String text = ti->get_text(0);
	String old_text = ti->get_metadata(0);
	bool restore_text = false;
	UndoRedo *undo_redo = EditorNode::get_singleton()->get_undo_redo();

	if (String(text).contains("/") || String(text).contains(":") || String(text).contains(",") || String(text).contains("[")) {
		restore_text = true;
	} else {
		if (ti->get_parent() == tree->get_root()) {
			// Renamed library

			if (player->call("has_animation_library", text)) {
				restore_text = true;
			} else {
				undo_redo->create_action(vformat(TTR("Rename Animation Library: %s"), text));
				undo_redo->add_do_method(player, "rename_animation_library", old_text, text);
				undo_redo->add_undo_method(player, "rename_animation_library", text, old_text);
				undo_redo->add_do_method(this, "_update_editor", player);
				undo_redo->add_undo_method(this, "_update_editor", player);
				updating = true;
				undo_redo->commit_action();
				updating = false;
				ti->set_metadata(0, text);
				if (text == "") {
					ti->set_suffix(0, TTR("[Global]"));
				} else {
					ti->set_suffix(0, "");
				}
			}
		} else {
			// Renamed anim
			StringName library = ti->get_parent()->get_metadata(0);
			Ref<AnimationLibrary> al = player->call("get_animation_library", library);

			if (al.is_valid()) {
				if (al->has_animation(text)) {
					restore_text = true;
				} else {
					undo_redo->create_action(vformat(TTR("Rename Animation: %s"), text));
					undo_redo->add_do_method(al.ptr(), "rename_animation", old_text, text);
					undo_redo->add_undo_method(al.ptr(), "rename_animation", text, old_text);
					undo_redo->add_do_method(this, "_update_editor", player);
					undo_redo->add_undo_method(this, "_update_editor", player);
					updating = true;
					undo_redo->commit_action();
					updating = false;

					ti->set_metadata(0, text);
				}
			} else {
				restore_text = true;
			}
		}
	}

	if (restore_text) {
		ti->set_text(0, old_text);
	}
}

void AnimationLibraryEditor::_button_pressed(TreeItem *p_item, int p_column, int p_button) {
	if (p_item->get_parent() == tree->get_root()) {
		// Library
		StringName lib_name = p_item->get_metadata(0);
		Ref<AnimationLibrary> al = player->call("get_animation_library", lib_name);
		switch (p_button) {
			case LIB_BUTTON_ADD: {
				add_library_dialog->set_title(TTR("Animation Name:"));
				add_library_name->set_text("");
				add_library_dialog->popup_centered();
				add_library_name->grab_focus();
				adding_animation = true;
				adding_animation_to_library = p_item->get_metadata(0);
				_add_library_validate("");
			} break;
			case LIB_BUTTON_LOAD: {
				adding_animation_to_library = p_item->get_metadata(0);
				List<String> extensions;
				ResourceLoader::get_recognized_extensions_for_type("Animation", &extensions);

				file_dialog->clear_filters();
				for (const String &K : extensions) {
					file_dialog->add_filter("*." + K);
				}

				file_dialog->set_title(TTR("Load Animation"));
				file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
				file_dialog->set_current_file("");
				file_dialog->popup_centered_ratio();

				file_dialog_action = FILE_DIALOG_ACTION_OPEN_ANIMATION;

			} break;
			case LIB_BUTTON_PASTE: {
				Ref<Animation> anim = EditorSettings::get_singleton()->get_resource_clipboard();
				if (!anim.is_valid()) {
					error_dialog->set_text(TTR("No animation resource in clipboard!"));
					error_dialog->popup_centered();
					return;
				}

				anim = anim->duplicate(); // Users simply dont care about referencing, so making a copy works better here.

				String base_name;
				if (anim->get_name() != "") {
					base_name = anim->get_name();
				} else {
					base_name = TTR("Pasted Animation");
				}

				String name = base_name;
				int attempt = 1;
				while (al->has_animation(name)) {
					attempt++;
					name = base_name + " " + itos(attempt);
				}

				UndoRedo *undo_redo = EditorNode::get_singleton()->get_undo_redo();

				undo_redo->create_action(vformat(TTR("Add Animation to Library: %s"), name));
				undo_redo->add_do_method(al.ptr(), "add_animation", name, anim);
				undo_redo->add_undo_method(al.ptr(), "remove_animation", name);
				undo_redo->add_do_method(this, "_update_editor", player);
				undo_redo->add_undo_method(this, "_update_editor", player);
				undo_redo->commit_action();

			} break;
			case LIB_BUTTON_FILE: {
				file_popup->clear();
				file_popup->add_item(TTR("Save"), FILE_MENU_SAVE_LIBRARY);
				file_popup->add_item(TTR("Save As"), FILE_MENU_SAVE_AS_LIBRARY);
				file_popup->add_separator();
				file_popup->add_item(TTR("Make Unique"), FILE_MENU_MAKE_LIBRARY_UNIQUE);
				file_popup->add_separator();
				file_popup->add_item(TTR("Open in Inspector"), FILE_MENU_EDIT_LIBRARY);
				Rect2 pos = tree->get_item_rect(p_item, 1, 0);
				Vector2 popup_pos = tree->get_screen_position() + pos.position + Vector2(0, pos.size.height);
				file_popup->popup(Rect2(popup_pos, Size2()));

				file_dialog_animation = StringName();
				file_dialog_library = lib_name;
			} break;
			case LIB_BUTTON_DELETE: {
				UndoRedo *undo_redo = EditorNode::get_singleton()->get_undo_redo();
				undo_redo->create_action(vformat(TTR("Remove Animation Library: %s"), lib_name));
				undo_redo->add_do_method(player, "remove_animation_library", lib_name);
				undo_redo->add_undo_method(player, "add_animation_library", lib_name, al);
				undo_redo->add_do_method(this, "_update_editor", player);
				undo_redo->add_undo_method(this, "_update_editor", player);
				undo_redo->commit_action();
			} break;
		}

	} else {
		// Animation
		StringName lib_name = p_item->get_parent()->get_metadata(0);
		StringName anim_name = p_item->get_metadata(0);
		Ref<AnimationLibrary> al = player->call("get_animation_library", lib_name);
		Ref<Animation> anim = al->get_animation(anim_name);
		ERR_FAIL_COND(!anim.is_valid());
		switch (p_button) {
			case ANIM_BUTTON_COPY: {
				if (anim->get_name() == "") {
					anim->set_name(anim_name); // Keep the name around
				}
				EditorSettings::get_singleton()->set_resource_clipboard(anim);
			} break;
			case ANIM_BUTTON_FILE: {
				file_popup->clear();
				file_popup->add_item(TTR("Save"), FILE_MENU_SAVE_ANIMATION);
				file_popup->add_item(TTR("Save As"), FILE_MENU_SAVE_AS_ANIMATION);
				file_popup->add_separator();
				file_popup->add_item(TTR("Make Unique"), FILE_MENU_MAKE_ANIMATION_UNIQUE);
				file_popup->add_separator();
				file_popup->add_item(TTR("Open in Inspector"), FILE_MENU_EDIT_ANIMATION);
				Rect2 pos = tree->get_item_rect(p_item, 1, 0);
				Vector2 popup_pos = tree->get_screen_position() + pos.position + Vector2(0, pos.size.height);
				file_popup->popup(Rect2(popup_pos, Size2()));

				file_dialog_animation = anim_name;
				file_dialog_library = lib_name;

			} break;
			case ANIM_BUTTON_DELETE: {
				UndoRedo *undo_redo = EditorNode::get_singleton()->get_undo_redo();
				undo_redo->create_action(vformat(TTR("Remove Animation from Library: %s"), anim_name));
				undo_redo->add_do_method(al.ptr(), "remove_animation", anim_name);
				undo_redo->add_undo_method(al.ptr(), "add_animation", anim_name, anim);
				undo_redo->add_do_method(this, "_update_editor", player);
				undo_redo->add_undo_method(this, "_update_editor", player);
				undo_redo->commit_action();
			} break;
		}
	}
}

void AnimationLibraryEditor::update_tree() {
	if (updating) {
		return;
	}

	tree->clear();
	ERR_FAIL_COND(!player);

	Color ss_color = get_theme_color(SNAME("prop_subsection"), SNAME("Editor"));

	TreeItem *root = tree->create_item();
	TypedArray<StringName> libs = player->call("get_animation_library_list");

	for (int i = 0; i < libs.size(); i++) {
		const StringName K = libs[i];
		TreeItem *libitem = tree->create_item(root);
		libitem->set_text(0, K);
		if (K == StringName()) {
			libitem->set_suffix(0, TTR("[Global]"));
		} else {
			libitem->set_suffix(0, "");
		}
		libitem->set_editable(0, true);
		libitem->set_metadata(0, K);
		libitem->set_icon(0, get_theme_icon("AnimationLibrary", "EditorIcons"));
		libitem->add_button(0, get_theme_icon("Add", "EditorIcons"), LIB_BUTTON_ADD, false, TTR("Add Animation to Library"));
		libitem->add_button(0, get_theme_icon("Load", "EditorIcons"), LIB_BUTTON_LOAD, false, TTR("Load animation from file and add to library"));
		libitem->add_button(0, get_theme_icon("ActionPaste", "EditorIcons"), LIB_BUTTON_PASTE, false, TTR("Paste Animation to Library from clipboard"));
		Ref<AnimationLibrary> al = player->call("get_animation_library", K);
		if (al->get_path().is_resource_file()) {
			libitem->set_text(1, al->get_path().get_file());
			libitem->set_tooltip(1, al->get_path());
		} else {
			libitem->set_text(1, TTR("[built-in]"));
		}
		libitem->add_button(1, get_theme_icon("Save", "EditorIcons"), LIB_BUTTON_FILE, false, TTR("Save animation library to resource on disk"));
		libitem->add_button(1, get_theme_icon("Remove", "EditorIcons"), LIB_BUTTON_DELETE, false, TTR("Remove animation library"));

		libitem->set_custom_bg_color(0, ss_color);

		List<StringName> animations;
		al->get_animation_list(&animations);
		for (const StringName &L : animations) {
			TreeItem *anitem = tree->create_item(libitem);
			anitem->set_text(0, L);
			anitem->set_editable(0, true);
			anitem->set_metadata(0, L);
			anitem->set_icon(0, get_theme_icon("Animation", "EditorIcons"));
			anitem->add_button(0, get_theme_icon("ActionCopy", "EditorIcons"), ANIM_BUTTON_COPY, false, TTR("Copy animation to clipboard"));
			Ref<Animation> anim = al->get_animation(L);

			if (anim->get_path().is_resource_file()) {
				anitem->set_text(1, anim->get_path().get_file());
				anitem->set_tooltip(1, anim->get_path());
			} else {
				anitem->set_text(1, TTR("[built-in]"));
			}
			anitem->add_button(1, get_theme_icon("Save", "EditorIcons"), ANIM_BUTTON_FILE, false, TTR("Save animation to resource on disk"));
			anitem->add_button(1, get_theme_icon("Remove", "EditorIcons"), ANIM_BUTTON_DELETE, false, TTR("Remove animation from Library"));
		}
	}
}

void AnimationLibraryEditor::show_dialog() {
	update_tree();
	popup_centered_ratio(0.5);
}

void AnimationLibraryEditor::_update_editor(Object *p_player) {
	emit_signal("update_editor", p_player);
}

void AnimationLibraryEditor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_update_editor", "player"), &AnimationLibraryEditor::_update_editor);
	ADD_SIGNAL(MethodInfo("update_editor"));
}

AnimationLibraryEditor::AnimationLibraryEditor() {
	set_title(TTR("Edit Animation Libraries"));

	file_dialog = memnew(EditorFileDialog);
	add_child(file_dialog);
	file_dialog->connect("file_selected", callable_mp(this, &AnimationLibraryEditor::_load_file));

	add_library_dialog = memnew(ConfirmationDialog);
	VBoxContainer *dialog_vb = memnew(VBoxContainer);
	add_library_name = memnew(LineEdit);
	dialog_vb->add_child(add_library_name);
	add_library_name->connect("text_changed", callable_mp(this, &AnimationLibraryEditor::_add_library_validate));
	add_child(add_library_dialog);

	add_library_validate = memnew(Label);
	dialog_vb->add_child(add_library_validate);
	add_library_dialog->add_child(dialog_vb);
	add_library_dialog->connect("confirmed", callable_mp(this, &AnimationLibraryEditor::_add_library_confirm));
	add_library_dialog->register_text_enter(add_library_name);

	VBoxContainer *vb = memnew(VBoxContainer);
	HBoxContainer *hb = memnew(HBoxContainer);
	hb->add_spacer(true);
	Button *b = memnew(Button(TTR("Add Library")));
	b->connect("pressed", callable_mp(this, &AnimationLibraryEditor::_add_library));
	hb->add_child(b);
	b = memnew(Button(TTR("Load Library")));
	b->connect("pressed", callable_mp(this, &AnimationLibraryEditor::_load_library));
	hb->add_child(b);
	vb->add_child(hb);
	tree = memnew(Tree);
	vb->add_child(tree);

	tree->set_columns(2);
	tree->set_column_titles_visible(true);
	tree->set_column_title(0, TTR("Resource"));
	tree->set_column_title(1, TTR("Storage"));
	tree->set_column_expand(0, true);
	tree->set_column_custom_minimum_width(1, EDSCALE * 250);
	tree->set_column_expand(1, false);
	tree->set_hide_root(true);
	tree->set_hide_folding(true);
	tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	tree->connect("item_edited", callable_mp(this, &AnimationLibraryEditor::_item_renamed));
	tree->connect("button_pressed", callable_mp(this, &AnimationLibraryEditor::_button_pressed));

	file_popup = memnew(PopupMenu);
	add_child(file_popup);
	file_popup->connect("id_pressed", callable_mp(this, &AnimationLibraryEditor::_file_popup_selected));

	add_child(vb);

	error_dialog = memnew(AcceptDialog);
	error_dialog->set_title(TTR("Error:"));
	add_child(error_dialog);
}
