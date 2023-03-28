/**
 * Autogenerated by Thrift Compiler (0.18.1)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

#include <math.h>

#include "structs_types.h"
#include <thrift/c_glib/thrift.h>

/* return the name of the constant */
const char *
toString_nodeType_t(int value) 
{
  static __thread char buf[16] = {0};
  switch(value) {
  case NODE_TYPE_T_SELECT_QUERY_NODE:return "NODE_TYPE_T_SELECT_QUERY_NODE";
  case NODE_TYPE_T_INSERT_QUERY_NODE:return "NODE_TYPE_T_INSERT_QUERY_NODE";
  case NODE_TYPE_T_UPDATE_QUERY_NODE:return "NODE_TYPE_T_UPDATE_QUERY_NODE";
  case NODE_TYPE_T_DELETE_QUERY_NODE:return "NODE_TYPE_T_DELETE_QUERY_NODE";
  case NODE_TYPE_T_NESTED_QUERY_NODE:return "NODE_TYPE_T_NESTED_QUERY_NODE";
  case NODE_TYPE_T_QUERY_SET_NODE:return "NODE_TYPE_T_QUERY_SET_NODE";
  case NODE_TYPE_T_OBJECT_NODE:return "NODE_TYPE_T_OBJECT_NODE";
  case NODE_TYPE_T_VALUES_NODE:return "NODE_TYPE_T_VALUES_NODE";
  case NODE_TYPE_T_ELEMENT_SET_NODE:return "NODE_TYPE_T_ELEMENT_SET_NODE";
  case NODE_TYPE_T_ELEMENT_NODE:return "NODE_TYPE_T_ELEMENT_NODE";
  case NODE_TYPE_T_KEY_NODE:return "NODE_TYPE_T_KEY_NODE";
  case NODE_TYPE_T_FOREIGN_KEY_NODE:return "NODE_TYPE_T_FOREIGN_KEY_NODE";
  case NODE_TYPE_T_INT_VAL_NODE:return "NODE_TYPE_T_INT_VAL_NODE";
  case NODE_TYPE_T_DOUBLE_VAL_NODE:return "NODE_TYPE_T_DOUBLE_VAL_NODE";
  case NODE_TYPE_T_BOOL_VAL_NODE:return "NODE_TYPE_T_BOOL_VAL_NODE";
  case NODE_TYPE_T_STR_VAL_NODE:return "NODE_TYPE_T_STR_VAL_NODE";
  case NODE_TYPE_T_FILTER_NODE:return "NODE_TYPE_T_FILTER_NODE";
  case NODE_TYPE_T_JOIN_NODE:return "NODE_TYPE_T_JOIN_NODE";
  case NODE_TYPE_T_OP_EQ_NODE:return "NODE_TYPE_T_OP_EQ_NODE";
  case NODE_TYPE_T_OP_NEQ_NODE:return "NODE_TYPE_T_OP_NEQ_NODE";
  case NODE_TYPE_T_OP_GT_NODE:return "NODE_TYPE_T_OP_GT_NODE";
  case NODE_TYPE_T_OP_GTE_NODE:return "NODE_TYPE_T_OP_GTE_NODE";
  case NODE_TYPE_T_OP_LE_NODE:return "NODE_TYPE_T_OP_LE_NODE";
  case NODE_TYPE_T_OP_LEE_NODE:return "NODE_TYPE_T_OP_LEE_NODE";
  case NODE_TYPE_T_OP_LIKE_NODE:return "NODE_TYPE_T_OP_LIKE_NODE";
  case NODE_TYPE_T_OP_AND_NODE:return "NODE_TYPE_T_OP_AND_NODE";
  case NODE_TYPE_T_OP_OR_NODE:return "NODE_TYPE_T_OP_OR_NODE";
  case NODE_TYPE_T_OP_NOT_NODE:return "NODE_TYPE_T_OP_NOT_NODE";
  default: g_snprintf(buf, 16, "%d", value); return buf;
  }
}

enum _value_tProperties
{
  PROP_VALUE_T_0,
  PROP_VALUE_T_INT_VAL,
  PROP_VALUE_T_DOUBLE_VAL,
  PROP_VALUE_T_BOOL_VAL,
  PROP_VALUE_T_STR_VAL
};

