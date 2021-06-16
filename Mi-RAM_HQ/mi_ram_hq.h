#ifndef MI_RAM_HQH
#define MI_RAM_HQH

#include "memoria.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <commons/config.h>
#include <bibliotecas.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/log.h>


t_log* logMiRAM;

typedef struct {

	uint32_t pid;
	uint32_t dlTareas;

} pcb;

typedef struct {

	uint32_t idTripulante;
	t_estado estado;
	uint32_t posX;
	uint32_t posY;
	uint32_t proximaAEjecutar;
	uint32_t dlPatota;

} tcb;


void mandarConfirmacionDisc(char* , int);
void actualizarTripulante(t_paquete*);
void atenderTripulantes(int*);
int esperarTripulante(int);
void manejarTripulante(int*);
void deserializarTareas(void*,t_list*,uint32_t);
void deserializarInfoPCB(t_paquete*);
void deserializarSolicitudTarea(t_paquete*,int);
void setearSgteTarea(tcb*);
void armarTarea(char*,t_list*);
void eliminarTarea(t_tarea*);
void eliminarPCB(pcb*);
void eliminarListaPCB(t_list*);
void eliminarListaTCB(t_list*);
void eliminarTCB(tcb* tcb);
void liberarDoblesPunterosAChar(char**);
tcb* buscarTripulante(int tcbAActualizar,pcb* patotaDeTripu);
void deserializarSegun(t_paquete*, int);
void deserializarTripulante(t_paquete*,int);
pcb* buscarPatota(uint32_t);
void asignarSiguienteTarea(tcb*);
void mandarTarea(t_tarea* , int);


#endif
