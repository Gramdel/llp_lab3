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
#include <errno.h>
#include <glib.h>
#include <glib-object.h>

#include "../gen-c_glib/structs_types.h"
#include "../gen-c_glib/zgdb_service.h"
#include "../db/zgdb/query_public.h"
#include "deserializer.h"


// ================ START OF DECLARATIONS ================

G_BEGIN_DECLS

struct _ZgdbServiceHandlerImpl {
    ZgdbServiceHandler parent_instance;
};
typedef struct _ZgdbServiceHandlerImpl ZgdbServiceHandlerImpl;

struct _ZgdbServiceHandlerImplClass {
    ZgdbServiceHandlerClass parent_class;
};
typedef struct _ZgdbServiceHandlerImplClass ZgdbServiceHandlerImplClass;

GType zgdb_service_handler_impl_get_type(void);

#define TYPE_ZGDB_SERVICE_HANDLER_IMPL (zgdb_service_handler_impl_get_type())
#define ZGDB_SERVICE_HANDLER_IMPL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_ZGDB_SERVICE_HANDLER_IMPL, ZgdbServiceHandlerImpl))
#define IS_ZGDB_SERVICE_HANDLER_IMPL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_ZGDB_SERVICE_HANDLER_IMPL))
#define ZGDB_SERVICE_HANDLER_IMPL_CLASS(c) (G_TYPE_CHECK_CLASS_CAST ((c), TYPE_ZGDB_SERVICE_HANDLER_IMPL, ZgdbServiceHandlerImplClass))
#define IS_ZGDB_SERVICE_HANDLER_IMPL_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c), TYPE_ZGDB_SERVICE_HANDLER_IMPL))
#define ZGDB_SERVICE_HANDLER_IMPL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_ZGDB_SERVICE_HANDLER_IMPL, ZgdbServiceHandlerImplClass))

static gboolean
zgdb_service_handler_impl_execute(ZgdbServiceIf* iface, gchar** _return, const astNode_t* tree, GError** error);

static zgdbFile* file;

G_END_DECLS


// ================ END OF DECLARATIONS ================


// ================ START OF IMPLEMENTATION ================


G_DEFINE_TYPE (ZgdbServiceHandlerImpl, zgdb_service_handler_impl, TYPE_ZGDB_SERVICE_HANDLER)

static gboolean
zgdb_service_handler_impl_execute(ZgdbServiceIf* iface, gchar** _return, const astNode_t* tree, GError** error) {
    THRIFT_UNUSED_VAR(iface);

    GString* result = g_string_new(NULL);
    astNode_t* querySetNode = tree->right->pdata[0];
    while (querySetNode) {
        astNode_t* queryNode = querySetNode->left->pdata[0];
        query* q = deserializeQueryNode(queryNode);
        bool errorOccurred = false;
        switch (tree->type) {
            case NODE_TYPE_T_SELECT_QUERY_NODE: {
                iterator* it;
                g_string_append(result, executeSelect(file, &errorOccurred, &it, q) ? "Successful SELECT!\n"
                                                                                    : "Failed to SELECT!\n");
                while (hasNext(it)) {
                    document* doc = next(file, it);
                    GString* printedDoc = printDocument(doc);
                    g_string_append(result, printedDoc->str);
                    g_string_free(printedDoc, true);
                    destroyDocument(doc);
                }
                destroyIterator(it);
                break;
            }
            case NODE_TYPE_T_INSERT_QUERY_NODE:
                g_string_append(result, executeInsert(file, &errorOccurred, q) ? "Successful INSERT!\n"
                                                                               : "Failed to INSERT!\n");
                break;
            case NODE_TYPE_T_UPDATE_QUERY_NODE:
                g_string_append(result, executeUpdate(file, &errorOccurred, q) ? "Successful UPDATE!\n"
                                                                               : "Failed to UPDATE!\n");
                break;
            case NODE_TYPE_T_DELETE_QUERY_NODE:
                g_string_append(result, executeDelete(file, &errorOccurred, q) ? "Successful DELETE!\n"
                                                                               : "Failed to DELETE!\n");
                break;
        }
        // Отлавливаем ошибку:
        if (errorOccurred) {
            destroyQuery(q);
            g_string_free(result, true);
            g_set_error(error, THRIFT_PROTOCOL_ERROR, THRIFT_PROTOCOL_ERROR_UNKNOWN,
                        "An error occurred while executing query!\n");
            return false;
        }
        querySetNode = querySetNode->right->len ? querySetNode->right->pdata[0] : NULL;
        destroyQuery(q);
    }

    *_return = g_strdup(result->str);
    g_string_free(result, true);
    return true;
}