/* reads a value_t object */
static gint32
value_t_read (ThriftStruct *object, ThriftProtocol *protocol, GError **error)
{
  gint32 ret;
  gint32 xfer = 0;
  gchar *name = NULL;
  ThriftType ftype;
  gint16 fid;
  guint32 len = 0;
  gpointer data = NULL;
  value_t * this_object = VALUE_T(object);

  /* satisfy -Wall in case these aren't used */
  THRIFT_UNUSED_VAR (len);
  THRIFT_UNUSED_VAR (data);
  THRIFT_UNUSED_VAR (this_object);

  /* read the struct begin marker */
  if ((ret = thrift_protocol_read_struct_begin (protocol, &name, error)) < 0)
  {
    if (name) g_free (name);
    return -1;
  }
  xfer += ret;
  if (name) g_free (name);
  name = NULL;

  /* read the struct fields */
  while (1)
  {
    /* read the beginning of a field */
    if ((ret = thrift_protocol_read_field_begin (protocol, &name, &ftype, &fid, error)) < 0)
    {
      if (name) g_free (name);
      return -1;
    }
    xfer += ret;
    if (name) g_free (name);
    name = NULL;

    /* break if we get a STOP field */
    if (ftype == T_STOP)
    {
      break;
    }

    switch (fid)
    {
      case 1:
        if (ftype == T_I32)
        {
          if ((ret = thrift_protocol_read_i32 (protocol, &this_object->intVal, error)) < 0)
            return -1;
          xfer += ret;
          this_object->__isset_intVal = TRUE;
        } else {
          if ((ret = thrift_protocol_skip (protocol, ftype, error)) < 0)
            return -1;
          xfer += ret;
        }
        break;
      case 2:
        if (ftype == T_DOUBLE)
        {
          if ((ret = thrift_protocol_read_double (protocol, &this_object->doubleVal, error)) < 0)
            return -1;
          xfer += ret;
          this_object->__isset_doubleVal = TRUE;
        } else {
          if ((ret = thrift_protocol_skip (protocol, ftype, error)) < 0)
            return -1;
          xfer += ret;
        }
        break;
      case 3:
        if (ftype == T_BOOL)
        {
          if ((ret = thrift_protocol_read_bool (protocol, &this_object->boolVal, error)) < 0)
            return -1;
          xfer += ret;
          this_object->__isset_boolVal = TRUE;
        } else {
          if ((ret = thrift_protocol_skip (protocol, ftype, error)) < 0)
            return -1;
          xfer += ret;
        }
        break;
      case 4:
        if (ftype == T_STRING)
        {
          if (this_object->strVal != NULL)
          {
            g_free(this_object->strVal);
            this_object->strVal = NULL;
          }

          if ((ret = thrift_protocol_read_string (protocol, &this_object->strVal, error)) < 0)
            return -1;
          xfer += ret;
          this_object->__isset_strVal = TRUE;
        } else {
          if ((ret = thrift_protocol_skip (protocol, ftype, error)) < 0)
            return -1;
          xfer += ret;
        }
        break;
      default:
        if ((ret = thrift_protocol_skip (protocol, ftype, error)) < 0)
          return -1;
        xfer += ret;
        break;
    }
    if ((ret = thrift_protocol_read_field_end (protocol, error)) < 0)
      return -1;
    xfer += ret;
  }

  if ((ret = thrift_protocol_read_struct_end (protocol, error)) < 0)
    return -1;
  xfer += ret;

  return xfer;
}

static gint32
value_t_write (ThriftStruct *object, ThriftProtocol *protocol, GError **error)
{
  gint32 ret;
  gint32 xfer = 0;

  value_t * this_object = VALUE_T(object);
  THRIFT_UNUSED_VAR (this_object);
  if ((ret = thrift_protocol_write_struct_begin (protocol, "value_t", error)) < 0)
    return -1;
  xfer += ret;
  if (this_object->__isset_intVal == TRUE) {
    if ((ret = thrift_protocol_write_field_begin (protocol, "intVal", T_I32, 1, error)) < 0)
      return -1;
    xfer += ret;
    if ((ret = thrift_protocol_write_i32 (protocol, this_object->intVal, error)) < 0)
      return -1;
    xfer += ret;

    if ((ret = thrift_protocol_write_field_end (protocol, error)) < 0)
      return -1;
    xfer += ret;
  }
  if (this_object->__isset_doubleVal == TRUE) {
    if ((ret = thrift_protocol_write_field_begin (protocol, "doubleVal", T_DOUBLE, 2, error)) < 0)
      return -1;
    xfer += ret;
    if ((ret = thrift_protocol_write_double (protocol, this_object->doubleVal, error)) < 0)
      return -1;
    xfer += ret;

    if ((ret = thrift_protocol_write_field_end (protocol, error)) < 0)
      return -1;
    xfer += ret;
  }
  if (this_object->__isset_boolVal == TRUE) {
    if ((ret = thrift_protocol_write_field_begin (protocol, "boolVal", T_BOOL, 3, error)) < 0)
      return -1;
    xfer += ret;
    if ((ret = thrift_protocol_write_bool (protocol, this_object->boolVal, error)) < 0)
      return -1;
    xfer += ret;

    if ((ret = thrift_protocol_write_field_end (protocol, error)) < 0)
      return -1;
    xfer += ret;
  }
  if (this_object->__isset_strVal == TRUE) {
    if ((ret = thrift_protocol_write_field_begin (protocol, "strVal", T_STRING, 4, error)) < 0)
      return -1;
    xfer += ret;
    if ((ret = thrift_protocol_write_string (protocol, this_object->strVal, error)) < 0)
      return -1;
    xfer += ret;

    if ((ret = thrift_protocol_write_field_end (protocol, error)) < 0)
      return -1;
    xfer += ret;
  }
  if ((ret = thrift_protocol_write_field_stop (protocol, error)) < 0)
    return -1;
  xfer += ret;
  if ((ret = thrift_protocol_write_struct_end (protocol, error)) < 0)
    return -1;
  xfer += ret;

  return xfer;
}

