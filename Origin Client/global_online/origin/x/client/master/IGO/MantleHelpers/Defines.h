#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
#endif    
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
#endif    
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

#ifndef SAFE_DESTROY
#define SAFE_DESTROY(p, c)     { if (p) { _grDestroyObjectInternal(p, c); (p)=NULL; } }
#endif
#ifndef SAFE_FREE
#define SAFE_FREE(p, c)        { if (p) { _grFreeMemoryInternal(p, c); (p)=NULL; } }
#endif

#define RESET_EACH_QUERY