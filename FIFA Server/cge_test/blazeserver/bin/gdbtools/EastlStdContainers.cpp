//Undef this so we don't pull in everything from Blaze
#undef EASTL_USER_CONFIG_HEADER

#include "EASTL/list.h"
#include "EASTL/vector.h"
#include "EASTL/deque.h"
#include "EASTL/stack.h"
#include "EASTL/set.h"
#include "EASTL/map.h"
#include "EASTL/hash_map.h"
#include "EASTL/hash_set.h"

using namespace eastl;

typedef ListNode<void* > _EastlVoidPtrListNodeType;
_EastlVoidPtrListNodeType _EastlVoidPtrListNode;

typedef rbtree_node<void*> _EastlVoidPtrSetNodeBaseType;
typedef rbtree_node<pair<void*, void*> > _EastlVoidPtrMapNodeBaseType;
_EastlVoidPtrSetNodeBaseType _EastlVoidPtrSetNodeBase;
_EastlVoidPtrMapNodeBaseType _EastlVoidPtrMapNodeBase;
