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
string urlStart = "http://www.4399.com";										//������ʼ��ַ



//û����
bool ParseURL(const string& url, string& host, string& resource)				//����������������Դ��
{
	string urlTmp = url;
	const string httpStr = "http://";
	const string httpsStr = "https://";
	size_t pos = urlTmp.find(httpStr);						//Ѱ��http://
	if (pos != string::npos) urlTmp = urlTmp.substr(httpStr.length());
	else
	{
		pos = urlTmp.find(httpsStr);						//Ѱ��https://
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

bool GetHttpResponse(const string& url, string& response, size_t& bytesRead)	//��ȡurl��ҳ��Դ���벢���ض�����ֽ���
{
	string host, resource;
	cout << "Begin to parse: " << url << "..." << endl;
	if (!ParseURL(url, host, resource))
	{
		cout << "Fail to parse: " << url << '!' << endl;
		return false;
	}

	//����SOCKET
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

	//������������ַ
	SOCKADDR_IN sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(80);								//htons: ��u_short����������Сβ˳��(С�˶���)��ɴ�β˳��(��˶���)
	memcpy((void*)&sa.sin_addr, hp->h_addr, 4);

	//��������
	cout << "Connecting to " << url << "..." << endl;
	if (connect(sock, (sockaddr*)&sa, sizeof(sa)))
	{
		closesocket(sock);
		cout << "Fail to connect to " << url << '!' << endl;
		return false;
	}

	//׼����������
	string request = "GET " + resource + " HTTP/1.1\r\nHost:" + host + "\r\nConnection:Close\r\n\r\n";

	//��������
	cout << "Sending data..." << endl;
	if (send(sock, request.c_str(), request.length(), 0) == SOCKET_ERROR)
	{
		closesocket(sock);
		cout << "Faile to send data to " << url << endl;
		return false;
	}

	//��������
	int contentLength = DEFAULT_PAGE_BUF_SIZE;						//����������
	char* pageBuf = new char[contentLength];						//������
	memset(pageBuf, 0, contentLength * sizeof(char));				//����
	bytesRead = 0;													//һ���������ֽ���
	int ret = 1;													//���ζ������ֽ���

	cout << "Begin to read...\nRead(bytes): ";
	while (ret > 0)
	{

		//��������
		ret = recv(sock, pageBuf + bytesRead, contentLength - bytesRead, 0);

		if (ret > 0) bytesRead += ret;
		if (contentLength - bytesRead < 100)						//�������������·����ڴ�
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

	//�����ȡ���ַ���������ֱ�Ӹ�ֵ����'\0'��ֹͣ
	memcpy((char*)response.c_str(), pageBuf, bytesRead + 1); 
	delete[] pageBuf;
	closesocket(sock);
	return true;
}


string ToFileName(const string& url)						//����ַת��Ϊ�ļ���
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

void relativePath(string& url, const string& fatherUrl)									//������Ե�ַ
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


//û����
void HTMLParse(const string& response, vector<string>& imgurls, const string& url)		//����HTML��Ѱ�ҳ����Ӻ�ͼƬ����
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
		if (visitedUrl.find(urlTmp) == visitedUrl.end())			//û�з��ʹ�����վ
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
		if (visitedImg.find(imgUrlTmp) == visitedImg.end())			//û�д�ͼƬ
		{
			visitedImg.insert(imgUrlTmp);
			imgurls.push_back(imgUrlTmp);
		}
		pos0 = response.find(tagImg, nextQ);
	}
}
void DownLoadImg(const vector<string>& imgUrls, const string& url) //����ͼƬ
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

		//��������
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

void BFS(const string& url)									//����һ��url
{
	string response;											//��¼��վ����
	size_t bytes;

	if (!GetHttpResponse(url, response, bytes))				//��ȡ��վԴ����
	{
		cout << "Cannot get the info of url: " << url << endl;
		return;
	}

	ofstream fout(string(".\\html\\") + ToFileName(url), ios::out);
	if (fout)												//����ҳԴ����д���ļ�������
	{
		fout << response << endl;
		fout.close();
	}
	else cout << "Cannot open the file: " << string(".\\html") + ToFileName(url) << endl;

	vector<string> imgUrls;									//�洢ͼƬurl
	cout << "Parsing url: " << url << "..." << endl;
	HTMLParse(response, imgUrls, url);							//����HTML��Ѱ�ҳ����Ӻ�ͼƬ����

	cout << "DownLoading image..." << endl;
	DownLoadImg(imgUrls, url);								//����ͼƬ

}

int main()
{
	WSADATA wsaData;

	if (!CreateDirectory(".\\img", NULL))					//����img�ļ��У��洢��������img
	{
		cout << "Directory img create failed!" << endl;
		return 1;
	}

	if (!CreateDirectory(".\\html", NULL))					//����html�ļ��У��洢������url����ҳ����
	{
		cout << "Directory html create failed!" << endl;
		return 1;
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData))				//��ʼ��SOCKET
	{
		cout << "WSA starting up failed!" << endl;
		return 1;
	}

	string urlStart = ::urlStart;							//��������ʼ��ַ

	visitedUrl.insert(urlStart);
	urlQue.push(urlStart);

	while (!urlQue.empty())									//����δ������url�����ɿ�ʼ����
	{
		string url = urlQue.front();
		urlQue.pop();
		cout << "Start parsing: " << url << "..." << endl;
		BFS(url);
	}

	WSACleanup();											//ȡ��SOCKETע��

	return 0;
}
