#include <dlfcn.h>

#include "botviamidlib.h"
#include "botviamidlibnolink.h"

int midlib()
{
	void* so_handle = dlopen("libbotviamidlibnolink.so", RTLD_LAZY);
	int (*botviamidlibnolink)() = (int (*)()) dlsym(so_handle, "botviamidlibnolink");
	
	return botviamidlib() + botviamidlibnolink();
}