static void
value_t_set_property (GObject *object,
                      guint property_id,
                      const GValue *value,
                      GParamSpec *pspec)
{
  value_t *self = VALUE_T (object);

  switch (property_id)
  {
    case PROP_VALUE_T_INT_VAL:
      self->intVal = g_value_get_int (value);
      self->__isset_intVal = TRUE;
      break;

    case PROP_VALUE_T_DOUBLE_VAL:
      self->doubleVal = g_value_get_double (value);
      self->__isset_doubleVal = TRUE;
      break;

    case PROP_VALUE_T_BOOL_VAL:
      self->boolVal = g_value_get_boolean (value);
      self->__isset_boolVal = TRUE;
      break;

    case PROP_VALUE_T_STR_VAL:
      if (self->strVal != NULL)
        g_free (self->strVal);
      self->strVal = g_value_dup_string (value);
      self->__isset_strVal = TRUE;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
value_t_get_property (GObject *object,
                      guint property_id,
                      GValue *value,
                      GParamSpec *pspec)
{
  value_t *self = VALUE_T (object);

  switch (property_id)
  {
    case PROP_VALUE_T_INT_VAL:
      g_value_set_int (value, self->intVal);
      break;

    case PROP_VALUE_T_DOUBLE_VAL:
      g_value_set_double (value, self->doubleVal);
      break;

    case PROP_VALUE_T_BOOL_VAL:
      g_value_set_boolean (value, self->boolVal);
      break;

    case PROP_VALUE_T_STR_VAL:
      g_value_set_string (value, self->strVal);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void 
value_t_instance_init (value_t * object)
{
  /* satisfy -Wall */
  THRIFT_UNUSED_VAR (object);
  object->intVal = 0;
  object->__isset_intVal = FALSE;
  object->doubleVal = 0;
  object->__isset_doubleVal = FALSE;
  object->boolVal = 0;
  object->__isset_boolVal = FALSE;
  object->strVal = NULL;
  object->__isset_strVal = FALSE;
}

static void 
value_t_finalize (GObject *object)
{
  value_t *tobject = VALUE_T (object);

  /* satisfy -Wall in case we don't use tobject */
  THRIFT_UNUSED_VAR (tobject);
  if (tobject->strVal != NULL)
  {
    g_free(tobject->strVal);
    tobject->strVal = NULL;
  }
}

static void
value_t_class_init (value_tClass * cls)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (cls);
  ThriftStructClass *struct_class = THRIFT_STRUCT_CLASS (cls);

  struct_class->read = value_t_read;
  struct_class->write = value_t_write;

  gobject_class->finalize = value_t_finalize;
  gobject_class->get_property = value_t_get_property;
  gobject_class->set_property = value_t_set_property;

  g_object_class_install_property
    (gobject_class,
     PROP_VALUE_T_INT_VAL,
     g_param_spec_int ("intVal",
                       NULL,
                       NULL,
                       G_MININT32,
                       G_MAXINT32,
                       0,
                       G_PARAM_READWRITE));

  g_object_class_install_property
    (gobject_class,
     PROP_VALUE_T_DOUBLE_VAL,
     g_param_spec_double ("doubleVal",
                          NULL,
                          NULL,
                          -INFINITY,
                          INFINITY,
                          0,
                          G_PARAM_READWRITE));

  g_object_class_install_property
    (gobject_class,
     PROP_VALUE_T_BOOL_VAL,
     g_param_spec_boolean ("boolVal",
                           NULL,
                           NULL,
                           FALSE,
                           G_PARAM_READWRITE));

  g_object_class_install_property
    (gobject_class,
     PROP_VALUE_T_STR_VAL,
     g_param_spec_string ("strVal",
                          NULL,
                          NULL,
                          NULL,
                          G_PARAM_READWRITE));
}

GType
value_t_get_type (void)
{
  static GType type = 0;

  if (type == 0) 
  {
    static const GTypeInfo type_info = 
    {
      sizeof (value_tClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) value_t_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (value_t),
      0, /* n_preallocs */
      (GInstanceInitFunc) value_t_instance_init,
      NULL, /* value_table */
    };

    type = g_type_register_static (THRIFT_TYPE_STRUCT, 
                                   "value_tType",
                                   &type_info, 0);
  }

  return type;
}

enum _astNode_tProperties
{
  PROP_AST_NODE_T_0,
  PROP_AST_NODE_T_LEFT,
  PROP_AST_NODE_T_RIGHT,
  PROP_AST_NODE_T_TYPE,
  PROP_AST_NODE_T_VAL
};

/* reads a ast_node_t object */
static gint32
ast_node_t_read (ThriftStruct *object, ThriftProtocol *protocol, GError **error)
{
  gint32 ret;
  gint32 xfer = 0;
  gchar *name = NULL;
  ThriftType ftype;
  gint16 fid;
  guint32 len = 0;
  gpointer data = NULL;
  astNode_t * this_object = AST_NODE_T(object);

  /* satisfy -Wall in case these aren't used */
  THRIFT_UNUSED_VAR (len);
  THRIFT_UNUSED_VAR (data);
  THRIFT_UNUSED_VAR (this_object);

  /* read the struct begin marker */
  if ((ret = thrift_protocol_read_struct_begin (protocol, &name, error)) < 0)
  {
    if (name) g_free (name);
    return -1;
  }
  xfer += ret;
  if (name) g_free (name);
  name = NULL;

  /* read the struct fields */
  while (1)
  {
    /* read the beginning of a field */
    if ((ret = thrift_protocol_read_field_begin (protocol, &name, &ftype, &fid, error)) < 0)
    {
      if (name) g_free (name);
      return -1;
    }
    xfer += ret;
    if (name) g_free (name);
    name = NULL;

    /* break if we get a STOP field */
    if (ftype == T_STOP)
    {
      break;
    }

    switch (fid)
    {
      case 1:
        if (ftype == T_STRUCT)
        {
          if ((ret = thrift_struct_read (THRIFT_STRUCT (this_object->left), protocol, error)) < 0)
          {
            return -1;
          }
          xfer += ret;
          this_object->__isset_left = TRUE;
        } else {
          if ((ret = thrift_protocol_skip (protocol, ftype, error)) < 0)
            return -1;
          xfer += ret;
        }
        break;
      case 2:
        if (ftype == T_STRUCT)
        {
          if ((ret = thrift_struct_read (THRIFT_STRUCT (this_object->right), protocol, error)) < 0)
          {
            return -1;
          }
          xfer += ret;
          this_object->__isset_right = TRUE;
        } else {
          if ((ret = thrift_protocol_skip (protocol, ftype, error)) < 0)
            return -1;
          xfer += ret;
        }
        break;
      case 3:
        if (ftype == T_I32)
        {
          gint32 ecast0;
          if ((ret = thrift_protocol_read_i32 (protocol, &ecast0, error)) < 0)
            return -1;
          xfer += ret;
          this_object->type = (nodeType_t)ecast0;
          this_object->__isset_type = TRUE;
        } else {
          if ((ret = thrift_protocol_skip (protocol, ftype, error)) < 0)
            return -1;
          xfer += ret;
        }
        break;
      case 4:
        if (ftype == T_STRUCT)
        {
          if ((ret = thrift_struct_read (THRIFT_STRUCT (this_object->val), protocol, error)) < 0)
          {
            return -1;
          }
          xfer += ret;
          this_object->__isset_val = TRUE;
        } else {
          if ((ret = thrift_protocol_skip (protocol, ftype, error)) < 0)
            return -1;
          xfer += ret;
        }
        break;
      default:
        if ((ret = thrift_protocol_skip (protocol, ftype, error)) < 0)
          return -1;
        xfer += ret;
        break;
    }
    if ((ret = thrift_protocol_read_field_end (protocol, error)) < 0)
      return -1;
    xfer += ret;
  }

  if ((ret = thrift_protocol_read_struct_end (protocol, error)) < 0)
    return -1;
  xfer += ret;

  return xfer;
}

static gint32
ast_node_t_write (ThriftStruct *object, ThriftProtocol *protocol, GError **error)
{
  gint32 ret;
  gint32 xfer = 0;

  astNode_t * this_object = AST_NODE_T(object);
  THRIFT_UNUSED_VAR (this_object);
  if ((ret = thrift_protocol_write_struct_begin (protocol, "astNode_t", error)) < 0)
    return -1;
  xfer += ret;
  if ((ret = thrift_protocol_write_field_begin (protocol, "left", T_STRUCT, 1, error)) < 0)
    return -1;
  xfer += ret;
  if ((ret = thrift_struct_write (THRIFT_STRUCT (this_object->left), protocol, error)) < 0)
    return -1;
  xfer += ret;

  if ((ret = thrift_protocol_write_field_end (protocol, error)) < 0)
    return -1;
  xfer += ret;
  if ((ret = thrift_protocol_write_field_begin (protocol, "right", T_STRUCT, 2, error)) < 0)
    return -1;
  xfer += ret;
  if ((ret = thrift_struct_write (THRIFT_STRUCT (this_object->right), protocol, error)) < 0)
    return -1;
  xfer += ret;

  if ((ret = thrift_protocol_write_field_end (protocol, error)) < 0)
    return -1;
  xfer += ret;
  if ((ret = thrift_protocol_write_field_begin (protocol, "type", T_I32, 3, error)) < 0)
    return -1;
  xfer += ret;
  if ((ret = thrift_protocol_write_i32 (protocol, (gint32) this_object->type, error)) < 0)
    return -1;
  xfer += ret;

  if ((ret = thrift_protocol_write_field_end (protocol, error)) < 0)
    return -1;
  xfer += ret;
  if ((ret = thrift_protocol_write_field_begin (protocol, "val", T_STRUCT, 4, error)) < 0)
    return -1;
  xfer += ret;
  if ((ret = thrift_struct_write (THRIFT_STRUCT (this_object->val), protocol, error)) < 0)
    return -1;
  xfer += ret;

  if ((ret = thrift_protocol_write_field_end (protocol, error)) < 0)
    return -1;
  xfer += ret;
  if ((ret = thrift_protocol_write_field_stop (protocol, error)) < 0)
    return -1;
  xfer += ret;
  if ((ret = thrift_protocol_write_struct_end (protocol, error)) < 0)
    return -1;
  xfer += ret;

  return xfer;
}

static void
ast_node_t_set_property (GObject *object,
                         guint property_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
  astNode_t *self = AST_NODE_T (object);

  switch (property_id)
  {
    case PROP_AST_NODE_T_LEFT:
      if (self->left != NULL)
        g_object_unref (self->left);
      self->left = g_value_dup_object (value);
      self->__isset_left = TRUE;
      break;

    case PROP_AST_NODE_T_RIGHT:
      if (self->right != NULL)
        g_object_unref (self->right);
      self->right = g_value_dup_object (value);
      self->__isset_right = TRUE;
      break;

    case PROP_AST_NODE_T_TYPE:
      self->type = g_value_get_int (value);
      self->__isset_type = TRUE;
      break;

    case PROP_AST_NODE_T_VAL:
      if (self->val != NULL)
        g_object_unref (self->val);
      self->val = g_value_dup_object (value);
      self->__isset_val = TRUE;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
ast_node_t_get_property (GObject *object,
                         guint property_id,
                         GValue *value,
                         GParamSpec *pspec)
{
  astNode_t *self = AST_NODE_T (object);

  switch (property_id)
  {
    case PROP_AST_NODE_T_LEFT:
      g_value_set_object (value, self->left);
      break;

    case PROP_AST_NODE_T_RIGHT:
      g_value_set_object (value, self->right);
      break;

    case PROP_AST_NODE_T_TYPE:
      g_value_set_int (value, self->type);
      break;

    case PROP_AST_NODE_T_VAL:
      g_value_set_object (value, self->val);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void 
ast_node_t_instance_init (astNode_t * object)
{
  /* satisfy -Wall */
  THRIFT_UNUSED_VAR (object);
  object->left = g_object_new (TYPE_AST_NODE_T, NULL);
  object->__isset_left = FALSE;
  object->right = g_object_new (TYPE_AST_NODE_T, NULL);
  object->__isset_right = FALSE;
  object->__isset_type = FALSE;
  object->val = g_object_new (TYPE_VALUE_T, NULL);
  object->__isset_val = FALSE;
}

static void 
ast_node_t_finalize (GObject *object)
{
  astNode_t *tobject = AST_NODE_T (object);

  /* satisfy -Wall in case we don't use tobject */
  THRIFT_UNUSED_VAR (tobject);
  if (tobject->left != NULL)
  {
    g_object_unref(tobject->left);
    tobject->left = NULL;
  }
  if (tobject->right != NULL)
  {
    g_object_unref(tobject->right);
    tobject->right = NULL;
  }
  if (tobject->val != NULL)
  {
    g_object_unref(tobject->val);
    tobject->val = NULL;
  }
}

static void
ast_node_t_class_init (astNode_tClass * cls)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (cls);
  ThriftStructClass *struct_class = THRIFT_STRUCT_CLASS (cls);

  struct_class->read = ast_node_t_read;
  struct_class->write = ast_node_t_write;

  gobject_class->finalize = ast_node_t_finalize;
  gobject_class->get_property = ast_node_t_get_property;
  gobject_class->set_property = ast_node_t_set_property;

  g_object_class_install_property
    (gobject_class,
     PROP_AST_NODE_T_LEFT,
     g_param_spec_object ("left",
                         NULL,
                         NULL,
                         TYPE_AST_NODE_T,
                         G_PARAM_READWRITE));

  g_object_class_install_property
    (gobject_class,
     PROP_AST_NODE_T_RIGHT,
     g_param_spec_object ("right",
                         NULL,
                         NULL,
                         TYPE_AST_NODE_T,
                         G_PARAM_READWRITE));

  g_object_class_install_property
    (gobject_class,
     PROP_AST_NODE_T_TYPE,
     g_param_spec_int ("type",
                       NULL,
                       NULL,
                       0,
                       27,
                       0,
                       G_PARAM_READWRITE));

  g_object_class_install_property
    (gobject_class,
     PROP_AST_NODE_T_VAL,
     g_param_spec_object ("val",
                         NULL,
                         NULL,
                         TYPE_VALUE_T,
                         G_PARAM_READWRITE));
}

GType
ast_node_t_get_type (void)
{
  static GType type = 0;

  if (type == 0) 
  {
    static const GTypeInfo type_info = 
    {
      sizeof (astNode_tClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ast_node_t_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (astNode_t),
      0, /* n_preallocs */
      (GInstanceInitFunc) ast_node_t_instance_init,
      NULL, /* value_table */
    };

    type = g_type_register_static (THRIFT_TYPE_STRUCT, 
                                   "astNode_tType",
                                   &type_info, 0);
  }

  return type;
}

/* constants */

enum _zgdbServiceExecuteArgsProperties
{
  PROP_ZGDB_SERVICE_EXECUTE_ARGS_0,
  PROP_ZGDB_SERVICE_EXECUTE_ARGS_TREE
};

/* reads a zgdb_service_execute_args object */
static gint32
zgdb_service_execute_args_read (ThriftStruct *object, ThriftProtocol *protocol, GError **error)
{
  gint32 ret;
  gint32 xfer = 0;
  gchar *name = NULL;
  ThriftType ftype;
  gint16 fid;
  guint32 len = 0;
  gpointer data = NULL;
  zgdbServiceExecuteArgs * this_object = ZGDB_SERVICE_EXECUTE_ARGS(object);

  /* satisfy -Wall in case these aren't used */
  THRIFT_UNUSED_VAR (len);
  THRIFT_UNUSED_VAR (data);
  THRIFT_UNUSED_VAR (this_object);

  /* read the struct begin marker */
  if ((ret = thrift_protocol_read_struct_begin (protocol, &name, error)) < 0)
  {
    if (name) g_free (name);
    return -1;
  }
  xfer += ret;
  if (name) g_free (name);
  name = NULL;

  /* read the struct fields */
  while (1)
  {
    /* read the beginning of a field */
    if ((ret = thrift_protocol_read_field_begin (protocol, &name, &ftype, &fid, error)) < 0)
    {
      if (name) g_free (name);
      return -1;
    }
    xfer += ret;
    if (name) g_free (name);
    name = NULL;

    /* break if we get a STOP field */
    if (ftype == T_STOP)
    {
      break;
    }

    switch (fid)
    {
      case 1:
        if (ftype == T_STRUCT)
        {
          if ((ret = thrift_struct_read (THRIFT_STRUCT (this_object->tree), protocol, error)) < 0)
          {
            return -1;
          }
          xfer += ret;
          this_object->__isset_tree = TRUE;
        } else {
          if ((ret = thrift_protocol_skip (protocol, ftype, error)) < 0)
            return -1;
          xfer += ret;
        }
        break;
      default:
        if ((ret = thrift_protocol_skip (protocol, ftype, error)) < 0)
          return -1;
        xfer += ret;
        break;
    }
    if ((ret = thrift_protocol_read_field_end (protocol, error)) < 0)
      return -1;
    xfer += ret;
  }

  if ((ret = thrift_protocol_read_struct_end (protocol, error)) < 0)
    return -1;
  xfer += ret;

  return xfer;
}

static gint32
zgdb_service_execute_args_write (ThriftStruct *object, ThriftProtocol *protocol, GError **error)
{
  gint32 ret;
  gint32 xfer = 0;

  zgdbServiceExecuteArgs * this_object = ZGDB_SERVICE_EXECUTE_ARGS(object);
  THRIFT_UNUSED_VAR (this_object);
  if ((ret = thrift_protocol_write_struct_begin (protocol, "zgdbServiceExecuteArgs", error)) < 0)
    return -1;
  xfer += ret;
  if ((ret = thrift_protocol_write_field_begin (protocol, "tree", T_STRUCT, 1, error)) < 0)
    return -1;
  xfer += ret;
  if ((ret = thrift_struct_write (THRIFT_STRUCT (this_object->tree), protocol, error)) < 0)
    return -1;
  xfer += ret;

  if ((ret = thrift_protocol_write_field_end (protocol, error)) < 0)
    return -1;
  xfer += ret;
  if ((ret = thrift_protocol_write_field_stop (protocol, error)) < 0)
    return -1;
  xfer += ret;
  if ((ret = thrift_protocol_write_struct_end (protocol, error)) < 0)
    return -1;
  xfer += ret;

  return xfer;
}

static void
zgdb_service_execute_args_set_property (GObject *object,
                                        guint property_id,
                                        const GValue *value,
                                        GParamSpec *pspec)
{
  zgdbServiceExecuteArgs *self = ZGDB_SERVICE_EXECUTE_ARGS (object);

  switch (property_id)
  {
    case PROP_ZGDB_SERVICE_EXECUTE_ARGS_TREE:
      if (self->tree != NULL)
        g_object_unref (self->tree);
      self->tree = g_value_dup_object (value);
      self->__isset_tree = TRUE;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
zgdb_service_execute_args_get_property (GObject *object,
                                        guint property_id,
                                        GValue *value,
                                        GParamSpec *pspec)
{
  zgdbServiceExecuteArgs *self = ZGDB_SERVICE_EXECUTE_ARGS (object);

  switch (property_id)
  {
    case PROP_ZGDB_SERVICE_EXECUTE_ARGS_TREE:
      g_value_set_object (value, self->tree);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void 
zgdb_service_execute_args_instance_init (zgdbServiceExecuteArgs * object)
{
  /* satisfy -Wall */
  THRIFT_UNUSED_VAR (object);
  object->tree = g_object_new (TYPE_AST_NODE_T, NULL);
  object->__isset_tree = FALSE;
}

static void 
zgdb_service_execute_args_finalize (GObject *object)
{
  zgdbServiceExecuteArgs *tobject = ZGDB_SERVICE_EXECUTE_ARGS (object);

  /* satisfy -Wall in case we don't use tobject */
  THRIFT_UNUSED_VAR (tobject);
  if (tobject->tree != NULL)
  {
    g_object_unref(tobject->tree);
    tobject->tree = NULL;
  }
}

static void
zgdb_service_execute_args_class_init (zgdbServiceExecuteArgsClass * cls)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (cls);
  ThriftStructClass *struct_class = THRIFT_STRUCT_CLASS (cls);

  struct_class->read = zgdb_service_execute_args_read;
  struct_class->write = zgdb_service_execute_args_write;

  gobject_class->finalize = zgdb_service_execute_args_finalize;
  gobject_class->get_property = zgdb_service_execute_args_get_property;
  gobject_class->set_property = zgdb_service_execute_args_set_property;

  g_object_class_install_property
    (gobject_class,
     PROP_ZGDB_SERVICE_EXECUTE_ARGS_TREE,
     g_param_spec_object ("tree",
                         NULL,
                         NULL,
                         TYPE_AST_NODE_T,
                         G_PARAM_READWRITE));
}

GType
zgdb_service_execute_args_get_type (void)
{
  static GType type = 0;

  if (type == 0) 
  {
    static const GTypeInfo type_info = 
    {
      sizeof (zgdbServiceExecuteArgsClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) zgdb_service_execute_args_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (zgdbServiceExecuteArgs),
      0, /* n_preallocs */
      (GInstanceInitFunc) zgdb_service_execute_args_instance_init,
      NULL, /* value_table */
    };

    type = g_type_register_static (THRIFT_TYPE_STRUCT, 
                                   "zgdbServiceExecuteArgsType",
                                   &type_info, 0);
  }

  return type;
}

enum _zgdbServiceExecuteResultProperties
{
  PROP_ZGDB_SERVICE_EXECUTE_RESULT_0,
  PROP_ZGDB_SERVICE_EXECUTE_RESULT_SUCCESS
};

/* reads a zgdb_service_execute_result object */
static gint32
zgdb_service_execute_result_read (ThriftStruct *object, ThriftProtocol *protocol, GError **error)
{
  gint32 ret;
  gint32 xfer = 0;
  gchar *name = NULL;
  ThriftType ftype;
  gint16 fid;
  guint32 len = 0;
  gpointer data = NULL;
  zgdbServiceExecuteResult * this_object = ZGDB_SERVICE_EXECUTE_RESULT(object);

  /* satisfy -Wall in case these aren't used */
  THRIFT_UNUSED_VAR (len);
  THRIFT_UNUSED_VAR (data);
  THRIFT_UNUSED_VAR (this_object);

  /* read the struct begin marker */
  if ((ret = thrift_protocol_read_struct_begin (protocol, &name, error)) < 0)
  {
    if (name) g_free (name);
    return -1;
  }
  xfer += ret;
  if (name) g_free (name);
  name = NULL;

  /* read the struct fields */
  while (1)
  {
    /* read the beginning of a field */
    if ((ret = thrift_protocol_read_field_begin (protocol, &name, &ftype, &fid, error)) < 0)
    {
      if (name) g_free (name);
      return -1;
    }
    xfer += ret;
    if (name) g_free (name);
    name = NULL;

    /* break if we get a STOP field */
    if (ftype == T_STOP)
    {
      break;
    }

    switch (fid)
    {
      case 0:
        if (ftype == T_STRING)
        {
          if (this_object->success != NULL)
          {
            g_free(this_object->success);
            this_object->success = NULL;
          }

          if ((ret = thrift_protocol_read_string (protocol, &this_object->success, error)) < 0)
            return -1;
          xfer += ret;
          this_object->__isset_success = TRUE;
        } else {
          if ((ret = thrift_protocol_skip (protocol, ftype, error)) < 0)
            return -1;
          xfer += ret;
        }
        break;
      default:
        if ((ret = thrift_protocol_skip (protocol, ftype, error)) < 0)
          return -1;
        xfer += ret;
        break;
    }
    if ((ret = thrift_protocol_read_field_end (protocol, error)) < 0)
      return -1;
    xfer += ret;
  }

  if ((ret = thrift_protocol_read_struct_end (protocol, error)) < 0)
    return -1;
  xfer += ret;

  return xfer;
}

static gint32
zgdb_service_execute_result_write (ThriftStruct *object, ThriftProtocol *protocol, GError **error)
{
  gint32 ret;
  gint32 xfer = 0;

  zgdbServiceExecuteResult * this_object = ZGDB_SERVICE_EXECUTE_RESULT(object);
  THRIFT_UNUSED_VAR (this_object);
  if ((ret = thrift_protocol_write_struct_begin (protocol, "zgdbServiceExecuteResult", error)) < 0)
    return -1;
  xfer += ret;
  if (this_object->__isset_success == TRUE) {
    if ((ret = thrift_protocol_write_field_begin (protocol, "success", T_STRING, 0, error)) < 0)
      return -1;
    xfer += ret;
    if ((ret = thrift_protocol_write_string (protocol, this_object->success, error)) < 0)
      return -1;
    xfer += ret;

    if ((ret = thrift_protocol_write_field_end (protocol, error)) < 0)
      return -1;
    xfer += ret;
  }
  if ((ret = thrift_protocol_write_field_stop (protocol, error)) < 0)
    return -1;
  xfer += ret;
  if ((ret = thrift_protocol_write_struct_end (protocol, error)) < 0)
    return -1;
  xfer += ret;

  return xfer;
}

static void
zgdb_service_execute_result_set_property (GObject *object,
                                          guint property_id,
                                          const GValue *value,
                                          GParamSpec *pspec)
{
  zgdbServiceExecuteResult *self = ZGDB_SERVICE_EXECUTE_RESULT (object);

  switch (property_id)
  {
    case PROP_ZGDB_SERVICE_EXECUTE_RESULT_SUCCESS:
      if (self->success != NULL)
        g_free (self->success);
      self->success = g_value_dup_string (value);
      self->__isset_success = TRUE;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
zgdb_service_execute_result_get_property (GObject *object,
                                          guint property_id,
                                          GValue *value,
                                          GParamSpec *pspec)
{
  zgdbServiceExecuteResult *self = ZGDB_SERVICE_EXECUTE_RESULT (object);

  switch (property_id)
  {
    case PROP_ZGDB_SERVICE_EXECUTE_RESULT_SUCCESS:
      g_value_set_string (value, self->success);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void 
zgdb_service_execute_result_instance_init (zgdbServiceExecuteResult * object)
{
  /* satisfy -Wall */
  THRIFT_UNUSED_VAR (object);
  object->success = NULL;
  object->__isset_success = FALSE;
}

static void 
zgdb_service_execute_result_finalize (GObject *object)
{
  zgdbServiceExecuteResult *tobject = ZGDB_SERVICE_EXECUTE_RESULT (object);

  /* satisfy -Wall in case we don't use tobject */
  THRIFT_UNUSED_VAR (tobject);
  if (tobject->success != NULL)
  {
    g_free(tobject->success);
    tobject->success = NULL;
  }
}

static void
zgdb_service_execute_result_class_init (zgdbServiceExecuteResultClass * cls)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (cls);
  ThriftStructClass *struct_class = THRIFT_STRUCT_CLASS (cls);

  struct_class->read = zgdb_service_execute_result_read;
  struct_class->write = zgdb_service_execute_result_write;

  gobject_class->finalize = zgdb_service_execute_result_finalize;
  gobject_class->get_property = zgdb_service_execute_result_get_property;
  gobject_class->set_property = zgdb_service_execute_result_set_property;

  g_object_class_install_property
    (gobject_class,
     PROP_ZGDB_SERVICE_EXECUTE_RESULT_SUCCESS,
     g_param_spec_string ("success",
                          NULL,
                          NULL,
                          NULL,
                          G_PARAM_READWRITE));
}

GType
zgdb_service_execute_result_get_type (void)
{
  static GType type = 0;

  if (type == 0) 
  {
    static const GTypeInfo type_info = 
    {
      sizeof (zgdbServiceExecuteResultClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) zgdb_service_execute_result_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (zgdbServiceExecuteResult),
      0, /* n_preallocs */
      (GInstanceInitFunc) zgdb_service_execute_result_instance_init,
      NULL, /* value_table */
    };

    type = g_type_register_static (THRIFT_TYPE_STRUCT, 
                                   "zgdbServiceExecuteResultType",
                                   &type_info, 0);
  }

  return type;
}

