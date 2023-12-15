//=============================================
//BLACKJACK
//=============================================

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define CARDNUM 13
#define PORTNUM 9000
#define IPADDR "127.0.0.1"
#define WAITING 3 //wating num
#define HAND 11

//===========================================
//Prototype
//===========================================

void ShowInfo();
void SetServerSocket();
void ErrorCheck(int num, char* str);
void SetBalance(int client_fd);
void GameStart();
void ReceiveBet(int client_fd);
void SendCardClient(int client_fd);
void SendBet(int client_fd);
int ReceiveRetry(int client_fd);
void SetServerDraw();
void ReceiveClientDraw(int client_fd);
void SendResult(int client_fd);
void GameResult();
void Shuffle();
void ServerDraw();
void ClientDraw();
void ServerPrintHand();
void ClientPrintHand();

//===========================================
//Global Variable
//===========================================

int server_fd, client_fd;
int balance;//소지금을 저장하는 변수이다. 
int bet;//현재 판의 베팅 금액을 저장하는 변수이다. 
int buff;//기존의 clientcard.draw의 값을 저장해둔 buffer이다. 
int win;//2:client win, 1: server win, 0: draw
int Deck_Spade[CARDNUM];//중복을 찾기 위한 변수이다. 문양별로 카드 갯수 만큼 만들었다. 
int Deck_Clover[CARDNUM];
int Deck_Heart[CARDNUM];
int Deck_Diamond[CARDNUM];
typedef struct {//뽑은 카드들의 정보를 모아놓은 구조체이다. 최대로 뽑을수 있는 카드는 11개로 1*4,2*4,3*3로 이 이상을 넘어가면 bust이다. 
	int suit[HAND];
	int number[HAND];
	int count;
	int draw;
	int bust;
} Card;
Card Client_Card;
Card Server_Card;

int main(int argc, char* argv[]) {
	system("clear");//화면 초기화ㅓ 

	SetServerSocket();//통신 설정 
	srand(time(NULL));//난수 생성을 위한 코드 
	SetBalance(client_fd);//소지금을 입력 받는 함수 
	do {//먼저 한번은 카드를 받아야 하기 때문에 do while문 사용 
		ReceiveBet(client_fd);//베팅금액을 받는 함수 
		ShowInfo();//현재 소지금과 배팅 금액을 출력하는 함수 
		GameStart();//소지금과 배팅 금액을 바탕으로 게임시작 
	} while (ReceiveRetry(client_fd));//client에 retry 의사를 수신하여 1,0으로 return 1:retry 2:stop 
	printf("Game End.\n");//게임이 끝남을 출력 
	close(server_fd);
	close(client_fd);
}
void SetServerSocket() {
	struct sockaddr_in server_add, client_add;
	socklen_t client_add_size;

	//TCP Socket Creating
	server_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	ErrorCheck(server_fd, "Creating Socket");

	//Struct variable setting
	memset(&client_add, 0, sizeof(client_add));
	memset(&server_add, 0, sizeof(server_add));
	server_add.sin_family = AF_INET;//Socket type
	server_add.sin_port = htons(PORTNUM);//portnumber
	server_add.sin_addr.s_addr = inet_addr(IPADDR);//ipaddress

	ErrorCheck(bind(server_fd, (struct sockaddr*)&server_add, sizeof(server_add)), "Socket Bind");

	ErrorCheck(listen(server_fd, WAITING), "Connection Ready...");

	client_add_size = sizeof(client_add);

	client_fd = accept(server_fd, (struct sockaddr*)&client_add, &client_add_size);
	printf("** Connection Request... <IP> %s || <PORT> %d **", inet_ntoa(client_add.sin_addr), ntohs(client_add.sin_port));
	ErrorCheck(client_fd, "Connection");
}

void ErrorCheck(int num, char* str) {
	if (num == -1) {
		fprintf(stderr, "%s ERROR\n", str);
		exit(1);
	}
	else {
		fprintf(stdout, "%s FINISH\n", str);
	}
}

void SetBalance(int client_fd) {
	int read_size;//읽어올 데이터의 크기를 저장하는 변수 
	while (1) {//입력을 받을때까지 wait
		read_size = recv(client_fd, &balance, sizeof(balance), 0);//소지금을 client로 부터 수신 
		if (read_size > 0) break;
	}
	system("clear");
	printf("******************************************\n");
	printf("*                                        *\n");
	printf("*        Player's budget is %d.         *\n", balance);
	printf("*                                        *\n");
	printf("******************************************\n");
	ErrorCheck(read_size, "Data Receive");
	fflush(stdout);//화면 초기화후 player의 budget 출력, 출력이 안될경우 fflush함수를 사용해 stdout 비우며 출력 
}

