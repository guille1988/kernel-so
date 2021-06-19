#include "mi_ram_hq.h"


t_list* listaPCB;
t_list* listaTCB;

pthread_mutex_t mutexListaTCB;
pthread_mutex_t mutexListaPCB;
pthread_mutex_t mutexListaTareas;
pthread_mutex_t mutexTripulante;

sem_t habilitarPatotaEnRam;

int main(void) {

	logMiRAM = iniciarLogger("/home/utnso/tp-2021-1c-holy-C/Mi-RAM_HQ/logMiRAM.log","Mi-Ram",1);

	logMemoria = iniciarLogger("/home/utnso/tp-2021-1c-holy-C/Mi-RAM_HQ/logMemoria.log","Memoria",1);

	cargar_configuracion();
	iniciarMemoria();

    tablasPaginasPatotas = list_create();

    pthread_mutex_init(&mutexListaTCB, NULL);
    pthread_mutex_init(&mutexListaPCB, NULL);
    pthread_mutex_init(&mutexListaTareas, NULL);
    pthread_mutex_init(&mutexTripulante, NULL);

	sem_init(&habilitarPatotaEnRam,0,1);

    int serverSock = iniciarConexionDesdeServidor(configRam.puerto);

    //Abro un thread manejar las conexiones
    pthread_t manejo_tripulante;
    pthread_create(&manejo_tripulante, NULL, (void*) atenderTripulantes, (void*) &serverSock);
    pthread_join(manejo_tripulante, (void**) NULL);

    //falta hacer funcion para destruir las tareas de la lista de tareas del pcb
    //falta hacer funcion para destruir los pcb de las lista de pcbs
    //Prototipo:

     //eliminarListaTCB(listaTCB);
     //eliminarListaPCB(listaPCB);

     return EXIT_SUCCESS;
}


void atenderTripulantes(int* serverSock) {

    while(1){

    	int* tripulanteSock = malloc(sizeof(int));
		*tripulanteSock = esperarTripulante(*serverSock);

		pthread_t t;
		//pthread_t t = malloc(sizeof(pthread_t));

		pthread_create(&t, NULL, (void*) manejarTripulante, (void*) tripulanteSock);

		//pthread_detach(t);
		pthread_join(t, (void**) NULL);
    }
}


int esperarTripulante(int serverSock) {

    struct sockaddr_in serverAddress;

    unsigned int len = sizeof(struct sockaddr);

    int socket_tripulante = accept(serverSock, (void*) &serverAddress, &len);

    log_info(logMiRAM, "Se conecto un cliente!\n");

    return socket_tripulante;
}

//CUANDO CREAS UN HILO HAY QUE PASAR SI O SI UN PUNTERO

void manejarTripulante(int *tripulanteSock) {

    t_paquete* paquete = recibirPaquete(*tripulanteSock);

    deserializarSegun(paquete,*tripulanteSock);
    free(tripulanteSock);
    //no estoy si close libera la memoria,
    //porque no recibe el puntero si no la
    //info que contiene la direccion de memoria
}


void deserializarSegun(t_paquete* paquete, int tripulanteSock){

	log_info(logMiRAM,"Deserializando el cod_op %d", paquete->codigoOperacion);

	int res;

	switch(paquete->codigoOperacion){
		case PATOTA:
			log_info(logMiRAM,"Voy a deserializar una patota");
			res = deserializarInfoPCB(paquete);
			log_info(logMiRAM,"El resultado de la operacion es: %d",res);
			if(res)
				mandarConfirmacionDisc("OK", tripulanteSock);
			else
				mandarConfirmacionDisc("ERROR", tripulanteSock);
			break;
		case TRIPULANTE:
		{
			log_info(logMiRAM,"Voy a deserializar un tripulante");
			t_tarea* tarea = deserializarTripulante(paquete);
			log_info(logMiRAM,"Se le va a mandar al tripulante la tarea de: %s",tarea->nombreTarea);
			if(tarea)
				mandarTarea(tarea, tripulanteSock);
			else
			{
				mandarTarea(tarea_error(), tripulanteSock);
			}
			break;
		}
		case EXPULSAR:
			log_info(logMiRAM,"Voy a expulsar un tripulante, todavia no esta emplementada");
//			deserializarTripulante(paquete,tripulanteSock);
			break;

		case ESTADO_TRIPULANTE:

			log_info(logMiRAM,"Voy a actualizar un tripulante");

			res = recibirActualizarTripulante(paquete);
			log_info(logMiRAM,"El resultado de la operacion es: %d",res);
			if(res)
				mandarConfirmacionDisc("OK", tripulanteSock);
			else
				mandarConfirmacionDisc("ERROR", tripulanteSock);
			break;

		case SIGUIENTE_TAREA:
		{
			log_info(logMiRAM, "Se le asigna la prox tarea a un tripulante");
			t_tarea* tarea = deserializarSolicitudTarea(paquete);
			log_info(logMiRAM,"Se le va a mandar al tripulante la tarea de: %s",tarea->nombreTarea);
			if(tarea)
				mandarTarea(tarea, tripulanteSock);
			else
			{
				mandarTarea(tarea_error(), tripulanteSock);
			}
			break;
		}
		default:
			log_info(logMiRAM,"No se puede deserializar ese tipo de estructura negro \n");
			exit(1);

		}
	eliminarPaquete(paquete);
	close(tripulanteSock);
}


