#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <queue>
#include <set>
#include <WinSock2.h>
#include <ctime>
#include <cstdlib>
#include <cassert>

#pragma warning(disable: 4996)

#define DEFAULT_PAGE_BUF_SIZE 1048576

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const string htmlDir("html"); 
const string imgDir("img"); 
const string htmlTextDir("htmlText"); 

queue<string> urlQue;
set<string> visitedUrl;
set<string> visitedImg;
string urlStart = "http://www.7k7k.com/tag/72/"; 								//������ʼ��ַ



//û����
bool ParseURL(string& url, string& host, string& resource)				//����������������Դ��
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
		url += '/'; 
	}
	else
	{
		host = urlTmp.substr(0, pos);
		resource = urlTmp.substr(pos);
	}
	return true;
}

bool GetHttpResponse(string& url, string& response, size_t& bytesRead)	//��ȡurl��ҳ��Դ���벢���ض�����ֽ���
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
		cout << "Fail to send data to " << url << endl;
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
			//�����ÿո�win�ļ������Ŀո���Զ���ȡ�����������ļ���ʧ��
			fileName += 'X';
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
	size_t pos1 = fatherUrl.find_last_of('/'), pos2; 
	if (fatherUrl.find("http") == 0)
	{
		size_t posTmp = fatherUrl.find(':'); 
		if (posTmp != string::npos)
		{
			posTmp += 1; 
			while (posTmp < fatherUrl.length() && fatherUrl[posTmp] == '/') ++posTmp; 
			pos2 = fatherUrl.find_first_of('/', posTmp); 
		}
		else pos2 = fatherUrl.find_first_of('/'); 
	}
	else pos2 = fatherUrl.find_first_of('/'); 
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

void saveHtmlCorr(const string& buf, const string& url)
{
	size_t htmlBegPos = 0; 
	string att1 = "src=\""; 
	size_t len = att1.length(); 
	size_t pos = buf.find(att1, htmlBegPos); 
	string fileName = ToFileName(url) + string(".html"); 
	ofstream foutCorr(".\\" + htmlDir + "\\" + fileName, ios::out); 
	if (!foutCorr)
	{
		ofstream ferr("error.txt", ios::app);
		if (!ferr) MessageBox(NULL, "Cannot attach to file \"error\"! ", NULL, MB_OK | MB_ICONWARNING);
		else
		{
			ferr << "Cannot open the file: " << ".\\" + htmlDir + "\\" + fileName << "\n\t->Source URL: " << url << endl;
			ferr.close();
		}
		return; 
	}

	////foutCorr << buf; 
	////return; 

	while (pos != string::npos)
	{
		pos += len; 
		size_t nextQ = buf.find('\"', pos);
		if (nextQ == string::npos) break; 
		string srcUrlTmp = buf.substr(pos, nextQ - pos); 
		relativePath(srcUrlTmp, url); 
		if (pos >= htmlBegPos)
		{
			if (pos != htmlBegPos) foutCorr << buf.substr(htmlBegPos, pos - htmlBegPos); 
			foutCorr << srcUrlTmp << '\"'; 
			htmlBegPos = nextQ + 1; 
		}
		pos = buf.find(att1, htmlBegPos); 
	}

	if (htmlBegPos != string::npos) foutCorr << buf.substr(htmlBegPos); 
	foutCorr.close(); 

}

