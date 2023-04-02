#ifndef _SERIALIZER_H_
#define _SERIALIZER_H_

#include "../gen-c_glib/structs_types.h"

astNode_t* serialize(nodeType* parentType, astNode* node);

#endif