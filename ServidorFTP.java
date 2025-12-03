import java.net.*;
import java.io.*;

public class ServidorFTP
{
  public static void main(String args[]) throws Exception
  {
    String nombreArchivo = "holamundo.pdf";
    ServerSocket serverSocket = null;
    Socket socket = null;
    System.out.println("Escuchando por el puerto 8000");
    serverSocket = new ServerSocket( 8000 );
    System.out.println("Esperando a que los clientes se conecten...");
    while(true)
    {
      socket = serverSocket.accept();
      System.out.println("Se conecto un cliente: " + socket.getInetAddress().getHostName());
      DataOutputStream dos = new DataOutputStream( socket.getOutputStream() );

      System.out.println("Paso 1: enviamos el nombre del archivo");
      dos.writeUTF( nombreArchivo );

      System.out.println("Paso 2: enviamos el tamano del archivo en bytes");
      File f = new File( nombreArchivo );
      long size = f.length();
      dos.writeUTF( Long.toString( size ) );

      System.out.println("Paso 3: enviamos el contenido del archivo");
      FileInputStream fis = new FileInputStream( nombreArchivo );
      int contador = 1;
      while( contador <= size )
      {
        int b = fis.read();
        System.out.println( "byte " + contador + ":" + b );
        dos.write( b );
        contador++;
      }
      fis.close();
      System.out.println("Cerrando la conexion");
      socket.close();
      socket = null;
    }
  }
}
