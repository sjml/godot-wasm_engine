#ifndef GODOT_WASMTIME_H
#define GODOT_WASMTIME_H

#include "core/reference.h"

#include "wasmtime/include/wasmtime.h"

class WasmProgram : public Reference {
	GDCLASS(WasmProgram, Reference);

protected:
	static void _bind_methods();
	static void _print_wasmtime_error(const char* msg, wasmtime_error_t* err, wasm_trap_t *trap);

private:
	wasmtime_store_t* store;
	wasmtime_module_t* module;
	wasmtime_instance_t instance;

public:
	WasmProgram();
	~WasmProgram();

	void load_wat(String wat_text);
	void run_wat();
};


#endif // GODOT_WASMTIME_H