static void zgdb_service_handler_impl_finalize(GObject* object) {
    ZgdbServiceHandlerImpl* self = ZGDB_SERVICE_HANDLER_IMPL (object);

    /* Chain up to the parent class */
    G_OBJECT_CLASS (zgdb_service_handler_impl_parent_class)->finalize(object);
}

static void zgdb_service_handler_impl_init(ZgdbServiceHandlerImpl* self) {
    // Нечего инициализировать
}

static void zgdb_service_handler_impl_class_init(ZgdbServiceHandlerImplClass* klass) {
    GObjectClass* gobject_class = G_OBJECT_CLASS (klass);
    ZgdbServiceHandlerClass* handler_class = ZGDB_SERVICE_HANDLER_CLASS (klass);

    /* Register our destructor */
    gobject_class->finalize = zgdb_service_handler_impl_finalize;

    /* Register our implementations of CalculatorHandler's methods */
    handler_class->execute = zgdb_service_handler_impl_execute;
}


// ================ END OF IMPLEMENTATION ================


int main(int argc, char* argv[]) {
#if (!GLIB_CHECK_VERSION(2, 36, 0))
    g_type_init ();
#endif
    // Проверка на количество аргументов:
    if (argc != 3) {
        printf("Usage: <filename> <port>\n");
        return 1;
    }

    char* filename = argv[1];
    int port = (int) strtol(argv[2], NULL, 10);

    // Базовая проверка на то, то порт перевёлся в число:
    if (port <= 0 || errno) {
        fprintf(stderr, "Error: port needs to be a positive number!\n");
        return 1;
    }

    // Открываем или создаём файл базы:
    file = loadFile(filename);
    if (!file) {
        file = createFile(filename);
        if (!file) {
            fprintf(stderr, "Error: couldn't open or create file!\n");
            return 1;
        }
    }

    ZgdbServiceHandlerImpl* handler = g_object_new(TYPE_ZGDB_SERVICE_HANDLER_IMPL, NULL);
    ZgdbServiceProcessor* processor = g_object_new(TYPE_ZGDB_SERVICE_PROCESSOR, "handler", handler, NULL);
    ThriftServerTransport* server_transport = g_object_new(THRIFT_TYPE_SERVER_SOCKET, "port", port, NULL);
    ThriftTransportFactory* transport_factory = g_object_new(THRIFT_TYPE_BUFFERED_TRANSPORT_FACTORY, NULL);
    ThriftProtocolFactory* protocol_factory = g_object_new(THRIFT_TYPE_BINARY_PROTOCOL_FACTORY, NULL);
    ThriftServer* server = g_object_new(THRIFT_TYPE_SIMPLE_SERVER, "processor", processor, "server_transport",
                                        server_transport, "input_transport_factory", transport_factory,
                                        "output_transport_factory", transport_factory, "input_protocol_factory",
                                        protocol_factory, "output_protocol_factory", protocol_factory, NULL);

    printf("Welcome to ZGDB Server!\n");
    printf("Opened file \"%s\"\n", filename);
    printf("Listening on port %d...\n", port);

    GError* error = NULL;
    thrift_server_serve(server, &error);

    closeFile(file);
    g_object_unref(server);
    g_object_unref(protocol_factory);
    g_object_unref(transport_factory);
    g_object_unref(server_transport);
    g_object_unref(processor);
    g_object_unref(handler);
    return 0;
}
