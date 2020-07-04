#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <queue>
#include <set>
#include <WinSock2.h>
#include <ctime>
#include <cstdlib>

#pragma warning(disable: 4996)

#define DEFAULT_PAGE_BUF_SIZE 1048576

#pragma comment(lib, "ws2_32.lib")

using namespace std;

queue<string> urlQue;
set<string> visitedUrl;
set<string> visitedImg;
string urlStart = "http://www.4399.com";										//遍历起始地址



//没问题
bool ParseURL(const string& url, string& host, string& resource)				//解析出主机名和资源名
{
	string urlTmp = url;
	const string httpStr = "http://";
	const string httpsStr = "https://";
	size_t pos = urlTmp.find(httpStr);						//寻找http://
	if (pos != string::npos) urlTmp = urlTmp.substr(httpStr.length());
	else
	{
		pos = urlTmp.find(httpsStr);						//寻找https://
		if (pos != string::npos) urlTmp = urlTmp.substr(httpStr.length());
	}
	size_t igr = 0;
	while (urlTmp[igr] != '\0' && urlTmp[igr] == '/') ++igr;
	urlTmp = urlTmp.substr(igr);
	if ((pos = urlTmp.find_first_of('/')) == string::npos)
	{
		host = urlTmp; 
		resource = "/"; 
	}
	else
	{
		host = urlTmp.substr(0, pos);
		resource = urlTmp.substr(pos);
	}
	return true;
}

bool GetHttpResponse(const string& url, string& response, size_t& bytesRead)	//获取url网页的源代码并返回读入的字节数
{
	string host, resource;
	cout << "Begin to parse: " << url << "..." << endl;
	if (!ParseURL(url, host, resource))
	{
		cout << "Fail to parse: " << url << '!' << endl;
		return false;
	}

	//建立SOCKET
	cout << "Begin to get host: " << url << "..." << endl;
	struct hostent* hp = gethostbyname(host.c_str());
	if (!hp)
	{
		cout << "Fail to get host: " << host << '!' << endl;
		return false;
	}
	cout << "Begin to build socket..." << endl;
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == -1 || sock == -2)
	{
		cout << "Fail to build socket!" << endl;
		return false;
	}

	//建立服务器地址
	SOCKADDR_IN sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(80);								//htons: 将u_short类型整数从小尾顺序(小端对齐)变成大尾顺序(大端对齐)
	memcpy((void*)&sa.sin_addr, hp->h_addr, 4);

	//建立连接
	cout << "Connecting to " << url << "..." << endl;
	if (connect(sock, (sockaddr*)&sa, sizeof(sa)))
	{
		closesocket(sock);
		cout << "Fail to connect to " << url << '!' << endl;
		return false;
	}

	//准备发送数据
	string request = "GET " + resource + " HTTP/1.1\r\nHost:" + host + "\r\nConnection:Close\r\n\r\n";

	//发送数据
	cout << "Sending data..." << endl;
	if (send(sock, request.c_str(), request.length(), 0) == SOCKET_ERROR)
	{
		closesocket(sock);
		cout << "Faile to send data to " << url << endl;
		return false;
	}

	//接收数据
	int contentLength = DEFAULT_PAGE_BUF_SIZE;						//缓冲区容量
	char* pageBuf = new char[contentLength];						//缓冲区
	memset(pageBuf, 0, contentLength * sizeof(char));				//清零
	bytesRead = 0;													//一共读到的字节数
	int ret = 1;													//本次读到的字节数

	cout << "Begin to read...\nRead(bytes): ";
	while (ret > 0)
	{

		//接收数据
		ret = recv(sock, pageBuf + bytesRead, contentLength - bytesRead, 0);

		if (ret > 0) bytesRead += ret;
		if (contentLength - bytesRead < 100)						//即将读满，重新分配内存
		{
			char* tmp = new char[contentLength * 2];
			memset(tmp, 0, contentLength * 2 * sizeof(char));
			memcpy(tmp, pageBuf, contentLength * sizeof(char));
			delete[] pageBuf;
			pageBuf = tmp;
			contentLength *= 2;
		}
		cout << ret << ' ';
	}
	cout << endl;
	pageBuf[bytesRead] = '\0';
	response.resize(bytesRead + 1); 

	//必须采取这种方法，否则直接赋值遇到'\0'就停止
	memcpy((char*)response.c_str(), pageBuf, bytesRead + 1); 
	delete[] pageBuf;
	closesocket(sock);
	return true;
}


string ToFileName(const string& url)						//将地址转换为文件名
{
	string fileName = "";
	for (size_t i = 0; i < url.length(); ++i)
	{
		switch (url[i])
		{
		case '\\': case '/': case ':': case '*': case '?': case '\"': case '<': case '>': case '|':
			fileName += ' ';
			break;
		default:
			fileName += url[i];
			break;
		}
	}
	return fileName;
}

void relativePath(string& url, const string& fatherUrl)									//处理相对地址
{
	size_t pos1 = fatherUrl.find_last_of('/'), pos2 = fatherUrl.find_first_of('/'); 
	if (url[0] == '/')
	{
		if (pos2 == string::npos) url = fatherUrl + url;
		else url = fatherUrl.substr(0, pos2) + url; 
	}
	else if (url.find("http") != 0 && pos1 != string::npos)
	{
		url = fatherUrl.substr(0, pos1 + 1) + url; 
	}
}


