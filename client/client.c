#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <thrift/c_glib/protocol/thrift_binary_protocol.h>
#include <thrift/c_glib/transport/thrift_buffered_transport.h>
#include <thrift/c_glib/transport/thrift_socket.h>
#include <glib-object.h>
#include <glib.h>

#include "graphql_ast.h"
#include "parser.tab.h"
#include "serializer.h"

#include "../gen-c_glib/zgdb_service.h"

extern void yyset_lineno(int num);

extern void yyrestart(FILE* f);

int main(int argc, char* argv[]) {
#if (!GLIB_CHECK_VERSION (2, 36, 0))
    g_type_init();
#endif
    // Проверка на количество аргументов:
    if (argc != 3) {
        printf("Usage: <hostname> <port>\n");
        return 1;
    }

    char* hostname = argv[1];
    int port = (int) strtol(argv[2], NULL, 10);

    // Базовая проверка на то, то порт перевёлся в число:
    if (port <= 0 || errno) {
        fprintf(stderr, "Error: port needs to be a positive number!\n");
        return 1;
    }

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
        goto exit;
    }

    printf("Successfully connected!\n");

    astNode* tree = NULL;
    gchar* response = NULL;
    while (true) {
        yyset_lineno(1); // сброс счётчика строк лексера
        yyrestart(stdin); // чистка буфера (чтобы после ошибок всё было ок)
        if (yyparse(&tree) == 0) {
            // Выход по слову "exit":
            if (!tree) {
                printf("Leaving...\n");
                break;
            }
            // Отправка запроса:
            printf("Sending request...\n");
            if (zgdb_service_client_execute(client, &response, serialize(NULL, tree), &error)) {
                printf("Response:\n%s\n", response);
                g_free(response);
                response = NULL;
            } else {
                g_printerr("Error: %s (code %d)\n", error->message, error->code);
                g_clear_error(&error);
                goto exit;
            }
            destroyNode(tree);
        }
    }

    exit:
    g_object_unref(socket);
    g_object_unref(transport);
    g_object_unref(protocol);
    g_object_unref(client);
    return 0;
}
