#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <vector>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <iostream>
#include <cstring>
#include <algorithm>

using namespace std;

#define KEY 2025
#define MSGLEN 5000
#define SERVER_TYPE 123456

typedef pair<int,string> pss;

int cnt;
vector<int >keys;
int keys_counter ;
struct msgStruct
{
    long mtype;
    char mtext[MSGLEN];
    int pid;

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

int main()
{
    int keys_counter = 0;

    int msgid = msgget(KEY,IPC_CREAT|0666);
    if(msgid  < 0)cout<<"msgget Error"<<endl;
    struct msgStruct mymsg,sentMsg;

    while(1)
    {
        // Receive the message from the Message Queue
        if(msgrcv(msgid,&mymsg,MSGLEN,SERVER_TYPE,0) < 0 ) cout<<"msgrcv Error"<<endl;


        // Parse the Message
        pss re = extract(mymsg.mtext);
        cout<<re.second;

        char ** cmd  = lexer(re.second.c_str(),&cnt);

        // If the message is for coupling, couple the sender to the server
        if( find(keys.begin(),keys.end(),re.first) == keys.end() && (strncmp(cmd[cnt-1],"couple",6) == 0))
        {
            keys.push_back(re.first);
            keys_counter++;
            sentMsg.mtype = re.first;
            strcpy(sentMsg.mtext,mymsg.mtext);
            if(msgsnd(msgid,&sentMsg,MSGLEN,0) < 0 )cout<<"msgsnd Error"<<endl;

            int iu = 0;
            cout<<"List of updated client pid's : \n";
            for(iu = 0; iu <keys.size();iu++)
                cout << keys[iu] <<" ";
            cout<<endl;
            continue;

        }

        // If the message is for uncoupling, uncouple the sender from the server
        if(strncmp(cmd[cnt-1],"uncouple",8) == 0)
        {
            keys.erase(find(keys.begin(),keys.end(),re.first));
            sentMsg.mtype = re.first;
            strcpy(sentMsg.mtext,mymsg.mtext);
            if(msgsnd(msgid,&sentMsg,MSGLEN,0) < 0 )cout<<"msgsnd Error"<<endl;

            int iu = 0;
            cout<<"List of updated client pid's : \n";
            for(iu = 0; iu <keys.size();iu++)
                cout << keys[iu] <<" ";
            cout<<endl;
            continue;
        }

        // Broadcast the message to all the coupled terminals except the sender
        int sz = keys.size(),ii;
        for(ii =0; ii<sz; ii++)
        {
            if(keys[ii] != re.first )
            {
                sentMsg.mtype = keys[ii];
                strcpy(sentMsg.mtext,mymsg.mtext);
                if(msgsnd(msgid,&sentMsg,MSGLEN,0) < 0 )cout<<"msgsnd Error"<<endl;
                cout<<"Message sent to :"<<keys[ii]<<endl;
            }
        }
    }
    return 0;

}
