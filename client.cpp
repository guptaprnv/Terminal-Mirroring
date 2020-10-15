#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <vector>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/wait.h>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;
#define KEY 2025
#define MSGLEN 5000

#define SERVER_TYPE 123456

typedef pair<int,string> pss;

int cnt;

vector<int >keys;
int keys_counter ;

int ppid;

struct msgStruct
{
    long mtype;
    char mtext[MSGLEN];


};


pair<int,string> extract(char s[])
{
    string pid_send,msg_new;
    int pid_int,iii = 0;
    while(s[iii]!='\t' && s[iii]!='\0')
    {
        pid_send += s[iii];
        iii++;
    }
    iii++;

    while(s[iii]!='\0')
    {
        msg_new += s[iii];
        iii++;
    }

    pid_int = atoi(pid_send.c_str());

    return pair<int,string>(pid_int,msg_new);
}

char* escape(char* buffer)
{
    int i,j;
    int l = strlen(buffer) + 1;
    char* dest  =  (char*)malloc(1024*sizeof(char));
    char* ptr=dest;
    for(i=0; i<l; i++)
    {
        if( buffer[i]=='\\' )
        {
            continue;
        }
        *ptr++ = buffer[i];
    }
    *ptr = '\0';
    return dest;
}

char** lexer(const char  *buffer,int * cnt)
{
    int i = 1,count  = 0,start = 0,f = 0;
    char ** a = (char**)malloc(1024*sizeof(char*));
    if(buffer[0] != ' ')
        start = 0;
    while(1)
    {
        if(buffer[i-1] == '\\' && buffer[i] == ' ')
        {
            f = 1;
            i = i+2;
            continue;
        }
        if((buffer[i] == ' ' || buffer[i] == '\n' || buffer[i] == '\0') && buffer[i-1] != ' ')
        {
            a[count++] = (char*)malloc(256*sizeof(char));
            strncpy(a[count-1],buffer + start,i - start);
            a[count-1][i-start] = '\0';
            if(f == 1)
            {
                strcpy(a[count-1],escape(a[count-1]));
                f = 0;
            }
        }
        else if(buffer[i] != ' ' && buffer[i-1] == ' ')
        {
            start = i;
        }
        if(buffer[i] == '\0' || buffer[i] == '\n')
            break;
        i++;
    }
    a[count] = NULL;
    *cnt  = count;
    return a;
}

void pwd()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s >", cwd);

}
// Function for executing cd(change directory) command
void changeDir(const char *path)
{
    struct stat st = {0};
    if (stat(path, &st) == -1)
    {
        printf("%s: No such file or directory\n",path);
    }
    else
    {
        chdir(path);
    }
}

