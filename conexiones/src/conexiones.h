/*
 * conexiones.h
 *
 *  Created on: 12 oct. 2021
 *      Author: utnso
 */

#ifndef CONEXIONES_H_
#define CONEXIONES_H_

#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#define INITIAL_SIZE 32

typedef struct {
	uint32_t size;
    uint32_t next;
    uint32_t read;
    void* data;
    bool has_fixed_size;
} buffer_t;

//SERIALIZACION

/**
 * @brief Crea un buffer mas eficiente en el uso de memoria gracias al tamaño fijo size
 * 
 * @param size Tamaño esperado de los datos del buffer (no la metadata)
 * @return buffer_t* 
 */
buffer_t* buffer_create_fixed_size (size_t size);


/**
 * @brief Crea un buffer de tamaño inicial INITIAL_SIZE cuyo tamaño ira aumentando de ser necesario
 * 
 * @return buffer_t* 
 */
buffer_t* buffer_create ();


/**
 * @brief Serializa un valor entero _x y lo agrega al buffer b
 * 
 * @param _x 
 * @param b 
 */
void serialize_int (int _x, buffer_t* b);


/**
 * @brief Deserializa y retorna un valor entero del buffer b e incrementa el puntero de lectura
 * 
 * @param b 
 * @return int 
 */
int deserialize_int (buffer_t* b);


/**
 * @brief Serializa un valor _x con punto flotante _x y lo agrega al buffer b 
 * @note PUEDE FALLAR ENTRE DISTINTAS ARQUITECTURAS
 * @param _x 
 * @param b 
 */
void serialize_float (float _x, buffer_t* b);


/**
 * @brief Deserializa y retorna un valor con punto flotante del buffer b e incrementa el puntero de lectura
 * @note PUEDE FALLAR ENTRE DISTINTAS ARQUITECTURAS
 * @param b 
 * @return float 
 */
float deserialize_float (buffer_t* b);


/**
 * @brief Serializa un string x (terminado en caracter nulo) y lo agrega al buffer b
 * 
 * @param x 
 * @param b 
 */
void serialize_string(char* x, buffer_t *b);


/**
 * @brief Deserializa y retorna un string del buffer b e incrementa el puntero de lectura
 * 
 * @param b 
 * @return char* 
 */
char* deserialize_string (buffer_t *b);


/**
 * @brief Agrega _size bytes al buffer b
 * @note usar para más flexibilidad
 * @param bytes 
 * @param _size 
 * @param b 
 */
void add_bytes_to_buffer(void* bytes, size_t _size, buffer_t* b);


/**
 * @brief lee una cadena de bytes del buffer b e incrementa el puntero de lectura
 * @note usar para más flexibilidad
 * @param b 
 * @return void* 
 */
void* read_bytes_from_buffer (buffer_t* b);


/**
 * @brief Libera la memoria ocupada por el buffer b
 * 
 * @param b 
 */
void buffer_destroy (buffer_t* b);


/**
 * @brief Resetea el puntero de lectura del buffer b
 * 
 */
void reset_read_pointer (buffer_t* b);


//CONEXIONES
int crear_conexion(char* ip, char* puerto);
int iniciar_servidor(char* ip, char* puerto);
int esperar_cliente(int socket_servidor);
void cerrar_conexion(int socket);

void enviar_buffer (buffer_t* b, int socket);
buffer_t* recibir_buffer (int socket);

#endif