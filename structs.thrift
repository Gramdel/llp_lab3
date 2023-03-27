enum __nodeType {
    SELECT_QUERY_NODE,
    INSERT_QUERY_NODE,
    UPDATE_QUERY_NODE,
    DELETE_QUERY_NODE,
    NESTED_QUERY_NODE,
    QUERY_SET_NODE,
    OBJECT_NODE,
    VALUES_NODE,
    ELEMENT_SET_NODE,
    ELEMENT_NODE,
    KEY_NODE,
    FOREIGN_KEY_NODE,
    INT_VAL_NODE,
    DOUBLE_VAL_NODE,
    BOOL_VAL_NODE,
    STR_VAL_NODE,
    FILTER_NODE,
    JOIN_NODE,
    OP_EQ_NODE,
    OP_NEQ_NODE,
    OP_GT_NODE,
    OP_GTE_NODE,
    OP_LE_NODE,
    OP_LEE_NODE,
    OP_LIKE_NODE,
    OP_AND_NODE,
    OP_OR_NODE,
    OP_NOT_NODE,
}

union __value {
    1: i32 intVal;
    2: double doubleVal;
    3: bool boolVal;
    4: string strVal;
}

struct __astNode {
    1: __astNode left;
    2: __astNode right;
    3: __nodeType type;
    4: __value val;
}