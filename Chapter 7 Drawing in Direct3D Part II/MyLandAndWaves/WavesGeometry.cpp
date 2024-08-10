#include "WavesGeometry.h"

WavesGeometry::WavesGeometry(int rowCount,
                             int colCount,
                             float timeStep,
                             float spatialStep,
                             float speed,
                             float damping)
    : waves_{rowCount, colCount, timeStep, spatialStep, speed, damping} {
    indices_.resize(3 * waves_.TriangleCount());

    int m = waves_.RowCount();
    int n = waves_.ColumnCount();
    int k = 0;
    for (int i = 0; i < m - 1; ++i) {
        for (int j = 0; j < n - 1; ++j) {
            indices_[k] = i * n + j;
            indices_[k + 1] = i * n + j + 1;
            indices_[k + 2] = (i + 1) * n + j;
            indices_[k + 3] = (i + 1) * n + j;
            indices_[k + 4] = i * n + j + 1;
            indices_[k + 5] = (i + 1) * n + j + 1;

            k += 6;  // next quad
        }
    }

    vertices_.resize(waves_.VertexCount());
    Update(0, 0);
}

void WavesGeometry::Update(float totalTimeFromStart, float deltaTimeSecond) {
    if (totalTimeFromStart - baseTime_ > 0.25f) {
        baseTime_ += 0.25f;

        int i = MathHelper::Rand(4, waves_.RowCount() - 5);
        int j = MathHelper::Rand(4, waves_.ColumnCount() - 5);

        float r = MathHelper::RandF(0.2f, 0.5f);

        waves_.Disturb(i, j, r);
    }
    waves_.Update(deltaTimeSecond);

    for (int i = 0; i < waves_.VertexCount(); ++i) {
        vertices_[i].pos = waves_.Position(i);
        vertices_[i].color = DirectX::XMFLOAT4(DirectX::Colors::Blue);
    }
}