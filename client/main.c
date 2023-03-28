#include <stdlib.h>
#include <stdio.h>

#include <thrift/c_glib/protocol/thrift_binary_protocol.h>
#include <thrift/c_glib/transport/thrift_buffered_transport.h>
#include <thrift/c_glib/transport/thrift_socket.h>
#include <glib-object.h>
#include <glib.h>

#include "graphql_ast.h"
#include "parser.tab.h"

#include "../gen-c_glib/zgdb_service.h"

int main() {
    #if (!GLIB_CHECK_VERSION (2, 36, 0))
        g_type_init();
    #endif

    ThriftSocket* socket = g_object_new (THRIFT_TYPE_SOCKET, "hostname", "localhost", "port", 9090, NULL);
    ThriftTransport* transport = g_object_new (THRIFT_TYPE_BUFFERED_TRANSPORT, "transport", socket, NULL);
    ThriftProtocol* protocol  = g_object_new (THRIFT_TYPE_BINARY_PROTOCOL, "transport", transport, NULL);
    ZgdbServiceIf* client = g_object_new (TYPE_ZGDB_SERVICE_CLIENT, "input_protocol",  protocol, "output_protocol", protocol, NULL);
    GError *error = NULL;

    thrift_transport_open (transport, &error);
    if (error) {
        printf("error\n");
        g_clear_error(&error);
        exit(-1);
    }

    astNode* tree;
    gchar* response = NULL;
    while (yyparse(&tree) == 0) {
        printNode(tree, 0);
        if (zgdb_service_client_execute(client, &response, tree, &error)) {
            printf("%s\n", response);
        } else {
            printf("ERROR!");
        }
        g_object_unref(response);
        g_clear_error(&error);
    }
    printf("ERROR!");
    destroyNode(tree);
    g_object_unref(socket);
    g_object_unref(transport);
    g_object_unref(protocol);
    g_object_unref(client);
	return 0;
}