int main()
{
    int cnt,i,j,k,envIdx;
    char homeDir[1024],temp[1024],currDir[1024];
    FILE *fp = NULL;
    strcpy(homeDir,getenv("HOME"));

    size_t size = 1024;
    char **cmd;

    changeDir(homeDir); //home directory

    int keys_counter = 0;
    ppid = getpid();

    size_t msgSize = sizeof(struct  msgStruct) - sizeof(long);

    int msgid = msgget(KEY,IPC_CREAT|0666);
    if(msgid < 0)cout << "msgget Error"<<endl;
    struct msgStruct sentMsg,mymsg;
    stringstream yy ;

    // Create a child process for receiving messages
    int prid = fork();

    if (prid == 0)//child process
    {
        char ** cmd;
        int uncpl = 1;
        while(1)
        {
            int iii = 0,pid_int;
            string msg_new,pid_send;
            msg_new.clear();
            pid_send.clear();

            // Receive a message via message queue
            if(msgrcv(msgid,&mymsg,MSGLEN,ppid,0) < 0 ) cout<<"msgrcv Error"<<endl;

            // Parse it
            pss re = extract(mymsg.mtext);

            if(uncpl == 0){
                fflush(stdout);
                cout<<re.second<<">";
                fflush(stdout);
            }

            cmd = lexer(re.second.c_str(),&cnt);

            // Handle Uncouple request
            if(strncmp(cmd[cnt-1],"uncouple",8) == 0)
            {
                fflush(stdout);
                cout<<"Successfully Uncoupled"<<"\n >";
                fflush(stdout);

                uncpl = 1;
            }

            // Handle Couple request
            if(strncmp(cmd[cnt-1],"couple",6) == 0)
            {
                fflush(stdout);
                cout<<"Successfully Coupled"<<"\n >";
                fflush(stdout);
                uncpl = 0;
            }
        }
    }
    else
    {
        //parent process
        int uncpl = 1;

        while(1)
        {
            char *input1 = (char *)malloc(1024*sizeof(char));

            pwd();
            int ii = 0;

            // Take the input
            while((input1[ii] = getchar()) !='\n' && input1[ii] != EOF) ii++;
            ii++;
            input1[ii] = '\0';
            string input = string(input1);

            // Handle input: uncouple
            if(strncmp(input1,"uncouple",8) == 0)
            {
                cout<<"Uncoupling from the server..."<<endl;
                stringstream uu;
                uu<<ppid<<"\t"<<string("From pid = ")<<ppid <<string(": ")<<input;
                sentMsg.mtype = SERVER_TYPE;
                strcpy(sentMsg.mtext,(uu.str()).c_str() );
                sentMsg.mtext[strlen(sentMsg.mtext)] = '\0';
                if(msgsnd(msgid,&sentMsg,MSGLEN,0) < 0 )cout<<"msgsnd Error"<<endl;
                uncpl = 1;
            }

            // Handle input: couple
            else if(strncmp(input1,"couple",6) == 0)
            {
                cout<<"Coupling you to the server..."<<endl;
                uncpl = 0;
            }

            // Send the input to the server in case it is coupled to it
            stringstream uu;
            uu<<ppid<<"\t"<<string("From pid = ")<<ppid <<string(": ")<<input ;

            sentMsg.mtype = SERVER_TYPE;
            strcpy(sentMsg.mtext,(uu.str()).c_str() );
            sentMsg.mtext[strlen(sentMsg.mtext)] = '\0';
            //sentMsg.pid = ppid;
            if(uncpl == 0){
                if(msgsnd(msgid,&sentMsg,MSGLEN,0) < 0 )cout<<"msgsnd Error"<<endl;
            }

            stringstream uus,out;
            uus <<string(getenv("HOME")) << "/" << ppid << "_stdout.txt";

            if(strcmp(input1,"\0") == 0 || strcmp(input1,"\n") == 0)
            {
                continue;                   // Ignore Execution of '\n' command
            }
            cmd = lexer(input1,&cnt);
            if(strncmp(input1,"uncouple",8) == 0) continue;
            else if(strncmp(input1,"couple",6) == 0) continue;
            // Handle Output

            // Close STDOUT
            int ffd = open( uus.str().c_str(),O_CREAT|O_WRONLY|O_TRUNC,0666);
            //fflush(stdout);
            int std_copy =  dup(1);
            int std_er = dup(2);
            close(1);
            close(2);
            dup2(ffd,1);
            dup2(ffd,2);
            close(ffd);
            //Command
            // Handle 'cd' command
            if(strcmp(cmd[0],"cd") == 0)
            {
                if(cnt == 1)
                {
                    changeDir(homeDir);
                }
                else
                {
                    changeDir(cmd[1]);
                }
            }
            // Handling 'exit' command
            else if(cnt ==1 && strcmp(cmd[0],"exit") == 0)
            {
                kill(prid,SIGTERM);
                return 0;
            }

            else system(input1);

            ifstream inFile;
            inFile.open( uus.str().c_str() );//open the input file

            stringstream strStream;
            strStream << inFile.rdbuf();//read the file
            string str = strStream.str();

            dup2(std_copy,STDOUT_FILENO);
            dup2(std_er,2);
            close(std_copy);
            close(std_er);


            out.str(std::string());

            out<<ppid;

            // Send the output in case it is coupled to the server
            strcpy(sentMsg.mtext, (out.str()+"\t"+str).c_str() );
            sentMsg.mtext[strlen(sentMsg.mtext)] = '\0';
            sentMsg.mtype =SERVER_TYPE;
            if(uncpl == 0){
                if(msgsnd(msgid,&sentMsg,MSGLEN,0) < 0 )cout<<"msgsnd Error"<<endl;
            }
            // Print the output on current terminal
            cout<<str;
        }
    }
}
