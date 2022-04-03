#include "wasm_engine.h"

#include "core/script_language.h"
#include "core/message_queue.h"

#include "wasmtime/include/wasm.h"
#include "wasmtime/include/wasmtime.h"


static wasm_engine_t *_engine = NULL;
static uint64_t _programCount = 0;


/* static */ void WasmProgram::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_game_object", "owner_object"), &WasmProgram::set_game_object);
	ClassDB::bind_method(D_METHOD("load_wat", "wat_text"), &WasmProgram::load_wat);
	ClassDB::bind_method(D_METHOD("run_wat"), &WasmProgram::run_wat);
}

/* static */ void WasmProgram::_print_wasmtime_error(const char* msg, wasmtime_error_t *err, wasm_trap_t *trap) {
	wasm_byte_vec_t error_message;
	if (err != NULL) {
		wasmtime_error_message(err, &error_message);
		wasmtime_error_delete(err);
	}
	else {
		wasm_trap_message(trap, &error_message);
		wasm_trap_delete(trap);
	}
	print_error(msg);
	print_error(error_message.data);
}

WasmProgram::WasmProgram() {
	if (_engine == NULL) {
		_engine = wasm_engine_new();
	}
	_programCount += 1;
	this->store = wasmtime_store_new(_engine, this, NULL);
	this->module = NULL;

	this->go = NULL;
}

WasmProgram::~WasmProgram() {
	if (this->module != NULL) {
		wasmtime_module_delete(this->module);
		this->module = NULL;

	}
	wasmtime_store_delete(this->store);
	this->store = NULL;

	_programCount -= 1;
	if (_programCount == 0) {
		wasm_engine_delete(_engine);
		_engine = NULL;
	}
}

void WasmProgram::set_game_object(Object* go) {
	this->go = go;

	List<MethodInfo> owner_method_list;
	Ref<Script> go_script = this->go->get_script();
	go_script.ptr()->get_script_method_list(&owner_method_list);
	int error = 0;
	for (int i=0; i < owner_method_list.size(); i++) {
		MethodInfo mi = owner_method_list[i];
		if (mi.name.begins_with("cmd_")) {
			for (int j=0; j < mi.arguments.size(); j++) {
				PropertyInfo arg = mi.arguments[j];
				if (arg.type == Variant::Type::NIL) {
					print_error("Cannot wrap method '" + mi.name + "'; all arguments must be type-hinted.");
					error = 1;
					continue;
				}
				else if (arg.type >= Variant::Type::STRING) {
					print_error("Cannot wrap method '" + mi.name + "'; arguments can only be int, float, or bool.");
					error = 2;
					continue;
				}
			}
			if (mi.return_val.type != Variant::Type::INT) {
				print_error("Cannot wrap method '" + mi.name + "'; return value must be type-hinted as int.");
				error = 3;
			}

			if (error != 0) {
				continue;
			}
			this->wrapped_methods.set(mi.name, mi);
		}
	}
}

