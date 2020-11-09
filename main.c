#include <stdio.h>
#include <stdlib.h>

#include <windows.h>


#define AddEnemy(a,b) ObjectInit(NewObject(), a, b, 40, 40, 'e')

typedef struct SPoint {
    float x, y;
} TPoint;

TPoint point (float x, float y)
{
    TPoint pt;
    pt.x = x;
    pt.y = y;
    return pt;
}

TPoint cam;

typedef struct SObject {
    TPoint pos;
    TPoint size;
    COLORREF brush;
    TPoint speed;
    char oType;
    float range, vecSpeed;
    BOOL isDel;
} TObject, *PObject;

BOOL ObjectCollision(TObject o1, TObject o2)
{
    return ((o1.pos.x + o1.size.x) > o2.pos.x) && (o1.pos.x < (o2.pos.x + o2.size.x)) &&
            ((o1.pos.y + o1.size.y) > o2.pos.y) && (o1.pos.y < (o2.pos.y + o2.size.y));
}

void ObjectInit (TObject *obj, float xPos, float yPos, float width, float height, char objType)
{
    obj->pos = point(xPos, yPos);
    obj->size = point(width, height);
    obj->brush = RGB(0, 255, 0);
    obj->speed = point(0,0);
    obj->oType = objType;
    if (objType == 'e') obj->brush = RGB(255,0,0);
    obj->isDel = FALSE;
}

void ObjectShow(TObject obj, HDC dc)
{
    SelectObject(dc, GetStockObject(DC_PEN));
    SetDCPenColor(dc, RGB(0,0,0));
    SelectObject(dc, GetStockObject(DC_BRUSH));
    SetDCBrushColor(dc, obj.brush);

    BOOL (*shape)(HDC, int, int, int, int);
    shape = obj.oType == 'e' ? Ellipse : Rectangle;
    shape(dc, (int)(obj.pos.x - cam.x), (int)(obj.pos.y - cam.y),
                    (int)(obj.pos.x + obj.size.x - cam.x), (int)(obj.pos.y + obj.size.y - cam.y));
}

void ObjectSetDestPoint(TObject *obj, float xPos, float yPos, float vecSpeed)
{
    TPoint xyLen = point(xPos - obj->pos.x, yPos - obj->pos.y);
    float dxy = sqrt(xyLen.x * xyLen.x  + xyLen.y * xyLen.y);
    obj->speed.x = xyLen.x / dxy * vecSpeed;
    obj->speed.y = xyLen.y / dxy * vecSpeed;
    obj->vecSpeed = vecSpeed;
}

RECT rct;
TObject player;

PObject mas = NULL;
int masCnt = 0;

BOOL needNewGame = FALSE;

void SetCameraFocus(TObject obj)
{
    cam.x = obj.pos.x - rct.right / 2;
    cam.y = obj.pos.y - rct.bottom / 2;
}

void ObjectMove(TObject *obj)
{
    if (obj->oType == 'e')
    {
        if (rand() % 40 == 1)
        {
            static float enemySpeed = 1.5;
            ObjectSetDestPoint(obj, player.pos.x, player.pos.y, enemySpeed);
        }
        if (ObjectCollision(*obj, player))
            needNewGame = TRUE;
    }
    obj->pos.x += obj->speed.x;
    obj->pos.y += obj->speed.y;

    if (obj->oType == '1')
    {
        obj->range = obj->vecSpeed;
        if (obj->range < 0)
           obj->isDel = TRUE;
        for (int i = 0; i < masCnt; i++)
            if ((mas[i].oType == 'e') && (ObjectCollision(*obj, mas[i])))
            {
                mas[i].isDel = TRUE;
                obj->isDel = TRUE;
            }
    }
}

PObject NewObject()
{
    masCnt++;
    mas = realloc(mas, sizeof(*mas) * masCnt);
    return mas + masCnt - 1;
}

void GenNewEnemy()
{
    static int rad = 300;
    int pos1 = (rand() % 2 == 0 ? -rad : rad);
    int pos2 = (rand() % (rad * 2)) - rad;
    int k = rand() % 100;
    if (k == 1)
        AddEnemy(player.pos.x + pos1, player.pos.y + pos2);
    if (k == 2)
        AddEnemy(player.pos.x + pos2, player.pos.y + pos1);
}

