/* #################################################################
##                      APL 2
##                  Ejercicio 5
## Corrales Mauro Exequiel 40137650
## Di Tommaso Giuliano 38695645
## Gramajo Hector Marcelo 34519918
################################################################# */

#include <stdlib.h>
#include <string>
#include <iostream>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <queue>
#include <thread>
#include <signal.h>

using namespace std;

string user = "";
int puerto = 0;
string servidor = "";
int socketComunicacion;
bool fix = true;

struct mensaje
{
    char origen[100];
    char destinatario[100];
    char titulo[100];
    char mensaje[500];
    char time[100];
};

deque<mensaje> inbox;

string tiempo()
{
    chrono::time_point t = chrono::system_clock::now();
    time_t t_time_t = chrono::system_clock::to_time_t(t);
    char tmp[20];
    strftime(tmp, sizeof(tmp), "%d/%m/%Y %H:%M:%S", localtime(&t_time_t));
    return string(tmp);
}

int ayuda()
{
    cout << "El cliente se ejecuta de las siguientes formas:" << endl;
    cout << "./cliente -u [usuario] -p [puerto del servidor] -s [ip del servidor]" << endl;
    cout << "El orden de los parametros no importa" << endl;
    cout << "Luego de iniciarse, quedara en modo interactivo y se podra interactuar mediante los siguientes comandos" << endl;
    cout << "SEND [USUARIO] [TITULO] \n [MENSAJE] \n ." << endl;
    cout << "El comando SEND enviara un mensaje al usuario indicado con el titulo indicado. Para finalizar y enviar el mensaje se tiene que mandar una linea con solamente un punto" << endl;
    cout << "Restricciones de tamaños : USUARIO <= 100 carac. ; TITULO <= 100 carac. ; MENSAJE <= 500 carac" << endl;
    cout << "LIST" << endl;
    cout << "El comando LIST listara todos los mensajes pendientes de lectura que se encuentran disponibles" << endl;
    cout << "READ [NRO_MENSAJE] deplegara el mensaje seleccionado para ser leido.Una vez leido el mensaje se eliminara del buzon de entrada" << endl;
    cout << "El comando READ enviara un mensaje al usuario indicado con el titulo indicado. Para finalizar y enviar el mensaje se tiene que mandar una linea con solamente un punto" << endl;
    cout << "EXIT" << endl;
    cout << "El comando EXIT cerrara el cliente" << endl;

    exit(0);
}

int hiloEscucha()
{
    while (true)
    {
        try
        {
            int bytesRecibidos = 0;
            mensaje message;
            bytesRecibidos = read(socketComunicacion, &message, sizeof(mensaje));
            if (bytesRecibidos > 0)
            {
                // cout << message.origen << message.destinatario << message.titulo << message.mensaje << endl;
                inbox.push_back(message);
            }
            else
            {
                cout << "El server se desconecto" << endl;
                close(socketComunicacion);
                exit(1);
            }
        }
        catch (exception e)
        {
            // cout << "arroje excepcion" << endl;
            // cout << e.what() << endl;
            break;
        }
    }
    return 0;
}

