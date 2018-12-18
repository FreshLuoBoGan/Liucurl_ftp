/************************************************************************
 *FileName:  ftp.h
 *Author:    Zhang Sheng
 *Version:   1.0
 *Date:      2018-9-18
 *Description: 接口类
 ************************************************************************/


#ifndef FTP_H_
#define FTP_H_

#include <stdio.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h> 
#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <sstream>
//#include <basic_streambuf.h>

#define MKD_TEMP_FILE 					"/temp1"
#define MKD_LOCAL_FILE					"/tmp" MKD_TEMP_FILE
#define MKD_COMMAND						"MKD "

#define UPLOAD_TEMP_FILE				"/tmp/useless1.temp"
#define UPLOAD_FILE_POSTFIX				".tmp"
#define RENAME_UPLOAD_FILE_POSTFIX		".jpg"
#define STRING_NULL						""
#define FTP_PREFIX						"ftp://"
#define LOW_SPEED_TIME					10L			//传输64KB，不得多于10秒
#define COMMAND_TIME_OUT				10000000L	//命令无法得到反馈，不得多于10秒

enum FTP_TransferText
{
	ASCLL,
	BINARY
};

enum FTP_Model
{
	PASV,
	PORT
};

struct FTPParams {
        std::string ip;//FTP服务端URL;
        unsigned int port;//FTP服务端端口，默认21;
        std::string username;//FTP服务端用户名;
        std::string password;//FTP服务端密码;
        std::string path;
		FTP_TransferText Text;
};

class CurlFTPManager 
{
	public:
		CurlFTPManager( const FTPParams &tcp_params );
		~CurlFTPManager(void);
		int Push(std::string FilePrefix, FILE *fd, int size, std::string path);	//上传数据
		bool FTP_Connect_Test(std::string *ip);
		const char *ftp_geterror(void);

	private:

		static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream);
		void CreateDirectory(void);
		void Clear_Class_Variable(void);
		void Int2Str(const int &int_temp,std::string &string_temp);
		void DeleteFile(char *postfix);
		static size_t get_callback(void *ptr, size_t size, size_t nmemb, void *stream);
		static int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
		int SetCMDDelFile(std::string file_name);

		std::string m_strResponse;
		CURL *curl;		//curl标识符
		CURLcode res;	//curl_easy_perform反馈值
		struct stat file_info;	//文件信息
		curl_off_t speed_upload, total_time;	//传输速度
		std::string m_strUserPwd;	//用户名和密码
		std::string m_strServerIP;	//服务端IP
		std::string m_strServerPORT;	//服务端IP
		std::string m_strServerURL;	//服务端URL
		struct curl_slist *headerlist = NULL;
		std::string m_strRemotePath;//路径
		std::string m_strUploadName;//上传名字
		std::string m_strJPGname;//修改名字为.jpg
		FTP_TransferText DataMode;

		std::string m_strMKD = MKD_COMMAND;
		FILE *fd,*fd_temp;
		int cycle_count = 0;
		bool ftp_model;
};

#endif
