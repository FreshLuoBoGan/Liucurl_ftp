#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <sys/socket.h>  
#include <netinet/tcp.h> 
#include <signal.h>
#include <sys/types.h>          /* See NOTES */
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ftp.h"

using namespace std;

/************************************************************************
 *FileName:  CurlFTPManager
 *Author:    Zhang Sheng
 *Date:      2018-9-18
 *Description: 
 ************************************************************************/
CurlFTPManager::CurlFTPManager( const FTPParams &tcp_params )
{
	struct hostent *addr_info = NULL;
	string ip;

	Int2Str(tcp_params.port,m_strServerPORT);

	addr_info = gethostbyname(tcp_params.ip.c_str());
	if(addr_info == NULL)
	{
		printf("!!! can not fetch serverip from host!\n");
		ip = tcp_params.ip;
	}
	else
	{
		char str[32];
		inet_ntop(addr_info->h_addrtype, addr_info->h_addr, str, sizeof(str));
		ip = str;
		printf("Got ServerIP %s\n", str);
	}

	//用户名密码拼凑		
	m_strUserPwd = tcp_params.username + ":" + tcp_params.password;
	//ftp://IP
	m_strServerIP = FTP_PREFIX + ip + ":" + m_strServerPORT;	//服务端IP
	//图片存放路径 /path
	m_strRemotePath = "/" + tcp_params.path;

	ftp_model = PASV;
	DataMode = BINARY;

	curl = curl_easy_init();

	/********************************curl第一次设置以及默认设置*************************************/
	if(NULL != curl)
	{
		curl_easy_setopt(curl, CURLOPT_USERPWD, m_strUserPwd.c_str()); 	//设置用户名、密码 
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);			//上传		
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);	//回调函数
		curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);					//便于调试

		if(	ASCLL == tcp_params.Text)	//设置数据传输格式
		{
			DataMode = ASCLL;
			curl_easy_setopt(curl, CURLOPT_TRANSFERTEXT, 1L);
		}
		else
		{
			DataMode = BINARY;
			curl_easy_setopt(curl, CURLOPT_TRANSFERTEXT, 0);	
		}
	}
}

CurlFTPManager::~CurlFTPManager()
{
	
}

/************************************************************************
 *FileName:  Push
 *Author:    Zhang Sheng
 *Date:      2018-9-18
 *Description: 上传文件
 ************************************************************************/
int CurlFTPManager::Push(std::string FilePrefix, FILE *fd, int size, std::string path)
{
	curl_off_t cl;
	struct timeval tv,tv0,tv1; 	
	gettimeofday(&tv0, NULL);

	//URL:ftp:127.0.0.1/path/FIleName.tmp(包含存放路径以及存放名)
	if( 0 != path.length())
	{
		m_strRemotePath = path;
	}

	m_strServerURL = m_strServerIP + m_strRemotePath + "/" +FilePrefix + UPLOAD_FILE_POSTFIX;

	//修改文件名：FIleName.tmp 修改为 FIleName.jpg
	m_strUploadName = "RNFR " + FilePrefix + UPLOAD_FILE_POSTFIX; 
	m_strJPGname 	= "RNTO " + FilePrefix + RENAME_UPLOAD_FILE_POSTFIX;
	headerlist 		= curl_slist_append(headerlist, m_strUploadName.c_str());
	headerlist 		= curl_slist_append(headerlist, m_strJPGname.c_str());
	m_strUploadName = "CWD " + m_strRemotePath; 
	headerlist 		= curl_slist_append(headerlist, m_strUploadName.c_str());
	
	fseek(fd,0,0);

	if(NULL != curl)
	{
	/*************************CURL设置及上传-start*****************************/
		//printf("%s,%d\n",m_strServerURL.c_str(),size);
		curl_easy_setopt(curl, CURLOPT_USERPWD, m_strUserPwd.c_str()); 	//设置用户名、密码 
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);			//上传		
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);	//回调函数
		curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);	
		curl_easy_setopt(curl, CURLOPT_READDATA, fd);		
		curl_easy_setopt(curl, CURLOPT_URL,m_strServerURL.c_str());			//URL
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,(curl_off_t)size);	//SIZE
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);	//回调函数	
		//过程监控：调用回电函数progress_callback
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_XFERINFODATA, curl);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
		//Less than 1 bytes/sec transferred the last 10 seconds
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, LOW_SPEED_TIME);		
  		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);	

		if(	ASCLL == DataMode)	//设置数据传输格式
		{
			curl_easy_setopt(curl, CURLOPT_TRANSFERTEXT, 1L);
		}
		else
		{
			curl_easy_setopt(curl, CURLOPT_TRANSFERTEXT, 0);	
		}
		
		if(PORT == ftp_model)
		{
			curl_easy_setopt(curl, CURLOPT_FTPPORT, "-");
		}
	
		res = curl_easy_perform(curl);
		
		curl_slist_free_all(headerlist);

		Clear_Class_Variable();

	/*************************CURL设置及上传-end*****************************/
		if( CURLE_FTP_WEIRD_PASV_REPLY == res )
		{
			ftp_model = PORT; 

			return 0;
		}
		else if (CURLE_FTP_PORT_FAILED == res ) //没有足够的端口使用
		{
			curl_global_cleanup();
 			curl_easy_cleanup(curl);
 			curl_global_init(CURL_GLOBAL_ACK_EINTR);
 			curl_global_init(CURL_GLOBAL_ALL);
			curl = curl_easy_init();

			return 0;
		}
		else if( CURLE_OK != res && CURLE_FTP_WEIRD_PASV_REPLY != res)
		{
			fprintf(stderr, "Push:curl_easy_perform() failed: %s\n",curl_easy_strerror(res),res);
			curl_global_cleanup();
 			curl_easy_cleanup(curl);
 			curl_global_init(CURL_GLOBAL_ACK_EINTR);
 			curl_global_init(CURL_GLOBAL_ALL);
			curl = curl_easy_init();
			return 0;
		}
		else
		{
			curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 0);
			res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_UPLOAD_T, &cl);
			size = cl;
		}

	}

	if(100 <= cycle_count)
	{
		cycle_count = 0;
		DeleteFile(".tmp");
	}
	else if(0 == cycle_count%20)
	{
		curl_global_cleanup();
		curl_easy_cleanup(curl);
		curl_global_init(CURL_GLOBAL_ACK_EINTR);
		curl_global_init(CURL_GLOBAL_ALL);
		curl = curl_easy_init();
	}

	cycle_count++;

	gettimeofday(&tv1, NULL);
	//printf("time:%ld\n",tv1.tv_sec*1000000+tv1.tv_usec-tv0.tv_usec-tv0.tv_sec*1000000 );

	return size;
}
/************************************************************************
 *FileName:  Clear_Class_Variable
 *Author:    Zhang Sheng
 *Date:      2018-9-28
 *Description: curl_easy_perform后均需清除成员变量
 ************************************************************************/