void ShowInfo() {
	system("clear");
	printf("******************************************\n");
	printf("*                                        *\n");
	printf("*        Player's budget is %d.         *\n", balance);
	printf("*   Betting amount for this game is %d.  *\n", bet);
	printf("*                                        *\n");
	printf("******************************************\n\n");
}//게임 진행중 현재 client의 소지금과 베팅금액 출력 

void GameStart() {
	Shuffle();//deck과 Client,Server의 카드를 초기화 
	while ((Server_Card.bust && Client_Card.bust) && (Server_Card.draw || Client_Card.draw)) {//게임 진행 조건은 (client가 draw(뽑기)를 원하거나 server가 draw를 원하고) 그리고(client가 bust가 아니고 server도 bust가 아니어야한다). 
		if (Server_Card.draw) {//server가 draw를 원하면 실행 
			ServerDraw();//server가 카드를 뽑는다. servercard에 number와 suit을 저장하고 bust인지 확인 
			ShowInfo();//상단의 정보 출력 
			SetServerDraw();//server가 카드를 추가로 뽑을지 결정하는 알고리즘이다. 
			ServerPrintHand();//server가 그동안 뽑은 카드를 출력한다. 
		}
		if (!Server_Card.draw) {//server가 draw를 하지 않는데 client가 추가로 draw할경우 ShowInfo가 불리지 않아 화면이 초기화 되지 않아 만든 조건문이다. 
			ShowInfo();//화면을 초기화 하며 상단에 정보를 출력한다. 
			ServerPrintHand();//server가 그동안 뽑은 카드를 출력한다. 
		}
		if (Client_Card.draw || !Server_Card.bust) {//client가 뽑기를 원하거나 server가 bust면 실행한다. 이는 server가 bust지만 이미 이전에 client에 draw를 request 받았을 가능성이 있기때문에 server bust일때 라는 조건을 추가했다. 
			if (buff == 0) break;//만일 client가 이전에 draw를 원하지 않는다고 전송 했을시 break로 탈출하게 된다. 
			ClientDraw();//client가 카드를 뽑는다. 
			SendCardClient(client_fd);//client가 그동안 뽑았던 카드 정보를 client로 전송한다. 
			ClientPrintHand();//client가 그동안 뽑은 카드를 출력한다. 
			if (!Client_Card.bust) break;//만일 client가 bust라면 다시 뽑을지 물어보지 않기 위해 break한다. 
			if (!Server_Card.bust) break;//만일 server가 bust라면 break한다. server가 버스트라면 기존에 client가 카드를 뽑는다고 요청을 보냈을수 있기때문에 이 상황을 처리하기 위해 작성했다. 
			ReceiveClientDraw(client_fd);//client가 다시 뽑을지에 대한 의사를 수신 받는다. 
		}
	}
	GameResult();//게임의 승패를 계산하고 server 카드를 client로 전송한다. client가 왜 졌는지 알아야 하기 때문이다. 
	SendResult(client_fd);//결과를 client로 전송한다. 
	SendBet(client_fd);//승패로 인한 balance 변동을 client로 전송한다. 
}

void SetServerDraw() {
	int sum = 0;//합을 저장하기 위한 변수이다. 
	for (int i = 0; i < Server_Card.count; i++) {
		sum += Server_Card.number[i];
	}//그동안 뽑은 카드의 합을 구한다. 
	if (sum > 18) {//만일 합이 18을 넘어가면 server는 더이상 카드를 뽑지 않는다. 
		Server_Card.draw = 0;
	}
}

void ReceiveClientDraw(int client_fd) {//client가 카드를 뽑을지에 대한 의사를 수신하는 함수이다. 
	ssize_t received_bytes = recv(client_fd, &buff, sizeof(Client_Card.draw), 0);
	Client_Card.draw = buff;//buff에 저장하여 clientcard.draw의 초기 값을 알수 있다. clientcard.draw가 0이 된게 client가 더이상 뽑고 싶지 않은건지 sever의 bust때문인지 알수 있다. 
}

void ReceiveBet(int client_fd) {//beting 금액을 수신받고 출력하는 함수이다. 
	int read_size = recv(client_fd, &bet, sizeof(bet), 0);
	printf("******************************************\n");
	printf("*                                        *\n");
	printf("*  Betting amount for this game is %d.  *\n", bet);
	printf("*                                        *\n");
	printf("******************************************\n");
	ErrorCheck(read_size, "Data Receive");
}

