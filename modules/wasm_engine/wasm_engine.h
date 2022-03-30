#ifndef GODOT_WASMTIME_H
#define GODOT_WASMTIME_H

#include "core/reference.h"

class WasmEngine : public Reference {
	GDCLASS(WasmEngine, Reference);

protected:
	static void _bind_methods();

public:
	bool init_engine();

	WasmEngine();
};


#endif // GODOT_WASMTIME_H
