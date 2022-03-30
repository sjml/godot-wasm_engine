#include "wasmtime_engine.h"

#include "wasmtime/include/wasm.h"
#include "wasmtime/include/wasmtime.h"

bool WasmTimeEngine::init_engine() {
	print_line("WasmTime Engine init...");

	wasm_engine_t *engine = wasm_engine_new();
	wasmtime_store_t *store = wasmtime_store_new(engine, NULL, NULL);

	return true;
}

void WasmTimeEngine::_bind_methods() {
	ClassDB::bind_method(D_METHOD("init_engine"), &WasmTimeEngine::init_engine);
}

WasmTimeEngine::WasmTimeEngine() {

}
