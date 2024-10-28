#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

const int targetRedMin = 200;
const int targetRedMax = 215;
const int targetGreenMin = 52;
const int targetGreenMax = 63;
const int targetBlueMin = 52;
const int targetBlueMax = 63;

double Distance(int x1, int y1, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

POINT GetCenter(int x1, int y1, int x2, int y2) {
    POINT center;
    center.x = (x1 + x2) / 2;
    center.y = (y1 + y2) / 2;
    return center;
}

bool IsCursorOnTarget(const POINT& targetPos, int threshold = 2)
{
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    return (abs(cursorPos.x - targetPos.x) <= threshold) && (abs(cursorPos.y - targetPos.y) <= threshold);
}

void MoveCursorTo(int x, int y, double scale)
{
    POINT currentPos;
    GetCursorPos(&currentPos);
    scale = scale / 2.5;

    int dx = static_cast<int>(scale * (x - currentPos.x));
    int dy = static_cast<int>(scale * (y - currentPos.y));

    mouse_event(MOUSEEVENTF_MOVE, dx, dy+35, 0, 0);
}

bool colorExistsInVerticalRange(void* data, int width, int height, const POINT& startPoint, int upperBound, int lowerBound,
    int targetRMin, int targetGMin, int targetBMin, int targetRMax, int targetGMax, int targetBMax)
{
    const unsigned int* pBuffer = reinterpret_cast<const unsigned int*>(data);

    for (int dy = upperBound; dy <= lowerBound; dy++)
    {
        int newY = startPoint.y + dy;

        if (newY >= height)
            continue;

        unsigned int pixel = pBuffer[newY * width + startPoint.x];
        unsigned char r = (pixel & 0x00FF0000) >> 16;
        unsigned char g = (pixel & 0x0000FF00) >> 8;
        unsigned char b = (pixel & 0x000000FF);

        if (r >= targetRMin && r <= targetRMax &&
            g >= targetGMin && g <= targetGMax &&
            b >= targetBMin && b <= targetBMax)
        {
            return true;
        }
    }

    return false;
}

bool nearestTargetExists = false;
POINT nearestTargetPos;
POINT center;
void CheckAndSetCursorToRedPixel(void* data, int width, int height, const D3D11_MAPPED_SUBRESOURCE& resource)
{
    const unsigned int* pBuffer = reinterpret_cast<const unsigned int*>(data);
        bool foundColor = false;

        double closestDistance = DBL_MAX;
        POINT closestTargetPos;

        int squareSize = 700;
        int squareLeft = (width - squareSize) / 2;
        int squareTop = (height - squareSize) / 2;
        int squareRight = squareLeft + squareSize;
        int squareBottom = squareTop + squareSize;

        bool cursorOnTarget = false;
        if (nearestTargetExists)
        {
            cursorOnTarget = IsCursorOnTarget(nearestTargetPos, 4);
            if (!cursorOnTarget)
            {
                nearestTargetExists = false;
            }
        }
        if (!cursorOnTarget)
        {
            if (!nearestTargetExists)
            {
            for (int y = squareBottom - 1; y >= squareTop; --y)
            {
                bool foundFirstPixel = false;
                int firstPixelX = 0;

                for (int x = squareLeft; x < squareRight - 1; ++x)
                {
                    unsigned int pixel = pBuffer[y * width + x];
                    unsigned char r = (pixel & 0x00FF0000) >> 16;
                    unsigned char g = (pixel & 0x0000FF00) >> 8;
                    unsigned char b = (pixel & 0x000000FF);

                    if ((r >= targetRedMin && r <= targetRedMax) &&
                        (g >= targetGreenMin && g <= targetGreenMax) &&
                        (b >= targetBlueMin && b <= targetBlueMax))
                    {
                        if (!foundFirstPixel)
                        {
                            foundFirstPixel = true;
                            firstPixelX = x;
                        }

                        if (x > firstPixelX + 4)
                        {
                            unsigned int rightPixel = pBuffer[y * width + x + 1];
                            unsigned char rightR = (rightPixel & 0x00FF0000) >> 16;
                            unsigned char rightG = (rightPixel & 0x0000FF00) >> 8;
                            unsigned char rightB = (rightPixel & 0x000000FF);

                            if ((rightR >= targetRedMin && rightR <= targetRedMax) &&
                                (rightG >= targetGreenMin && rightG <= targetGreenMax) &&
                                (rightB >= targetBlueMin && rightB <= targetBlueMax))
                            {
                                nearestTargetExists = !nearestTargetExists;
                                center = GetCenter(x + 1, y, firstPixelX, y);

                                double distanceToCenter = Distance(center.x, center.y, x + 1, y);

                                if (distanceToCenter < closestDistance)
                                {
                                    closestDistance = distanceToCenter;
                                    closestTargetPos = center;
                                    foundColor = true;
                                }

                            }
                        }
                    }
                }

                if (foundColor)
                {
                    break;
                }
            }
        }
        int targetYellowRMin = 240;
        int targetYellowGMin = 197;
        int targetYellowBMin = 46;
        int targetYellowRMax = 249;
        int targetYellowGMax = 206;
        int targetYellowBMax = 55;
        if (foundColor)
        {
            nearestTargetExists = true;
            nearestTargetPos = closestTargetPos;
            if (!colorExistsInVerticalRange(data, width, height, center, 10, 50, targetYellowRMin, targetYellowGMin, targetYellowBMin, targetYellowRMax, targetYellowGMax, targetYellowBMax))
            {
                if (GetAsyncKeyState(VK_XBUTTON1))
                    MoveCursorTo(closestTargetPos.x, closestTargetPos.y, 1.5);
            }
        }
    }
}

int main()
{
    D3D_FEATURE_LEVEL featureLevel;
    ID3D11Device* pDevice = nullptr;
    ID3D11DeviceContext* pContext = nullptr;

    HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &pDevice, &featureLevel, &pContext);
    if (FAILED(hr)) {
        printf("Failed to create Direct3D device\n");
        return 0;
    }

    IDXGIFactory1* pDXGIFactory = nullptr;
    IDXGIAdapter1* pAdapter = nullptr;
    hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pDXGIFactory);
    if (FAILED(hr)) {
        printf("Failed to create DXGI factory\n");
        pDevice->Release();
        pContext->Release();
        return 0;
    }

    hr = pDXGIFactory->EnumAdapters1(0, &pAdapter);
    if (FAILED(hr)) {
        printf("Failed to enumerate adapters\n");
        pDevice->Release();
        pContext->Release();
        pDXGIFactory->Release();
        return 0;
    }

    IDXGIOutput* pOutput = nullptr;
    hr = pAdapter->EnumOutputs(0, &pOutput);
    if (FAILED(hr)) {
        printf("Failed to enumerate outputs\n");
        pDevice->Release();
        pContext->Release();
        pAdapter->Release();
        pDXGIFactory->Release();
        return 0;
    }

    IDXGIOutput1* pOutput1 = nullptr;
    hr = pOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&pOutput1);
    if (FAILED(hr)) {
        printf("Failed to query IDXGIOutput1\n");
        pOutput->Release();
        pDevice->Release();
        pContext->Release();
        pAdapter->Release();
        pDXGIFactory->Release();
        return 0;
    }

    DXGI_OUTPUT_DESC desktopDesc;
    hr = pOutput1->GetDesc(&desktopDesc);
    if (FAILED(hr)) {
        printf("Failed to get output description\n");
        pOutput1->Release();
        pDevice->Release();
        pContext->Release();
        pOutput->Release();
        pAdapter->Release();
        pDXGIFactory->Release();
        return 0;
    }

    IDXGIAdapter* pParentAdapter = nullptr;
    hr = pOutput->GetParent(__uuidof(IDXGIAdapter), (void**)&pParentAdapter);
    if (FAILED(hr)) {
        printf("Failed to get parent adapter\n");
        pOutput1->Release();
        pOutput->Release();
        pDevice->Release();
        pContext->Release();
        pAdapter->Release();
        pDXGIFactory->Release();
        return 0;
    }

    IDXGIOutputDuplication* pDesktopDuplication = nullptr;
    hr = pOutput1->DuplicateOutput(pDevice, &pDesktopDuplication);
    if (FAILED(hr)) {
        printf("Failed to duplicate output\n");
        pParentAdapter->Release();
        pOutput1->Release();
        pOutput->Release();
        pDevice->Release();
        pContext->Release();
        pAdapter->Release();
        pDXGIFactory->Release();
        return 0;
    }

    DXGI_OUTDUPL_DESC duplicatedOutputDesc;
    pDesktopDuplication->GetDesc(&duplicatedOutputDesc);

    D3D11_TEXTURE2D_DESC textureDesc;
    ZeroMemory(&textureDesc, sizeof(textureDesc));
    textureDesc.Width = desktopDesc.DesktopCoordinates.right - desktopDesc.DesktopCoordinates.left;
    textureDesc.Height = desktopDesc.DesktopCoordinates.bottom - desktopDesc.DesktopCoordinates.top;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_STAGING;
    textureDesc.BindFlags = 0;
    textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    textureDesc.MiscFlags = 0;

    ID3D11Texture2D* pTexture = nullptr;
    hr = pDevice->CreateTexture2D(&textureDesc, NULL, &pTexture);
    if (FAILED(hr)) {
        printf("Failed to create texture\n");
        pDesktopDuplication->Release();
        pParentAdapter->Release();
        pOutput1->Release();
        pOutput->Release();
        pDevice->Release();
        pContext->Release();
        pAdapter->Release();
        pDXGIFactory->Release();
        return 0;
    }

    while (true)
    {
        IDXGIResource* pDesktopResource = nullptr;
        DXGI_OUTDUPL_FRAME_INFO frameInfo;
        HRESULT hr = pDesktopDuplication->AcquireNextFrame(500, &frameInfo, &pDesktopResource);
        if (hr == DXGI_ERROR_WAIT_TIMEOUT)
        {
            continue;
        }
        else if (FAILED(hr))
        {
            printf("Failed to acquire next frame, error code: %x\n", hr);
            break;
        }
        ID3D11Texture2D* pDesktopImage;
        hr = pDesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pDesktopImage));
        if (FAILED(hr))
        {
            printf("Failed to acquire the texture interface from the DXGI resource, error code: %x\n", hr);
            break;
        }

        pContext->CopyResource(pTexture, pDesktopImage);

        D3D11_MAPPED_SUBRESOURCE resource;
        hr = pContext->Map(pTexture, 0, D3D11_MAP_READ, 0, &resource);
        if (SUCCEEDED(hr))
        {
            CheckAndSetCursorToRedPixel(resource.pData, textureDesc.Width, textureDesc.Height, resource);
            pContext->Unmap(pTexture, 0);
        }

        pDesktopDuplication->ReleaseFrame();
    }

    pDesktopDuplication->Release();
    if (pContext)
    {
        pContext->ClearState();
        pContext->Flush();
        pContext->Release();
    }
    if (pDevice)
    {
        pDevice->Release();
    }


    pTexture->Release();
    pDesktopDuplication->Release();
    pParentAdapter->Release();
    pOutput1->Release();
    pOutput->Release();
    pDevice->Release();
    pContext->Release();
    pAdapter->Release();
    pDXGIFactory->Release();

    printf("Done!\n");

    return 0;
}