#ifndef GODOT_WASMTIME_H
#define GODOT_WASMTIME_H

#include "core/reference.h"

class WasmTimeEngine : public Reference {
	GDCLASS(WasmTimeEngine, Reference);

protected:
	static void _bind_methods();

public:
	bool init_engine();

	WasmTimeEngine();
};


#endif // GODOT_WASMTIME_H
