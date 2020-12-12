// VideostreamerClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#pragma comment(lib,"ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

//#include <Winsock.h>
#include <WinSock2.h>
#include <iostream>
#include <vector>
#include <string>
#include <stdio.h>
#include <opencv2\opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <math.h>
#include <sys/types.h>
#include <string>
#include <iomanip>
#include <sstream>

using namespace std;
using namespace cv; 

void DownloadFile(SOCKET Socket) {
	cout << "client audio download";
	if (Socket == NULL) {
		return;
	}
	while (1) {
		char filename[1024];
		recv(Socket, filename, sizeof(filename), 0);
		if (filename[0] == '.') {
			break;
		}
		FILE** fp = NULL; //= fopen(filename, "r");
		fopen_s(fp, filename, "r");
		fseek(*fp, 0, SEEK_END);
		long FileSize = ftell(*fp);
		char GotFileSize[1024];
		_itoa_s(FileSize, GotFileSize, 10);
		send(Socket, GotFileSize, 1024, 0);
		rewind(*fp);
		long SizeCheck = 0;
		char* mfcc;
		if (FileSize > 1499) {
			mfcc = (char*)malloc(1500);
			while (SizeCheck < FileSize) {
				int Read = fread_s(mfcc, 1500, sizeof(char), 1500, *fp);
				int Sent = send(Socket, mfcc, Read, 0);
				SizeCheck += Sent;
			}
		}
		else {
			mfcc = (char*)malloc(FileSize + 1);
			fread_s(mfcc, FileSize, sizeof(char), FileSize, *fp);
			send(Socket, mfcc, FileSize, 0);
		}
		fclose(*fp);
		Sleep(500);
		free(mfcc);
	}
	return;
}


int main()
{
	WSADATA wsaData;
	WORD DllVersion = MAKEWORD(2, 1);

	if (WSAStartup(DllVersion, &wsaData) != 0) {
		MessageBoxA(NULL, "Startup error", "ERROR", MB_OK | MB_ICONERROR);
		exit(1);
		cout << "controllo non funziona" << endl;
	}

	SOCKADDR_IN addr;
	int sizeofaddr = sizeof(addr);

	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(1111); //porta da aprire
	addr.sin_family = AF_INET;

	SOCKET Connection = socket(AF_INET, SOCK_STREAM, NULL); //SOCK_STREAM,NULL

	if (connect(Connection, (SOCKADDR*)&addr, sizeofaddr) != 0) {
		MessageBoxA(NULL, "connection error", "error", MB_OK | MB_ICONERROR);
	}


	cout << "Client connected" << endl;


	char messaggio[512];
	recv(Connection, messaggio, sizeof(messaggio), NULL);
	cout << messaggio << endl;

	//Messaggio di test
	string expected(messaggio);
	if (!expected.compare("TEST")) {
		cout << "TEST OK" << endl;
		memset(&messaggio[0], 0, sizeof(messaggio));
		strcpy_s(messaggio, "CLIENT CHIEDE FILM DISPONIBILI");
		send(Connection, messaggio, sizeof(messaggio), NULL);
	}
	else {
		cout << "TEST NOT OK" << endl;
	}
	//lista films 
	int Nfilms;
	vector<string> List_of_films;
	recv(Connection, (char*)&Nfilms, sizeof(Nfilms), NULL);
	cout << "LISTA DI FILM" << endl;
	cout << "Numero Films: " << Nfilms << endl;
	cout << "-------------------" << endl;
	for (int i = 0;i < Nfilms;i++) {
		memset(&messaggio[0], 0, sizeof(messaggio));
		string temp;
		recv(Connection, messaggio, sizeof(messaggio), NULL);
		//cout << messaggio << endl;
		List_of_films.push_back((string)messaggio);
		cout << i << ":" << (string)List_of_films[i] << endl;

	}
	cout << "-------------------" << endl;
	// seleziona films 
	int Selected;
	bool cond;

	do
	{
		cout << "Enter an integer number:";
		cin >> Selected;

		cond = cin.fail();

		cin.clear();
		cin.ignore(INT_MAX, '\n');

	} while (cond);

	//send choosen film to server 
	Selected = htonl(Selected);
	send(Connection, (char*)&Selected, sizeof(Selected), NULL);


	//opencv code 

	Mat img;
	img = Mat::zeros(480, 640, CV_8UC3);
	int imgSize = img.total() * img.elemSize();
	uchar *iptr = img.data;
	int bytes = 0;
	int key = 0;
	
	//make img continuos
	if (!img.isContinuous()) {
		img = img.clone();
	}

	std::cout << "Image Size:" << imgSize << std::endl;
	
	
	// inizialize quality factor 
	int QualityFactor = 100 ;
	int Modality = 0; 

	cout << "CHOOSE THE MODALITY: 0 for jpeg_compression 1 for ffmpeg_compression" << endl;
	cin >> Modality;
	send(Connection, (char*)&Modality, sizeof(Modality), NULL);
	bool end_of_film = 1; 

	//motion jpeg modality
	if (Modality == 0) {
		cond = 1;
		do
		{
			cout << "CHOOSE THE QUALITY FACTOR: 0 to 100 " << endl;
			cin >> QualityFactor;

			cond = cin.fail();
			cin.clear();
			cin.ignore(INT_MAX, '\n');

		} while (cond);

		send(Connection, (char*)&QualityFactor, sizeof(QualityFactor), NULL);

		//receive stream 
		int bufSize = 0;

		//receive audio file
		
		//DownloadFile(Connection);
		
		 
		while (end_of_film) {


			//receive buffer size 
			recv(Connection, (char*)&bufSize, sizeof(int), NULL);
			cout << "BUFSIZE: " << bufSize << endl;

			if (bufSize == 0) {
				cout << "buffer size equal to zero" << endl;
				break;
			}
			//receive image 
			if (bufSize != 0) {

				vector<uchar> buf(bufSize, 0);

				if ((bytes = recv(Connection, (char*)buf.data(), bufSize * sizeof(char), MSG_WAITALL)) == -1) { //(char*)iptr, imgSize,MSG_WAITALL
					std::cerr << "recv failed, received bytes = " << bytes << std::endl;
					break;
				}

				//cout << "BUFSIZE: " << buf.size() << endl;
				img = imdecode(Mat(buf), 1);
				namedWindow("CV Video Client", CV_WINDOW_NORMAL);
				resizeWindow("CV Video Client", 720, 576);
				imshow("CV Video Client", img); 
			}

			if (key = cv::waitKey(10) >= 0) break;
			if (img.empty()) end_of_film = 0;
			int k = waitKey(1);
			if (k == 27)
			{
				break;
			}

		}
		cout << "film finito" << endl;
	}
	//ffmpeg compression modality
	if(Modality==1){

		cout << "Modality: 1"<<endl;
		cond = 1;
		int BitRate = 0; 

		do
		{
			cout << "CHOOSE THE BIT RATE IN KILOBITS" << endl;
			cin >> BitRate;
			cond = cin.fail();
			cin.clear();
			cin.ignore(INT_MAX, '\n');

		} while (cond);
		send(Connection, (char*)&BitRate, sizeof(BitRate), NULL);
		int counter = 0;

		system("ffplay udp://127.0.0.1:1234");
	}


	cout<<"client ended"<<endl;
	closesocket(Connection);
	//system("pause");
	return 0;
}