int ReceiveRetry(int client_fd) {//게임을 다시할지에 대한 정보를 수신하는 함수 이다. 
	int retry;
	int read_size = recv(client_fd, &retry, sizeof(retry), 0);
	return retry;
}

void SendCardClient(int client_fd) {//client가 그동안 뽑은 카드의 정보를 담은 구조체를 client로 전송하는 함수이다. 
	ssize_t send_bytes = send(client_fd, &Client_Card, sizeof(Client_Card), 0);
}

void SendResult(int client_fd) {//계산한 결과를 client에 전송하는 함수이다. 
	ssize_t send_bytes = send(client_fd, &win, sizeof(win), 0);
}

void SendBet(int client_fd) {//승패에 따른 balance 변동을 계산하고 client에 전송하는 함수이다. 
	int after_balance = balance;
	if (win == 2) {
		after_balance += bet;
	}
	else if (win == 1) {
		after_balance -= bet;
	}
	ssize_t send_bytes = send(client_fd, &after_balance, sizeof(win), 0);
	balance = after_balance;//만일 draw인경우 balance는 그대로 다시 저장된다. 
}

void GameResult() {//결과를 계산하는 함수이다. client 승리:2 client 패배:1 0:무승부 
	int Client_Sum = 0;//client가 그동안 뽑은 카드의 총합을 저장하는 변수이다. 
	int Server_Sum = 0;//server가 그동안 뽑은 카드의 총합을 저장하는 변수이다. 
	ssize_t send_bytes = send(client_fd, &Server_Card, sizeof(Server_Card), 0);
	printf("=========================================\n");
	printf("||             Game Result             ||\n");
	if (!Client_Card.bust && !Server_Card.bust) {//client server가 모두 bust일때 상태이다. 
		win = 0;
		printf("||                Draw.                ||\n");
		printf("=========================================\n");
		printf("Waiting for player...");
		fflush(stdout);
		return;
	}
	else if (!Client_Card.bust) {//client가 bust일때 상태이다. 
		win = 1;
		printf("||             Dealer Win.             ||\n");
		printf("=========================================\n");
		printf("Waiting for player...");
		fflush(stdout);
		return;
	}
	else if (!Server_Card.bust) {//server가 bust일때 상태이다. 
		win = 2;
		printf("||             Dealer Lose.            ||\n");
		printf("=========================================\n");
		printf("Waiting for player...");
		fflush(stdout);
		return;
	}//아니라면 총합을 구해 client와 server를 비교하여 승패를 가른다. 
	for (int i = 0; i < Client_Card.count; i++) {
		Client_Sum += Client_Card.number[i];
	}
	for (int i = 0; i < Server_Card.count; i++) {
		Server_Sum += Server_Card.number[i];
	}
	if (Client_Sum > Server_Sum) {
		win = 2;
		printf("||             Dealer Lose.            ||\n");
		printf("=========================================\n");
		printf("Waiting for player...\n");
	}
	else if (Client_Sum < Server_Sum) {
		win = 1;
		printf("||             Dealer Win.             ||\n");
		printf("=========================================\n");
		printf("Waiting for player...\n");
	}
	else {
		win = 0;
		printf("||               Draw.                 ||\n");
		printf("=========================================\n");
		printf("Waiting for player...\n");
	}
	fflush(stdout);
}

void Shuffle() {//카드를 섞는 함수로 deck,client,server.buff를 모두 초기상태로 리셋한다. 
	buff = 1;
	Client_Card.bust = 1;
	Server_Card.bust = 1;
	Client_Card.draw = 1;
	Server_Card.draw = 1;
	Client_Card.count = 1;
	Server_Card.count = 1;
	for (int i = 0; i < 11; i++) {
		Deck_Spade[i] = 0;
		Deck_Clover[i] = 0;
		Deck_Heart[i] = 0;
		Deck_Diamond[i] = 0;
		Client_Card.suit[i] = 0;
		Client_Card.number[i] = 0;
		Server_Card.suit[i] = 0;
		Server_Card.number[i] = 0;
	}
	for (int i = 11; i < 13; i++) {
		Deck_Spade[i] = 0;
		Deck_Clover[i] = 0;
		Deck_Heart[i] = 0;
		Deck_Diamond[i] = 0;
	}
}

