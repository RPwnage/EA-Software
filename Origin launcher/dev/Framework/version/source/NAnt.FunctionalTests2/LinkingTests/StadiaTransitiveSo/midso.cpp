#include <dlfcn.h>

#include "botviamidso.h"
#include "botviamidsonolink.h"

__attribute__((visibility("default"))) int midso()
{
	void* so_handle = dlopen("libbotviamidsonolink.so", RTLD_LAZY);
	int (*botviamidsonolink)() = (int (*)()) dlsym(so_handle, "botviamidsonolink");
	
	return botviamidso() + botviamidsonolink();
}