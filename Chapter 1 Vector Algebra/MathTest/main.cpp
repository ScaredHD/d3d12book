#include <iostream>

#include <DirectXMath.h>
#include <DirectXPackedVector.h>

// Enable SSE2 and /fp:fast floating point model in C/C++ -> Code Generation

using namespace DirectX;

std::ostream& XM_CALLCONV operator<<(std::ostream& os, FXMVECTOR v) {
    XMFLOAT3 dest;
    XMStoreFloat3(&dest, v);

    os << "(" << dest.x << ", " << dest.y << ", " << dest.z << ")";
    return os;
}


int main() {
    XMVECTOR v;

    v = XMVectorSet(1.0, 2.0, 3.0, 4.0);
    std::cout << v << "\n";
}