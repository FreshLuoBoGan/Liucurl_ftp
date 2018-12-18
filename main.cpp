
using namespace std;
#include "ftp/ftp.h"

#define DATA_SIZE 16*1024*8+25
#define DATA_NAME /dsafds1

char s[DATA_SIZE]="fdsafdsaf";
char name[100] = "DATA_NAME";
FTPParams FTP_Params = {"111.111.111.111",//ip
						21,
						"111",//user
						"111",//passwd
						"561",//path
						BINARY};

struct timeval tv,tv0; 
int main()
{
	printf("!!!start\n");
	CurlFTPManager *curl_test = new CurlFTPManager(FTP_Params);
		FILE *snd_fd = fopen("/home/zhangsheng/etcher-electron-1.4.4-linux-ia32.zip", "rb+");

	printf("\n\nsnd_fd:%d\n",snd_fd);

   	fseek(snd_fd,0,SEEK_END);
    int nFileLen = ftell(snd_fd);
    fseek(snd_fd, 0, SEEK_SET);

	while(1)
	{
		gettimeofday(&tv0, NULL);

		//memset(name,0,99);

		//sprintf(name,"/DATA_NAME_%d_%d",tv0.tv_sec,tv0.tv_usec);

		


	curl_test->Push(name, snd_fd, nFileLen,"");usleep(100*1000*1);
	}



	//curl_test->Push("/dsafds2",s,16*1024+1);

	//curl_test->Push("/dsafds3",s,16*1024);

	//printf("\nwait.......................................\n");

	//usleep(1000*1000*60*6);

	//curl_test->Push("/dsafds4",s,16*1024);

	printf("!!!end\n");
	return 1;
}
