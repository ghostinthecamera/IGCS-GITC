// ????????????????????????????????????????????????????????????????
//  RiemannCubic.cpp   (fixed – May 2025)
// ????????????????????????????????????????????????????????????????
#include "stdafx.h"
#include "RiemannCubic.h"
#include "CameraPath.h"
#include "PathUtils.h"

#include <algorithm>
#include <cmath>

namespace IGCS
{
    using namespace DirectX;

    // ???????????????????????  helpers (unchanged)  ???????????????
    namespace
    {
        inline XMVECTOR QuaternionLogImproved(FXMVECTOR qIn)
        {
            const XMVECTOR q = XMQuaternionNormalize(qIn);

            const float cosHalf = XMVectorGetW(q);
            const XMVECTOR v = XMVectorSetW(q, 0.0f);
            const float vLen2 = XMVectorGetX(XMVector3LengthSq(v));

            if (vLen2 < 1e-6f)
                return v;                          // tiny rotation ? log?vector part

            const float halfAngle = std::acos(std::clamp(cosHalf, -1.0f, 1.0f));
            const float vLen = std::sqrt(vLen2);
            return XMVectorScale(v, halfAngle / vLen);
        }

        inline float AdaptiveTension(FXMVECTOR l0, FXMVECTOR l1, FXMVECTOR l2)
        {
            XMVECTOR d1 = XMVectorSubtract(l1, l0);
            XMVECTOR d2 = XMVectorSubtract(l2, l1);

            const float n1 = XMVectorGetX(XMVector3Length(d1));
            const float n2 = XMVectorGetX(XMVector3Length(d2));
            if (n1 < 1e-5f || n2 < 1e-5f)
                return 0.5f;

            d1 = XMVectorScale(d1, 1.0f / n1);
            d2 = XMVectorScale(d2, 1.0f / n2);
            const float align = XMVectorGetX(XMVector3Dot(d1, d2));
            return 0.2f + 0.3f * (align + 1.0f) * 0.5f;   // ? 0.2 … 0.5
        }

        inline XMVECTOR TangentTorqueMinimal(FXMVECTOR q0In, FXMVECTOR q1, FXMVECTOR q2In)
        {
            // we’re allowed to *read* FXMVECTORs, but locals must be XMVECTOR:
            XMVECTOR q0 = q0In;
            XMVECTOR q2 = q2In;

            if (XMVectorGetX(XMQuaternionDot(q0, q1)) < 0.0f) q0 = XMVectorNegate(q0);
            if (XMVectorGetX(XMQuaternionDot(q1, q2)) < 0.0f) q2 = XMVectorNegate(q2);

            XMVECTOR w1 = QuaternionLogImproved(XMQuaternionMultiply(XMQuaternionInverse(q0), q1));
            XMVECTOR w2 = QuaternionLogImproved(XMQuaternionMultiply(XMQuaternionInverse(q1), q2));

            w1 = XMVectorScale(w1, 2.0f);
            w2 = XMVectorScale(w2, 2.0f);
            return XMVectorLerp(w1, w2, 0.5f);
        }

        inline XMVECTOR ClampMag(FXMVECTOR v, float maxLen)
        {
            const float len = XMVectorGetX(XMVector3Length(v));
            return len > maxLen ? XMVectorScale(v, maxLen / len) : v;
        }

        inline XMVECTOR Hermite(FXMVECTOR p0, FXMVECTOR d0,
            FXMVECTOR p1, FXMVECTOR d1,
            float     t)
        {
            const float t2 = t * t;
            const float t3 = t2 * t;

            const float  h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
            const float  h10 = t3 - 2.0f * t2 + t;
            const float  h01 = -2.0f * t3 + 3.0f * t2;
            const float  h11 = t3 - t2;

            XMVECTOR res = XMVectorScale(p0, h00);
            res = XMVectorAdd(res, XMVectorScale(d0, h10));
            res = XMVectorAdd(res, XMVectorScale(p1, h01));
            res = XMVectorAdd(res, XMVectorScale(d1, h11));
            return res;
        }
    } // unnamed namespace
    // ?????????????????????????????????????????????????????????????


