#include "stdafx.h"
#include <experimental\filesystem>
#pragma comment(lib,"ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <io.h>
#include <WinSock2.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <stdio.h>
#include <fstream>
#include <direct.h>
#include <opencv2\opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <math.h>
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/videoio/videoio.hpp"
#include <iterator>

namespace fs = std::experimental::filesystem::v1;
using namespace std;
using namespace cv;


void DownloadFile(SOCKET Socket) {
	cout << "server audio upload" << endl;
	if (Socket == NULL) {
		return;
	}
	while (1) {
		char localfile[1024] = "localfile";
		char filename[1024] = "localfile";
		send(Socket, filename, sizeof(filename), 0);
		char GotFileSize[1024];
		recv(Socket, GotFileSize, 1024, 0);
		long FileSize = atoi(GotFileSize);
		long SizeCheck = 0;
		FILE *fp;
		fopen_s(&fp, localfile, "w");
		char* mfcc;
		if (FileSize > 1499) {
			mfcc = (char*)malloc(1500);
			while (SizeCheck < FileSize) {
				int Received = recv(Socket, mfcc, 1500, 0);
				SizeCheck += Received;
				fwrite(mfcc, 1, Received, fp);
				fflush(fp);
				printf("Filesize: %d\nSizecheck: %d\nReceived: %d\n\n", FileSize, SizeCheck, Received);
			}
		}
		else {
			mfcc = (char*)malloc(FileSize + 1);
			int Received = recv(Socket, mfcc, FileSize, 0);
			fwrite(mfcc, 1, Received, fp);
			fflush(fp);
		}
		fclose(fp);
		Sleep(500);
		free(mfcc);
	}
}


