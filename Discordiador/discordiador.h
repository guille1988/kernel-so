#ifndef EJERCICIO10_H_
#define EJERCICIO10_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <memory.h>
#include <commons/config.h>


void iniciar_conexion(void*);

void crearConfig();



typedef struct{
	char* IP;
	int puerto;
} puertoEIP;

typedef struct {
    uint32_t size; // Tamaño del payload
    void* stream; // Payload
} t_buffer;

typedef struct {
    uint8_t codigo_operacion;
    t_buffer* buffer;
} t_paquete;

typedef struct {
	uint32_t dni;
	uint8_t edad;
	uint32_t pasaporte;
	uint32_t nombre_length;
	char* nombre;
} t_persona;

typedef enum{
	PERSONA
} tipoDeDato;

void comunicarse(t_persona);

#endif /* EJERCICIO10_H_ */
