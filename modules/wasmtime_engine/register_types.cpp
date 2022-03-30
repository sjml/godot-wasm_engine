#include "register_types.h"

#include "core/class_db.h"
#include "wasmtime_engine.h"

void register_wasmtime_engine_types() {
	ClassDB::register_class<WasmTimeEngine>();
}

void unregister_wasmtime_engine_types() {

}
