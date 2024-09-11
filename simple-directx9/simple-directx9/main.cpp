#pragma comment( lib, "winmm.lib" )

#include <d3d9.h>
#include <d3dx9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <string>

#define SAFE_RELEASE(p) {if(p){(p)->Release();(p)=NULL;}}//安全に解放する

LPDIRECT3D9             g_pD3D = NULL;//Direct3D9
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL;//レンダリングデバイス
LPD3DXFONT    g_pFont = NULL;//フォント
LPD3DXMESH    pMesh = NULL;//メッシュ
D3DMATERIAL9* pMaterials = NULL;//マテリアル
LPDIRECT3DTEXTURE9* pTextures = NULL;//テクスチャ

DWORD dwNumMaterials = 0;

//テキスト描画
void TextDraw(LPD3DXFONT pFont, char* text, int X, int Y)
{
    RECT rect = { X,Y,0,0 };//描画位置
    //スプライトポインタ(無くても良し),描画文字,文字数(-1で全部),描画範囲,フォーマット,色
    pFont->DrawText(NULL, text, -1, &rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(255, 0, 0, 0));
}

//Direct3D初期化
HRESULT InitD3D(HWND hWnd)
{
    //Direct3Dを生成する
    if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION))) { return E_FAIL; }

    //デバイス生成用のパラメーター
    D3DPRESENT_PARAMETERS d3dpp;//パラメーター構造体
    ZeroMemory(&d3dpp, sizeof(d3dpp));//ゼロで初期化
    d3dpp.Windowed = TRUE;//ウインドウモードで起動
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;//バックバッファのスワップエフェクト Direct3Dにスワップエフェクトをまかせる
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;//バックバッファのフォーマット今表示されているモニタの設定と同じ
    d3dpp.BackBufferCount = 1;//バックバッファの数
    d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;//マルチサンプリングは行わない
    d3dpp.MultiSampleQuality = 0;//マルチサンプリングは行わないので0
    d3dpp.EnableAutoDepthStencil = TRUE;//Direct3Dに深度バッファの管理をまかせる
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;//深度バッファのフォーマット（通常はこの値で問題ない）
    d3dpp.hDeviceWindow = hWnd;//カバーウィンドウ＝アプリケーションのウィンドウ
    d3dpp.Flags = 0;//フラグは使わない
    d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;//今のリフレッシュレートをそのまま使う
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;//モニタの垂直回帰を待つ

    //Direct3Dデバイスの生成 HAL(ハードウェアアクセラレーション)
    if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice))) {
        //HALが駄目ならHEL(ソフトウェアエミュレーション)
        if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice))) {
            //駄目なら終了
            return(E_FAIL);
        }
    }

    //フォントの生成 MSゴシック
   //デバイス,文字高さ,文字幅,フォントスタイル,ミップマップのレベル,斜体にするかどうか,文字セット,出力精度,出力品質,フォントピッチとファミリ,フォント名,フォントポインタ
    HRESULT hr = D3DXCreateFont(g_pd3dDevice, 20, 0, FW_HEAVY, 1, false, SHIFTJIS_CHARSET, OUT_TT_ONLY_PRECIS,
        ANTIALIASED_QUALITY, FF_DONTCARE, "ＭＳ ゴシック", &g_pFont);
    if FAILED(hr) { return(E_FAIL); }


    // Xファイルからメッシュをロードする 
    LPD3DXBUFFER pD3DXMtrlBuffer = NULL;

    if (FAILED(D3DXLoadMeshFromX("cube.x", D3DXMESH_SYSTEMMEM,
        g_pd3dDevice, NULL, &pD3DXMtrlBuffer, NULL,
        &dwNumMaterials, &pMesh)))
    {
        MessageBox(NULL, "Xファイルの読み込みに失敗しました", NULL, MB_OK);
        return E_FAIL;
    }
    D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
    pMaterials = new D3DMATERIAL9[dwNumMaterials];
    pTextures = new LPDIRECT3DTEXTURE9[dwNumMaterials];

    for (DWORD i = 0; i < dwNumMaterials; i++)
    {
        pMaterials[i] = d3dxMaterials[i].MatD3D;
        pMaterials[i].Ambient = pMaterials[i].Diffuse;
        pTextures[i] = NULL;
        if (d3dxMaterials[i].pTextureFilename != NULL &&
            lstrlen(d3dxMaterials[i].pTextureFilename) > 0)
        {
            if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice,
                d3dxMaterials[i].pTextureFilename,
                &pTextures[i])))
            {
                MessageBox(NULL, "テクスチャの読み込みに失敗しました", NULL, MB_OK);
            }
        }
    }
    pD3DXMtrlBuffer->Release();
    // Zバッファー処理を有効にする
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    // ライトを有効にする
    g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    // アンビエントライト（環境光）を設定する
    g_pd3dDevice->SetRenderState(D3DRS_AMBIENT, 0x00111111);
    // スペキュラ（鏡面反射）を有効にする
    g_pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
    return S_OK;
}

