//Programa de servidor web que atiende un cliente y le envía un archivo PDF
//Definición de bibliotecas
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>

#define PORT 4898 //Puerto de conexión
#define BACKLOG 1 //Número de conexiones pendientes permitidas
#define PDF_PATH "holamundo.pdf" //Ruta del PDF que leerá

int main(void) //Función main principal
{
  struct sockaddr_in servidor; //Estructura que almacena la IP y puerto del servidor
  struct sockaddr_in cliente; //Estructura que almacena la IP y puerto del cliente
  socklen_t longClient; //Longitud de la dirección del cliente
  int fd_s = -1, fd_c = -1; //Descriptores de socket el servidor y cliente
  int opt = 1; //Opción para setsockopt

  // 1) socket()
  printf("1) Creando socket del servidor...\n");
  /*Crea un socket TCP, con direcciones IPv4
  y devuelve un file descriptor para manejar el socket*/
  fd_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd_s < 0)
  {
    perror("socket"); //Imprime el error si no se crea el socket
    exit(EXIT_FAILURE); //Termina el programa si falla
  }

  // Reutilizar puerto (opcional pero util en desarrollos)
  /*Se ingresa el socket a reutilizar y se activa la opcion SO_REUSEADDR
  para permitir que el socket se vuelva a usar inmediatamente después de cerrarlo*/
  if (setsockopt(fd_s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
  {
    perror("setsockopt(SO_REUSEADDR)"); // Se avisa del error
  }

  // 2) sockaddr_in servidor
  /*memset rellena un bloque de memoria con un valor en este caso es 0
  para evitar que haya datos basura*/
  memset(&servidor, 0, sizeof(servidor));
  servidor.sin_family = AF_INET; //Direcciones de la familia IPv4
  servidor.sin_addr.s_addr = INADDR_ANY; //Acepta conexiones de cualquier Interfaz o IP
  //htons() Convierte el puerto a formato de red Network Byte Order
  //para la compartibilidad
  servidor.sin_port = htons((u_short)PORT); 
  memset(&(servidor.sin_zero), '\0', 8);//Rellena con ceros para la estructura

  // 3) bind()
  printf("2) Haciendo bind() al puerto %d...\n", PORT);
  /* Se asocia el socket a la IP y puerto especificados en struct servidor */
  if (bind(fd_s, (struct sockaddr *)&servidor, sizeof(servidor)) < 0)
  {
    perror("bind"); //Imprime el error si falla el bind
    close(fd_s); //Se cierra el socket antes de salir
    exit(EXIT_FAILURE);
  }

  // 4) listen()
  printf("3) Escuchando (listen)...\n");
  /* Se pone al socket en modo escucha por lo que esta listo para aceptar conexiones
  El BACKLOG es el numero maximo de conexiones que pueden estar en la cola */
  if (listen(fd_s, BACKLOG) < 0) {
        perror("listen");
        close(fd_s);
        exit(EXIT_FAILURE);
    }

  // 5) accept()
  longClient = sizeof(cliente);
  printf("4) Esperando conexión de un cliente...\n");
  /*Pausa el programa hasta que un cliente se conecte
  Cuando el cliente se conecta se crea un nuevo socket
  Se llena la estructura cliente con la informacion de la conexion*/
  fd_c = accept(fd_s, (struct sockaddr *)&cliente, &longClient);
  if (fd_c < 0)
  {
    perror("accept");
    close(fd_s);
    exit(EXIT_FAILURE);
  }

  // (Opcional) Resolver host del cliente
  /*Intenta encontrar el nombre del host a partir de la dirección IP del cliente*/
  struct hostent *info_cliente = gethostbyaddr((char *)&cliente.sin_addr, sizeof(struct in_addr), AF_INET);
  if (info_cliente && info_cliente->h_name)
  {
    printf("Conectado desde: %s\n", info_cliente->h_name);
  }
  else
  {
    char iptxt[INET_ADDRSTRLEN];
    /*Convierte la dirección IP de binario a texto*/
    inet_ntop(AF_INET, &cliente.sin_addr, iptxt, sizeof(iptxt));
    printf("Conectado desde IP: %s\n", iptxt);
  }

  // 6) Leer (y descartar) la solicitud HTTP del cliente (si llegó desde un navegador)
  //    No necesitamos parsear; sólo vaciamos el buffer de entrada para evitar datos pendientes.
  {
    char reqbuf[1024];
    // Usamos recv con MSG_DONTWAIT para no bloquear si no hay petición (ej. clientes "raw").
    /* Se usa MSG_DONTWAIT para no bloquear y leer la petición del HTTP del navegador*/
    ssize_t n = recv(fd_c, reqbuf, sizeof(reqbuf), MSG_DONTWAIT);
    (void)n; // ignoramos el contenido
  }

  // 7) Abrir el archivo PDF y obtener su tamaño
  /* El archivo se abre en modo lectura y devuelve un file descriptor
  para el manejo del archivo */
  int pdf_fd = open(PDF_PATH, O_RDONLY);
  if (pdf_fd < 0)
  {
    perror("open(hola_mundo.pdf)");
    // Enviar un 404 simple si no existe
    const char *hdr404 =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain; charset=utf-8\r\n"
            "Content-Length: 23\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Archivo no encontrado.\n";
    /*Se envía el error al cliente*/
    send(fd_c, hdr404, strlen(hdr404), 0);
    close(fd_c);
    close(fd_s);
    return EXIT_FAILURE;
  }


  struct stat st; // Se usa para almancenar la información del archivo
  /* Obtiene la información del archivo y se guarda en st*/
  if (fstat(pdf_fd, &st) < 0)
  {
    perror("fstat");
    close(pdf_fd);
    close(fd_c);
    close(fd_s);
    return EXIT_FAILURE;
  }
  off_t pdf_size = st.st_size; // Obtiene el tamaño del archivo PDF

  // 8) Enviar cabeceras HTTP
  char header[512];
  /* Se crea la cabecera HTTP y se guarda en header, sizeof se asegura que no se 
  escriba fuera de la memoria asignada */
  int header_len = snprintf(
  header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/pdf\r\n" //Le dice al navegador que es un PDF
        "Content-Length: %jd\r\n" // Tamaño del PDF
        "Cache-Control: no-store\r\n" // No almacenar en caché
        "Connection: close\r\n" // Avisa que se cierra la conexión al terminar
        "\r\n", //Linea obligatoria que separa cabeceras del cuerpo
        (intmax_t)pdf_size
  );
  if (header_len < 0 || header_len >= (int)sizeof(header)) {
    fprintf(stderr, "Error creando cabeceras HTTP\n");
    close(pdf_fd);
    close(fd_c);
    close(fd_s);
    return EXIT_FAILURE;
  }
  // Envía cabecera HTTP al cliente que es el navegador
  ssize_t sent = send(fd_c, header, header_len, 0);
  if (sent < 0)
  {
    perror("send(header)");
    close(pdf_fd);
    close(fd_c);
    close(fd_s);
    return EXIT_FAILURE;
  }

  // 9) Enviar el contenido del PDF (bucle robusto)
  {
    char buf[8192]; //Buffer para leer el PDF en partes
    ssize_t r; //Bytes leídos
    /*Bucle para leer el archivo PDF y enviarlo al cliente en partes*/
    while ((r = read(pdf_fd, buf, sizeof(buf))) > 0)
    {
      char *p = buf; //Puntero al inicio del buffer para recorrerlo
      ssize_t left = r; //Bytes que faltan por enviar

      /* Bucle para enviar los datos restantes */
      while (left > 0)
      {
        ssize_t w = send(fd_c, p, left, 0);
        if (w < 0) {
          if (errno == EINTR) // Si se interrumpe por una señal se reintenta
            continue;
          perror("send(data)");
          close(pdf_fd);
          close(fd_c);
          close(fd_s);
          return EXIT_FAILURE;
        }
        left -= w; // Actualiza los bytes restantes
        p += w; // Mueve el puntero hacia adelante
      }
    }
    if (r < 0) // En caso de que read falle
    {
      perror("read(pdf)");
      // cerramos igual
    }
  }

  // 10) Cerrar descriptores
  close(pdf_fd);
  printf("PDF enviado correctamente. Cerrando conexión.\n");
  close(fd_c); //Cierra el socket del cliente
  close(fd_s); //Cierra el socket del servidor
  return EXIT_SUCCESS;
}