void DelObjects()
{
    int i = 0;
    while (i < masCnt)
    {
        if (mas[i].isDel)
        {
            masCnt--;
            mas[i] = mas[masCnt];
            mas = realloc(mas, sizeof(*mas) * masCnt);
        }
        else
            i++;
    }
}

void AddBullet(float xPos, float yPos, float x, float y)
{
    PObject obj = NewObject();
    ObjectInit(obj, xPos, yPos, 10, 10, '1');
    ObjectSetDestPoint(obj, x, y, 4);
    obj->range = 300;
}

void PlayerControl()
{
    static int playerSpeed = 2;
    player.speed.x = 0;
    player.speed.y = 0;
    if (GetKeyState('W') < 0) player.speed.y = -playerSpeed;
    if (GetKeyState('S') < 0) player.speed.y = playerSpeed;
    if (GetKeyState('A') < 0) player.speed.x = -playerSpeed;
    if (GetKeyState('D') < 0) player.speed.x = playerSpeed;
    if ((player.speed.x != 0) && (player.speed.y != 0))
        player.speed = point(player.speed.x * 0.7, player.speed.y * 0.7);
}

void WinInit()
{
    needNewGame = FALSE;
    masCnt = 0;
    mas = realloc(mas, 0);

    ObjectInit(&player, 100, 100, 40, 40, 'p');
}

void WinMove()
{
    if (needNewGame)
        WinInit();

    PlayerControl();
    ObjectMove(&player);
    SetCameraFocus(player);

    for (int i = 0; i < masCnt; i++)
        ObjectMove(mas + i);

    GenNewEnemy();
    DelObjects();
}

void WinShow(HDC dc)
{
    HDC memDC = CreateCompatibleDC(dc);
    HBITMAP memBM = CreateCompatibleBitmap(dc, rct.right - rct.left, rct.bottom - rct.top);
    SelectObject(memDC, memBM);

    SelectObject(memDC, GetStockObject(DC_PEN));
    SelectObject(memDC, GetStockObject(DC_BRUSH));
    SetDCBrushColor(memDC, RGB(200, 200, 200));

    static int rectSize = 200;
    int dx = (int)(cam.x) % rectSize;
    int dy = (int)(cam.y) % rectSize;
    for (int i = -1; i < (rct.right / rectSize) + 2; i++)
        for (int j = -1; j < (rct.bottom / rectSize) + 2; j++)
            Rectangle(memDC, -dx+(i*rectSize), -dy+(j*rectSize), -dx+((i+1)*rectSize), -dy+((j+1)*rectSize));

    ObjectShow(player, memDC);

    for (int i = 0; i < masCnt; i++)
        ObjectShow(mas[i], memDC);

    BitBlt(dc, 0, 0, rct.right - rct.left, rct.bottom - rct.top, memDC, 0, 0, SRCCOPY);
    DeleteDC(memDC);
    DeleteObject(memBM);
}

LRESULT  WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_DESTROY)
    {
        PostQuitMessage(0);
    }
    else if (message == WM_MOUSEMOVE)
    {
        int xPos = LOWORD(lParam);
        int yPos = HIWORD(lParam);
    }
    else if (message == WM_LBUTTONDOWN)
    {
        int xPos = LOWORD(lParam);
        int yPos = HIWORD(lParam);
        AddBullet(player.pos.x + player.size.x / 2, player.pos.y + player.size.y / 2, xPos + cam.x, yPos + cam.y);
    }
    else if (message == WM_SIZE)
    {
        GetClientRect(hWnd, &rct);
    }

    else return DefWindowProcA( hWnd,  message,  wParam,  lParam);
}

int main()
{
    WNDCLASSA wcl;
        memset(&wcl, 0, sizeof(WNDCLASSA));
        wcl.lpszClassName = "My window";
        wcl.lpfnWndProc = WndProc;
        wcl.hCursor = LoadCursorA(NULL, IDC_CROSS);
    RegisterClassA(&wcl);

    HWND hwnd;
    hwnd = CreateWindow("My window", "Window name", WS_OVERLAPPEDWINDOW,
                         10, 10, 640, 480, NULL, NULL, NULL, NULL);

    HDC dc = GetDC(hwnd);

    ShowWindow(hwnd, SW_SHOWNORMAL);

    WinInit();

    MSG msg;
    while (1)
    {
        if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            WinMove();
            WinShow(dc);
            Sleep(5);
        }
    }
    printf("Hello world!\n");
    return 0;
}