int main() {
	//recupera lista di films 
	vector<string> List_of_films; 
	string path = "CV_DATASET"; //"\\Users\\samuele\\Documents\\films"; 
	for (auto & p : (fs::directory_iterator(path))) {
		List_of_films.push_back(p.path().filename().string()); 
	}

	WSADATA wsaData;
	WORD DllVersion = MAKEWORD(2, 2);

	if (WSAStartup(DllVersion, &wsaData) != 0) {
		MessageBoxA(NULL, "Startup error", "ERROR", MB_OK | MB_ICONERROR);
		cout << "controllo non funziona" << endl;
		exit(1);
	}

	SOCKADDR_IN addr;
	int sizeofaddr = sizeof(addr);

	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(1111); //porta da aprire
	addr.sin_family = AF_INET;


	SOCKET slisten = socket(AF_INET, SOCK_STREAM, NULL); //SOCK_STREAM,NULL
	bind(slisten, (SOCKADDR*)&addr, sizeofaddr);
	listen(slisten, SOMAXCONN);

	SOCKET newConnection = accept(slisten, (SOCKADDR*)&addr, &sizeofaddr);

	if (newConnection == 0) {
		cout << "no connection" << endl;
	}
	else {
		cout << "Server connected" << endl;
		char messaggio[512] = "TEST";
		send(newConnection, messaggio, sizeof(messaggio), NULL); //nota che per int indirizzi 
		
		//initialize connection
		memset(&messaggio[0], 0, sizeof(messaggio));
		recv(newConnection, messaggio, sizeof(messaggio), NULL);
		string expected(messaggio);
		
		if (!expected.compare("CLIENT CHIEDE FILM DISPONIBILI")) {
			cout << expected << endl;

			int Nfilms = List_of_films.size();
			send(newConnection, (char*)&Nfilms, sizeof(Nfilms), NULL);

			//send list of possible files  
			for (int i = 0;i < List_of_films.size();i++) {
				string a(List_of_films[i]);
				char Buffer[512];
				strcpy_s(Buffer, a.c_str());
				send(newConnection, Buffer, sizeof(Buffer), NULL);


			}

			cout << "LISTA FILM INVIATA" << endl;

			//receive choosen film
			int ChoosenFilm;
			recv(newConnection, (char*)&ChoosenFilm, sizeof(ChoosenFilm), NULL);
			ChoosenFilm = ntohl(ChoosenFilm);
			

			cout << "film scelto: " << List_of_films[(int)ChoosenFilm] << endl;

			//string FpegCommand = "ffmpeg - i" + FilmPath + "- v 0 - vcodec mpeg4 - f mpegts udp ://127.0.0.1:23000";
			char Command[512];
			int BitRate = 30;
			char BitRate_Str[10];
			char name[512]; //"video.avi";
			string FilmPath = path + '\\' + List_of_films[(int)ChoosenFilm];

			

			cout << "film path" << FilmPath << endl;

			strcpy_s(name, (FilmPath).c_str());
			sprintf_s(name, "%s", name);
			sprintf_s(BitRate_Str, "%d", BitRate);

			// invio lo stream
			cout << "path: " << path << endl;

			//read video 
			VideoCapture cap(FilmPath);
			//find fps 
			double fps = cap.get(CV_CAP_PROP_FPS);

			// opencv code 
			Mat img, imgGray;
			img = Mat::zeros(480, 640, CV_8UC3);
			imgGray = img; 
			

			//make it continuous
			if (!img.isContinuous()) {
				img = img.clone();
			}

			int imgSize = img.total() * img.elemSize();
			int bytes = 0;
			int key;


			//make img continuos
			if (!img.isContinuous()) {
				img = img.clone();
				imgGray = img.clone();
			}

			std::cout << "Image Size:" << imgSize << std::endl;
		    
			//RECEIVE MODALITY
			int Modality;
			recv(newConnection, (char*)&Modality, sizeof(Modality), NULL);
			cout << "ho ricevuto la modalita': " <<Modality<< endl;

			//jpeg encode
			vector<uchar> buf;
			vector<int> params = vector<int>(2);
			params[0] = 1;
			params[1] = 100; //full quality

			if(Modality == 1){
				cout << "sono in modality" << endl;
				//receive compression rate
				int Bit_Rate=0;
				recv(newConnection, (char*)&Bit_Rate, sizeof(Bit_Rate), NULL);
				//fps

				cout << "Bit Rate: " << Bit_Rate<<"K" << endl;
				string Command = "ffmpeg -i " + FilmPath + " -vcodec libx264 -vb " + to_string(Bit_Rate -94) + "k -maxrate " + to_string(Bit_Rate -94) +"k -bufsize " + to_string((Bit_Rate-94)/8) + "k -g 60 -tune zerolatency -vprofile baseline -level 2.1 -acodec aac -ab 64000 -ar 48000 -ac 2 -vbsf h264_mp4toannexb -strict experimental -f mpegts udp://127.0.0.1:1234";
				
				//string Command = "ffmpeg -i " + FilmPath + " -c:v libx264 -ab 94k -vb " + to_string(Bit_Rate -94) + "k -g 60 -tune zerolatency -vprofile baseline -level 2.1 -acodec aac -ab 64000 -ar 48000 -ac 2 -vbsf h264_mp4toannexb -strict experimental -f mpegts udp://127.0.0.1:1234";
				cout << Command << endl;
				//while (1);

				system(Command.c_str());
			}
			else {
				// receive quality factor 
				int QualityFactor;
				recv(newConnection, (char*)&QualityFactor, sizeof(QualityFactor), NULL);

				//jpeg encode 
				vector<uchar> buf;
				vector<int> params = vector<int>(2);
				params[0] = 1;
				params[1] = QualityFactor; //10 
				
				int bufSize = 1;
				while (cap.isOpened()) {

					/* get a frame from camera */
					cap >> imgGray;

					//controllo per l'immagine vuota 
					if (imgGray.empty()) {
						cout << "immagine vuota" << endl;
						break;
					}

					//encode image
					imencode(".jpg", imgGray, buf, params);

					//getting buffer dimension 
					bufSize = buf.size();
					
					//send buffer dimension
					send(newConnection, (char*)&bufSize, sizeof(bufSize), 0);

					//send processed image
					if ((bytes = send(newConnection, (char*)buf.data(), bufSize * sizeof(char), 0)) < 0) { 
						std::cerr << "bytes = " << bytes << std::endl;
						break;
					}

					waitKey(5);
					int k = waitKey(1);
					if (k == 27)
					{
						break;
					}

				}
				//checking if the frame is empty
				// reach to the end of the video file
				bufSize = 0;
				send(newConnection, (char*)&bufSize, sizeof(bufSize), 0);
			}
	}
}
	remove("output.mp4");
	cout << "ended program" << endl;
	return 0;
}

