//BSRN Projekt Alternative 1
//Shalin Jacoby
//1406548
//Sockets Server
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>


int run = 1; // Speichert den Status einer Endlosschleife
pid_t childproc_conv; //pid_t gibt PID der Prozesse zurück
pid_t childproc_stat;
pid_t childproc_log;
pid_t childproc_report;

// Signal-Handler-Funktion für "SIGINT", die die Endlosschleife beendet:
void sigint_handler(int sig)
{
    printf("Signal SIGINT erhalten\n");
    run = 0;

}
void cleanup()
{
    // Kindprozesse beenden
    kill(childproc_conv, SIGTERM);// SIGTERM: Signal um  Prozess anzuzeigen, dass er beendet werden soll
    kill(childproc_stat, SIGTERM);
    kill(childproc_log, SIGTERM);
    kill(childproc_report, SIGTERM);
}




void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]){
signal(SIGINT, sigint_handler);

    while(run) {

    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("Fehler beim Öffnen des Sockets");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = 8080;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("Fehler beim Binden des Sockets");

    listen(sockfd, 5);

    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
        error("Fehler beim Akzeptieren der Verbindung");

    // Hier ist der Prozess Conv!
    int childproc_conv = fork();
    if (childproc_conv == 0)
    {
        printf("Conv: erzeugt Zufallszahlen und sendet sie an Log und Stat\n");
        printf("Conv: PID: %i\n", getpid());

        int zufallszahlen[10];
        for (int i = 0; i < 10; i++)
        {
            zufallszahlen[i] = rand() % 100;

            //
            printf("%d ", zufallszahlen[i] = rand() % 100);
            //

        }

        n = write(newsockfd, zufallszahlen, sizeof(int) * 10);
        if (n < 0)
            error("Fehler beim Schreiben des Sockets");

        exit(0);
    }

    // Hier ist der Prozess Log!
    int childproc_log = fork();
    if (childproc_log == 0)
    {
        printf("Log: liest Messwerte von Conv und schreibt sie in einer lokalen Datei\n");
        printf("Log: PID: %i\n", getpid());

        int zufallszahlen[10];

        n = read(newsockfd, zufallszahlen, sizeof(int) * 10);
        if (n < 0)
            error("Fehler beim Lesen des Sockets");

        FILE *lokal_file;
        lokal_file = fopen("log", "w");
        if (lokal_file == 0)
        {
            printf("Fehler beim Öffnen der Datei\n");
            exit(1);
        }

        for (int i = 0; i < 10; i++)
        {
            fprintf(lokal_file, "%d\n", zufallszahlen[i]);
        }
        fclose(lokal_file);

        exit(0);
    }

    // Hier ist der Prozess Stat!
    int childproc_stat = fork();
    if (childproc_stat == 0)
    {
        printf("Stat: liest Messwerte von Conv und berechnet statistische Daten\n");
        printf("Stat: PID: %i\n", getpid());

        int zufallszahlen[10];
        float summe = 0;
        float mittelwert;

        n = read(newsockfd, zufallszahlen, sizeof(int) * 10);
        if (n < 0)
            error("Fehler beim Lesen des Sockets");

        for (int i = 0; i < 10; i++)
        {
            summe += zufallszahlen[i];
        }
        mittelwert = summe / 10;

        n = write(newsockfd, &summe, sizeof(float));
        if (n < 0)
            error("Fehler beim Schreiben des Sockets");

        n = write(newsockfd, &mittelwert, sizeof(float));
        if (n < 0)
            error("Fehler beim Schreiben des Sockets");

        exit(0);
    }

    // Hier ist der Prozess Report!
    int childproc_report = fork();
    if (childproc_report == 0)
    {
        printf("Report: greift auf Ergebnisse von Stat zu\n");
        printf("Report: PID: %i\n", getpid());

        float summe, mittelwert;

        n = read(newsockfd, &summe, sizeof(float));
        if (n < 0)
            error("Fehler beim Lesen des Sockets");
        n = read(newsockfd, &mittelwert, sizeof(float));
        if (n < 0)
            error("Fehler beim Lesen des Sockets");

        printf("\n");
        printf("Summe der Zufallszahlen: %.2f\n", summe);
        printf("Mittelwert der Zufallszahlen: %.2f\n", mittelwert);

        exit(0);
    }

    // Warten auf Kindprozesse
     wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);

    close(newsockfd);
    close(sockfd);

    }
    cleanup();
    return 0;
}

