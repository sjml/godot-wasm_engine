#include "wasm_engine.h"

#include "wasmtime/include/wasm.h"
#include "wasmtime/include/wasmtime.h"


static wasm_engine_t *_engine = NULL;
static u_int64_t _programCount = 0;

static void _printTestMessage() {
	print_line("This message printed by a call from WASM into Godot.");
}

static wasm_trap_t* callback(
	void* env,
	wasmtime_caller_t* caller,
	const wasmtime_val_t* args,
	size_t nargs,
	wasmtime_val_t* results,
	size_t nresults
) {
	_printTestMessage();
	return NULL;
}

/* static */ void WasmProgram::_bind_methods() {
	// ClassDB::bind_method(D_METHOD("init_engine"), &WasmProgram::init_engine);
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
	this->wasm_store = wasmtime_store_new(_engine, NULL, NULL);
	wasmtime_context_t* context = wasmtime_store_context(this->wasm_store);

	const char* wat =
		"(module"
			"(func $call_to_engine (import \"\" \"call_to_engine\"))"
			"(func (export \"run\") (call $call_to_engine))"
		")";

	wasm_byte_vec_t wasm;
	wasmtime_error_t* err = wasmtime_wat2wasm(wat, strlen(wat), &wasm);
	if (err != NULL) {
		WasmProgram::_print_wasmtime_error("failed to parse wat", err, NULL);
		return;
	}

	wasmtime_module_t* module = NULL;
	err = wasmtime_module_new(_engine, (uint8_t*)wasm.data, wasm.size, &module);
	wasm_byte_vec_delete(&wasm);
	if (err != NULL) {
		WasmProgram::_print_wasmtime_error("failed to compile module", err, NULL);
		return;
	}

	wasm_functype_t *test_msg = wasm_functype_new_0_0();
	wasmtime_func_t test;
	wasmtime_func_new(context, test_msg, callback, NULL, NULL, &test);

	wasm_trap_t* trap = NULL;
	wasmtime_instance_t instance;
	wasmtime_extern_t import;
	import.kind = WASMTIME_EXTERN_FUNC;
	import.of.func = test;
	err = wasmtime_instance_new(context, module, &import, 1, &instance, &trap);
	if (err != NULL || trap != NULL) {
		WasmProgram::_print_wasmtime_error("failed to instantiate", err, trap);
		return;
	}

	wasmtime_extern_t run;
	bool ok = wasmtime_instance_export_get(context, &instance, "run", 3, &run);
	if (!ok || run.kind != WASMTIME_EXTERN_FUNC) {
		print_error("Could not find 'run' function in WASM.");
		return;
	}

	err = wasmtime_func_call(context, &run.of.func, NULL, 0, NULL, 0, &trap);
	if (err != NULL || trap != NULL) {
		WasmProgram::_print_wasmtime_error("failed to call function", err, trap);
		return;
	}

	wasmtime_module_delete(module);
}

WasmProgram::~WasmProgram() {
	wasmtime_store_delete(this->wasm_store);

	_programCount -= 1;
	if (_programCount == 0) {
		wasm_engine_delete(_engine);
		_engine = NULL;
	}
}