//û����
void HTMLParse(const string& response, vector<string>& imgurls, const string& url)		//����HTML��Ѱ�ҳ����Ӻ�ͼƬ����
{
	string tagHref = "href=\"";
	size_t pos = response.find(tagHref);
	ofstream fout("url.txt", ios::app);
	ostringstream sout; 
	size_t htmlBegPos = response.find("\r\n\r\n"); 
	if (htmlBegPos != string::npos) htmlBegPos += strlen("\r\n\r\n"); 
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
			urlQue.push(urlTmp);
			visitedUrl.insert(urlTmp); 
			fout << urlTmp << endl;
		}
		if (pos >= htmlBegPos)
		{
			if (pos != htmlBegPos) sout << response.substr(htmlBegPos, pos - htmlBegPos); 
			sout << urlTmp << '\"'; 
			htmlBegPos = nextQ + 1; 
		}
		pos = response.find(tagHref, nextQ);
	}
	fout << endl << endl;
	fout.close();
	if (htmlBegPos != string::npos) sout << response.substr(htmlBegPos); 
	saveHtmlCorr(sout.str(), url); 

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
void DownLoadImg(vector<string>& imgUrls, const string& url) //����ͼƬ
{
	string foldName = ".\\" + imgDir + "\\" + ToFileName(url); 
	if (foldName.length() > 200) foldName = foldName.substr(0, 195) + "..."; 
	bool hasCreated = false; 
	string image;
	size_t byteRead;
	for (size_t i = 0; i < imgUrls.size(); ++i)
	{
		size_t posd = imgUrls[i].find_last_of('.');
		if (posd == string::npos) continue;
		string ext = imgUrls[i].substr(posd + 1);
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
				string imgName = ToFileName(imgUrls[i].substr(index + 1));
				if (!hasCreated)
				{
					if (!CreateDirectory(foldName.c_str(), NULL))
					{
						cout << "Fail to create directory: " << foldName << '!' << endl;
					}
					hasCreated = true; 
				}
				ofstream fout((foldName + "\\" + imgName).c_str(), ios::binary | ios::out);
				if (!fout)
				{
					ofstream ferr("error.txt", ios::app);
					if (!ferr) MessageBox(NULL, "Cannot attach to file \"error\"! ", NULL, MB_OK | MB_ICONWARNING);
					else
					{
						ferr << "Cannot open the file: " << foldName + '\\' + imgName << "!\n\t->Source URL: " << imgUrls[i] << endl; 
						ferr.close(); 
					}
					continue; 
				}
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

void BFS(string& url)									//����һ��url
{
	string response;											//��¼��վ����
	size_t bytes;

	if (!GetHttpResponse(url, response, bytes))				//��ȡ��վԴ����
	{
		cout << "Cannot get the info of url: " << url << endl;
		return;
	}

	string fileName = ToFileName(url) + string(".txt"); 
	ofstream fout(".\\" + htmlTextDir + "\\" + fileName, ios::out);
	if (fout)												//����ҳԴ����д���ļ�������
	{
		fout << response << endl;
		fout.close();
	}
	else
	{
		ofstream ferr("error.txt", ios::app);
		if (!ferr) MessageBox(NULL, "Cannot attach to file \"error\"! ", NULL, MB_OK | MB_ICONWARNING);
		else
		{
			ferr << "Cannot open the file: " << ".\\" + htmlTextDir + "\\" + fileName << "\n\t->Source URL: " << url << endl;
			ferr.close();
		}
	}

	vector<string> imgUrls;									//�洢ͼƬurl
	cout << "Parsing url: " << url << "..." << endl;
	HTMLParse(response, imgUrls, url);							//����HTML��Ѱ�ҳ����Ӻ�ͼƬ����

	cout << "DownLoading image..." << endl;
	DownLoadImg(imgUrls, url);								//����ͼƬ

}

int main()
{
	WSADATA wsaData;

	if (!CreateDirectory((".\\" + imgDir).c_str(), NULL))					//����img�ļ��У��洢��������img
	{
		cout << "Warning: Directory img create failed!" << endl;
	}

	if (!CreateDirectory((".\\" + htmlTextDir).c_str(), NULL))				//����htmlText�ļ��У��洢������url����ҳ����
	{
		cout << "Warning: Directory htmlText create failed!" << endl;
	}

	if (!CreateDirectory((".\\" + htmlDir).c_str(), NULL))					//����html�ļ��У��洢�޸Ĺ���html�ļ�
	{
		cout << "Warning: Directory html create failed!" << endl;
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData))				//��ʼ��SOCKET
	{
		cout << "WSA starting up failed!" << endl;
		return 1;
	}

	string urlStart = ::urlStart;							//��������ʼ��ַ

	cout << "Please input the start adress (input \"-d\" to use default): " << endl; 
	string input; 
	getline(cin, input); 
	if (input != "-d")
	{
		if (input.find("http") != 0) input = "http://" + input; 
		urlStart = input; 
	}

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
