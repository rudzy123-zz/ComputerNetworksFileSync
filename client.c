#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVER_TCP_PORT 3000  /* well-known port */
#define BUFLEN     524288    /* buffer length */
//RudyM
int main(int argc, char **argv)
{
  int   n, i, bytes_to_read, bytes_to_write;
  int   sd, port, choice;
  struct  hostent   *hp;
  struct  sockaddr_in server;
  char  *host, *bp;
  struct stat fstat;
  struct PDU
  {
    char type;
    int  length;
    char data[BUFLEN];  
    
  } rpdu, spdu;

  switch(argc){
  case 2:
    host = argv[1];
    port = SERVER_TCP_PORT;
    break;
  case 3:
    host = argv[1];
    port = atoi(argv[2]);
    break;
  default:
    fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
    exit(1);
  }

  /* Create a stream socket */  
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr, "Can't creat a socket\n");
    exit(1);
  }

  bzero((char *)&server, sizeof(struct sockaddr_in));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  if (hp = gethostbyname(host)) 
    bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
  else if ( inet_aton(host, (struct in_addr *) &server.sin_addr) ){
    fprintf(stderr, "Can't get server's address\n");
    exit(1);
  }

  /* Connecting to the server */
  if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
    fprintf(stderr, "Can't connect \n");
    exit(1);
  }

while(1)
  {
    printf("\nWhat would you like to do?\n");
    printf("1. Download a file\n2. Upload a file\n3. Change file directory\n4.      List current directory\n5. Quit\n");
    //Save client's choice
    scanf("%d", &choice);
    
  switch(choice)
  {
  case 1: //-- Download request
        
    printf("Name of the file you would like to download:\n");
    n = read(0, spdu.data, BUFLEN);  //Store filename inside the send PDU
    spdu.type = 'D';
    spdu.data[n-1] = '\0';     //Terminate filename string

    printf("You have requested the following file: %s \n", spdu.data);        
    spdu.length = strlen(spdu.data);
    send(sd, (char *)&spdu, sizeof(spdu), 0); 

    //Check if there was an error
    recv(sd, (char *)&rpdu, sizeof(rpdu), 0);  
    if (rpdu.type == 'E')
    { 

    printf("An error occurred. Message from the server: %s\n", rpdu.data);

    }
    else if(rpdu.type == 'R')  //Begin download
    {
      FILE *fr = fopen(spdu.data, "w");

        
bytes_to_write = rpdu.length; //Get file size from length field //of the received PDU

      while(bytes_to_write >  0)
      {
        recv(sd, (char *)&rpdu, sizeof(rpdu), 0); //r2
        n = fwrite(rpdu.data, sizeof(char), bytes_to_write, fr);
        bytes_to_write -= n;



        if(bytes_to_write == 0) 
        {
          fclose(fr);
          printf("File downloaded successfully.\n");
        }
      }
    }
    
    break;    //return to choice selection menu
   case 2: //-- Upload request

    printf("Name of the file you would like to upload:\n");
    n = read(0, spdu.data, BUFLEN);   //Store filename inside the send PDU
    spdu.type = 'U';            
    spdu.data[n-1] = '\0';      //Terminate filename string

    printf("You requested to upload: %s\n", spdu.data);       
    spdu.length = strlen(spdu.data);     //Set the length field to the filename length
    send(sd, (char *)&spdu, sizeof(spdu), 0); 
        
    recv(sd, (char *)&rpdu, sizeof(rpdu), 0); 
    if (rpdu.type == 'E')       //Check if there was an error
    { 
      printf("An error occurred. Message from the server: %s\n", rpdu.data);
    }
    else if(rpdu.type == 'R')  //Begin upload
    { 
      printf("Server is ready. Beginning upload...\n");
      FILE *fs = fopen(spdu.data, "r");
      lstat(spdu.data,&fstat);
      bytes_to_read = fstat.st_size;
      spdu.length = fstat.st_size;

      if (fs == NULL) 
      {
        printf("Cannot upload because the file does not exist.\n");
        break;
      }     
      while(bytes_to_read > 0)
      {
        n = fread(spdu.data, sizeof(char), bytes_to_read, fs);
        spdu.type = 'F';
        spdu.length = bytes_to_read;
        send(sd, (char *)&spdu, sizeof(spdu), 0);  
        bytes_to_read -= n;
      
        if(bytes_to_read == 0) 
        {
        fclose(fs);
        printf("File uploaded successfully.\n");
        }         
       }
       }
    
        break;    //return to choice selection menu


    case 3: //-- Change file directory request
            
      printf("Directory to change to:\n");
      n = read(0, spdu.data, BUFLEN); //Store directory name inside the send //PDU
      spdu.type = 'P';            
      spdu.data[n-1] = '\0';        //Terminate directory name string
      send(sd, (char *)&spdu, sizeof(spdu), 0); 

      recv(sd, (char *)&rpdu, sizeof(rpdu), 0); 
            
      if (rpdu.type == 'E')
      { 
        printf("An error occurred. Message from the server: %s", rpdu.data);
      }

      else if(rpdu.type == 'R') 
      {
        printf("Directory changed. Current directory: %s\n", spdu.data);
      }
      
      break;    //return to choice selection menu

    case 4: //-- List directory request
          
      spdu.type = 'L';
      send(sd, (char *)&spdu, sizeof(spdu), 0); //Send L-type PDU

      recv(sd, (char *)&rpdu, sizeof(rpdu), 0);

      if(rpdu.type == 'l')
      {
        printf("\nThe files in the current directory are:\n%s\n\n", rpdu.data);
      }
        
      break;    //return to choice selection menu
    case 5: //-- Quit
      printf("You've chosen to quit. Goodbye\n");
      close(sd);
      exit(0);

    default: 
      printf("Invalid choice.\n");
      break;
        
    }
  }
}

