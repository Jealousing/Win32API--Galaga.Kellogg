#include <windows.h>
#include <time.h>
#include <string.h>
#include "resource.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE g_hInst;
LPCTSTR lpszClass = TEXT("Dongu Galaga");//윈도우 창 이름설정
static HWND hWnd;
static HDC hdc, BACKDC;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance
    , LPSTR lpszCmdParam, int nCmdShow)
{
    MSG Message;
    WNDCLASS WndClass;
    g_hInst = hInstance;

    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    WndClass.hInstance = hInstance;
    WndClass.lpfnWndProc = WndProc;
    WndClass.lpszClassName = lpszClass;
    WndClass.lpszMenuName = NULL;
    WndClass.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClass(&WndClass);

    hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 1000,//윈도우 크기 설정
        NULL, (HMENU)NULL, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);

    while (GetMessage(&Message, NULL, 0, 0)) {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }
    return (int)Message.wParam;
}

//단순 매크로로 상수 지정
#define MAX_OBJ 1000            //오브젝트 최대 개수
#define PLAYER_MAX_OBJ 5    //플레이어 발사체 개수
#define PLAYER_MOVE 20       //플레이어 이동속도
#define NPC_MOVEX 5             //NPC 좌우 이동속도
#define OBJ_MOVEY 20           //발사체 이동속도
#define NPC_AREAX 35           //npc x좌표 좌우영역
#define NPC_AREAY 60           //npc y좌표 상하영역
#define PLAYER_AREAX 38     //플레이어 x좌표 좌우영역
#define PLAYER_AREAY 40     //플레이어 y좌표 상하영역

//구조체 선언 플레이어,npc의 뼈대설정
typedef struct OBJ
{
    int x;              //x좌표
    int y;              //y좌표
    int savey;      //y좌표저장
    int stack;      //스텍
    bool isActive;//활동체크
};

//플레이어 정보
static OBJ Player_Obj[MAX_OBJ]; //플레이어 발사체 구조체
static int Player_posX;                //플레이어 x좌표
static int Player_posY;                //플레이어 y좌표
static int Player_Obj_Count = 0;  //플레이어 발사체 카운트
static int Obj_Count = 0;             //발사체 최대 갯수 카운트

//npc 정보
static OBJ Npc_pos[MAX_OBJ* MAX_OBJ];                 //npc 구조체
static int NpcNum = 0;

//npc 발사체 정보
static OBJ Npc_Obj[MAX_OBJ ]; //npc발사체 구조체
static int Npc_Obj_Count = 0;                   //npc 발사체 카운트
static int Stack = 0;                                 //npc 발사체 지연을 위한 변수

//게임정보
static int Round = 0;                 //게임 라운드
static bool start = FALSE;        //게임 시작여부


void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit)
{//비트맵 그리기 함수 (플레이어, npc, 발사체)
    HDC MemDC;
    HBITMAP OldBitmap;
    BITMAP Bitmap_;
    int BitmapX, BitmapY;

    MemDC = CreateCompatibleDC(hdc);
    OldBitmap = (HBITMAP)SelectObject(MemDC, hBit);

    GetObject(hBit, sizeof(BITMAP), &Bitmap_);
    BitmapX = Bitmap_.bmWidth;
    BitmapY = Bitmap_.bmHeight;

    BitBlt(hdc, x, y, BitmapX, BitmapY, MemDC, 0, 0, SRCCOPY);

    SelectObject(MemDC, OldBitmap);
    DeleteDC(MemDC);
}
void DrawWallpapers(HDC hdc, int x, int y, HBITMAP hBit)
{//비트맵 그리기 함수(배경화면)
    HDC MemDC;
    HBITMAP OldBitmap;
    BITMAP Bitmap_;
    int BitmapX, BitmapY;

    MemDC = CreateCompatibleDC(hdc);
    OldBitmap = (HBITMAP)SelectObject(MemDC, hBit);

    GetObject(hBit, sizeof(BITMAP), &Bitmap_);
    BitmapX = Bitmap_.bmWidth;
    BitmapY = Bitmap_.bmHeight;

    StretchBlt(hdc, 0, 0, x, y, MemDC, 0, 0, BitmapX, BitmapY, SRCCOPY);

    SelectObject(MemDC, OldBitmap);
    DeleteDC(MemDC);
}

void TEXTBOX()
{//텍스트박스 출력 함수
    if (!start)//게임이 멈추면 실행
    {
        char CH[100], subCH[100] = { " 라운드 클리어" };
        wsprintf(CH, "%d", Round);
        wsprintf(CH, "%s%s", CH, subCH);
        MessageBox(hWnd, "다음라운드로 가시겠습니까?", CH, MB_OK);
    }
}

