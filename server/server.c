#include <thrift/c_glib/thrift.h>
#include <thrift/c_glib/protocol/thrift_binary_protocol_factory.h>
#include <thrift/c_glib/protocol/thrift_protocol_factory.h>
#include <thrift/c_glib/server/thrift_server.h>
#include <thrift/c_glib/server/thrift_simple_server.h>
#include <thrift/c_glib/transport/thrift_buffered_transport_factory.h>
#include <thrift/c_glib/transport/thrift_server_socket.h>
#include <thrift/c_glib/transport/thrift_server_transport.h>

#include <stdio.h>
#include <stdbool.h>
#include <glib.h>
#include <glib-object.h>

#include "../gen-c_glib/structs_types.h"
#include "../gen-c_glib/zgdb_service.h"


// ================ START OF DECLARATIONS ================

G_BEGIN_DECLS

struct _ZgdbServiceHandlerImpl
{
  ZgdbServiceHandler parent_instance;
};
typedef struct _ZgdbServiceHandlerImpl ZgdbServiceHandlerImpl;

struct _ZgdbServiceHandlerImplClass
{
  ZgdbServiceHandlerClass parent_class;
};
typedef struct _ZgdbServiceHandlerImplClass ZgdbServiceHandlerImplClass;

GType zgdb_service_handler_impl_get_type (void);
#define TYPE_ZGDB_SERVICE_HANDLER_IMPL (zgdb_service_handler_impl_get_type())
#define ZGDB_SERVICE_HANDLER_IMPL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_ZGDB_SERVICE_HANDLER_IMPL, ZgdbServiceHandlerImpl))
#define IS_ZGDB_SERVICE_HANDLER_IMPL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_ZGDB_SERVICE_HANDLER_IMPL))
#define ZGDB_SERVICE_HANDLER_IMPL_CLASS(c) (G_TYPE_CHECK_CLASS_CAST ((c), TYPE_ZGDB_SERVICE_HANDLER_IMPL, ZgdbServiceHandlerImplClass))
#define IS_ZGDB_SERVICE_HANDLER_IMPL_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c), TYPE_ZGDB_SERVICE_HANDLER_IMPL))
#define ZGDB_SERVICE_HANDLER_IMPL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_ZGDB_SERVICE_HANDLER_IMPL, ZgdbServiceHandlerImplClass))

static gboolean zgdb_service_handler_impl_execute (ZgdbServiceIf *iface, gchar ** _return, const astNode_t * tree, GError **error);

G_END_DECLS


// ================ END OF DECLARATIONS ================


// ================ START OF IMPLEMENTATION ================


G_DEFINE_TYPE (ZgdbServiceHandlerImpl,
               zgdb_service_handler_impl,
               TYPE_ZGDB_SERVICE_HANDLER)

static gboolean zgdb_service_handler_impl_execute (ZgdbServiceIf *iface, gchar ** _return, const astNode_t * tree, GError **error) {
    THRIFT_UNUSED_VAR(iface);

    *_return = "TEST";

    return true;
}

static void
zgdb_service_handler_impl_finalize (GObject *object)
{
  ZgdbServiceHandlerImpl *self =
    ZGDB_SERVICE_HANDLER_IMPL (object);

  /* Chain up to the parent class */
  G_OBJECT_CLASS (zgdb_service_handler_impl_parent_class)->
    finalize (object);
}

/* TutorialCalculatorHandler's instance initializer (constructor) */
static void
zgdb_service_handler_impl_init (ZgdbServiceHandlerImpl *self)
{
  // Нечего инициализировать
}

/* TutorialCalculatorHandler's class initializer */
static void
zgdb_service_handler_impl_class_init (ZgdbServiceHandlerImplClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ZgdbServiceHandlerClass *handler_class =
    ZGDB_SERVICE_HANDLER_CLASS (klass);

  /* Register our destructor */
  gobject_class->finalize = zgdb_service_handler_impl_finalize;

  /* Register our implementations of CalculatorHandler's methods */
  handler_class->execute = zgdb_service_handler_impl_execute;
}


// ================ END OF IMPLEMENTATION ================


int main(int argc, char *argv[]) {
    #if (!GLIB_CHECK_VERSION(2, 36, 0))
        g_type_init ();
    #endif

    const int port = 9090;

    ZgdbServiceHandlerImpl *handler = g_object_new(TYPE_ZGDB_SERVICE_HANDLER_IMPL, NULL);
    ZgdbServiceProcessor *processor = g_object_new(TYPE_ZGDB_SERVICE_PROCESSOR, "handler", handler, NULL);
    ThriftServerTransport *server_transport = g_object_new(THRIFT_TYPE_SERVER_SOCKET, "port", port, NULL);
    ThriftTransportFactory *transport_factory = g_object_new(THRIFT_TYPE_BUFFERED_TRANSPORT_FACTORY, NULL);
    ThriftProtocolFactory *protocol_factory = g_object_new(THRIFT_TYPE_BINARY_PROTOCOL_FACTORY, NULL);
    ThriftServer *server = g_object_new(THRIFT_TYPE_SIMPLE_SERVER,
                          "processor", processor,
                          "server_transport", server_transport,
                          "input_transport_factory", transport_factory,
                          "output_transport_factory", transport_factory,
                          "input_protocol_factory", protocol_factory,
                          "output_protocol_factory", protocol_factory,
                          NULL);

    printf("Listening on port: %d\n", port);
    GError *error = NULL;
    thrift_server_serve(server, &error);

    g_object_unref(server);
    g_object_unref(protocol_factory);
    g_object_unref(transport_factory);
    g_object_unref(server_transport);

    g_object_unref(processor);
    g_object_unref(handler);

    return 0;
}