void CurlFTPManager::Clear_Class_Variable(void)
{
	m_strServerURL 	= "";
	
	m_strJPGname 	= "";

	m_strUploadName = "";

	headerlist 		= NULL;
}
/************************************************************************
 *FileName:  FTP_Connect_Test
 *Author:    Zhang Sheng
 *Date:      2018-11-9
 *Description: FTP连接测试
 ************************************************************************/
bool CurlFTPManager::FTP_Connect_Test(string *ip)
{
	struct timeval tv,tv0,tv1; 	
	gettimeofday(&tv0, NULL);

	CURL *curl;	

	*ip = m_strServerIP;

	curl = curl_easy_init();
	if(NULL != curl)
	{
		printf("m_strServerIP.c_str():%s\n",m_strServerIP.c_str());

		curl_easy_setopt(curl, CURLOPT_USERPWD, m_strUserPwd.c_str()); 	//设置用户名、密码 
		curl_easy_setopt(curl, CURLOPT_URL,m_strServerIP.c_str());			//URL
		curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L); 
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_XFERINFODATA, curl);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);   

		res = curl_easy_perform(curl);	

		if( CURLE_OK != res )
		{
			fprintf(stderr, "ConnectTest:curl_easy_perform() failed: %s\n",curl_easy_strerror(res),res);
			curl_easy_cleanup(curl);
			return false;
		}
		else
		{
			curl_easy_cleanup(curl);
			return true;
		}
	}
}


/************************************************************************
 *FileName:  read_callback
 *Author:    Zhang Sheng
 *Date:      2018-9-18
 *Description: 回调函数
 ************************************************************************/
size_t CurlFTPManager::read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t retcode;
	curl_off_t nread;

	if(NULL == stream)
	{
		retcode = 0;
	}
	else
	{		
		retcode = fread(ptr, size, nmemb, (FILE *)stream);

		nread = (curl_off_t)retcode;
	}
	return retcode;
}
void CurlFTPManager::Int2Str(const int &int_temp,std::string &string_temp)
{
  std::stringstream os;

  os << int_temp;

  string_temp =  os.str();
}

/************************************************************************
 *FileName:  SetCMDDelFile
 *Author:    Zhang Sheng
 *Date:      2018-10-19
 *Description: ftp下发删除命令
 ************************************************************************/
int CurlFTPManager::SetCMDDelFile(std::string file_name)
{

	std::string strServerURL;
	strServerURL = m_strServerIP + m_strRemotePath + "/" ;

	std::string strDelFile;
	CURLcode res;

	strDelFile = "DELE " + file_name;


	if(PORT == ftp_model)
	{
		curl_easy_setopt(curl, CURLOPT_FTPPORT, "-");
	}
	curl_easy_setopt(curl, CURLOPT_USERPWD, m_strUserPwd.c_str()); 	//设置用户名、密码 
	curl_easy_setopt(curl, CURLOPT_URL, strServerURL.c_str());
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,(curl_off_t)0);	//size
	curl_easy_setopt(curl, CURLOPT_READDATA, NULL);
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 0);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, strDelFile.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_callback);

	res = curl_easy_perform(curl);

	if(res != CURLE_OK)
	{
		fprintf(stderr, "SetCMDDelFile:curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
	}

	return res;
}
size_t CurlFTPManager::get_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	if(NULL != stream)
	{
		std::string *m_strDataBuffer;

		m_strDataBuffer = (std::string *)stream;

		m_strDataBuffer->append((char *)ptr,nmemb);
	}

	return nmemb;
}
/************************************************************************
 *FileName:  DeleteFile
 *Author:    Zhang Sheng
 *Date:      2018-10-19
 *Description: 获取文件列表以及删除文件
 ************************************************************************/
