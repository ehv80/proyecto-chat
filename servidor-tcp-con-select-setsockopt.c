/* Archivo: servidor-tcp-con-select-setsockopt.c
 * Abre un socket TCP (ORIENTADO A CONEXIÓN) 'sock'
 * y otro conector 'sock1'.
 * 
 * Con setsockopt(2) aplica al conector 'sock' y a
 * 'sock1' la opción 'SO_REUSEADDR' del nivel 
 * 'SOL_SOCKET' con el valor de la variable 'yes' 
 * para permitir que más de una instancia del 
 * proceso cliente pueda conectarse al mismo puerto
 * en forma simultánea. 
 * La comunicación bidireccional solo se logra 
 * entre un par cliente servidor, el resto de 
 * los clientes pueden conectarse 
 * pero no podrán enviar ni recibir mensajes. 
 * Una vez establecida la conexión, 
 * verifica si existen paquetes de entrada 
 * por el socket ó por STDIN.
 * 
 * Si se reciben paquetes por el socket, 
 * los envía a STDOUT (read(2)).
 *
 * Si se reciben paquetes por STDIN, 
 * los envía por el socket (write(2)).
*/ 

/* ARCHIVOS DE CABECERA */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/time.h>
#include<sys/select.h>

/* DEFINICIONES */
#define PORT 5000
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
int main(){
	// Para setsockopt:
	const int yes = 1;
	int sock, sock1, cuanto, largo;
	struct sockaddr_in sin, sin1;
	char linea[SIZE];
	fd_set in, in_orig;
	struct timeval tv;
	/* 'sock'  es el que escucha
	 * 'sock1' es el que acepta y recibe */
	if((sock = socket(PF_INET, SOCK_STREAM,0))<0)
		error("socket");
	// Familia de direcciones de 'sock'
	sin.sin_family=AF_INET;
	/* Puerto, con bytes en orden de red, 
	 * para 'sock' */
	sin.sin_port=htons(PORT);
	/* Dirección de internet, con bytes 
	 * en orden de red, para 'sock' */
	sin.sin_addr.s_addr=INADDR_ANY;

	/* Aplica al conector 'sock' la opción
	 * 'SO_REUSEADDR' del nivel 'SOL_SOCKET'
	 * con el valor de 'yes' */ 
	if( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
		error("setsockopt: sock");

	//publica dirección-puerto 'sin' de 'sock'
	if(bind(sock, (sad)&sin, sizeof sin) < 0)
		error("bind");
	/* Pone en escucha a 'sock',
	 * 5 es tamaño de cola espera */
	if(listen(sock, 5) < 0 )
		error("listen");

	largo = sizeof sin1;
	
	/* Aplica al conector 'sock1' la opción
	 * 'SO_REUSEADDR' del nivel 'SOL_SOCKET'
	 * con el valor de 'yes' 
	 *if( setsockopt(sock1, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
	 *	error("setsockopt: sock1");
	 */
	
	/* Por 'sock1' acepta conexiones TCP desde 
	 * dirección remota 'sin1', mediante 
	 * dirección local 'sin' de 'sock' */
	 if((sock1 = accept(sock, (sad)&sin1, &largo)) < 0) 	
	 	error("accept");
	 
		 
	/* Aplica al conector 'sock1' la opción
	 * 'SO_REUSEADDR' del nivel 'SOL_SOCKET'
	 * con el valor de 'yes' */ 
	if( setsockopt(sock1, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
		 error("setsockopt: sock1");		
	

	/* Lo que se lea de STDIN mediante dirección 'sin' de 'sock', 
	 * se escribirá en 'sock1' usando la dirección-puerto 'sin1' */
	
	/* Lo que se lea por 'sock1' desde dirección-puerto remotos 'sin1', 
	 * se escribirá en STDOUT */
	
	/* Tiene select(2) */
	/* Limpia el conjunto de descriptores 
	 * de ficheros 'in_orig' */
	FD_ZERO(&in_orig);
	// Añade STDIN al conjunto 'in_orig'
	FD_SET(0, &in_orig);
	// Añade 'sock1' al conjunto 'in_orig'
	FD_SET(sock1, &in_orig);
	
	/* Tiene 1 hora */
	// Tiempo hasta que select(2) retorne: 3600 segundos
	tv.tv_sec = TIME;
	tv.tv_usec=0;
	for(;;){
		// Copia conjunto 'in_orig' en 'in'
		memcpy(&in, &in_orig, sizeof in);
		// Ve si hay algo para leer en el conjunto in
		if((cuanto=select(MAX(0,sock1)+1,&in,NULL,NULL,&tv))<0)
			error("select: error");
		// Si tiempo de espera de select(2) termina ==> error
		if(cuanto==0)
			error("timeout");
		/* Averiguamos donde hay algo para leer */
		// Si hay para leer desde STDIN
		if(FD_ISSET(0,&in)){
			/* Lee hasta 1024 caracteres de STDIN,
			 * los pone en 'linea' */
			fgets(linea, SIZE, stdin);
			// Escribe contenido de 'linea' en 'sock1'
			if( write(sock1, linea, strlen(linea)) < 0 )
				error("write");
		}
		// Si hay para leer desde 'sock1' remoto
		if(FD_ISSET(sock1,&in)){
			/* Lee hasta 1024 caracteres de 'sock1',
			 * y los pone en 'linea' */
			if( (cuanto = read(sock1,linea,1024)) < 0 )
				error("read");
			// Si lectura devuelve 0 ==> parar ejecucion
			else if( cuanto == 0 )
				break;
			// Marcar fin del buffer con '0'
			linea[cuanto]=0;
			/* Imprime en pantalla dirección de internet
			 * del cliente desde donde vienen datos */
			printf("\nDe la direccion[ %s ] : puerto[ %d ] --- llega el mensaje:\n",
					inet_ntoa(sin1.sin_addr),
					ntohs(sin1.sin_port));
			printf("%s \n",linea);
		}
	}
	close(sock1); // Cierra el conector TCP 'sock1'
	close(sock);  // Cierra el conector TCP 'sock'
	return 0;     // Finalización exitosa	
}
/* Archivo: servidor-tcp-con-select-setsockopt.c */