int validateParameters(int argc, const char *argv[])
{
    if (argc == 1)
    {
        cout << "Parametros Insuficientes" << endl;
        return 1;
    }
    if (argc < 7)
    {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
        {
            ayuda();
        }
        else
        {
            cout << "Parametros Incorrectos" << endl;
            exit(1);
        }
    }
    for (int i = 1; i <= argc - 1; i++)
    {
        if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--user") == 0)
        {
            i++;
            user = argv[i];
            // cout << user << endl;
        }
        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--puerto") == 0)
        {
            i++;
            puerto = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--servidor") == 0)
        {
            i++;
            servidor = argv[i];
        }
        else
        {
            cout << "Parametros incorrectos" << endl;
            exit(1);
        }
    }
    if (user.empty() || puerto == 0 || servidor.empty())
    {
        cout << "No se setearon correctamente los parametros. utilice -h o --help para conocer el funcionamiento del cliente" << endl;
        return 1;
    }

    return 0;
}

int validarComando(string buffer)
{
    string comando = buffer.substr(0, 4);
    //ignorar mayuscula/minuscula
    for (size_t i = 0; i < sizeof(comando); i++)
    {
        comando[i] = toupper(comando[i]);
    }
    string buffMensaje;
    if (comando == "SEND")
    {
        int i = buffer.find_first_of(" ", 5UL);
        string dest = buffer.substr(5UL, i - 5UL);
        int i2 = buffer.find_first_of("\n", i + 1);
        string titulo = buffer.substr(i + 1);
        do
        {
            getline(cin, buffer);
            if (buffer != ".")
            {
                buffMensaje += buffer + '\n';
            }
        } while (buffer != ".");

        // cout << buffMensaje << endl;

        mensaje message = {.origen = "", .destinatario = "", .titulo = "", .mensaje = "", .time = "23123123"};
        strcpy(message.origen, user.c_str());
        strcpy(message.destinatario, dest.c_str());
        strcpy(message.titulo, titulo.c_str());
        strcpy(message.mensaje, buffMensaje.c_str());
        // cout << tiempo().c_str() << endl;
        strcpy(message.time, tiempo().c_str());

        send(socketComunicacion, &message, sizeof(mensaje), 0);
        /* char codResp;
        read(socketComunicacion, &codResp, sizeof(char)); */
        /*  if (codResp != '0')
         {
             cout << "ocurrio un error. Por favor envie el mensaje nuevamente" << endl;
         } */
    }
    else if (comando == "LIST")
    {
        if (fix){
            inbox.pop_front();
            fix = false;
        }
        if (!inbox.empty())
        {
            int index = 0;
            cout << "nro\t\tRemitente\t\tEnviado\t\tTitulo" << endl;
            for (deque<mensaje>::iterator it = inbox.begin(); it != inbox.end(); it++)
            {
                index++;
                cout << index << "\t\t" << it->origen << "\t\t\t" << it->time << "\t" << it->titulo << endl;
            }
        }
        else
        {
            cout << "No hay mensajes disponibles :(" << endl;
        }
    }
    else if (comando == "READ")
    {
        try
        {
            int i = buffer.find_first_of(" ", 5UL);
            int nro = atoi(buffer.substr(5UL, i - 5UL).c_str()) - 1;
            mensaje mens = inbox.at(nro);

            cout << "Remitente : " << mens.origen << endl;
            cout << "Enviado : " << mens.time << endl;
            cout << "Titulo : " << mens.titulo << endl;
            cout << "Mensaje : " << mens.mensaje << endl;
            inbox.erase(inbox.begin() + nro);
        }
        catch (exception e)
        {
            cout << "no se pudo acceder al mensaje" << endl;
        }
    }
    else if (comando == "EXIT")
    {
        shutdown(socketComunicacion, SHUT_RDWR);
        close(socketComunicacion);
        exit(0);
    }
    else
    {
        cout << "comando desconocido" << endl;
    }
    return 0;
}

int main(int argc, const char *argv[])
{
    signal(SIGINT, SIG_IGN); // salir mediante comando exit unicamente.
    signal(SIGTERM, SIG_IGN);
    if (validateParameters(argc, argv) == 0)
    {
        struct sockaddr_in socketConfig;
        memset(&socketConfig, '0', sizeof(socketConfig));

        socketConfig.sin_family = AF_INET;
        socketConfig.sin_port = htons(puerto);
        inet_pton(AF_INET, servidor.c_str(), &socketConfig.sin_addr);

        socketComunicacion = socket(AF_INET, SOCK_STREAM, 0);

        int resultadoConexion = connect(socketComunicacion,
                                        (struct sockaddr *)&socketConfig, sizeof(socketConfig));
        if (resultadoConexion < 0)
        {
            cout << "Error en la conexión" << resultadoConexion << endl;
            return EXIT_FAILURE;
        }

        read(socketComunicacion, &resultadoConexion, sizeof(char));

        if (resultadoConexion == '1')
        {
            cout << "No se puede conectar con el servidor" << endl;
            exit(1);
        }

        thread th(hiloEscucha);
        // cout << user << endl;
        send(socketComunicacion, user.c_str(), sizeof(user), 0);

        string buff;
        while (true)
        {
            cout << "Ingrese comando a ejecutar" << endl;
            getline(cin, buff);
            validarComando(buff);
        }
        th.join();
        close(socketComunicacion);
        cout << "cerrando cliente" << endl;
        return 0;
    }
    else
    {
        return 1;
    }
}