void WasmProgram::load_wat(String wat_text) {
	if (this->go == NULL) {
		print_error("Cannot call 'load_wat' without a game object.");
		return;
	}

	wasm_trap_t* trap = NULL;
	wasmtime_error_t* err = NULL;

	wasm_byte_vec_t wasm;
	CharString wat_text_str = wat_text.utf8();
	err = wasmtime_wat2wasm(wat_text_str.get_data(), wat_text_str.length(), &wasm);
	if (err != NULL) {
		WasmProgram::_print_wasmtime_error("failed to parse wat", err, NULL);
		return;
	}

	if (this->module != NULL) {
		wasmtime_module_delete(this->module);
		this->module = NULL;
	}
	err = wasmtime_module_new(_engine, (uint8_t*)wasm.data, wasm.size, &this->module);
	wasm_byte_vec_delete(&wasm);
	if (err != NULL) {
		WasmProgram::_print_wasmtime_error("failed to compile module", err, NULL);
		return;
	}

	wasmtime_context_t* context = wasmtime_store_context(this->store);
	wasmtime_extern_t* imports = (wasmtime_extern_t*)memalloc(sizeof(wasmtime_extern_t) * this->wrapped_methods.size());

	const String* key = NULL;
	int import_idx = -1;
	while ((key = this->wrapped_methods.next(key))) {
		import_idx += 1;
		wasm_valtype_vec_t params, results;
		wasm_valtype_vec_new_empty(&results);
		MethodInfo mi = this->wrapped_methods.get(*key);
		wasm_valtype_vec_new_uninitialized(&params, mi.arguments.size());
		for (int i=0; i < mi.arguments.size(); i++) {
			if (mi.arguments[i].type == Variant::Type::BOOL) {
				params.data[i] = wasm_valtype_new_i32();
			}
			else if (mi.arguments[i].type == Variant::Type::INT) {
				params.data[i] = wasm_valtype_new_i64();
			}
			else if (mi.arguments[i].type == Variant::Type::REAL) {
				params.data[i] = wasm_valtype_new_f64();
			}
		}

		wasm_functype_t* wrapped_func_type = wasm_functype_new(&params, &results);
		wasmtime_func_t wrapped_func;
		wasmtime_func_new(context, wrapped_func_type, &WasmProgram::_wrapper_callback, (void*)key, NULL, &wrapped_func);

		imports[import_idx].kind = WASMTIME_EXTERN_FUNC;
		imports[import_idx].of.func = wrapped_func;
	}

	err = wasmtime_instance_new(context, this->module, imports, import_idx+1, &this->instance, &trap);
	memfree(imports);
	if (err != NULL || trap != NULL) {
		WasmProgram::_print_wasmtime_error("failed to instantiate", err, trap);
		return;
	}
}

void WasmProgram::run_wat() {
	if (this->module == NULL) {
		print_error("Cannot call 'run_wat' before loading program.");
		return;
	}
	wasmtime_context_t* context = wasmtime_store_context(this->store);

	wasmtime_extern_t run;
	bool ok = wasmtime_instance_export_get(context, &this->instance, "run", 3, &run);
	if (!ok || run.kind != WASMTIME_EXTERN_FUNC) {
		print_error("Could not find 'run' function in WASM.");
		return;
	}

	wasm_trap_t* trap = NULL;
	wasmtime_error_t* err = wasmtime_func_call(context, &run.of.func, NULL, 0, NULL, 0, &trap);
	if (err != NULL || trap != NULL) {
		WasmProgram::_print_wasmtime_error("failed to call function", err, trap);
		return;
	}
}

/* static */ wasm_trap_t* WasmProgram::_wrapper_callback(void* env, wasmtime_caller_t* caller, const wasmtime_val_t* args, size_t nargs, wasmtime_val_t* results, size_t nresults) {
	String* cmd_name = (String*)env;
	wasmtime_context_t* ctx = wasmtime_caller_context(caller);

	WasmProgram* calling_program = (WasmProgram*)wasmtime_context_get_data(ctx);

	MethodInfo mi = calling_program->wrapped_methods.get(*cmd_name);
	assert(nargs == mi.arguments.size());

	Array gd_args;
	for (int i=0; i < mi.arguments.size(); i++) {
		if (mi.arguments[i].type == Variant::Type::BOOL) {
			gd_args.push_back(Variant(args[i].of.i32));
		}
		else if (mi.arguments[i].type == Variant::Type::INT) {
			gd_args.push_back(Variant(args[i].of.i64));
		}
		else if (mi.arguments[i].type == Variant::Type::REAL) {
			gd_args.push_back(Variant(args[i].of.f64));
		}
	}

	// TODO: figure out how to defer this
	calling_program->go->callv(*cmd_name, gd_args);
	// MessageQueue::get_singleton()->push_call(calling_program->go->get_instance_id(), *cmd_name, gd_args);
	return NULL;
}
