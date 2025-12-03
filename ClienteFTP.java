import java.net.*;
import java.io.*;

public class ClienteFTP
{
  public static void main(String a[]) throws Exception
  {
    Socket socket = null;
    System.out.println("Me conecto al puerto 8000 del servidor");
    socket = new Socket(a[0],8000);
    DataInputStream dis = new DataInputStream( socket.getInputStream() );

    System.out.println("Paso 1: leo el nombre del archivo");
    String nombreArchivo = dis.readUTF();
    System.out.println("Creamos un archivo nuevo con ese nombre para que se llame igual que en el servidor");
    FileOutputStream fos = new FileOutputStream( nombreArchivo );

    System.out.println("Paso 2: leo el tamano del archivo");
    long size = Long.parseLong( dis.readUTF() );

    System.out.println("Paso 3: leo el contenido del archivo byte por byte");
    int contador = 1;
    while( contador <= size )
    {
      int b = dis.read();
      System.out.println( "byte " + contador + ":" + b );
      fos.write( b );
      contador++;
    }
    fos.close();
    socket.close();
    System.out.println( "Fin de transferencia" );
  }
}