void CurlFTPManager::DeleteFile(char *postfix)
{
	CURL *DFcurl;
	CURLcode res;

	std::string strServerURL;
	std::string strListDataTemp;
	std::string list_rsp;

	strServerURL = m_strServerIP + m_strRemotePath + "/";

	char *poiFileList;

	//get file list
	DFcurl = curl_easy_init();
	if(DFcurl) {
		if(PORT == ftp_model)
		{
			curl_easy_setopt(DFcurl, CURLOPT_FTPPORT, "-");
		}
		
		curl_easy_setopt(DFcurl, CURLOPT_URL, strServerURL.c_str());
		curl_easy_setopt(DFcurl, CURLOPT_USERPWD, m_strUserPwd.c_str());
		curl_easy_setopt(DFcurl, CURLOPT_CUSTOMREQUEST, "NLST");
		curl_easy_setopt(DFcurl, CURLOPT_WRITEDATA, &list_rsp);
		curl_easy_setopt(DFcurl, CURLOPT_WRITEFUNCTION, get_callback);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_XFERINFODATA, curl);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
		//curl_easy_setopt(DFcurl, CURLOPT_VERBOSE, 1L);

		res = curl_easy_perform(DFcurl);

		if(res != CURLE_OK)
		fprintf(stderr, "DeleteFile:curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
		
		curl_easy_cleanup(DFcurl);
	}

	//提取.tmp文件
	while(1) {
	int nIdx = 0;
	
	nIdx = list_rsp.find("\n");
	if(nIdx == -1){
		break;
	}

	strListDataTemp = list_rsp.substr(0 , nIdx);
	int tpos = strListDataTemp.find(postfix);

	if(tpos > 0){
	SetCMDDelFile(strListDataTemp);
	}

		if(list_rsp.length() >= (nIdx + 1) )
		{
			list_rsp = list_rsp.substr(nIdx + 1, list_rsp.length() - nIdx - 1);
		}
		else
		{
			break;
		}
	}

	free(poiFileList);
}

const char *CurlFTPManager::ftp_geterror()
{
	return curl_easy_strerror(res);	
}

int CurlFTPManager::progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	CURL *curl = (CURL *)(clientp);

	curl_off_t APPCONNECT_time;
	curl_off_t CONNECT_time;
	curl_off_t PRETRANSFER_time;
	curl_off_t REDIRECT_time;
	curl_off_t TOtal_time;
	curl_off_t NameLookup_time;
	curl_off_t StartTransfer_time;

	curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME_T, &NameLookup_time);
	curl_easy_getinfo(curl, CURLINFO_APPCONNECT_TIME_T, &APPCONNECT_time);
	curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME_T, &CONNECT_time);
	curl_easy_getinfo(curl, CURLINFO_PRETRANSFER_TIME_T, &PRETRANSFER_time);
	curl_easy_getinfo(curl, CURLINFO_STARTTRANSFER_TIME_T, &StartTransfer_time);
	curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME_T, &TOtal_time);
	curl_easy_getinfo(curl, CURLINFO_REDIRECT_TIME_T, &REDIRECT_time);

	//传输之前，登录、命令未得到反馈，10秒则退出。
	if(0 == StartTransfer_time && 0 < NameLookup_time && COMMAND_TIME_OUT < TOtal_time - NameLookup_time)
	{
		//printf("%d,%d,%d\n",0 == StartTransfer_time,0 < NameLookup_time,10000000L < TOtal_time - NameLookup_time);
		return 1;
	}

	// > RNFR 20181126111219594_0.tmp
	// < 350 RNFR accepted - file exists, ready for destination
	// > RNTO 20181126111219594_0.jpg
	// < 250 File successfully renamed or moved
	// > CWD /_test_beij
	// < 250 OK. Current directory is /_test_beij
	long response_code;
	static bool Start_Time = false;
	static curl_off_t res_Start_time;
	curl_off_t res_end_time;

	//传输之后，修改文件名命令未得到反馈，10秒则退出。
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
	if(0 < StartTransfer_time && ( 226 == response_code || 350 == response_code || 250 == response_code) )
	{
		if(!Start_Time)
		{
			res_Start_time = TOtal_time;
			Start_Time = true;
		}
		res_end_time = TOtal_time;

		if(COMMAND_TIME_OUT < res_end_time - res_Start_time)return 1;
	}
	else
	{	
		Start_Time = false;
	}

	return 0;//如果回复为1,就会中断上传进程；
}