void NpcMake(RECT rt)
{//npc 생성 함수
    for (int i = 0; i < NpcNum; i++)
    {//npc 숫자만큼반복
        int check = 0;
        while (true)
        {
           //x 좌표 랜덤 지정
            check = Npc_pos[i].x =rand() % rt.right;
            if (rt.right / 2 > check)
                Npc_pos[i].x += NPC_AREAX;
            else
                Npc_pos[i].x -= NPC_AREAY;
            
            //영역에 들어올때까지 반복
            if (check+ NPC_AREAX >rt.left&& rt.right >check- NPC_AREAX)
                break;
        }
        Npc_pos[i].y = rand() % 300 + 30;//y좌표 랜덤설정
        Npc_pos[i].isActive = TRUE;//활동 시작
        if (rand() % 2 == 0) //랜덤으로 방향설정
        {
            Npc_pos[i].stack = 1;
        }
        else
        {
            Npc_pos[i].stack = -1;
        }
    }
    Obj_Count = 0; //발사체 갯수 초기화
    Stack = 0;//npc 생성하면서 초기화
}
void RoundCheck(int Round,RECT rt)
{//라운드에 대한 npc마릿수 설정
    start = TRUE;
    if (Round == 0)//npc 마릿수 = 난이도 
    {
        NpcNum = (Round+1)*3;
    }
    else if (Round == 1)
    {
        NpcNum = Round * 5;
    }
    else if (Round == 2)
    {
        NpcNum = Round * 4;
    }
    else if(Round==3)
    {
        NpcNum = (Round - 1) * 4 + 1;
    }
    else
    {
        NpcNum++;
    }
    NpcMake(rt);//체크후 npc제작
}
bool RoundClear()
{//ture면 라운드가 클리어됨
    int Check = 0;
    for (int i = 0; i < NpcNum; i++)
    {
        if (Npc_pos[i].isActive == false)
            Check++;//활동멈춘 npc의 갯수체크
    }
    if (Check == NpcNum)
    {//활동멈춘 npc = npc의 숫자 실행
        Round++;//라운드 증가
        for (int i = 0; i < MAX_OBJ; i++)
        {//초기화
            Player_Obj[i].isActive = false;
            Player_Obj[i].x = 700;
            Player_Obj[i].y = 1100;
            Obj_Count = 0;
            
        }
        return true;
    }
    return false;
}

void NpcKill(int wParam)
{//npc죽음 체크
    int num = 0;
    int set = 0;
    for (int i = 0; i < NpcNum; i++)
    {//플레이어 발사체와 npc위치비교
        if (Player_Obj[wParam].y - NPC_AREAY <= Npc_pos[i].y && Npc_pos[i].y <= Player_Obj[wParam].y + NPC_AREAY)
        {//y좌표비교
            if (Player_Obj[wParam].x - NPC_AREAX <= Npc_pos[i].x && Npc_pos[i].x <= Player_Obj[wParam].x + NPC_AREAX)
            {//x좌표 비교
                for (int j = 0; j < NpcNum; j++)
                {
                    if (Npc_pos[j].isActive == TRUE)
                        num++;//살아있는 npc 체크
                    else
                        set++;//죽어있는 npc 체크
                }
                if (num == NpcNum-set)//총량이 맞는지 확인
                {//죽음으로 체크 및 초기화
                    Npc_pos[i].x = 700;
                    Npc_pos[i].y = 1000;
                    Npc_pos[i].isActive = FALSE;
                    Player_Obj[wParam].x = -770;
                    Player_Obj[wParam].y = -1100;
                    Player_Obj[wParam].isActive = FALSE;
                    Obj_Count--;
                }
               //리셋
                num = 0;
                set = 0;
            }
        }
    }
    
}

