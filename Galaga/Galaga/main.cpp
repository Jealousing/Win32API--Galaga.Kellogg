#include <windows.h>
#include <time.h>
#include <string.h>
#include "resource.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE g_hInst;
LPCTSTR lpszClass = TEXT("Dongu Galaga");//������ â �̸�����
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
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 1000,//������ ũ�� ����
        NULL, (HMENU)NULL, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);

    while (GetMessage(&Message, NULL, 0, 0)) {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }
    return (int)Message.wParam;
}

//�ܼ� ��ũ�η� ��� ����
#define MAX_OBJ 1000            //������Ʈ �ִ� ����
#define PLAYER_MAX_OBJ 5    //�÷��̾� �߻�ü ����
#define PLAYER_MOVE 20       //�÷��̾� �̵��ӵ�
#define NPC_MOVEX 5             //NPC �¿� �̵��ӵ�
#define OBJ_MOVEY 20           //�߻�ü �̵��ӵ�
#define NPC_AREAX 35           //npc x��ǥ �¿쿵��
#define NPC_AREAY 60           //npc y��ǥ ���Ͽ���
#define PLAYER_AREAX 38     //�÷��̾� x��ǥ �¿쿵��
#define PLAYER_AREAY 40     //�÷��̾� y��ǥ ���Ͽ���

//����ü ���� �÷��̾�,npc�� ���뼳��
typedef struct OBJ
{
    int x;              //x��ǥ
    int y;              //y��ǥ
    int savey;      //y��ǥ����
    int stack;      //����
    bool isActive;//Ȱ��üũ
};

//�÷��̾� ����
static OBJ Player_Obj[MAX_OBJ]; //�÷��̾� �߻�ü ����ü
static int Player_posX;                //�÷��̾� x��ǥ
static int Player_posY;                //�÷��̾� y��ǥ
static int Player_Obj_Count = 0;  //�÷��̾� �߻�ü ī��Ʈ
static int Obj_Count = 0;             //�߻�ü �ִ� ���� ī��Ʈ

//npc ����
static OBJ Npc_pos[MAX_OBJ* MAX_OBJ];                 //npc ����ü
static int NpcNum = 0;

//npc �߻�ü ����
static OBJ Npc_Obj[MAX_OBJ ]; //npc�߻�ü ����ü
static int Npc_Obj_Count = 0;                   //npc �߻�ü ī��Ʈ
static int Stack = 0;                                 //npc �߻�ü ������ ���� ����

//��������
static int Round = 0;                 //���� ����
static bool start = FALSE;        //���� ���ۿ���


