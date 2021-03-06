#define _CRT_SECURE_NO_WARNINGS

#include "windows.h"
#include <iostream>
#include "interface_h.h"
#include "stdio.h"
#include <string>


using namespace std;

void authorize();
void mainLoop();
void help();
boolean upload();
boolean download();
boolean fdelete();

int login = 0;

int main() {

	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	RPC_STATUS status;
	RPC_CSTR stringBinding = NULL;
	char ip[16];
	char port[5];

	//cout << "connect <ip-address> <port>" << endl;
	//cout << "> connect ";
	cout << "ip" << endl;
	cin >> ip;
	cout << "port" << endl;
	cin >> port;


	status = RpcStringBindingComposeA(
		NULL, //uuid для подключения
		(RPC_CSTR)"ncacn_ip_tcp", //использование протокола TCP/IP
		(RPC_CSTR)ip, //IP-адрес сервера
		(RPC_CSTR)port, //порт сервера
		NULL, //Опции сети, зависящие от протокола
		&stringBinding); //результат соединения

	if (status) //если соединение не прошло
		exit(status);

	status = RpcBindingFromStringBindingA( //переводит строковый заголовок в binding
		stringBinding, //строковый binding 
		&rpcHandle); //результат функции (определен в idl)

	if (status)
		exit(status);

	RpcTryExcept{
		//cout << "login <user_name> <password>" << endl;
		//cout << "> login ";
		authorize();

		while (true) {
			status = RpcMgmtIsServerListening(rpcHandle);
			if (status)
			{
				if (status == RPC_S_NOT_LISTENING)
				{
					cout << "Сервер не слушает." << endl;
					return 1;
				}
				else
				{
					mainLoop();
				}
			}
		}
	} RpcExcept(1) {
		cerr
			<< "Превышен таймаут."
			<< RpcExceptionCode()
			<< endl;
	} RpcEndExcept

		status = RpcStringFreeA(&stringBinding); //очистка памяти, выделенной под строку

	if (status)
		exit(status);

	status = RpcBindingFree(&rpcHandle); //очистка памяти, выделенной под binding

	if (status)
		exit(status);
}

void* __RPC_USER midl_user_allocate(size_t size)
{
	return malloc(size);
}

void __RPC_USER midl_user_free(void* p) //освобождение памяти
{
	free(p);
}

void authorize() {

	unsigned char username[30];
	unsigned char password[20];

	if (login != 0) {
		RevertToSelf();
		login = 0;
	}
	for (;;) {
		cout << "username" << endl;
		cin >> username;
		cout << "password" << endl;
		cin >> password;

		if (impersonization(rpcHandle, username, password) == 1)
			break;
		else
			cout << "Неверные данные." << endl;
	}
	login = 1;
	cout
		<< "Авторизация прошла успешно." << endl;

	help();
}

void mainLoop() {
	for (;;) {
		char inc[10];
		cout << endl;
		cout << "> ";
		cin >> inc;

		string in = (string)inc;

		if (in == "login")
			authorize();
		else if (in == "upload")
			upload();
		else if (in == "download")
			download();
		else if (in == "remove")
			fdelete();
		else if (in == "help")
			help();
		else if (in == "exit")
			exit(0);
		else
			cout << "Wrong command " << endl;
	}
}

void help() {
	cout
		<< endl
		<< "Commands: " << endl
		<< endl
		<< "login <user_name> <password>" << endl
		<< "upload <local_file_path>\t<remote_file_path>" << endl
		<< "download <remote_file_path>\t<local_file_path>" << endl
		<< "remove <remote_file_path>" << endl
		<< "help" << endl
		<< "exit" << endl;
}
boolean upload() {
	FILE* file;
	char from[255];
	char to[255];
	char data[1024];
	long readData;

	//cin.ignore();
	cout << "from" << endl;
	cin>>from;
	cout << "to" << endl;
	cin>>to;


	file = fopen(from, "rb");
	if (!file) {
		cout << "Ошибка чтения файла. Неверный путь." << endl << endl;
		return false;
	}

	if (createFile((RPC_CSTR)to) != 0) {
		cout << "Недосточно прав." << endl << endl;
		return false;
	}

	while (!feof(file)) {
		readData = fread(data, 1, 512, file);
		writeToFile((RPC_CSTR)to, (RPC_CSTR)data, readData);
	}

	cout << "Файл скопирован успешно." << endl << endl;
	fclose(file);
	return true;
}

boolean download() {
	FILE* file;
	char from[255];
	char to[255];
	char data[1024];
	long readData;
	long pos = 0;

	//cin.ignore();
	cout << "from" << endl;
	cin.getline(from, sizeof(from), '\t');
	cout << "to" << endl;
	cin.getline(to, sizeof(to));

	file = fopen(to, "wb");
	if (!file) {
		cout << "Ошибка открытия файла на запись." << endl << endl;
		return false;
	}

	while (readData = readFrom((RPC_CSTR)from, (RPC_CSTR)data, pos)) {

		if (readData == -1) {
			fclose(file);
			DeleteFile((LPCWSTR)to);
			cout << "Недостаточно прав." << endl << endl;
			return false;
		}

		fwrite(data, 1, readData, file);
		pos += readData;
	}

	if (readData != -1)
		cout << "Файл успешно загружен." << endl << endl;

	fclose(file);
	return true;
}

boolean fdelete() {
	char path[512];
	int err;

	cin.ignore();
	cin.getline(path, sizeof(path));

	err = deleteFile((RPC_CSTR)path);

	if (err == 1) {
		cout << "Файл удален успешно." << endl << endl;
		return true;
	}

	cout << "Недостаточно прав." << endl << endl;
	return false;
}