    // ————————————————————————————————————————————————————————————
    //  Four-node (free) interpolation
    // ————————————————————————————————————————————————————————————
    XMVECTOR RiemannCubicRotation4Node(float                         globalT,
        const std::vector<CameraNode>& nodes,
        const CameraPath& path)
    {
        const int totalSegs = static_cast<int>(nodes.size()) - 1;
        if (totalSegs < 1)
            return XMQuaternionIdentity();

        int seg = static_cast<int>(std::floor(globalT));
        seg = std::clamp(seg, 0, totalSegs - 1);

        if (seg >= totalSegs)
        {
            seg = totalSegs - 1;
            globalT = static_cast<float>(totalSegs);
        }
        const float tLocal = globalT - seg;

        int i0, i1, i2, i3;
        path.getnodeSequence(i0, i1, i2, i3, globalT);

        // *mutable* locals ? XMVECTOR
        XMVECTOR q0 = XMQuaternionNormalize(nodes[i0].rotation);
        XMVECTOR q1 = XMQuaternionNormalize(nodes[i1].rotation);
        XMVECTOR q2 = XMQuaternionNormalize(nodes[i2].rotation);
        XMVECTOR q3 = XMQuaternionNormalize(nodes[i3].rotation);

        if (XMVectorGetX(XMQuaternionDot(q0, q1)) < 0.0f) q0 = XMVectorNegate(q0);
        if (XMVectorGetX(XMQuaternionDot(q1, q2)) < 0.0f) q2 = XMVectorNegate(q2);
        if (XMVectorGetX(XMQuaternionDot(q2, q3)) < 0.0f) q3 = XMVectorNegate(q3);

        XMVECTOR l0 = QuaternionLogImproved(q0);
        XMVECTOR l1 = QuaternionLogImproved(q1);
        XMVECTOR l2 = QuaternionLogImproved(q2);
        XMVECTOR l3 = QuaternionLogImproved(q3);

        const float a1 = AdaptiveTension(l0, l1, l2);
        const float a2 = AdaptiveTension(l1, l2, l3);

        XMVECTOR d1 = ClampMag(XMVectorScale(TangentTorqueMinimal(q0, q1, q2), a1), 1.0f);
        XMVECTOR d2 = ClampMag(XMVectorScale(TangentTorqueMinimal(q1, q2, q3), a2), 1.0f);

        XMVECTOR logP = Hermite(l1, d1, l2, d2, tLocal);
        return XMQuaternionNormalize(XMQuaternionExp(logP));
    }


    // ————————————————————————————————————————————————————————————
    //  Monotone-safe variant (also fixed)
    // ————————————————————————————————————————————————————————————
    XMVECTOR RiemannCubicRotationMonotonic(float                         globalT,
        const std::vector<CameraNode>& nodes,
        const CameraPath& path)
    {
        const int totalSegs = static_cast<int>(nodes.size()) - 1;
        if (totalSegs < 1)
            return XMQuaternionIdentity();

        int seg = static_cast<int>(std::floor(globalT));
        seg = std::clamp(seg, 0, totalSegs - 1);

        if (seg >= totalSegs)
        {
            seg = totalSegs - 1;
            globalT = static_cast<float>(totalSegs);
        }
        const float tLocal = globalT - seg;

        int i0, i1, i2, i3;
        path.getnodeSequence(i0, i1, i2, i3, globalT);

        XMVECTOR q0 = XMQuaternionNormalize(nodes[i0].rotation);
        XMVECTOR q1 = XMQuaternionNormalize(nodes[i1].rotation);
        XMVECTOR q2 = XMQuaternionNormalize(nodes[i2].rotation);
        XMVECTOR q3 = XMQuaternionNormalize(nodes[i3].rotation);

        if (XMVectorGetX(XMQuaternionDot(q0, q1)) < 0.0f) q0 = XMVectorNegate(q0);
        if (XMVectorGetX(XMQuaternionDot(q1, q2)) < 0.0f) q2 = XMVectorNegate(q2);
        if (XMVectorGetX(XMQuaternionDot(q2, q3)) < 0.0f) q3 = XMVectorNegate(q3);

        XMVECTOR l0 = QuaternionLogImproved(q0);
        XMVECTOR l1 = QuaternionLogImproved(q1);
        XMVECTOR l2 = QuaternionLogImproved(q2);
        XMVECTOR l3 = QuaternionLogImproved(q3);

        const float a1 = AdaptiveTension(l0, l1, l2);
        const float a2 = AdaptiveTension(l1, l2, l3);

        XMVECTOR d1 = ClampMag(XMVectorScale(TangentTorqueMinimal(q0, q1, q2), a1), 2.0f);
        XMVECTOR d2 = ClampMag(XMVectorScale(TangentTorqueMinimal(q1, q2, q3), a2), 2.0f);

        // --- monotonicity guard ----------------------------------------------------------
        const XMVECTOR chord = XMVectorSubtract(l2, l1);
        const float    chordLen = XMVectorGetX(XMVector3Length(chord));

        if (chordLen > 1e-5f)
        {
            const XMVECTOR dir = XMVectorScale(chord, 1.0f / chordLen);
            const float    d1Proj = XMVectorGetX(XMVector3Dot(d1, dir));
            const float    d2Proj = XMVectorGetX(XMVector3Dot(d2, dir));

            if (d1Proj < 0.0f) d1 = XMVectorZero();
            if (d2Proj < 0.0f) d2 = XMVectorZero();

            const float maxProj = 3.0f * chordLen;
            if ((d1Proj + d2Proj) > maxProj)
            {
                const float s = maxProj / (d1Proj + d2Proj);
                d1 = XMVectorScale(d1, s);
                d2 = XMVectorScale(d2, s);
            }
        }
        else
        {
            d1 = d2 = XMVectorZero();
        }

        XMVECTOR logP = Hermite(l1, d1, l2, d2, tLocal);
        return XMQuaternionNormalize(XMQuaternionExp(logP));
    }
} // namespace IGCS
// ????????????????????????????????????????????????????????????????
