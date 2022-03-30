#include "register_types.h"

#include "core/class_db.h"
#include "wasm_engine.h"

void register_wasm_engine_types() {
	ClassDB::register_class<WasmEngine>();
}

void unregister_wasm_engine_types() {

}