//没问题
void HTMLParse(const string& response, vector<string>& imgurls, const string& url)		//解析HTML，寻找超链接和图片链接
{
	string tagHref = "href=\"";
	size_t pos = response.find(tagHref);
	ofstream fout("url.txt", ios::app);
	if (!fout) MessageBox(NULL, "Cannot open url.txt!", NULL, MB_ICONWARNING | MB_OK);
	while (pos != string::npos)
	{
		pos += tagHref.length();
		size_t nextQ = response.find('\"', pos);
		if (nextQ == string::npos) break;
		string urlTmp = response.substr(pos, nextQ - pos);
		relativePath(urlTmp, url); 
		if (visitedUrl.find(urlTmp) == visitedUrl.end())			//没有访问过此网站
		{
			urlQue.push(response.substr(pos, nextQ - pos));
			fout << response.substr(pos, nextQ - pos) << endl;
		}
		pos = response.find(tagHref, nextQ);
	}
	fout << endl << endl;
	fout.close();

	string tagImg = "<img";
	string att1 = "src=\"";
	string att2 = "lazy-src=\"";
	size_t pos0 = response.find(tagImg);
	while (pos0 != string::npos)
	{
		size_t pos2 = response.find(att2, pos0);
		if (pos2 == string::npos || pos2 > response.find('>', pos0))
		{
			pos = response.find(att1, pos0);
			if (pos == string::npos)
			{
				pos0 = response.find(tagImg, pos0 + 1);
				continue;
			}
			pos += att1.length();
		}
		else pos = pos2 + att2.length();
		size_t nextQ = response.find('\"', pos);
		if (nextQ == string::npos) break;
		string imgUrlTmp = response.substr(pos, nextQ - pos);
		relativePath(imgUrlTmp, url); 
		if (visitedImg.find(imgUrlTmp) == visitedImg.end())			//没有此图片
		{
			visitedImg.insert(imgUrlTmp);
			imgurls.push_back(imgUrlTmp);
		}
		pos0 = response.find(tagImg, nextQ);
	}
}
void DownLoadImg(const vector<string>& imgUrls, const string& url) //下载图片
{
	string foldName = ".\\img\\" + ToFileName(url);
	if (!CreateDirectory(foldName.c_str(), NULL))
	{
		cout << "Fail to create directory: " << foldName << '!' << endl;
	}
	string image;
	size_t byteRead;
	for (size_t i = 0; i < imgUrls.size(); ++i)
	{
		int pos = imgUrls[i].find_last_of('.');
		if (pos == string::npos) continue;
		string ext = imgUrls[i].substr(pos + 1);
		if (ext != "bmp" && ext != "jpg" && ext != "jpeg" && ext != "gif" && ext != "png") continue;

		//下载内容
		if (GetHttpResponse(imgUrls[i], image, byteRead))
		{
			size_t pos = image.find("\r\n\r\n");
			if (pos == string::npos) continue;
			pos += strlen("\r\n\r\n");
			size_t index = imgUrls[i].find_last_of('/');
			if (index != string::npos)
			{
				string imgName = imgUrls[i].substr(index + 1);
				ofstream fout(foldName + '\\' + imgName, ios::binary);
				if (!fout) continue;
				fout.write(image.c_str() + pos, byteRead - pos);
				fout.close();
			}
			/*size_t index = imgUrls[i].find_last_of('/');
			if (index != string::npos)
			{
				string imgName = imgUrls[i].substr(index + 1);
				ofstream fout(foldName + '\\' + imgName, ios::binary);
				if (!fout) continue;
				fout.write(image.c_str(), byteRead);
				fout.close();
			}*/
		}



	}
}

void BFS(const string& url)									//处理一个url
{
	string response;											//记录网站内容
	size_t bytes;

	if (!GetHttpResponse(url, response, bytes))				//获取网站源代码
	{
		cout << "Cannot get the info of url: " << url << endl;
		return;
	}

	ofstream fout(string(".\\html\\") + ToFileName(url), ios::out);
	if (fout)												//把网页源代码写入文件并保存
	{
		fout << response << endl;
		fout.close();
	}
	else cout << "Cannot open the file: " << string(".\\html") + ToFileName(url) << endl;

	vector<string> imgUrls;									//存储图片url
	cout << "Parsing url: " << url << "..." << endl;
	HTMLParse(response, imgUrls, url);							//解析HTML，寻找超链接和图片链接

	cout << "DownLoading image..." << endl;
	DownLoadImg(imgUrls, url);								//下载图片

}

int main()
{
	WSADATA wsaData;

	if (!CreateDirectory(".\\img", NULL))					//创建img文件夹，存储爬出来的img
	{
		cout << "Directory img create failed!" << endl;
		return 1;
	}

	if (!CreateDirectory(".\\html", NULL))					//创建html文件夹，存储经过的url的网页内容
	{
		cout << "Directory html create failed!" << endl;
		return 1;
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData))				//初始化SOCKET
	{
		cout << "WSA starting up failed!" << endl;
		return 1;
	}

	string urlStart = ::urlStart;							//遍历的起始地址

	visitedUrl.insert(urlStart);
	urlQue.push(urlStart);

	while (!urlQue.empty())									//存在未解析的url，即可开始遍历
	{
		string url = urlQue.front();
		urlQue.pop();
		cout << "Start parsing: " << url << "..." << endl;
		BFS(url);
	}

	WSACleanup();											//取消SOCKET注册

	return 0;
}
