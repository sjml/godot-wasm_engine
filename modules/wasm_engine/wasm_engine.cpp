#include "wasm_engine.h"

#include "wasmtime/include/wasm.h"
#include "wasmtime/include/wasmtime.h"


static wasm_engine_t *_engine = NULL;
static uint64_t _programCount = 0;

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
	this->store = wasmtime_store_new(_engine, NULL, NULL);
	this->module = NULL;
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

void WasmProgram::load_wat(String wat_text) {
	wasm_byte_vec_t wasm;
	CharString wat_text_str = wat_text.utf8();
	wasmtime_error_t* err = wasmtime_wat2wasm(wat_text_str.get_data(), wat_text_str.length(), &wasm);
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

	wasm_functype_t *test_msg = wasm_functype_new_0_0();
	wasmtime_func_t test;
	wasmtime_func_new(context, test_msg, callback, NULL, NULL, &test);

	wasm_trap_t* trap = NULL;
	wasmtime_extern_t import;
	import.kind = WASMTIME_EXTERN_FUNC;
	import.of.func = test;
	err = wasmtime_instance_new(context, this->module, &import, 1, &this->instance, &trap);
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
