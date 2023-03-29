#include <stdlib.h>
#include <stdio.h>

#include <thrift/c_glib/protocol/thrift_binary_protocol.h>
#include <thrift/c_glib/transport/thrift_buffered_transport.h>
#include <thrift/c_glib/transport/thrift_socket.h>
#include <glib-object.h>
#include <glib.h>

#include "graphql_ast.h"
#include "parser.tab.h"
#include "convertor.h"

#include "../gen-c_glib/structs_types.h"
#include "../gen-c_glib/zgdb_service.h"

extern void yyrestart(FILE* f);

int main() {
#if (!GLIB_CHECK_VERSION (2, 36, 0))
    g_type_init();
#endif

    char* hostname = "localhost";
    int port = 9090;

    ThriftSocket* socket = g_object_new(THRIFT_TYPE_SOCKET, "hostname", hostname, "port", port, NULL);
    ThriftTransport* transport = g_object_new(THRIFT_TYPE_BUFFERED_TRANSPORT, "transport", socket, NULL);
    ThriftProtocol* protocol = g_object_new(THRIFT_TYPE_BINARY_PROTOCOL, "transport", transport, NULL);
    ZgdbServiceIf* client = g_object_new(TYPE_ZGDB_SERVICE_CLIENT, "input_protocol", protocol, "output_protocol",
                                         protocol, NULL);

    printf("Welcome to ZGDB Client!\n");
    printf("Connecting to %s:%d...\n", hostname, port);

    GError* error = NULL;
    thrift_transport_open(transport, &error);
    if (error) {
        g_printerr("Error: %s (code %d)\n", error->message, error->code);
        g_clear_error(&error);
        g_object_unref(socket);
        g_object_unref(transport);
        g_object_unref(protocol);
        g_object_unref(client);
        exit(1);
    }

    printf("Successfully connected!\n");

    astNode* tree = NULL;
    gchar* response = NULL;
    while (true) {
        yyrestart(stdin);
        if (yyparse(&tree) == 0) {
            if (zgdb_service_client_execute(client, &response, convert(tree), &error)) {
                printf("Response:\n%s\n", response);
            } else {
                g_printerr("Error: %s (code %d)\n", error->message, error->code);
            }
            destroyNode(tree);
            g_clear_error(&error);
        }
    }
    g_free(response);
}
