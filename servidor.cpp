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
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <map>
#include <mutex>
#include <vector>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>

using namespace std;

void cerrarServer(int s);

int cantidadMaximaConexiones = 2;
int puerto = 0;
int socketEscucha;

struct mensaje
{
    char origen[100];
    char destinatario[100];
    char titulo[100];
    char mensaje[500];
    char time[100];
};

map<string, int> usuarios;
map<string, vector<mensaje>> inboxUsuarios;
mutex usuariosMutex;
sem_t *servidor_ejecutandose = sem_open("srv_ejec", O_CREAT, 0600, 1); // para verificar que solo se ejecute 1 server
timespec tm2;

int ayuda()
{
    cout << "El servidor se ejecuta de las siguientes formas: " << endl;
    cout << "./servidor -p [numero de puerto|REQUERIDO] -c [cantidad de clientes simultaneos permitidos|OPCIONAL|default=2]" << endl;
    cout << "El orden de los parametros no tiene influencia" << endl;
    exit(1);
}

int validateParameters(int argc, const char *argv[])
{
    if (argc == 1)
    {
        cout << "Parametros Insuficientes" << endl;
        exit(1);
    }
    if (argc <= 2)
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
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--puerto") == 0)
        {
            i++;
            puerto = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--conexiones") == 0)
        {
            i++;
            cantidadMaximaConexiones = atoi(argv[i]);
        }
        else
        {
            cout << "Parametros incorrectos" << endl;
            exit(1);
        }
    }
    if (puerto == 0)
    {
        cout << "No se indico numero de puerto" << endl;
        exit(1);
    }
    return 0;
}

void hilo(int socketId)
{
    try
    {
        mensaje message;
        char usuario[100];
        int bytesRecibidos = read(socketId, &usuario, sizeof(usuario) - 1);
        if (bytesRecibidos <= 0)
        {
            // cout << "El cliente se desconecto" << endl;
            close(socketId);
            cantidadMaximaConexiones++;
        }
        else
        {
            // cout << "usuario conexion: " << usuario << socketId << endl;
            usuariosMutex.lock();
            // cout << "socketId: " << socketId << endl;
            if (usuarios[usuario]){
                close(socketId);
            }else{
                usuarios[usuario] = socketId;
            }
            // cout << "socketId dentro del map: " << usuarios[usuario] << endl;
            usuariosMutex.unlock();
            while (true)
            {
                int bytes = read(socketId, &message, sizeof(mensaje));
                if (bytes <= 0)
                {
                    // cout << "El cliente se desconecto" << endl;
                    close(socketId);
                    usuarios.erase(usuario);
                    cantidadMaximaConexiones++;
                    break;
                }
                else
                {
                    usuariosMutex.lock();
                    if (usuarios[message.destinatario])
                    {
                        // cout << message.origen << message.destinatario << message.titulo << message.mensaje << usuarios[message.destinatario] << endl;
                        send(usuarios[message.destinatario], &message, sizeof(mensaje), 0);
                    }
                    else
                    {
                        send(socketId, "1", sizeof(char), 0);
                    }
                    usuariosMutex.unlock();
                }
            }
        }
    }
    catch (exception e)
    {
        cout << "ERROR" << endl;
        cout << e.what() << endl;
    }
}

int main(int argc, const char *argv[])
{
    try
    {
        signal(SIGINT, cerrarServer);
        signal(SIGTERM, cerrarServer);
        signal(SIGPIPE, NULL);
        if (validateParameters(argc, argv) == 0)
        {
            // verificar que el server no se este ejecutando.
            clock_gettime(CLOCK_REALTIME, &tm2);
            tm2.tv_sec += 1;
            if (sem_timedwait(servidor_ejecutandose, &tm2) == -1)
            {
                cout << "El servidor ya se esta ejecutando." << endl;
                sem_close(servidor_ejecutandose);
                return 0;
            }
            pid_t pid = fork();
            if (pid != 0) // finalizar el padre, queda el hijo ejecutando en 2do plano.
                return 0;
            struct sockaddr_in serverConfig;
            memset(&serverConfig, '0', sizeof(serverConfig));
            serverConfig.sin_family = AF_INET; // 127.0.0.1
            serverConfig.sin_addr.s_addr = htonl(INADDR_ANY);
            serverConfig.sin_port = htons(puerto); // Mayor 1023
            socketEscucha = socket(AF_INET, SOCK_STREAM, 0);
            bind(socketEscucha, (struct sockaddr *)&serverConfig, sizeof(serverConfig));
            listen(socketEscucha, cantidadMaximaConexiones);
            while (true)
            {
                int socketComunicacion = accept(socketEscucha, (struct sockaddr *)NULL, NULL);
                if (cantidadMaximaConexiones > 0)
                {
                    send(socketComunicacion, "0", sizeof("0"), 0);
                    cantidadMaximaConexiones--;
                    thread th(hilo, socketComunicacion);
                    th.detach();
                }
                else
                {
                    send(socketComunicacion, "1", sizeof("1"), 0);
                    close(socketComunicacion);
                }
            }
        }
        else
        {
            return 1;
        }
    }
    catch (exception e)
    {
        cout << e.what() << endl;
        exit(1);
    }
}

void cerrarServer(int s)
{
    map<string, int>::iterator it = usuarios.begin();
    for (it; it != usuarios.end(); it++)
    {
        close(it->second);
    }
    close(socketEscucha);
    sem_unlink("srv_ejec");
    cout << "server cerrado correctamente" << endl;
    exit(0);
}