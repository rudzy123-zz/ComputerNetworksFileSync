#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>

#define BUFLEN 524288      /* buffer length */
#define SERVER_TCP_PORT 3000  /* well-known port */
/*Rudy's Use CS5651 UMD File Syncronization*/
int serve(int);
void reaper(int);

int main(int argc, char *argv[]) {
  int    sd, new_sd, port, client_len;
  struct sockaddr_in client, server; // client addr
                                                                           
  switch(argc){
    case 1:
      port = SERVER_TCP_PORT;
      break;
    case 2:
      port = atoi(argv[1]);
      break;
    default:
      fprintf(stderr, "Usage: %d [port]\n", argv[0]);
      exit(1);
    }

  /* Create a stream socket */  
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr, "Can't creat a socket\n");
    exit(1);
  }

  /* Bind an address to the socket  */
  bzero((char *)&server, sizeof(struct sockaddr_in));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
    fprintf(stderr, "Can't bind name to socket\n");
    exit(1);
  }

  /* queue up to 5 connect requests  */
  listen(sd, 5);

  (void) signal(SIGCHLD, reaper);

  while(1) 
  {
    client_len = sizeof(client);
    new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
    printf("Server created.\n\n");

    if(new_sd < 0)
    {
      fprintf(stderr, "Can't accept client \n");
      exit(1);
    }
    switch (fork())
    {
    case 0:   /* child */
    (void) close(sd);
    exit(serve(new_sd));
    default:    /* parent */
    (void) close(new_sd);
    break;
    case -1:
    fprintf(stderr, "fork: error\n");
    }
   }  
}
int serve(int sd)
{ 
  struct stat fstat;    //Used for file statistics
  int n, bytes_to_write, i, j;
  DIR *dir;     //Directory
  struct dirent *entry;   //Directory entry

  struct PDU
  {
    char type;
    int  length;
    char data[BUFLEN];
    
  } rpdu, spdu;

 while(1) 
{
    rpdu.type = 'X'; //Set PDU type to unknown. This avoids looping errors caused by           //the switch statement

    //Receive the client's choice of action
    recv(sd, (char*)&rpdu, sizeof(rpdu), 0);    
    printf("Received a %c type PDU.\n", rpdu.type);





    //Perform the requested operation based on their choice
    switch (rpdu.type)
    { 
     case 'D': //-- Client download request;
     printf("Client has requested to download a file.\n");
     FILE *fs = fopen(rpdu.data,"r"); 

     if (fs == NULL) //-- Error Reporting
     {
    fprintf(stderr, "ERROR: File %s not found on Server.\n", rpdu.data);  
    spdu.type = 'E';
         sprintf(spdu.data, "Unfortunately, %s was not found on the server.", rpdu.data);
    send(sd,(char *)&spdu, sizeof(spdu),0);  
     }
      

    else 
    {
   lstat(rpdu.data,&fstat);
   int bytes_to_read = fstat.st_size;
   spdu.length = fstat.st_size;
   spdu.type = 'R';
   send(sd, (char *)&spdu, sizeof(spdu), 0);  
 printf("%s was found in the server.\nBeginning file Transfer.....\n", rpdu.data);  
          
   printf ("Size of the file  = %d bytes \n", bytes_to_read);
      
   while(bytes_to_read > 0)
   {
    n = fread(spdu.data, sizeof(char), bytes_to_read, fs);
    bytes_to_read -= n;
    spdu.type = 'F';
    spdu.length = fstat.st_size;  
    send(sd, (char *)&spdu, sizeof(spdu) ,0);   
          
    if(bytes_to_read == 0) 
    {
      fclose(fs);
      printf("File sent successfully.\n");
    }         
    }         
     }
  
     break;
  case 'U': //-- Client upload request
  printf("Client had requested to upload a file named : %s.\n", rpdu.data);
  FILE *fr = fopen(rpdu.data, "w");

  if (fr == NULL) //-- Error Reporting
  {
    fprintf(stderr, "Failed to create file.\n");
    spdu.type = 'E';
sprintf(spdu.data,"Unfortunately, %s could not be created on the server.\n",rpdu.data);
    send(sd,(char *)&spdu, sizeof(spdu) ,0);  
    break;
  }
  else
  {
    spdu.type = 'R';
    send(sd, (char *)&spdu, sizeof(spdu), 0);   //Notify the client that the                    //server is ready

recv(sd, (char *)&rpdu, sizeof(rpdu), 0);  //Receive an F-type PDU with                //the file length
    bytes_to_write = rpdu.length;

    while  (bytes_to_write > 0)
    {
      n = fwrite(rpdu.data, sizeof(char), bytes_to_write, fr);
      bytes_to_write -= n;
            
      if(bytes_to_write == 0) 
      {
        fclose(fr);
        printf("File uploaded successfully.\n");
      }
    }
  }       
  break;  

  case 'P': //-- Client change directory request

  printf("Client has requested a change of directory to: %s\n", rpdu.data);
  char *directory = rpdu.data;  //Directory string was received in the first              //recv() call 

  if(chdir(directory) == -1) 
  {
    spdu.type = 'E';
    sprintf(spdu.data,"The directory: '%s' does not exist\n",rpdu.data);
    send(sd, (char *)&spdu, sizeof(spdu) ,0);
    break;
  }
  
else
  {
    spdu.type = 'R';
    send(sd, (char *)&spdu, sizeof(spdu) ,0);
  }
        
break;

case 'L': //-- Client list directory request

  if((dir = opendir(".")) != NULL)    //Open the current working directory
  {
    spdu.data[0] = '\0';
    while ((entry = readdir(dir)) != NULL)  //Add each file name to the list
    {
      strcat(spdu.data, entry->d_name);
      strcat(spdu.data, " "); 
    }
  
    closedir(dir);
  }
  
spdu.type = 'l';
  send(sd, (char *)&spdu, sizeof(spdu) ,0);
  printf("List of items in current directory sent.\n");
  
break;
  default: //-- Default case
    
fprintf(stderr, "Client made an invalid choice or chose to quit.");
  close(sd);
  exit(0);
      
      }
  }

  return(0);
}

/*  reaper    */
void  reaper(int sig)
{
  int status;
  while(wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}

