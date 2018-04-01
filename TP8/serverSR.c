#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include "csapp.h"
#include "utils.h"

#define MAX_NAME_LEN 256
#define NPROC 3
#define PORT 2121
#define BLOC_SIZE 20

void server_ls(int connfd){

  struct dirent **files;
  int n;
  int i;
  char buf[MAXLINE];

  n = scandir(".",&files,0,alphasort);//TODO: à modifier par rapport au repertoire courant
  if(n<0) return;
  Rio_writen(connfd,&n,sizeof(int));

  int t,tmax=0;
  for(i=0;i<n;i++){
    t=strlen(files[i]->d_name);
    if(t>tmax) tmax=t;
  }
  
  for(i=0;i<n;i++){
    strncpy(buf,files[i]->d_name,MAXLINE-100);
    sprintf(buf,"%-*s",tmax,files[i]->d_name);
    if(files[i]->d_type!=DT_DIR)
      sprintf(buf,"%s [%d]",buf,file_size(files[i]->d_name));
    else
      sprintf(buf,"%s <dir>",buf);
    Rio_writen(connfd,buf,strlen(buf));
    Rio_writen(connfd,"\n",1);
    free(files[i]);
  }
  free(files);
}

//Envoie d'un fichier au client
void server_get(int connfd,rio_t rio,char* fileName)
{
  int fd;
  size_t m;
  char buf3[BLOC_SIZE];
  rio_t riof;

  fd=open(fileName,O_RDONLY,0);
      
  //Si impossible d'ouvrir fichier(n'existe pas), envoie erreur
  if(fd<0){
    Rio_writen(connfd, "NOT_FOUND\n", 10);
    printf("Erreur: %s n'existe pas\n", fileName);
    return;
  }
  //sinon envoie ok
  Rio_writen(connfd, "ok", 3);

  //envoie au client la taille du fichier à télécharger
  int tailleTotale = file_size(fileName);
  printf("Accès: %s (%d octets).\n", fileName,tailleTotale);
  Rio_writen(connfd, &tailleTotale, sizeof(int));

  //envoie au client de la date de dernière modification du fichier
  time_t dateModification;
  dateModification = file_date_fd(fd);
  Rio_writen(connfd, &dateModification, sizeof(time_t));
  
  //reception du numéro de l'octet à partir duquel envoyer le fichier au client (le client ne retéléchargera pas la partie du fichier qu'il possède déjà)
  int departEnvoie = 0;
  Rio_readnb(&rio, &departEnvoie, sizeof(int));
  
  Rio_readinitb(&riof, fd);
  int i;
  char tmp;
  //on se place au bon endroit dans le fichier 
  for(i=0;i<departEnvoie;i++){
    Rio_readnb(&riof,&tmp,1);
  }

  //Envoie du fichier
  printf("Envoie de %d octets...\n",tailleTotale-departEnvoie);
  while((m=Rio_readnb(&riof, buf3, BLOC_SIZE)) != 0){
    if(rio_writen(connfd, buf3, m)<0){
      break;
    }
  }
  printf("Envoie Terminé.\n");
  Close(fd);
  return;
}

void server_interpreter(int connfd){
  char ligne[MAXLINE];
  char* buf;
  rio_t rio;
  int n;

  Rio_readinitb(&rio, connfd);

  //Lecture des commandes
  while ((n = Rio_readlineb(&rio, ligne, MAXLINE)) != 0) {
    buf = strtok(ligne," \n");
    if(strcmp(buf,"get")==0){
      buf=strtok(NULL," \n");
      if(buf!=NULL){
	server_get(connfd,rio,buf);
      }
      else{
	printf("Utilisation: get <nom_de_fichier>\n");
      }
    }
    else if(strcmp(buf,"ls")==0){
      server_ls(connfd);
    }
    else if(strcmp(buf,"bye")==0){
      return;
    }
    else{
      printf("Commande inconnue: %s\n",buf);
    }
    
  }
}

void handler1()
{
  pid_t pid;
  while((pid = waitpid(-1, NULL, WNOHANG)) > 0){//on traite tous les signaux recus
    printf("Handler reaped child %d\n", (int)pid);
  }
  return;
}

/* 
 * Note that this code only works with IPv4 addresses
 * (IPv6 is not supported)
 */
int main()
{
  int listenfd, connfd,i;
  socklen_t clientlen;
  struct sockaddr_in clientaddr;
  char client_ip_string[INET_ADDRSTRLEN];
  char client_hostname[MAX_NAME_LEN];

  Signal(SIGCHLD, handler1);
    
  clientlen = (socklen_t)sizeof(clientaddr);

  listenfd = Open_listenfd(PORT);
  
  for(i=0;i<NPROC;i++){
    if(fork()==0){
      while (1) {
	connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
	/* determine the name of the client */
	Getnameinfo((SA *) &clientaddr, clientlen,client_hostname, MAX_NAME_LEN, 0, 0, 0);

	/* determine the textual representation of the client's IP address */
	Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,INET_ADDRSTRLEN);
	
	printf("server connected to %s (%s)\n", client_hostname,client_ip_string);

	/********************/
	//TODO: Envoie du répertoire courant avec pwd
	/*char cwd[MAXLINE];
	getcwd(cwd,sizeof(cwd));
	int l=strlen(cwd);
	Rio_writen(connfd,&l,sizeof(int));
	Rio_writen(connfd,cwd,l);*/
	
	//Traitement des commandes
	server_interpreter(connfd);
	/*******************/
	Close(connfd);
	printf("server closed to %s (%s)\n\n", client_hostname,client_ip_string);
      }
      exit(0); //ne devrait pas être exécuté
    }
  }
  //boucle du père
  while(1);
  return 0;
}