//終了時解放処理
VOID Cleanup()
{
    SAFE_RELEASE(pMesh);
    SAFE_RELEASE(g_pFont);
    SAFE_RELEASE(g_pd3dDevice);
    SAFE_RELEASE(g_pD3D);
}

//レンダリング
VOID Render()
{
    //デバイスが無いなら終了
    if (NULL == g_pd3dDevice) { return; }

    //ワールドトランスフォーム（絶対座標変換）
    D3DXMATRIXA16 matWorld, matRotation;
    D3DXMatrixRotationY(&matWorld, timeGetTime() / 3000.0f);
    D3DXMatrixRotationX(&matRotation, 0.5f);
    D3DXMatrixMultiply(&matWorld, &matWorld, &matRotation);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
    // ビュートランスフォーム（視点座標変換）

    D3DXVECTOR3 vecEyePt(0.0f, 3.0f, -1.5f); //カメラ（視点）位置
    D3DXVECTOR3 vecLookatPt(0.0f, 0.0f, 0.0f);//注視位置
    D3DXVECTOR3 vecUpVec(0.0f, 1.0f, 0.0f);//上方位置
    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH(&matView, &vecEyePt, &vecLookatPt, &vecUpVec);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);
    // プロジェクショントランスフォーム（射影変換）
    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
    // ライトをあてる 白色で鏡面反射ありに設定
    D3DXVECTOR3 vecDirection(1, -1, -1);
    D3DLIGHT9 light;
    ZeroMemory(&light, sizeof(D3DLIGHT9));
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r = 1.0f;
    light.Diffuse.g = 1.0f;
    light.Diffuse.b = 1.0f;
    light.Specular.r = 1.0f;
    light.Specular.g = 1.0f;
    light.Specular.b = 1.0f;
    D3DXVec3Normalize((D3DXVECTOR3*)&light.Direction, &vecDirection);
    light.Range = 200.0f;
    g_pd3dDevice->SetLight(0, &light);
    g_pd3dDevice->LightEnable(0, TRUE);

    //バックバッファのクリア
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
        D3DCOLOR_XRGB(100, 100, 100), 1.0f, 0);
    //シーンの開始
    if (SUCCEEDED(g_pd3dDevice->BeginScene()))
    {
        //ここに処理内容を書く

        for (DWORD i = 0; i < dwNumMaterials; i++)
        {
            g_pd3dDevice->SetMaterial(&pMaterials[i]);
            g_pd3dDevice->SetTexture(0, pTextures[i]);
            pMesh->DrawSubset(i);
        }
        std::string msg = "Xファイルの読み込みと表示";
//        TextDraw(g_pFont, msg.c_str(), 0, 0);

        //シーンの終了
        g_pd3dDevice->EndScene();
    }

    //バックバッファを表画面に反映させる
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

//メッセージプロシージャ
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY://終了時
        Cleanup();
        PostQuitMessage(0);
        return 0;

    case WM_PAINT://ウインドウ描画時
        Render();
        return 0;
    case WM_SIZE://ウインドウサイズ変更時
        InvalidateRect(hWnd, NULL, true);//画面更新
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

//メイン関数
INT WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, INT)
{
    //ウインドウクラスの登録
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      "Window1", NULL };
    RegisterClassEx(&wc);

    //タイトルバーとウインドウ枠の分を含めてウインドウサイズを設定
    RECT rect;
    SetRect(&rect, 0, 0, 640, 480);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    rect.right = rect.right - rect.left;
    rect.bottom = rect.bottom - rect.top;
    rect.top = 0;
    rect.left = 0;

    //ウインドウの生成
    HWND hWnd = CreateWindow("Window1", "Hello DirectX9 World !!",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rect.right, rect.bottom,
        NULL, NULL, wc.hInstance, NULL);

    //Direct3D初期化
    if (SUCCEEDED(InitD3D(hWnd)))
    {
        //ウインドウ表示
        ShowWindow(hWnd, SW_SHOWDEFAULT);
        UpdateWindow(hWnd);

        //メッセージループ
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    UnregisterClass("Window1", wc.hInstance);
    return 0;
}