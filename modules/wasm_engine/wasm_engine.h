#ifndef GODOT_WASMTIME_H
#define GODOT_WASMTIME_H

#include "core/reference.h"

#include "wasmtime/include/wasmtime.h"

class WasmProgram : public Reference {
	GDCLASS(WasmProgram, Reference);

private:
	static void _bind_methods();
	static void _print_wasmtime_error(const char* msg, wasmtime_error_t* err, wasm_trap_t *trap);
	static wasm_trap_t* _wrapper_callback(void* env, wasmtime_caller_t* caller, const wasmtime_val_t* args, size_t nargs, wasmtime_val_t* results, size_t nresults);

	wasmtime_store_t* store;
	wasmtime_module_t* module;
	wasmtime_instance_t instance;

	Object* go;
	HashMap<String, MethodInfo> wrapped_methods;

public:
	WasmProgram();
	~WasmProgram();

	void set_game_object(Object* go);

	void load_wat(String wat_text);
	void run_wat();
};


#endif // GODOT_WASMTIME_H
