/* Archivo: servidor-tcp-select-setsockopt-para-2-clientes.c
 * 
 * Abre un socket TCP (ORIENTADO A CONEXIÓN) 'sock_oyente'
 * asociado a un par 'direccion-puerto' llamado 'sin_oyente',
 * y otros conectores TCP: 
 * 		
 * 		- 'sock_aceptador_1' asociado al par 
 * 		  'direccion-puerto''sin_cliente_1'
 * 		
 * 		- 'sock_aceptador_2' asociado al par
 * 		  'direccion-puerto''sin_cliente_2'
 * 		
 * 
 * Con setsockopt(2) aplica al conector 'sock_oyente' y a
 * 'sock_aceptador_1' y a 'sock_aceptador_2' en la opción 
 * 'SO_REUSEADDR' del nivel 'SOL_SOCKET' el valor de la 
 * constante 'yes' para permitir que más de una instancia 
 * del PROCESO CLIENTE TCP pueda conectarse al mismo puerto
 * en forma simultánea.
 * 
 * La comunicación bidireccional solo se logra 
 * entre un par cliente servidor, el resto de 
 * los clientes pueden conectarse 
 * pero no podrán enviar ni recibir mensajes. 
 * 
 * Una vez establecida la conexión, 
 * verifica si existen paquetes de entrada:
 * 
 * 		- por el conector 'sock_aceptador_1' 
 * 		  con 'direccion-puerto 'sin_cliente_1' 
 * ó 
 * 		- por el conector 'sock_aceptador_2' 
 * 		  con 'direccion-puerto 'sin_cliente_2' 
 * 
 * Si se reciben paquetes por el conector:
 *  
 * 		- 'sock_aceptador_1' con 
 * 		  'direccion-puerto' 'sin_cliente_1' 
 * 		  los almacena en un buffer y luego 
 * 		  lo envía al conector 'sock_aceptador_2' con
 * 		  'direccion-puerto' 'sin_cliente_2'
 *
 * Pero si se reciben paquetes por el conector:
 * 		  
 * 		- 'sock_aceptador_2' con 
 * 		  'direccion-puerto' 'sin_cliente_2' 
 * 		  los almacena en un buffer y luego 
 * 		  lo envía al conector 'sock_aceptador_1' con
 * 		  'direccion-puerto' 'sin_cliente_1'
 * 
 * Si se reciben paquetes por STDIN, 
 * NO HACE NADA.
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
#define PORT 9000
#define MAX(x,y) ((x)>(y) ? (x) : (y))
#define SIZE 1024
#define TIME 3600

/* SINONIMOS */
/* 'sad' es tipo equivalente a 'puntero a struct sockaddr' */
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
	
	int sock_oyente, sock_aceptador_1, sock_aceptador_2, cuanto, largo_1, largo_2;
	struct sockaddr_in sin_oyente, sin_cliente_1, sin_cliente_2;
	
	/* BUFFER DE 1024 BYTES, es decir 1024 CARACTERES 
	 * para los mensajes que lleguen del cliente 1, y
	 * para los mensajes que lleguen del cliente 2*/
	char mensajes_de_1[SIZE];
	char mensajes_de_2[SIZE];
	
	/* CONJUNTO DE DESCRIPTORES DE FICHERO DE ENTRADA */
	/* 1º Implementación: 'cliente 1' y 'cliente 2' son
	 * considerados como descriptores de entrada */
	fd_set in, in_original;
	
	/* ESTRUCTURA DE DATOS donde se guarda el tiempo que
	 * dura la llamada a select(2) */
	struct timeval tv;
	
	/* 'sock_oyente' es el que escucha
	 * 'sock_aceptador_1' es el que acepta y recibe desde cliente 1
	 * 'sock_aceptador_2' es el que acepta y recibe desde cliente 2 */

	/* CREA EL CONECTOR TCP: sock_oyente */
	if((sock_oyente = socket(PF_INET, SOCK_STREAM,0) ) < 0)
		error("socket_oyente");
	
	/* Familia de direcciones de 'sock_oyente' */
	sin_oyente.sin_family = AF_INET;
	/* Puerto, con bytes en orden de red, para 'sock_oyente' */
	sin_oyente.sin_port = htons(PORT);
	/* Dirección de internet, con bytes en orden de red, para 'sock_oyente' */
	sin_oyente.sin_addr.s_addr = INADDR_ANY;

	/* Aplica al conector TCP 'sock_oyente' en la opción 'SO_REUSEADDR'
	 * del nivel 'SOL_SOCKET' el valor de 'yes' */ 
	if( setsockopt(sock_oyente, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
		error("setsockopt: sock_oyente");

	/* BINDING: PUBLICACIÓN del par 'dirección-puerto'
	 * 'sin_oyente' asociado al CONECTOR TCP 'sock_oyente' */
	if( bind( sock_oyente, (sad)&sin_oyente, sizeof sin_oyente) < 0)
		error("bind: [sock_oyente:sin_oyente]");
	
	/* LISTNENING: Pone en ESCUCHA al CONETCOR TCP 'sock_oyente',
	 * 5 es el tamaño de cola TCP de espera */
	if( listen( sock_oyente, 5 ) < 0 )
		error("listen: [sock_oyente:sin_oyente]");

	largo_1 = sizeof sin_cliente_1;
	largo_2 = sizeof sin_cliente_2;
	
	
	/* ACEPTING: Por el CONECTOR 'sock_aceptador_1' ACEPTA CONEXIONES TCP 
	 * desde CONECTORES TCP 'REMOTOS' con 
	 * 'dirección-puerto-remoto' 'sin_cliente_1', 
	 * mediante 
	 * 'dirección-puerto-local' 'sin_oyente' asociado a 'sock_oyente' 
	 * O bién:  Por el CONECTOR 'sock_aceptador_2' ACEPTA CONEXIONES TCP 
	 * desde CONECTORES TCP 'REMOTOS' con 
	 * 'dirección-puerto-remoto' 'sin_cliente_2', 
	 * mediante 
	 * 'dirección-puerto-local' 'sin_oyente' asociado a 'sock_oyente' */
	 
	if((sock_aceptador_1 = accept(sock_oyente, (sad)&sin_cliente_1, &largo_1)) < 0) 	
	 	error("accept: [sock_aceptador_1:sin_cliente_1] <···> [sock_oyente:sin_oyente]");
	
	/* Aplica al CONECTOR TCP 'sock_aceptador_1' en la opción
	 * 'SO_REUSEADDR' del nivel 'SOL_SOCKET' el valor de 'yes' 
	 */
	if( setsockopt(sock_aceptador_1, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
	 	 error("setsockopt: sock_aceptador_1");
	 
	
	if((sock_aceptador_2 = accept(sock_oyente, (sad)&sin_cliente_2, &largo_1)) < 0)
		error("accept: [sock_aceptador_2:sin_cliente_2] <···> [sock_oyente:sin_oyente]");
		 
	
	/* Aplica al conector 'sock_aceptador_2' en la opción 'SO_REUSEADDR' 
	 * del nivel 'SOL_SOCKET' el valor de 'yes' */ 
	if( setsockopt(sock_aceptador_2, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
		 error("setsockopt: sock_aceptador_2");		

	/* Lo que se lea por 
	 * 	'sock_oyente' mediante 'direccion-puerto' 'sin_oyente' 
	 * 	desde 
	 * 	'sock_aceptador_1' con 'direccion-puerto-remoto' 'sin_cliente_1'
	 * 	se almacenará en el buffer 'mensajes_de_1'
	 * 	para ser enviado a través de 
	 * 	'sock_aceptador_2' con 'direccion-puerto-remoto' 'sin_cliente_2' 
	 * 	(siempre y cuando no haya nada que leer por éste)
	 */
	
	/* Lo que se lea por 
	 * 	'sock_oyente' mediante 'direccion-puerto' 'sin_oyente' 
	 * 	desde 
	 * 	'sock_aceptador_2' con 'direccion-puerto-remoto' 'sin_cliente_2'
	 * 	se almacenará en el buffer 'mensajes_de_2'
	 * 	para ser enviado a través de 
	 * 	'sock_aceptador_1' con 'direccion-puerto' 'sin_cliente_1' 
	 * 	(siempre y cuando no haya nada que leer por éste)
	 */
	
	/* Tiene select(2) */
	
	/* Limpia el conjunto de descriptores  de ficheros 'in_orig' */
	FD_ZERO(&in_original);
	
	// Añade 'sock_aceptador_1' al conjunto 'in_orig'
	FD_SET(sock_aceptador_1, &in_original);
	// Añade 'sock_aceptador_2' al conjunto 'in_orig'
	FD_SET(sock_aceptador_2, &in_original);
	
	/* Tiene 1 hora */
	// Tiempo hasta que select(2) retorne: 3600 segundos
	tv.tv_sec = TIME;
	tv.tv_usec=0;
	for(;;){
		// Copia conjunto 'in_origina' en 'in'
		memcpy(&in, &in_original, sizeof in);
		// Ve si hay algo para leer en el conjunto in
		if((cuanto = select( MAX(sock_aceptador_1,sock_aceptador_2)+1, &in, NULL, NULL, &tv))<0)
			error("select: error");
		// Si tiempo de espera de select(2) termina ==> error
		if(cuanto==0)
			error("timeout");
		
		/* Averiguamos donde hay algo para leer */
		
		// Si hay para leer desde 'sock_aceptador_1'
		if( FD_ISSET(sock_aceptador_1,&in) )
		{
			/* Lee hasta 1024 caracteres del 'sock_aceptador_1' y los pone en 'mensajes_de_1' */
			//fgets(mensajes_de_1, SIZE, sock_aceptador_1);
			if( (cuanto = read(sock_aceptador_1, mensajes_de_1, SIZE )) < 0 )
				error("read: sock_aceptador_1");
			// Si lectura devuelve 0 ==> parar ejecucion
			else if( cuanto == 0 )
				break;
			
			// Marcar fin del buffer con EL CARACTER NULO: '0'
			mensajes_de_1[cuanto]=0;
			
			/* SIEMPRE Y CUANDO NO HAYA NADA QUE LEER EN 'sock_aceptador_2' 
			 * escribe el contenido de 'mensajes_de_1' en 'sock_aceptaodor_2' */
			if( !FD_ISSET(sock_aceptador_2, &in) )
			{
				if( write(sock_aceptador_2, mensajes_de_1, strlen(mensajes_de_1) ) < 0 )
					error("write: mensajes_de_1 on sock_aceptador_2");
			}
		}
		// Si hay para leer desde 'sock_aceptador_2' 
		if( FD_ISSET(sock_aceptador_2,&in) )
		{
			/* Lee hasta 1024 caracteres del 'sock_aceptador_2' y los pone en 'mensajes_de_2' */
			if( ( cuanto = read( sock_aceptador_2, mensajes_de_2, SIZE) ) < 0 )
				error("read: sock_aceptador_2");
			// Si lectura devuelve 0 ==> parar ejecucion
			else if( cuanto == 0 )
				break;
			// Marcar fin del buffer con EL CARACTER NULO: '0'
			mensajes_de_2[cuanto]=0;
			
			/* Imprime en pantalla dirección de internet
			 * del cliente desde donde vienen datos 
			 *printf("\nDe la direccion[ %s ] : puerto[ %d ] --- llega el mensaje:\n",
			 *		inet_ntoa(sin1.sin_addr),
			 *		ntohs(sin1.sin_port));
			 *printf("%s \n",linea);
			 */
			
			/* SIEMPRE Y CUANDO NO HAYA NADA QUE LEER EN 'sock_aceptador_1' 
			 * escribe el contenido de 'mensajes_de_2' en 'sock_aceptaodor_1' */
			if( !FD_ISSET(sock_aceptador_1, &in) )
			{
				if( write(sock_aceptador_1, mensajes_de_2, strlen(mensajes_de_2) ) < 0 )
					error("write: mensajes_de_2 on sock_aceptador_1");
			}
		}
	}
	close(sock_aceptador_1); // Cierra el conector TCP 'sock_aceptador_1'
	close(sock_aceptador_2);  // Cierra el conector TCP 'sock_aceptador_2'
	close(sock_oyente);	//Cierra el conector TCP 'sock_oyente'
	return 0;     // Finalización exitosa	
}
/* Archivo: servidor-tcp-select-setsockopt-para-2-clientes.c */
