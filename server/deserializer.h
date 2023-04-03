#ifndef _DESERIALIZER_H_
#define _DESERIALIZER_H_

#include "../gen-c_glib/structs_types.h"
#include "../db/zgdb/query.h"

element* deserializeElementNode(astNode_t* elementNode);

documentSchema* deserializeValuesNode(astNode_t* valuesNode, const char* schemaName);

condition* deserializeOperationNode(astNode_t* operationNode);

query* deserializeQueryNode(astNode_t* queryNode);

#endif