void mandarConfirmacionDisc(char* aMandar, int socket) {

	t_paquete* aEnviar = armarPaqueteCon((void*) aMandar, STRING);
	log_info(logMiRAM,"Mandando confirmacion de %s a Discordiador",aMandar);
	enviarPaquete(aEnviar,socket);
}


int recibirActualizarTripulante(t_paquete* paquete) {
	tcb* nuevoTCB = malloc(sizeof(tcb));

	uint32_t idPatota;
	t_estado estado;

	void* stream = paquete->buffer->stream;

	int offset=0;

	memcpy(&(idPatota),stream,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(nuevoTCB->idTripulante),stream + offset,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(estado),stream + offset,sizeof(t_estado));
	offset += sizeof(t_estado);
	nuevoTCB->estado = asignarEstadoTripu(estado);

	memcpy(&(nuevoTCB->posX),stream + offset,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(nuevoTCB->posY),stream + offset,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	log_info(logMiRAM,"Recibi el tribulante de id %d de la  patota %d en estado %c, en pos X: %d | pos Y: %d",
			nuevoTCB->idTripulante, idPatota, nuevoTCB->estado, nuevoTCB->posX, nuevoTCB->posY);

	return actualizarTripulante(nuevoTCB,idPatota);

}


t_tarea* deserializarSolicitudTarea(t_paquete* paquete) {

	uint32_t idPatota;
	uint32_t idTripulante;
	void * stream = paquete->buffer->stream;
	int offset = 0;
	memcpy(&(idPatota), stream + offset,sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&(idTripulante), stream + offset, sizeof(uint32_t));

	return asignarProxTarea(idPatota, idTripulante);
}



/*

void setearSgteTarea(tcb *buscado){
	t_list_iterator * iterator = list_iterator_create(buscado->patota->listaTareas);
	while(list_iterator_has_next(iterator)){
		t_tarea* tarea = list_iterator_next(iterator);
		if(tarea == buscado->proximaAEjecutar){
			tarea = list_iterator_next(iterator);
			buscado->proximaAEjecutar = tarea;
			log_info(logMiRAM,"TRIPU: %d - PROX. TAREA: %s",buscado->idTripulante,tarea->nombreTarea);
			break;
		}
	}
	list_iterator_destroy(iterator);
}
*/

/*
void deserializarTareas(void* stream,t_list* listaTareas,uint32_t tamanio){

    char* string = malloc(tamanio);
    memcpy(string,stream,tamanio);

    char** arrayDeTareas = string_split(string,"\n");

    //strcmp(arrayDeTareas[i], "") != 0
    for(int i =0; arrayDeTareas[i] != NULL; i++){

    	log_info(logMiRAM,"Procesando... %s\n",arrayDeTareas[i]);
        armarTarea(arrayDeTareas[i],listaTareas);
    }

    t_tarea * tareaFinal = malloc(sizeof(t_tarea));
    tareaFinal->nombreTarea = strdup("TAREA_NULA");
	list_add(listaTareas,tareaFinal);

	liberarDoblesPunterosAChar(arrayDeTareas);
    free(string);

}*/


int deserializarInfoPCB(t_paquete* paquete) {

	pcb* nuevoPCB = malloc(sizeof(pcb));
	void* stream = paquete->buffer->stream;
	uint32_t tamanio;
	int offset = 0;

	memcpy(&(nuevoPCB->pid),stream,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(tamanio),stream + offset,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	//nuevoPCB->listaTareas = list_create();

	char* stringTareas = malloc(tamanio);
	memcpy(stringTareas,stream + offset,tamanio);

	char* tareasDelimitadas = delimitarTareas(stringTareas);

	free(stringTareas);

	sem_wait(&habilitarPatotaEnRam);
	return guardarPCB(nuevoPCB,tareasDelimitadas);

}


char* delimitarTareas(char* stringTareas) {

    char* tareasDelimitadas = string_new();

    char** arrayDeTareas = string_split(stringTareas,"\n");

    for(int i =0; arrayDeTareas[i] != NULL; i++){

        string_trim(&arrayDeTareas[i]);
        string_append(&arrayDeTareas[i],"|");
        string_append(&tareasDelimitadas, arrayDeTareas[i]);
    }

    string_append(&tareasDelimitadas,"TAREA_NULA 0;0;0;0");

    liberarDoblesPunterosAChar(arrayDeTareas);

    log_info(logMiRAM, "Tareas delimitadas: %s", tareasDelimitadas);

    return tareasDelimitadas;
}


/*
tcb* buscarTripulante(int idTCBAActualizar,pcb* patotaDeTripu) {

	bool idIgualA(tcb* tcbAComparar){

			bool a;

			a = tcbAComparar->idTripulante == idTCBAActualizar && (tcbAComparar->patota == patotaDeTripu) ;

			//log_info(logMiRAM,"Comparado con primer patota %d \n",a);

			return a;
		}

		tcb* tripulanteCorrespondiente;

		tripulanteCorrespondiente = list_find(listaTCB,(void*) idIgualA);

			if(tripulanteCorrespondiente == NULL){

			log_error(logMiRAM,"No existe ese TCB negro\n");

			exit(1);

			}
			else{

			return tripulanteCorrespondiente;
			// ASIGNAR TAREATRIPULANTE A LA PRIMERA POSICION DE LA LISTA DE LAS TAREAS DEL PCB

			}
}*/


t_tarea* deserializarTripulante(t_paquete* paquete) {

	tcb* nuevoTCB = malloc(sizeof(tcb));

	uint32_t idPatota;
	t_estado estado;

	void* stream = paquete->buffer->stream;

	int offset=0;

	memcpy(&(idPatota),stream,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(nuevoTCB->idTripulante),stream + offset,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(estado),stream + offset,sizeof(t_estado));
	offset += sizeof(t_estado);
	nuevoTCB->estado = asignarEstadoTripu(estado);

	memcpy(&(nuevoTCB->posX),stream + offset,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(nuevoTCB->posY),stream + offset,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	log_info(logMiRAM,"Recibi el tribulante de id %d de la  patota %d en estado %c",nuevoTCB->idTripulante, idPatota, nuevoTCB->estado);

	if(nuevoTCB->idTripulante == -1){
		sem_post(&habilitarPatotaEnRam);
		return tarea_error();
	}

	return guardarTCB(nuevoTCB,idPatota);

	//NOS QUEDAMOS ACA
	//BUSCAR EN RAM LA TAREA A DARLE, YA ENTRA EN JUEGO EL TEMA DE IR A BUSCAR A LAS PAGINAS SEGUN DESPLAZ ETC.
}


char asignarEstadoTripu(t_estado estado) {

	if(estado == NEW) return 'N';
	if(estado == READY) return 'R';
	if(estado == EXEC) return 'E';
	if(estado == BLOCKED) return 'B';
	else{
		log_error(logMiRAM, "El estado recibido del tripulante es invalido");
		exit(1);
	}

}



pcb* buscarPatota(uint32_t idPatotaBuscada) {

	bool idIgualA(pcb* pcbAComparar){

		bool a;

		a = pcbAComparar->pid == idPatotaBuscada;

		log_info(logMiRAM,"Comparado patota %d con patota de lista %d \n",pcbAComparar->pid,idPatotaBuscada);

		return a;
	}

	pcb* patotaCorrespondiente;

	patotaCorrespondiente = list_find(listaPCB,(void*) idIgualA);

		if(patotaCorrespondiente == NULL){

		log_error(logMiRAM,"No existe PCB para ese TCB negro\n");

		exit(1);

		}
		else{

		return patotaCorrespondiente;
		// ASIGNAR TAREATRIPULANTE A LA PRIMERA POSICION DE LA LISTA DE LAS TAREAS DEL PCB

		}
}

/*
void asignarSiguienteTarea(tcb* tripulante) {

	//Hago que su proxima tarea a ejecutar sea la primera de las lista,
	//despues hay que hablar bien como va a pedirle las tareas
	tripulante->proximaAEjecutar = list_get(tripulante->patota->listaTareas,0);


	log_info(logMiRAM,"Asignando la tarea: %s\n", tripulante->proximaAEjecutar->nombreTarea);

}
*/


void mandarTarea(t_tarea* tarea, int socketTrip) {

	t_paquete* paqueteEnviado = armarPaqueteCon((void*) tarea, TAREA);

	log_info(logMiRAM,"Tarea enviada\n");

	enviarPaquete(paqueteEnviado, socketTrip);
}


t_tarea* tarea_error() {
	t_tarea* tareaError = malloc(sizeof(t_tarea));
	tareaError->nombreTarea = "TAREA_ERROR";
	tareaError->parametro = 0;
	tareaError->posX = 0;
	tareaError->posY = 0;
	tareaError->tiempo = 0;

	return tareaError;
}







