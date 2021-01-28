/* Archivo: cliente-tcp-con-select-setsockopt.c 
 * Abre un socket TCP (ORIENTADO A CONEXIÓN) 'sock'.
 *
 * Mediante setsockopt(2) aplica a la opción de nivel
 * 'SOL_SOCKET' llamada 'SO_REUSEADDR' el valor de
 * la constante 'yes' para el conector TCP 'sock'.
 * 
 * Inicia una conexión TCP hacia un servidor
 * TCP remoto. Una vez conectado, verifica 
 * si existen paquetes de entrada 
 * por el socket ó por STDIN.
 *
 * Si se reciben paquetes por el socket, 
 * los envía a STDOUT (read(2)).
 * 
 * Si se reciben paquetes por STDIN, 
 * los envía por el socket (write(2)).
 **/

/* ARCHIVOS DE CABECERA */
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/time.h>
#include<sys/select.h>
#include<string.h>

/* DEFINICIONES */
#define PORT 9000
#define MAX(x,y) ((x)>(y) ? (x) : (y))
#define SIZE 1024
#define TIME 3600
				   
/* SINONIMOS */
typedef struct sockaddr *sad;

/* FUNCIONES */
void error(char *s){
    perror(s);
    exit(-1);
}

/* FUNCION PRINCIPAL MAIN */
int main(int argc, char **argv){
	
	if(argc < 2){
		fprintf(stderr,"usa: %s ipaddr \n", argv[0]);
		exit(-1);
	}
	
	/* Por 'sock' va a iniciar conexion TCP a un servidor TCP remoto */
	
	const int yes = 1; // Para setsockopt(2)
	
	int sock, cuanto;
	
	// Direccion de internet para 'sock'
	struct sockaddr_in sin;
	
	// Buffer de 1024 caracteres (un arreglo)
	char linea[SIZE];
	
	// Conjuntos de descriptores de ficheros
	fd_set in, in_orig;
	
	/* Tiempo limite que debe transcurrir antes que select(2) retorne */
	struct timeval tv;
	
	// Abre un conector TCP 'sock'
	if((sock = socket(PF_INET, SOCK_STREAM,0))<0)
		error("socket");
	
	/* Familia de direcciones a la que pertenece 'sin', para 'sock' */
	sin.sin_family = AF_INET;
	
	// Puerto de 'sin', para 'sock'
	sin.sin_port = htons(PORT);
	
	/* Transforma argv[1] 
	 * (la direccion IP pasada como argumento)
	 * desde formato 'ascii' 
	 * (puntero a cadena de caracteres)
	 * al formato 'network', y 
	 * guarda en la 'struct in_addr sin.sin_addr'
	 * que a su vez es miembro de la 
	 * 'struct sockaddr_in sin'.
	 * Se trata de la direccion IP del servidor 
	 * al que se quiere conectar.
	 **/
	inet_aton( argv[1], &sin.sin_addr );

	/* Mediante setsockopt(2) aplica a la opción de nivel
	 * 'SOL_SOCKET' llamada 'SO_REUSEADDR' el valor de la 
	 * constante 'yes' para el conector 'sock'. */
	if( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
		error("setsockopt");
	
	/* 'sock' inicia la conexion TCP hacia la dirección remota 'sin.sin_addr' */
	if( connect(sock, (sad)&sin, sizeof sin)<0)
	    error("conect");
	
	/* Tiene select(2) */
	
	/* Limpia el conjunto de descriptores de fichero 'in_orig' */
	FD_ZERO(&in_orig);
	/* Añade al conjuto 'in_orig' el  descriptor STDIN: 0 */
	FD_SET(0, &in_orig);
	/* Añade al conjunto 'in_orig' el  descriptor 'sock' */
	FD_SET(sock,&in_orig);
	
	/* Tiene 1 hora */
	/* Tiempo hasta que select(2) regrese: 3600 segundos */
	tv.tv_sec=TIME;
	tv.tv_usec=0;
	
	for(;;){
		/* Copia contenido del conjunto  'in_orig' en 'in' */
		memcpy(&in, &in_orig, sizeof in);
		// Ve si hay algo en el conjunto 'in'
		if((cuanto = select( MAX(0,sock)+1, &in, NULL, NULL, &tv))<0)
			error("select");
		/* Si el tiempo de espera para que select(2) retorne expira */
		if(cuanto==0)
			error("timeout");

		/* averiguamos donde hay algo para leer*/

		// Si hay para leer desde STDIN
		if(FD_ISSET(0,&in)){
			/* Lee hasta 1024 caracteres de STDIN y los pone en 'linea' */
			fgets(linea, SIZE, stdin);
			/* Escribe el contenido  de 'linea' en sock */
			if( write(sock, linea, strlen(linea)) < 0 )
				error("write: linea on sock");
		}
		//Si hay para leer desde sock
		if(FD_ISSET(sock,&in)){
			/* Lee hasta 1024 caracteres  de 'sock' y los pone en 'linea' */
			if( (cuanto = read(sock, linea, SIZE) ) < 0)
				error("read: from sock to linea"); // Error al leer
			else if( cuanto==0 ) break;	
			/* Si lectura devuelve 0 entonces parar la ejecucion */
			
			// Marca el final del buffer con EL CARACTER NULO: '0'
			linea[cuanto]=0;
			
			/* Imprime en pantalla la direccion del
			 * servidor desde donde vienen datos
			 *printf("\nDe la direccion[ %s ] : puerto [ %d ] --- llega el mensaje:\n",
			 * 		inet_ntoa(sin.sin_addr),
			 *		ntohs(sin.sin_port));
			 */
			
			/* Imprime en pantalla lo que vino 
			 * desde 'sock' mediante buffer 'linea' */
			printf("%s \n",linea);
		}
	}
	close(sock);
	return 0;	//Finalización exitosa
}
/* Fin Archivo: cliente-tcp-con-select-setsockopt.c */
