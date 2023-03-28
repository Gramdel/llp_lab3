THRIFT := `pkg-config --cflags --libs thrift_c_glib`

all: server

clean:
	rm -rf server

server: server.c
	gcc ../gen-c_glib/structs_types.c ../gen-c_glib/zgdb_service.c server.c $(THRIFT) -o server

.PHONY: all clean server