void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit)
{//��Ʈ�� �׸��� �Լ� (�÷��̾�, npc, �߻�ü)
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
{//��Ʈ�� �׸��� �Լ�(���ȭ��)
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
{//�ؽ�Ʈ�ڽ� ��� �Լ�
    if (!start)//������ ���߸� ����
    {
        char CH[100], subCH[100] = { " ���� Ŭ����" };
        wsprintf(CH, "%d", Round);
        wsprintf(CH, "%s%s", CH, subCH);
        MessageBox(hWnd, "��������� ���ðڽ��ϱ�?", CH, MB_OK);
    }
}

void NpcMake(RECT rt)
{//npc ���� �Լ�
    for (int i = 0; i < NpcNum; i++)
    {//npc ���ڸ�ŭ�ݺ�
        int check = 0;
        while (true)
        {
           //x ��ǥ ���� ����
            check = Npc_pos[i].x =rand() % rt.right;
            if (rt.right / 2 > check)
                Npc_pos[i].x += NPC_AREAX;
            else
                Npc_pos[i].x -= NPC_AREAY;
            
            //������ ���ö����� �ݺ�
            if (check+ NPC_AREAX >rt.left&& rt.right >check- NPC_AREAX)
                break;
        }
        Npc_pos[i].y = rand() % 300 + 30;//y��ǥ ��������
        Npc_pos[i].isActive = TRUE;//Ȱ�� ����
        if (rand() % 2 == 0) //�������� ���⼳��
        {
            Npc_pos[i].stack = 1;
        }
        else
        {
            Npc_pos[i].stack = -1;
        }
    }
    Obj_Count = 0; //�߻�ü ���� �ʱ�ȭ
    Stack = 0;//npc �����ϸ鼭 �ʱ�ȭ
}
void RoundCheck(int Round,RECT rt)
{//���忡 ���� npc������ ����
    start = TRUE;
    if (Round == 0)//npc ������ = ���̵� 
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
    NpcMake(rt);//üũ�� npc����
}
bool RoundClear()
{//ture�� ���尡 Ŭ�����
    int Check = 0;
    for (int i = 0; i < NpcNum; i++)
    {
        if (Npc_pos[i].isActive == false)
            Check++;//Ȱ������ npc�� ����üũ
    }
    if (Check == NpcNum)
    {//Ȱ������ npc = npc�� ���� ����
        Round++;//���� ����
        for (int i = 0; i < MAX_OBJ; i++)
        {//�ʱ�ȭ
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
{//npc���� üũ
    int num = 0;
    int set = 0;
    for (int i = 0; i < NpcNum; i++)
    {//�÷��̾� �߻�ü�� npc��ġ��
        if (Player_Obj[wParam].y - NPC_AREAY <= Npc_pos[i].y && Npc_pos[i].y <= Player_Obj[wParam].y + NPC_AREAY)
        {//y��ǥ��
            if (Player_Obj[wParam].x - NPC_AREAX <= Npc_pos[i].x && Npc_pos[i].x <= Player_Obj[wParam].x + NPC_AREAX)
            {//x��ǥ ��
                for (int j = 0; j < NpcNum; j++)
                {
                    if (Npc_pos[j].isActive == TRUE)
                        num++;//����ִ� npc üũ
                    else
                        set++;//�׾��ִ� npc üũ
                }
                if (num == NpcNum-set)//�ѷ��� �´��� Ȯ��
                {//�������� üũ �� �ʱ�ȭ
                    Npc_pos[i].x = 700;
                    Npc_pos[i].y = 1000;
                    Npc_pos[i].isActive = FALSE;
                    Player_Obj[wParam].x = -770;
                    Player_Obj[wParam].y = -1100;
                    Player_Obj[wParam].isActive = FALSE;
                    Obj_Count--;
                }
               //����
                num = 0;
                set = 0;
            }
        }
    }
    
}

void PlayerDie()
{//�÷��̾� ���� üũ
    if (start)
    {
        for (int i = 0; i < NpcNum; i++)
        {
            if (Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].x - PLAYER_AREAX <= Player_posX && Player_posX <= Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].x + PLAYER_AREAX)
            {
                if (Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].y - PLAYER_AREAY+30 <= Player_posY && Player_posY <= Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].y + PLAYER_AREAY*1.5)
                {//��ǥ ���� �й�ó��
                    start = FALSE;//���Ӹ���
                    char CH[100], subCH[100] = { "���ӿ� �й��Ͽ����ϴ�.\nŬ���� ����: " };
                    wsprintf(CH, "%d", Round);
                    wsprintf(subCH, "%s%s", subCH, CH);//���� �����ֱ� ó��
                    if (MessageBox(hWnd, subCH, "Game Over", MB_OK) == IDOK)
                    {//ok��ư������ ���Ӳ���
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
    //��Ʈ�� �ҷ�����
    static HBITMAP Wallpapers = LoadBitmap(g_hInst, MAKEINTRESOURCE(ID_Wallpapers)),
        Player = LoadBitmap(g_hInst, MAKEINTRESOURCE(ID_PLAYER)),
        Player_obj = LoadBitmap(g_hInst, MAKEINTRESOURCE(ID_PLAYER_OBJ)),
        Npc = LoadBitmap(g_hInst, MAKEINTRESOURCE(ID_NPC)),
        Npc_obj = LoadBitmap(g_hInst, MAKEINTRESOURCE(ID_NPC_OBJ));
    
    switch (iMessage) {
    case WM_CREATE://���۽� ����
        start = TRUE;//���ӽ���
        GetClientRect(hWnd, &rt);//âũ��ҷ�����
        RoundCheck(Round,rt);//���� üũ �� npc����
        //�÷��̾� ó����ǥ ����
        Player_posX = 300;
        Player_posY = 900;
        for (int i = 0; i < NpcNum; i++)
        {//npc �߻�ü ����
            Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].isActive = TRUE; //Ȱ�� ����
            Npc_Obj[i + MAX_OBJ + +PLAYER_MAX_OBJ].x = Npc_pos[i].x + 30; //�̹����� �°� ����
            Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].y = Npc_pos[i].y + 50;//�̹����� �°� ����
            Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].savey = Npc_pos[i].y + 50; //��ǥ����
            Npc_Obj[i + MAX_OBJ + +PLAYER_MAX_OBJ].stack = i * NpcNum - PLAYER_MAX_OBJ;//���������� ���� ���ü���
            SetTimer(hWnd, i + MAX_OBJ + PLAYER_MAX_OBJ, 50, NULL);//Ÿ�̸ӽ���
        }
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;
    case WM_KEYDOWN://Ű�Է�
        //����Ű�Է½� �̵�
        if (GetKeyState(VK_LEFT) & 0x8000&&rt.left+40<Player_posX)
            Player_posX -= PLAYER_MOVE;
        if (GetKeyState(VK_RIGHT) & 0x8000&& rt.right -40>Player_posX)
            Player_posX += PLAYER_MOVE;
        if (GetKeyState(VK_UP) & 0x8000&&rt.top<Player_posY)
            Player_posY -= PLAYER_MOVE;
        if (GetKeyState(VK_DOWN) & 0x8000&&rt.bottom>Player_posY)
            Player_posY += PLAYER_MOVE;
        //�����̽��� ������ �߻�
        if (GetAsyncKeyState(VK_SPACE) && Obj_Count < PLAYER_MAX_OBJ)
        {
            Obj_Count++;//ī��Ʈ���� �ִ� �߼� üũ��
            Player_Obj[Player_Obj_Count].isActive = TRUE; //Ȱ�� ����
            Player_Obj[Player_Obj_Count].x = Player_posX - 17; //�÷��̾� ��ġ������ ��ǥ����
            Player_Obj[Player_Obj_Count].y = Player_posY - 100;
            SetTimer(hWnd, Player_Obj_Count, 50, NULL);//Ÿ�̸ӽ���
            Player_Obj_Count++;//ī��Ʈ ���� �ִ�ƽø� Ƚ���� ����Ǹ� �ʱ�ȭ
            if (Player_Obj_Count == MAX_OBJ)
            {
                Player_Obj_Count = 0;
            }
            break;
        }
        if (GetKeyState(VK_ESCAPE) & 0x8000)
        {//esc ������ ����
            PostQuitMessage(0);
            break;
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    case WM_PAINT:
    {//�׸���κ�
        if (start)
        {//������ �����ϰ������� �׸���
            hdc = BeginPaint(hWnd, &ps);
            RECT rect;
            GetClientRect(hWnd, &rect);

            BACKDC = CreateCompatibleDC(hdc);

            NewBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
            OldBitmap = (HBITMAP)SelectObject(BACKDC, NewBitmap);

            PatBlt(BACKDC, 0, 0, rect.right, rect.bottom, WHITENESS);

            //���׸���
            DrawWallpapers(BACKDC, rt.right, rt.bottom, Wallpapers);
            //�÷��̾� �׸���
            DrawBitmap(BACKDC, Player_posX - 40, Player_posY - 50, Player);
            //�÷��̾� �߻�ü �׸���
            for (int i = 0; i < MAX_OBJ; i++)
            {
                if (Player_Obj[i].isActive == TRUE)//Ȱ��ȭ ���ִ°͸� �׸�
                    DrawBitmap(BACKDC, Player_Obj[i].x, Player_Obj[i].y, Player_obj);
            }
            //npc �׸���
            for (int i = 0; i < MAX_OBJ; i++)
            {
                if (Npc_pos[i].isActive == TRUE)//Ȱ��ȭ ���ִ°͸� �׸�
                    DrawBitmap(BACKDC, Npc_pos[i].x, Npc_pos[i].y, Npc);
            }
            //npc �߻�ü �׸���
            for (int i = 0; i < NpcNum; i++)
            {
                if (Npc_Obj[i + MAX_OBJ + PLAYER_MAX_OBJ].isActive == TRUE)//Ȱ��ȭ ���ִ°͸� �׸�
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
    {//Ÿ�̸�
        PlayerDie();//�÷��̾ �׾����� Ȯ��
        if (RoundClear())//���尡 Ŭ��� �Ǿ����� Ȯ���Ѵ� 
        {
            RoundCheck(Round, rt);//���� üũ �� ����
            start = FALSE;//���������߰�
            TEXTBOX();//�ؽ�Ʈ ���
            start = TRUE;//���� �����
            for (int i = 0; i < NpcNum; i++)
            {//npc �߻�ü �ʱ�ȭ �� ���� 
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
        {//�÷��̾� �߻�ü�� �������̸� �̵��Ÿ���ŭ Ÿ�̸� ����� ������
            Player_Obj[wParam ].y -= OBJ_MOVEY;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        if (Player_Obj[wParam ].y<=-50)
        {//���� ����� ����
            Player_Obj[wParam].y = 1200;
            Player_Obj[wParam ].isActive = FALSE;
            Obj_Count--;
        }

        if (Npc_Obj[wParam].isActive == TRUE)
        {//npc ������Ʈ �������̸�
            if (Npc_Obj[wParam].stack < 10000)
            {//������ ��ä������ ���� �ѹ��� ��°� ����
                Npc_Obj[wParam].x= Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].x+25;//�̹��� ����
                int number = NpcNum / 2;
                if (Stack < number)
                    Npc_Obj[wParam].stack = 10000;
                Stack++;

                srand((unsigned int)(time(NULL)));
                Npc_Obj[wParam].stack += (rand()* Stack*3) %1000+Stack;
            }
            else
            {//������ ��ä���������� ������
                Npc_Obj[wParam].y += OBJ_MOVEY - 10;
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }

        if (Npc_Obj[wParam].y >= 950&& Npc_Obj[wParam].isActive == TRUE)
        {//���� �Ѿ�� �ٽ� npc�κ��� �߻�ü �߻�
            Npc_Obj[wParam].y = Npc_Obj[wParam].savey;
            Npc_Obj[wParam].x = Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].x+25;
            if (Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].isActive == FALSE)
            {
                Npc_Obj[wParam].isActive = FALSE;
                Npc_Obj[wParam].y = -100;
            }
        }
        if (Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].isActive == TRUE)
        {//npc �¿��̵� ���� ȭ�鳡�� �����ϸ� �ݴ�������� ����
            if (Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].x > rt.right - NPC_AREAY)
                Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].stack *= -1;//sign = -1;
            if (Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].x < rt.left)
                Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].stack *= -1;//sign = 1;
            //�����ο�����
            Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].x += Npc_pos[wParam - MAX_OBJ - PLAYER_MAX_OBJ].stack * NPC_MOVEX;
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }

        NpcKill(wParam);//npc �׾����� Ȯ��
        
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}