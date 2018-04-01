#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ncurses.h>
#include "csapp.h"
#include "utils.h"
#define PORT 2121

void client_ls(int clientfd,rio_t rio,char* ligne){
  Rio_writen(clientfd, ligne, strlen(ligne));
  char buf2[MAXLINE];

  int i;
  int nbFiles=0;
  Rio_readn(clientfd,&nbFiles,sizeof(int));

  for(i=0;i<nbFiles;i++){
    Rio_readlineb(&rio,buf2,MAXLINE);
    if(strstr(buf2,"<dir>")!=NULL)
          printf("\033[34m%s\033[0m",buf2);
    else
      printf("%s",buf2);
  }
}

void client_get(int clientfd,rio_t rio,char* ligne){
  char buf[MAXLINE];
  char* buf2;
  int fd;
  time_t t1, t2;
  
  //envoie de l'entrée au serveur
  Rio_writen(clientfd, ligne, strlen(ligne));
    
  //enlever le \n de buf1
  // sscanf(buf,"%s",buf2);
  buf2 = strtok(ligne," \n");
  buf2 = strtok(NULL," \n");
    
  //lire message succès ou erreur
  if(Rio_readnb(&rio, buf, 3)>0){
    //char* res = strtok(buf,"\n");
    if(strcmp(buf,"ok")!=0){
      printf("Erreur: ce fichier n'existe pas.\n");
      return;
    }
  }    
  else{
    fprintf(stderr,"Serveur déconnecté.\n");
    exit(1);
  }

  //Reception taille fichier à télécharger
  int tailleTotale;
  Rio_readnb(&rio, &tailleTotale, sizeof(int));
  printf("Téléchargement de %s (%d octets).\n",buf2,tailleTotale);
    
  //Reception date de dernière modification du fichier à télécharger
  time_t dateModificationServeur;
  Rio_readnb(&rio, &dateModificationServeur, sizeof(time_t));

  /*
   *Ouverture ou création fichier:
   *    Si le client possède déjà un fichier de même nom:
   *        -si le fichier du serveur est plus récent que le fichier du client, on réécrit tout le fichier
   *        -sinon on écrit à la suite du fichier existant
   *    Sinon
   *        -on crée un nouveau fichier
   */
  int departTelechargement = 0; //Numéro de l'octet à partir duquel commencer à télécharger le fichier
  struct stat infoFichierClient; 
  if (stat(buf2, &infoFichierClient) == 0){
    //Le client possède déjà le fichier
    time_t dateModificationClient = infoFichierClient.st_mtime;
    if(difftime(dateModificationServeur,dateModificationClient)>0){
      //Le fichier du serveur est plus récent que le fichier du client => on retélécharge complètement le fichier
      fd = Open(buf2,O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
      departTelechargement = 0;
    }
    else{
      //Le fichier du serveur n'est pas plus récent que le fichier du client => on télécharge seulement la partie manquante du fichier
      fd = Open(buf2,O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
      departTelechargement = infoFichierClient.st_size;
    }
  }
  else{
    //Le client ne possède pas le fichier => on le crée
    fd = Open(buf2,O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    departTelechargement = 0;
  }

  int nbOctetsATelecharger = tailleTotale-departTelechargement;

  //Envoie du numéro de l'octet à partir duquel commencer à télécharger le fichier
  Rio_writen(clientfd, &departTelechargement, sizeof(int));

  if(nbOctetsATelecharger>0)
    printf("%d octets vont être téléchargés.\n",nbOctetsATelecharger);
  
  //temps début transfert
  t1 = time(NULL);
  int m,totalBytes=0,bufferSize;
  bufferSize = MAXLINE;//Nombre d'octets à lire à chaque Read
  while(bufferSize>nbOctetsATelecharger-totalBytes) bufferSize/=2;//On ne liera jamais + que le fichier (permet de ne pas avoir de Read bloquant)
  //lecture du fichier envoyé par le serveur, on s'arrête quand on a lu le bon nombre d'octets ou que le serveur crash
  while(totalBytes<nbOctetsATelecharger && ((m=Rio_readnb(&rio, buf, bufferSize)) > 0)){
    if(m<0){
      fprintf(stderr,"Serveur déconnecté.\n");
      exit(1);
    }
    Rio_writen(fd, buf, m);//écriture dans le fichier du client
    totalBytes+=m;//Nombre d'octets lu courrant
    //printf("(%d/%d)\n",totalBytes,nbOctetsATelecharger);
    while(bufferSize>nbOctetsATelecharger-totalBytes) bufferSize/=2;//On ne liera jamais + que le fichier (permet de ne pas avoir de Read bloquant)
  }

  //temps fin transfert
  t2 = time(NULL);
  if(nbOctetsATelecharger!=totalBytes){
    printf("Transfer incomplete.\n");
  }
  else{
    if(tailleTotale-departTelechargement==0)
      printf("Rien à télécharger.\n");
    else
      printf("Transfer successfully complete.\n");
  }
  printf("%d bytes received in %ld seconds (%d Kbytes/s)\n",totalBytes,t2-t1,totalBytes/max(1,t2-t1));
  Close(fd);
}

void parse_input(int clientfd,rio_t rio){
  char ligne[MAXLINE];
  char lignecpy[MAXLINE];
  char* buf;
  printf("\033[31mftp>\033[0m ");
  while (Fgets(ligne, MAXLINE, stdin) != NULL) {
    strcpy(lignecpy,ligne);
    buf = strtok(lignecpy," \n");
    if(buf!=NULL) {
      if(strcmp(buf,"get")==0){
	buf = strtok(NULL," \n");
	if(buf!=NULL){
	  client_get(clientfd,rio,ligne);
	}
	else{
	  printf("Utilisation: get <nom_de_fichier>\n");
	}
      }
      else if(strcmp(buf,"ls")==0){
	client_ls(clientfd,rio,ligne);
      }
      else if(strcmp(buf,"bye")==0){
	return;
      }
      else{
	printf("Commande inconnue: %s\n",buf);
      }
    }
    printf("\033[31mftp>\033[0m ");
  }
}

int main(int argc, char **argv)
{
  int clientfd;
  char *host;
  rio_t rio;
  
  if (argc != 2) {
    fprintf(stderr, "usage: %s <host>\n", argv[0]);
    exit(0);
  }
  host = argv[1];

  /*
   * Note that the 'host' can be a name or an IP address.
   * If necessary, Open_clientfd will perform the name resolution
   * to obtain the IP address.
   */
  clientfd = Open_clientfd(host, PORT);
    
  /*
   * At this stage, the connection is established between the client
   * and the server OS ... but it is possible that the server application
   * has not yet called "Accept" for this connection
   */
  printf("client connected to server OS\n"); 
    
  Rio_readinitb(&rio, clientfd);

  //lecture entrée
  parse_input(clientfd,rio);
  
  Close(clientfd);
  exit(0);
}