void PlayerDie()
{//플레이어 죽음 체크
    if (start)
    {
        for (int i = 0; i < NpcNum; i++)
        {
            if (Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].x - PLAYER_AREAX <= Player_posX && Player_posX <= Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].x + PLAYER_AREAX)
            {
                if (Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].y - PLAYER_AREAY+30 <= Player_posY && Player_posY <= Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].y + PLAYER_AREAY*1.5)
                {//좌표 비교후 패배처리
                    start = FALSE;//게임멈춤
                    char CH[100], subCH[100] = { "게임에 패배하였습니다.\n클리어 라운드: " };
                    wsprintf(CH, "%d", Round);
                    wsprintf(subCH, "%s%s", subCH, CH);//라운드 보여주기 처리
                    if (MessageBox(hWnd, subCH, "Game Over", MB_OK) == IDOK)
                    {//ok버튼누르면 게임꺼짐
                        PostQuitMessage(0);
                        break;
                    }
                }
            }
        }
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{//api
    HBITMAP NewBitmap;
    HBITMAP OldBitmap;
    PAINTSTRUCT ps;
    static RECT rt;
    //비트맵 불러오기
    static HBITMAP Wallpapers = LoadBitmap(g_hInst, MAKEINTRESOURCE(ID_Wallpapers)),
        Player = LoadBitmap(g_hInst, MAKEINTRESOURCE(ID_PLAYER)),
        Player_obj = LoadBitmap(g_hInst, MAKEINTRESOURCE(ID_PLAYER_OBJ)),
        Npc = LoadBitmap(g_hInst, MAKEINTRESOURCE(ID_NPC)),
        Npc_obj = LoadBitmap(g_hInst, MAKEINTRESOURCE(ID_NPC_OBJ));
    
    switch (iMessage) {
    case WM_CREATE://시작시 실행
        start = TRUE;//게임시작
        GetClientRect(hWnd, &rt);//창크기불러오기
        RoundCheck(Round,rt);//라운드 체크 및 npc생성
        //플레이어 처음좌표 설정
        Player_posX = 300;
        Player_posY = 900;
        for (int i = 0; i < NpcNum; i++)
        {//npc 발사체 설정
            Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].isActive = TRUE; //활동 시작
            Npc_Obj[i + MAX_OBJ + +PLAYER_MAX_OBJ].x = Npc_pos[i].x + 30; //이미지에 맞게 설정
            Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].y = Npc_pos[i].y + 50;//이미지에 맞게 설정
            Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].savey = Npc_pos[i].y + 50; //좌표저장
            Npc_Obj[i + MAX_OBJ + +PLAYER_MAX_OBJ].stack = i * NpcNum - PLAYER_MAX_OBJ;//랜덤생성을 위한 스택설정
            SetTimer(hWnd, i + MAX_OBJ + PLAYER_MAX_OBJ, 50, NULL);//타이머실행
        }
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;
    case WM_KEYDOWN://키입력
        //방향키입력시 이동
        if (GetKeyState(VK_LEFT) & 0x8000&&rt.left+40<Player_posX)
            Player_posX -= PLAYER_MOVE;
        if (GetKeyState(VK_RIGHT) & 0x8000&& rt.right -40>Player_posX)
            Player_posX += PLAYER_MOVE;
        if (GetKeyState(VK_UP) & 0x8000&&rt.top<Player_posY)
            Player_posY -= PLAYER_MOVE;
        if (GetKeyState(VK_DOWN) & 0x8000&&rt.bottom>Player_posY)
            Player_posY += PLAYER_MOVE;
        //스페이스바 누르면 발사
        if (GetAsyncKeyState(VK_SPACE) && Obj_Count < PLAYER_MAX_OBJ)
        {
            Obj_Count++;//카운트증가 최대 발수 체크용
            Player_Obj[Player_Obj_Count].isActive = TRUE; //활동 시작
            Player_Obj[Player_Obj_Count].x = Player_posX - 17; //플레이어 위치에따른 좌표설정
            Player_Obj[Player_Obj_Count].y = Player_posY - 100;
            SetTimer(hWnd, Player_Obj_Count, 50, NULL);//타이머실행
            Player_Obj_Count++;//카운트 증가 최대맥시멈 횟수가 실행되면 초기화
            if (Player_Obj_Count == MAX_OBJ)
            {
                Player_Obj_Count = 0;
            }
            break;
        }
        if (GetKeyState(VK_ESCAPE) & 0x8000)
        {//esc 누르면 종료
            PostQuitMessage(0);
            break;
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    case WM_PAINT:
    {//그리기부분
        if (start)
        {//게임이 실행하고있으면 그리기
            hdc = BeginPaint(hWnd, &ps);
            RECT rect;
            GetClientRect(hWnd, &rect);

            BACKDC = CreateCompatibleDC(hdc);

            NewBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
            OldBitmap = (HBITMAP)SelectObject(BACKDC, NewBitmap);

            PatBlt(BACKDC, 0, 0, rect.right, rect.bottom, WHITENESS);

            //배경그리기
            DrawWallpapers(BACKDC, rt.right, rt.bottom, Wallpapers);
            //플레이어 그리기
            DrawBitmap(BACKDC, Player_posX - 40, Player_posY - 50, Player);
            //플레이어 발사체 그리기
            for (int i = 0; i < MAX_OBJ; i++)
            {
                if (Player_Obj[i].isActive == TRUE)//활성화 되있는것만 그림
                    DrawBitmap(BACKDC, Player_Obj[i].x, Player_Obj[i].y, Player_obj);
            }
            //npc 그리기
            for (int i = 0; i < MAX_OBJ; i++)
            {
                if (Npc_pos[i].isActive == TRUE)//활성화 되있는것만 그림
                    DrawBitmap(BACKDC, Npc_pos[i].x, Npc_pos[i].y, Npc);
            }
            //npc 발사체 그리기
            for (int i = 0; i < NpcNum; i++)
            {
                if (Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].isActive == TRUE)//활성화 되있는것만 그림
                {
                    DrawBitmap(BACKDC, Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].x, Npc_Obj[i + MAX_OBJ + +PLAYER_MAX_OBJ].y, Npc_obj);
                }
            }
            BitBlt(hdc, 0, 0, rect.right, rect.bottom, BACKDC, 0, 0, SRCCOPY);
            SelectObject(BACKDC, OldBitmap);
            DeleteDC(BACKDC);
            DeleteObject(NewBitmap);

            EndPaint(hWnd, &ps);
            return 0;
        }
        return 0;
    }
        
    case WM_TIMER:
    {//타이며
        PlayerDie();//플레이어가 죽었는지 확인
        if (RoundClear())//라운드가 클리어가 되었는지 확인한다 
        {
            RoundCheck(Round, rt);//라운드 체크 및 설정
            start = FALSE;//게임잠깐멈추고
            TEXTBOX();//텍스트 출력
            start = TRUE;//게임 재실행
            for (int i = 0; i < NpcNum; i++)
            {//npc 발사체 초기화 및 실행 
                Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].isActive = TRUE;
                Npc_Obj[i + MAX_OBJ + +PLAYER_MAX_OBJ].x = Npc_pos[i].x + 30;
                Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].y = Npc_pos[i].y + 50;
                Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].savey = Npc_pos[i].y + 50;
                Npc_Obj[i + MAX_OBJ + +PLAYER_MAX_OBJ].stack = i * NpcNum - PLAYER_MAX_OBJ;
                SetTimer(hWnd, i + MAX_OBJ + PLAYER_MAX_OBJ, 50, NULL);
            }
            InvalidateRect(hWnd, NULL, FALSE);
        }
        if (Player_Obj[wParam].isActive == TRUE)
        {//플레이어 발사체가 실행중이면 이동거리만큼 타이머 실행당 움직임
            Player_Obj[wParam ].y -= OBJ_MOVEY;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        if (Player_Obj[wParam ].y<=-50)
        {//범윕 벗어나면 삭제
            Player_Obj[wParam].y = 1200;
            Player_Obj[wParam ].isActive = FALSE;
            Obj_Count--;
        }

        if (Npc_Obj[wParam].isActive == TRUE)
        {//npc 오브젝트 실행중이면
            if (Npc_Obj[wParam].stack < 10000)
            {//스텍이 다채워지면 실행 한번에 쏘는걸 방지
                Npc_Obj[wParam].x= Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].x+25;//이미지 설정
                int number = NpcNum / 2;
                if (Stack < number)
                    Npc_Obj[wParam].stack = 10000;
                Stack++;

                srand((unsigned int)(time(NULL)));
                Npc_Obj[wParam].stack += (rand()* Stack*3) %1000+Stack;
            }
            else
            {//스택이 다채워져있으면 움직임
                Npc_Obj[wParam].y += OBJ_MOVEY - 10;
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }

        if (Npc_Obj[wParam].y >= 950&& Npc_Obj[wParam].isActive == TRUE)
        {//범위 넘어가면 다시 npc로부터 발사체 발사
            Npc_Obj[wParam].y = Npc_Obj[wParam].savey;
            Npc_Obj[wParam].x = Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].x+25;
            if (Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].isActive == FALSE)
            {
                Npc_Obj[wParam].isActive = FALSE;
                Npc_Obj[wParam].y = -100;
            }
        }
        if (Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].isActive == TRUE)
        {//npc 좌우이동 설정 화면끝에 도달하면 반대방향으로 진행
            if (Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].x > rt.right - NPC_AREAY)
                Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].stack *= -1;//sign = -1;
            if (Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].x < rt.left)
                Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].stack *= -1;//sign = 1;
            //방향대로움직임
            Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].x += Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].stack * NPC_MOVEX;
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }

        NpcKill(wParam);//npc 죽었는지 확인
        
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}