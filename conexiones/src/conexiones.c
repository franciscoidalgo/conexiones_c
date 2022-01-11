#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "conexiones.h"

#define SUCCESS 0
#define FAILED -1


//SERIALIZACION

buffer_t* buffer_create_fixed_size (size_t size){
    buffer_t* b = malloc(sizeof(buffer_t));
    b->size = size;
    b->next = 0;
    b->data = malloc(size);
    b->has_fixed_size = true;

    return b;
}

buffer_t* buffer_create (){
    buffer_t* b = malloc(sizeof(buffer_t));
    b->size = INITIAL_SIZE;
    b->next = 0;
    b->data = malloc(INITIAL_SIZE);
    b->has_fixed_size = false;

    return b;
}

static void reserve_space (buffer_t* b, size_t space){
    if((b->next * space) > b->size){
        if(b->has_fixed_size){
            fprintf(stderr, "Overflow in fixed-size buffer");
            exit(-1);
        }
        b->data = realloc(b->data, b->size * 2);
        b->size *= 2;
    }
}

void serialize_int (int _x, buffer_t* b){
    uint32_t x = htonl(_x);

    reserve_space(b, sizeof(x));

    memcpy(((char*)b->data) + b->next, &x, sizeof(x));
    b->next += sizeof(x);
}

int deserialize_int (buffer_t* b){
    uint32_t aux;
    int x;

    memcpy(&aux, ((char*)b->data) + b->read, sizeof(aux));
    b->read += sizeof(aux);

    x = ntohl(aux);

    return x;
}

void serialize_float (float x, buffer_t* b){ 
    reserve_space(b, sizeof(x));

    memcpy(((char*)b->data) + b->next, &x, sizeof(x));
    b->next += sizeof(x);
}

float deserialize_float (buffer_t* b){
    float x;

    memcpy(&x, ((char*)b->data) + b->read, sizeof(x));
    b->read += sizeof(x);

    return x;
}


void serialize_string(char* x, buffer_t *b){
    uint32_t lenght = strlen(x);

    reserve_space(b, lenght + sizeof(lenght));

    memcpy(((char*)b->data) + b->next, &lenght, sizeof(lenght));
    b->next += sizeof(lenght);
    memcpy(((char*)b->data) + b->next, x, lenght);
    b->next += lenght; 
}

char* deserialize_string (buffer_t *b){
    char* x;
    uint32_t lenght;

    memcpy(&lenght, ((char*)b->data) + b->read, sizeof(lenght));
    b->read += sizeof(lenght);
    x = malloc(lenght + 1);
    memcpy(x, ((char*)b->data) + b->read, lenght);
    x[lenght] = '\0';
    b->read += lenght;

    return x;
}

void add_bytes_to_buffer(void* bytes, size_t _size, buffer_t* b){
    uint32_t size = _size;
    
    reserve_space(b, size + sizeof(size));
    
    memcpy(((char*)b->data) + b->next, &size, sizeof(size));
    b->next += sizeof(size);
    memcpy(((char*)b->data) + b->next, bytes, size);
    b->next += size;
}

void* read_bytes_from_buffer (buffer_t* b){
    void* x;
    uint32_t lenght;

    memcpy(&lenght, ((char*)b->data) + b->read, sizeof(lenght));
    b->read += sizeof(lenght);
    x = malloc(lenght);
    memcpy(x, ((char*)b->data) + b->read, lenght);
    b->read += lenght;

    return x;
}

static void* serialize_buffer (buffer_t* b){
    void* x = malloc(sizeof(uint32_t) + b->next);
    uint32_t offset = 0;

    memcpy(x, &(b->next), sizeof(b->next));
    offset += sizeof(b->size);
    memcpy(((char*)x) + offset, b->data, b->next);

    return x;
}

void buffer_destroy (buffer_t* b){
    free(b->data);
    free(b);
}

void reset_read_pointer (buffer_t* b){
    b->read = 0;
}

//----------------------------------------------CONEXIONES-----------------------------------------------------------------------//

struct addrinfo* generar_info(char* ip, char* puerto) {
	struct addrinfo hints, *serv_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(ip, puerto, &hints, &serv_info) != SUCCESS)
		perror("Error generando informacion para la ip y el puerto deseados");

	return serv_info;
}

socket_t crear_conexion(char* ip, char* puerto) {
	int socket_servidor;
	struct addrinfo *serv_info = generar_info(ip, puerto);

	if ((socket_servidor = socket(serv_info->ai_family, serv_info->ai_socktype,
			serv_info->ai_protocol)) == FAILED
			|| connect(socket_servidor, serv_info->ai_addr,
					serv_info->ai_addrlen) == FAILED)
		perror("Error creando conexion");
	freeaddrinfo(serv_info);

	return socket_servidor;
}

socket_t iniciar_servidor(char* ip, char* puerto) {
	int socket_servidor;
	struct addrinfo *serv_info, *current_info;
	serv_info = generar_info(ip, puerto);

	for (current_info = serv_info; current_info != NULL; current_info =
			current_info->ai_next) {
		if ((socket_servidor = socket(current_info->ai_family,
				current_info->ai_socktype, current_info->ai_protocol)) == FAILED)
			continue;
		if (bind(socket_servidor, current_info->ai_addr,
				current_info->ai_addrlen) == FAILED) {
			close(socket_servidor);
			continue;
		}
		break;
	}
	if (listen(socket_servidor, SOMAXCONN) == FAILED)
		perror("Error iniciando el servidor");
	freeaddrinfo(serv_info);

	return socket_servidor;
}

void cerrar_conexion(socket_t socket) {
	close(socket);
}

socket_t esperar_cliente(socket_t socket_servidor) {
	int socket_cliente;
	struct sockaddr dir_cliente;
	int tam_direccion = sizeof(struct sockaddr_in);

	socket_cliente = accept(socket_servidor, &dir_cliente,
			(socklen_t*) &tam_direccion);

	return socket_cliente;
}

void enviar_buffer (buffer_t* b, socket_t socket){
    void* bytes = serialize_buffer(b);

    send(socket, bytes, sizeof(b->next) + b->next, 0);

    free(bytes);
}

buffer_t* recibir_buffer (socket_t socket){
    buffer_t* b = malloc(sizeof(buffer_t));
    b->read = 0;
    b->has_fixed_size = true;

    recv(socket, &(b->size), sizeof(b->size), MSG_WAITALL);
    b->data = malloc(b->size);
    b->next = b->size;
    recv(socket, b->data, b->size, MSG_WAITALL);

    return b;
}