void ServerDraw() {
	int draw_num;//숫자에 들어갈 난수를 저장하는 변수이다. 
	int draw_suit;//문양에 들어갈 난수를 저장하는 변수이다. 
	int sum = 0;//bust인지 계산하기 위한 변수이다. 
	int same = 1;//중복 문자인지 검사하기 위한 변수이다. 
	do {//난수를 먼저 뽑고 비교하고 다시 뽑을지 결정해야하므로 do while문을 사용했다. 
		draw_num = rand() % 13 + 1;//숫자 난수 생성 코드, 1~13까지의 난수 저장 
		draw_suit = rand() % 4 + 1;//문양 난수 생성 코드, 1~4까지의 난수 저장 
		for (int i = 1; i < Server_Card.count; i++) {//그동안 뽑은 카드중에 같은 카드가 있는지 확인하는 코드이다. 
			if (Server_Card.number[i] == draw_num && Server_Card.suit[i] == draw_suit) {//만일 servercard.number와 servercard.suit가 현재 뽑은 카드와 모두 일치할때 상태이다. 
				same = 1;//조건을 만족하면 same을 1로저장하여 중복되지 않을때까지 뽑는다. 
				break;//해당 카드는 중복이므로 break해서 연산량을 줄였다. 
			}
			else same = 0;//만일 중복이 아니라면 0으로 저장후 다른 카드가 중복인지 for문을 통해 확인한다. 
		}
	} while (same && Server_Card.count > 1);//servercard의 count가 1이면 중복할 카드를 가지지 않았기 때문에 한번 실행후 loop를 빠져나온다. 
	Server_Card.suit[Server_Card.count] = draw_suit;//문양을 저장한다. 
	Server_Card.number[Server_Card.count] = draw_num;//숫자를 저장한다. 
	Server_Card.count += 1;//카드 갯수를 1증가한다. 
	for (int i = 1; i < Server_Card.count; i++) {
		sum += Server_Card.number[i];
	}//카드의 총합을 구한다. 
	if (sum > 21) {//뽑은 카드들이 bust되는지 확인하고 bust라면 servercard의 draw와 bust를 0으로 초기화해서 결과 계산시 알수 있게한다. 
		Server_Card.draw = 0;
		Server_Card.bust = 0;
		Client_Card.draw = 0;//client카드는 server가 bust이므로 더 뽑을 필요없이 승리이기 때문에 draw를 0으로 저장한다. 하지만 server가 클라이언트보다 먼저 뽑기 때문에 client가 뽑기 요청을 보냈다면 buff에 저장해둔 변수로 비교한다. 
	}
}

void ClientDraw() {//client가 카드를 뽑는 함수이다. 
	int sum = 0;
	int same = 1;
	int draw_num;
	int draw_suit;
	do {
		draw_num = rand() % 13 + 1;
		draw_suit = rand() % 4 + 1;
		for (int i = 1; i < Client_Card.count; i++) {
			if (Client_Card.number[i] == draw_num && Client_Card.suit[i] == draw_suit) {
				same = 1;
				break;
			}
			else same = 0;
		}
	} while (same && Client_Card.count > 1);
	Client_Card.suit[Client_Card.count] = draw_suit;
	Client_Card.number[Client_Card.count] = draw_num;
	Client_Card.count += 1;
	for (int i = 1; i < Client_Card.count; i++) {
		sum += Client_Card.number[i];
	}
	if (sum > 21) {
		Client_Card.bust = 0;//만일 client가 bust라면 gamestart함수의 while문 안에 clientcard.draw조건문에서 break를 통해 나오므로 servercard.draw는 저장하지 않는다. 
	}
}

void ClientPrintHand() {//client가 뽑은 카드를 출력하는 함수이다. 
	printf("Player's Card:");
	for (int i = 1; i < Client_Card.count; i++) {
		switch (Client_Card.suit[i]) {
		case 1:
			printf(" Spade[%d]", Client_Card.number[i]);
			break;
		case 2:
			printf(" Diamond[%d]", Client_Card.number[i]);
			break;
		case 3:
			printf(" Heart[%d]", Client_Card.number[i]);
			break;
		case 4:
			printf(" Clover[%d]", Client_Card.number[i]);
			break;
		default:
			break;
		}
	}
	fflush(stdout);
	printf("\n\n");
}

void ServerPrintHand() {//server가 뽑은 카드를 출력하는 함수이다. 
	printf("Dealer Card:");
	for (int i = 1; i < Server_Card.count; i++) {
		switch (Server_Card.suit[i]) {
		case 1:
			printf(" Spade[%d]", Server_Card.number[i]);
			break;
		case 2:
			printf(" Diamond[%d]", Server_Card.number[i]);
			break;
		case 3:
			printf(" Heart[%d]", Server_Card.number[i]);
			break;
		case 4:
			printf(" Clover[%d]", Server_Card.number[i]);
			break;
		default:
			break;
		}
	}
	fflush(stdout);
	printf("\n\n");
}
