/*Este programa es un cliente de servidor HTTP, muy similar al programa anterior con la diferencia de que ahora envía la GET / HTTP/1.0<enter><enter>.*/
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

int main (int argc, char* const argv[])
{
 struct sockaddr_in servidor;
 struct hostent* hostinfo;
 char buffer[10000];

 /* Crea el socket y devuelve el file descriptor */
 int socket_fd = socket (PF_INET, SOCK_STREAM, 0);

 /* Inicializa la estructura */
 servidor.sin_family = AF_INET;
 hostinfo = gethostbyname (argv[1]);
 if (hostinfo == NULL)
  return 1;
 else
  servidor.sin_addr = *((struct in_addr *) hostinfo->h_addr);
 /* Servidor Web utiliza el puerto 80. */
 servidor.sin_port = htons (80);

 /* Se conecta al servidor */
 if (connect (socket_fd, (struct sockaddr *)&servidor, sizeof (struct sockaddr_in)) == -1) {
  perror ("connect");
  return 1;
 }

 /*
 * El cliente escribe (envia) la cabecera propia de la especificacion HTTP
 * y el servidor responde con una respuesta codigo HTML
 * 
 */
 /* Envia el comando GET / para obtener la pagina principal o index */
 sprintf (buffer, "GET / HTTP/1.0\n\n");
 write (socket_fd, buffer, strlen (buffer));

 /* Lee la respuesta del socket  */
 ssize_t num_bytes_leidos = read (socket_fd, buffer, 10000);
 printf("numero de bytes leidos=%d\n", num_bytes_leidos);

 /* Despues de leer la respuesta, se imprime a salida estandar */
 // fwrite (buffer, sizeof (char), num_bytes_leidos, stdout);
 printf("%s\n", buffer);

 close(socket_fd);
 return 0;
}
