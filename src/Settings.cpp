#include <Settings.h>

#define S_MIN_OCTREE_HEIGHT 2
#define S_MAX_OCTREE_HEIGHT 9

Settings::Settings( )
{
    // Default settings
    // most parameters are scene dependent

    // D3DRenderer settings
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    mWndWidth = 1024;
    mWndHeight = 768;

    mRenderOutput = RO_COLOR;
    mShadowMapRes = 2048; // 4096 for quality picture
    mShadowMapZNear = 1.0f;
    mShadowMapZFar = 5000.0f;
    mShadowBias = 3600;

    mBlurRadius = 6.0f;
    mBlurSharpness = 100000.0f;

    mDirectInfluence = 1.0f;
    mIndirectInfluence = 1.0f;
    mAOInfluence = 1.0f;

    // VCT settings
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    mVCTEnable = true;
    mOctreeHeight = Clamp( 8, S_MIN_OCTREE_HEIGHT, S_MAX_OCTREE_HEIGHT ); // TODO it's possible to use 9, but there is a bug for now
    mOctreeBufferRes = 4096;
    mBrickBufferRes = 255;

    mVCTDebugOctreeLevel = mOctreeHeight - 1;
    mVCTDebugBuffer = DebugBuffer::DBUF_OPACITY;

    mVCTDebugView = false;
    mVCTDebugConeDir = 0;

    mVCTDebugOctreeFirstLevel = 6;
    mVCTDebugOctreeLastLevel = 2;

    mVCTLambdaFalloff = 0.06f;
    mVCTLocalConeOffset = 0.02f;
    mVCTWorldConeOffset = 12.2f;
    mVCTIndirectAmplification = 6.0f;
    mVCTStepCorrection = 0.76f;
    mVCTUseOpacityBuffer = true;
    mVCTConeTracingRes = 400; // 800 for quality picture

    mShowAO = false;

    // Scene settings
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    mLightAnimation = false;
    mLightAnimationSpeed = 0.07f;
    mSunYaw = 1.0f;
    mSunPitch = 1.26f;
    mLightDistance = 1.0f;
    mSunColor[0] = 0.95f; mSunColor[1] = 0.97f; mSunColor[2] = 0.86f;
    mSunPower = 1.3f;

    mMouseSens = 0.005f;
    mInitCamPos[0] = 896; mInitCamPos[1] = 558; mInitCamPos[2] = 63;
    mInitCamPhi = -0.065f;
    mInitCamTheta = -1.881f;
    mCameraSpeed = 500.0f;

    // Common settings
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    mFrameDeltaTime = 0.0166f; // 60 FPS cap

#ifdef _DEBUG
    mShaderDir = "FXBin/Debug/";
#else
    mShaderDir = "FXBin/Release/";
#endif
    mMediaDir = "Media/";

    mSceneFn = "Media/sponza/sponza.bin";
    mSaveSceneFn = "Media/sponza/sponza.bin";
    mSaveScene = false;
}